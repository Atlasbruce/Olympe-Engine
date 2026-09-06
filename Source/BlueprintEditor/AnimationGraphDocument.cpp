#include "AnimationGraphDocument.h"
#include "../../Source/json_helper.h"
#include "../../Source/system/system_utils.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

#include "../TiledLevelLoader/include/TilesetParser.h"

using json = nlohmann::json;

namespace Olympe {

namespace {
    static std::string LowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    static bool PathExists(const std::string& path)
    {
#ifdef _WIN32
        return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
        std::ifstream file(path.c_str());
        return file.good();
#endif
    }
}

AnimationGraphDocument::AnimationGraphDocument()
    : m_isDirty(false)
    , m_renderer(nullptr)
    , m_suppressDirtyNotifications(false)
    , m_nextNodeId(1)
{
    Clear();
}

AnimationGraphDocument::~AnimationGraphDocument() {}

void AnimationGraphDocument::SetRenderer(IGraphRenderer* renderer) { m_renderer = renderer; }
IGraphRenderer* AnimationGraphDocument::GetRenderer() { return m_renderer; }
const IGraphRenderer* AnimationGraphDocument::GetRenderer() const { return m_renderer; }

void AnimationGraphDocument::Clear()
{
    m_name = "Untitled Animation Graph";
    m_description.clear();
    m_filePath.clear();
    m_defaultState = "Idle";
    m_isDirty = false;
    m_sources.clear();
    m_states.clear();
    m_transitions.clear();
    m_availableClips.clear();
    m_nextNodeId = 1;
    m_data = BuildJson();
}

std::vector<std::string> AnimationGraphDocument::GetSourceNames() const
{
    std::vector<std::string> names;
    for (size_t i = 0; i < m_sources.size(); ++i)
    {
        names.push_back(m_sources[i].sourceName);
    }
    return names;
}

std::vector<std::string> AnimationGraphDocument::GetSourceClipNames(size_t index) const
{
    if (index >= m_sources.size())
    {
        return std::vector<std::string>();
    }
    return m_sources[index].clips;
}

void AnimationGraphDocument::SetName(const std::string& name) { m_name = name.empty() ? "Untitled Animation Graph" : name; m_isDirty = true; m_data = BuildJson(); }
void AnimationGraphDocument::SetDescription(const std::string& description) { m_description = description; m_isDirty = true; m_data = BuildJson(); }
void AnimationGraphDocument::SetDefaultState(const std::string& stateName) { if (!stateName.empty()) { m_defaultState = stateName; m_isDirty = true; m_data = BuildJson(); } }

void AnimationGraphDocument::AddSource(const std::string& filePath)
{
    if (filePath.empty() || HasSource(filePath)) return;
    ImportTSXSource(filePath, nullptr);
}

void AnimationGraphDocument::RemoveSource(size_t index)
{
    if (index >= m_sources.size()) return;
    m_sources.erase(m_sources.begin() + index);
    RebuildAvailableClips();
    m_isDirty = true;
    m_data = BuildJson();
}

std::string AnimationGraphDocument::MakeUniqueStateName(const std::string& baseName) const
{
    std::string candidate = baseName.empty() ? "State" : baseName;
    int suffix = 1;
    while (FindStateIndex(candidate) >= 0) candidate = baseName + "_" + std::to_string(suffix++);
    return candidate;
}

bool AnimationGraphDocument::AddState(const std::string& stateName)
{
    if (stateName.empty() || FindStateIndex(stateName) >= 0) return false;
    StateDefinition state;
    state.name = stateName;
    state.directionClips.resize(kDirectionCount);
    for (size_t i = 0; i < kDirectionCount; ++i) state.directionClips[i].direction = DirectionToString(static_cast<Direction>(i));
    m_states.push_back(state);
    m_isDirty = true;
    m_data = BuildJson();
    return true;
}

bool AnimationGraphDocument::AddStateFromClip(const std::string& stateName, const std::string& clipName)
{
    if (!AddState(stateName))
    {
        return false;
    }
    StateDefinition* state = GetState(stateName);
    if (!state)
    {
        return false;
    }
    state->defaultClip = clipName;
    state->x = static_cast<double>(m_nextNodeId * 160);
    state->y = static_cast<double>(m_nextNodeId * 80);
    ++m_nextNodeId;
    for (size_t i = 0; i < kDirectionCount; ++i)
    {
        state->directionClips[i].direction = DirectionToString(static_cast<Direction>(i));
        state->directionClips[i].clip = clipName;
    }
    m_isDirty = true;
    m_data = BuildJson();
    return true;
}

bool AnimationGraphDocument::RenameState(const std::string& oldName, const std::string& newName)
{
    if (oldName.empty() || newName.empty() || oldName == newName || FindStateIndex(newName) >= 0) return false;
    StateDefinition* state = GetState(oldName);
    if (!state) return false;
    state->name = newName;
    UpdateStateNamesInReferences(oldName, newName);
    if (m_defaultState == oldName) m_defaultState = newName;
    m_isDirty = true;
    m_data = BuildJson();
    return true;
}

bool AnimationGraphDocument::RemoveState(const std::string& stateName)
{
    int index = FindStateIndex(stateName);
    if (index < 0) return false;
    m_states.erase(m_states.begin() + index);
    m_transitions.erase(std::remove_if(m_transitions.begin(), m_transitions.end(), [&](const TransitionDefinition& t) { return t.fromState == stateName || t.toState == stateName; }), m_transitions.end());
    if (m_defaultState == stateName) m_defaultState = m_states.empty() ? "Idle" : m_states.front().name;
    m_isDirty = true;
    m_data = BuildJson();
    return true;
}

bool AnimationGraphDocument::SetStatePosition(const std::string& stateName, double x, double y) { StateDefinition* s = GetState(stateName); if (!s) return false; s->x = x; s->y = y; m_isDirty = true; m_data = BuildJson(); return true; }
bool AnimationGraphDocument::SetStateDefaultClip(const std::string& stateName, const std::string& clipName) { StateDefinition* s = GetState(stateName); if (!s) return false; s->defaultClip = clipName; m_isDirty = true; m_data = BuildJson(); return true; }
bool AnimationGraphDocument::SetStateDirectionClip(const std::string& stateName, Direction direction, const std::string& clipName) { StateDefinition* s = GetState(stateName); if (!s || !IsDirectionValid(direction)) return false; size_t index = static_cast<size_t>(direction); if (s->directionClips.size() < kDirectionCount) s->directionClips.resize(kDirectionCount); s->directionClips[index].direction = DirectionToString(direction); s->directionClips[index].clip = clipName; m_isDirty = true; m_data = BuildJson(); return true; }
bool AnimationGraphDocument::AddStateEvent(const std::string& stateName, const std::string& eventName) { StateDefinition* s = GetState(stateName); if (!s || eventName.empty() || std::find(s->events.begin(), s->events.end(), eventName) != s->events.end()) return false; s->events.push_back(eventName); m_isDirty = true; m_data = BuildJson(); return true; }
bool AnimationGraphDocument::RemoveStateEvent(const std::string& stateName, const std::string& eventName) { StateDefinition* s = GetState(stateName); if (!s) return false; std::vector<std::string>::iterator it = std::remove(s->events.begin(), s->events.end(), eventName); if (it == s->events.end()) return false; s->events.erase(it, s->events.end()); m_isDirty = true; m_data = BuildJson(); return true; }
bool AnimationGraphDocument::AddTransition(const std::string& fromState, const std::string& toState, const std::string& condition, double duration) { if (fromState.empty() || toState.empty() || FindStateIndex(fromState) < 0 || FindStateIndex(toState) < 0) return false; TransitionDefinition t; t.fromState = fromState; t.toState = toState; t.condition = condition; t.duration = duration; m_transitions.push_back(t); m_isDirty = true; m_data = BuildJson(); return true; }
bool AnimationGraphDocument::RemoveTransition(size_t index) { if (index >= m_transitions.size()) return false; m_transitions.erase(m_transitions.begin() + index); m_isDirty = true; m_data = BuildJson(); return true; }

bool AnimationGraphDocument::HasSource(const std::string& filePath) const
{
    const std::string normalized = NormalizePath(filePath);
    for (size_t i = 0; i < m_sources.size(); ++i)
    {
        if (m_sources[i].filePath == normalized)
            return true;
    }
    return false;
}

int AnimationGraphDocument::FindStateIndex(const std::string& stateName) const { for (size_t i = 0; i < m_states.size(); ++i) if (m_states[i].name == stateName) return static_cast<int>(i); return -1; }
std::string AnimationGraphDocument::GetStateNameAt(size_t index) const { return index < m_states.size() ? m_states[index].name : ""; }
const AnimationGraphDocument::StateDefinition* AnimationGraphDocument::GetState(const std::string& stateName) const { int index = FindStateIndex(stateName); return index < 0 ? nullptr : &m_states[static_cast<size_t>(index)]; }
AnimationGraphDocument::StateDefinition* AnimationGraphDocument::GetState(const std::string& stateName) { int index = FindStateIndex(stateName); return index < 0 ? nullptr : &m_states[static_cast<size_t>(index)]; }

std::string AnimationGraphDocument::ResolveClipForState(const std::string& stateName, Direction direction) const
{
    const StateDefinition* state = GetState(stateName);
    if (!state) return "";
    if (IsDirectionValid(direction) && state->directionClips.size() == kDirectionCount)
    {
        const std::string& clip = state->directionClips[static_cast<size_t>(direction)].clip;
        if (!clip.empty()) return clip;
    }
    return state->defaultClip;
}

bool AnimationGraphDocument::IsDirectionValid(Direction direction) const { return static_cast<int>(direction) >= 0 && static_cast<size_t>(direction) < kDirectionCount; }
std::string AnimationGraphDocument::DirectionToString(Direction direction) { switch (direction) { case Direction::North: return "N"; case Direction::NorthEast: return "NE"; case Direction::East: return "E"; case Direction::SouthEast: return "SE"; case Direction::South: return "S"; case Direction::SouthWest: return "SW"; case Direction::West: return "W"; case Direction::NorthWest: return "NW"; default: return "N"; } }
AnimationGraphDocument::Direction AnimationGraphDocument::StringToDirection(const std::string& text, bool* ok) { const std::string value = LowerCopy(text); if (ok) *ok = true; if (value == "n") return Direction::North; if (value == "ne") return Direction::NorthEast; if (value == "e") return Direction::East; if (value == "se") return Direction::SouthEast; if (value == "s") return Direction::South; if (value == "sw") return Direction::SouthWest; if (value == "w") return Direction::West; if (value == "nw") return Direction::NorthWest; if (ok) *ok = false; return Direction::North; }

std::vector<std::string> AnimationGraphDocument::ValidateBinding() const
{
    std::vector<std::string> warnings;
    if (m_sources.empty()) warnings.push_back("No TSX sources imported.");
    if (m_states.empty()) warnings.push_back("No animation states defined.");
    if (m_defaultState.empty()) warnings.push_back("Default state is not set.");
    for (size_t i = 0; i < m_states.size(); ++i)
    {
        const StateDefinition& state = m_states[i];
        if (state.name.empty()) warnings.push_back("A state has no name.");
        if (state.defaultClip.empty()) warnings.push_back("State '" + state.name + "' has no default clip.");
        if (state.directionClips.size() != kDirectionCount) warnings.push_back("State '" + state.name + "' does not define 8 directions.");
    }
    for (size_t i = 0; i < m_transitions.size(); ++i) if (FindStateIndex(m_transitions[i].fromState) < 0 || FindStateIndex(m_transitions[i].toState) < 0) warnings.push_back("Transition references an unknown state.");
    return warnings;
}

std::vector<std::string> AnimationGraphDocument::GetAllKnownClips() const { return m_availableClips; }

nlohmann::json AnimationGraphDocument::BuildRuntimeJson() const
{
    json root = json::object();
    root["schema_version"] = 1;
    root["blueprintType"] = "AnimationGraph";
    root["name"] = m_name;
    root["description"] = m_description;
    root["defaultState"] = m_defaultState;
    root["sources"] = json::array();
    for (size_t i = 0; i < m_sources.size(); ++i)
    {
        json source = json::object();
        source["filePath"] = m_sources[i].filePath;
        source["sourceName"] = m_sources[i].sourceName;
        source["clips"] = json::array();
        for (size_t j = 0; j < m_sources[i].clips.size(); ++j) source["clips"].push_back(m_sources[i].clips[j]);
        root["sources"].push_back(source);
    }
    root["states"] = json::array();
    for (size_t i = 0; i < m_states.size(); ++i)
    {
        const StateDefinition& state = m_states[i];
        json item = json::object();
        item["name"] = state.name;
        item["defaultClip"] = state.defaultClip;
        item["events"] = json::array();
        for (size_t e = 0; e < state.events.size(); ++e) item["events"].push_back(state.events[e]);
        item["blendTime"] = state.blendTime;
        item["loop"] = state.loop;
        item["priority"] = state.priority;
        item["x"] = state.x;
        item["y"] = state.y;
        item["directionClips"] = json::array();
        for (size_t d = 0; d < state.directionClips.size(); ++d) { json clip = json::object(); clip["direction"] = state.directionClips[d].direction; clip["clip"] = state.directionClips[d].clip; item["directionClips"].push_back(clip); }
        root["states"].push_back(item);
    }
    root["transitions"] = json::array();
    for (size_t i = 0; i < m_transitions.size(); ++i)
    {
        json item = json::object();
        item["from"] = m_transitions[i].fromState;
        item["to"] = m_transitions[i].toState;
        item["condition"] = m_transitions[i].condition;
        item["duration"] = m_transitions[i].duration;
        item["priority"] = m_transitions[i].priority;
        item["interruptible"] = m_transitions[i].interruptible;
        root["transitions"].push_back(item);
    }
    return root;
}

bool AnimationGraphDocument::ExportRuntimeJson(const std::string& filePath) const { if (filePath.empty()) return false; std::ofstream ofs(filePath.c_str()); if (!ofs.good()) return false; ofs << BuildRuntimeJson().dump(2); return true; }

bool AnimationGraphDocument::Load(const std::string& filePath) { json root; if (!JsonHelper::LoadJsonFromFile(filePath, root)) return false; if (!LoadFromJson(root)) return false; m_filePath = filePath; m_isDirty = false; return true; }
bool AnimationGraphDocument::Save(const std::string& filePath) { const std::string targetPath = filePath.empty() ? m_filePath : filePath; if (targetPath.empty()) return false; std::ofstream ofs(targetPath.c_str()); if (!ofs.good()) return false; ofs << BuildJson().dump(2); m_filePath = targetPath; m_isDirty = false; return true; }
std::string AnimationGraphDocument::GetName() const { return m_name.empty() ? "Untitled Animation Graph" : m_name; }

nlohmann::json AnimationGraphDocument::BuildJson() const
{
    json root = BuildRuntimeJson();
    root["schema_version"] = 2;
    return root;
}

bool AnimationGraphDocument::LoadFromJson(const json& root)
{
    if (!root.is_object()) return false;
    m_name = JsonHelper::GetString(root, "name", "Untitled Animation Graph");
    m_description = JsonHelper::GetString(root, "description", "");
    m_defaultState = JsonHelper::GetString(root, "defaultState", "Idle");
    m_sources.clear(); m_states.clear(); m_transitions.clear();
    if (root.contains("sources") && root["sources"].is_array())
        for (const auto& sourceJson : root["sources"]) { SourceDefinition s; s.filePath = JsonHelper::GetString(sourceJson, "filePath", ""); s.sourceName = JsonHelper::GetString(sourceJson, "sourceName", ""); if (sourceJson.contains("clips") && sourceJson["clips"].is_array()) for (const auto& clip : sourceJson["clips"]) if (clip.is_string()) s.clips.push_back(clip.get<std::string>()); m_sources.push_back(s); }
    if (root.contains("states") && root["states"].is_array())
        for (const auto& stateJson : root["states"]) { StateDefinition s; s.name = JsonHelper::GetString(stateJson, "name", ""); s.defaultClip = JsonHelper::GetString(stateJson, "defaultClip", ""); s.blendTime = JsonHelper::GetFloat(stateJson, "blendTime", 0.1f); s.loop = JsonHelper::GetBool(stateJson, "loop", true); s.priority = JsonHelper::GetInt(stateJson, "priority", 0); s.x = JsonHelper::GetFloat(stateJson, "x", 0.0f); s.y = JsonHelper::GetFloat(stateJson, "y", 0.0f); if (stateJson.contains("directionClips") && stateJson["directionClips"].is_array()) for (const auto& clipJson : stateJson["directionClips"]) { DirectionClipMapping m; m.direction = JsonHelper::GetString(clipJson, "direction", "N"); m.clip = JsonHelper::GetString(clipJson, "clip", ""); s.directionClips.push_back(m); } if (s.directionClips.size() < kDirectionCount) s.directionClips.resize(kDirectionCount); if (stateJson.contains("events") && stateJson["events"].is_array()) for (const auto& eventJson : stateJson["events"]) if (eventJson.is_string()) s.events.push_back(eventJson.get<std::string>()); m_states.push_back(s); }
    if (root.contains("transitions") && root["transitions"].is_array())
        for (const auto& transitionJson : root["transitions"]) { TransitionDefinition t; t.fromState = JsonHelper::GetString(transitionJson, "from", ""); t.toState = JsonHelper::GetString(transitionJson, "to", ""); t.condition = JsonHelper::GetString(transitionJson, "condition", ""); t.duration = JsonHelper::GetFloat(transitionJson, "duration", 0.1f); t.priority = JsonHelper::GetInt(transitionJson, "priority", 0); t.interruptible = JsonHelper::GetBool(transitionJson, "interruptible", true); m_transitions.push_back(t); }
    RebuildAvailableClips(); m_isDirty = false; m_data = BuildJson(); return true;
}

void AnimationGraphDocument::UpdateStateNamesInReferences(const std::string& oldName, const std::string& newName)
{
    for (size_t i = 0; i < m_transitions.size(); ++i) { if (m_transitions[i].fromState == oldName) m_transitions[i].fromState = newName; if (m_transitions[i].toState == oldName) m_transitions[i].toState = newName; }
}

void AnimationGraphDocument::RebuildAvailableClips()
{
    m_availableClips.clear();
    for (size_t i = 0; i < m_sources.size(); ++i) for (size_t j = 0; j < m_sources[i].clips.size(); ++j) m_availableClips.push_back(m_sources[i].clips[j]);
}

std::string AnimationGraphDocument::NormalizePath(const std::string& path) { std::string result = path; std::replace(result.begin(), result.end(), '\\', '/'); return result; }
std::string AnimationGraphDocument::ExtractClipNameFromTsx(const json& root, const std::string& fallbackName) { if (root.contains("name") && root["name"].is_string()) return root["name"].get<std::string>(); return fallbackName; }

bool AnimationGraphDocument::ImportTSXSource(const std::string& filePath, std::vector<std::string>* errors)
{
    return ImportTSXFile(filePath, errors);
}

bool AnimationGraphDocument::ImportTSXFile(const std::string& filePath, std::vector<std::string>* errors)
{
    const std::string normalized = NormalizePath(filePath);
    if (normalized.empty())
    {
        if (errors) errors->push_back("Cannot parse TSX source: empty path");
        return false;
    }
    if (HasSource(normalized))
    {
        return true;
    }
    Olympe::Tiled::TilesetParser parser;
    Olympe::Tiled::TiledTileset tileset;

    std::string resolvedPath = normalized;
    if (!PathExists(resolvedPath))
    {
        if (PathExists(filePath))
        {
            resolvedPath = filePath;
        }
        else
        {
            std::string cwdPath = std::string(".\\") + filePath;
            if (PathExists(cwdPath))
            {
                resolvedPath = cwdPath;
            }
        }
    }

    if (!parser.ParseFile(resolvedPath, tileset))
    {
        SYSTEM_LOG << "[AnimationGraphDocument] TSX parse failed for '" << resolvedPath << "' (original='" << filePath << "')\n";
        if (errors) errors->push_back("Cannot parse TSX source: " + filePath);
        return false;
    }

    if (tileset.name.empty())
    {
        std::string baseName = normalized;
        size_t slash = baseName.find_last_of("/\\");
        if (slash != std::string::npos)
        {
            baseName = baseName.substr(slash + 1);
        }
        size_t dot = baseName.find_last_of('.');
        if (dot != std::string::npos)
        {
            baseName = baseName.substr(0, dot);
        }
        tileset.name = baseName;
    }

    if (tileset.tiles.empty())
    {
        // Some TSX files are image-based banks with tilecount only; synthesize clip entries.
        if (tileset.tilecount > 0)
        {
            for (int i = 0; i < tileset.tilecount; ++i)
            {
                tileset.tiles.push_back(Olympe::Tiled::TiledTile());
                tileset.tiles.back().id = i;
            }
        }
    }

    if (tileset.tiles.empty())
    {
        if (errors) errors->push_back("Cannot parse TSX source: " + filePath);
        return false;
    }

    SourceDefinition source;
    source.filePath = resolvedPath;
    source.sourceName = tileset.name.empty() ? ExtractClipNameFromTsx(json::object(), filePath) : tileset.name;

    source.clips.push_back(source.sourceName);
    m_sources.push_back(source);
    RebuildAvailableClips();
    m_isDirty = true;
    m_data = BuildJson();
    return true;
}

bool AnimationGraphDocument::ImportTSXDirectory(const std::string& directoryPath, std::vector<std::string>* errors)
{
    std::string normalized = NormalizePath(directoryPath);
    std::vector<std::string> files;
#ifdef _WIN32
    std::string pattern = normalized; if (!pattern.empty() && pattern[pattern.size() - 1] != '/' && pattern[pattern.size() - 1] != '\\') pattern += "\\"; pattern += "*.tsx";
    WIN32_FIND_DATAA findData; HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData); if (hFind == INVALID_HANDLE_VALUE) { if (errors) errors->push_back("TSX directory not found: " + directoryPath); return false; }
    do { if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) files.push_back(normalized + "\\" + findData.cFileName); } while (FindNextFileA(hFind, &findData) != 0); FindClose(hFind);
#else
    if (errors) errors->push_back("TSX directory scanning is only implemented on Windows in this build.");
    return false;
#endif
    bool allOk = true; for (size_t i = 0; i < files.size(); ++i) if (!ImportTSXSource(files[i], errors)) allOk = false; return allOk;
}

std::vector<std::string> AnimationGraphDocument::ValidateImportedSources() const
{
    std::vector<std::string> errors;
    for (size_t i = 0; i < m_sources.size(); ++i) { const SourceDefinition& source = m_sources[i]; if (source.filePath.empty()) errors.push_back("A source has no file path."); if (source.clips.empty()) errors.push_back("Source '" + source.filePath + "' does not expose any clips."); }
    return errors;
}

} // namespace Olympe
