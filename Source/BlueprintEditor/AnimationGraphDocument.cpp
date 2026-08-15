#include "AnimationGraphDocument.h"
#include "../../Source/json_helper.h"
#include "../../Source/system/system_utils.h"
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

namespace Olympe {

AnimationGraphDocument::AnimationGraphDocument()
    : m_isDirty(false)
    , m_renderer(nullptr)
    , m_nextNodeId(1)
    , m_selectedStateIndex(-1)
    , m_selectedTransitionIndex(-1)
{
    MarkDefaultContent("Untitled Animation Graph");
}

AnimationGraphDocument::~AnimationGraphDocument() {}

void AnimationGraphDocument::SetRenderer(IGraphRenderer* renderer)
{
    m_renderer = renderer;
}

IGraphRenderer* AnimationGraphDocument::GetRenderer()
{
    return m_renderer;
}

const IGraphRenderer* AnimationGraphDocument::GetRenderer() const
{
    return m_renderer;
}

void AnimationGraphDocument::Clear()
{
    m_name = "Untitled Animation Graph";
    m_description.clear();
    m_bankRef.clear();
    m_prefabRef.clear();
    m_animationGraphRef.clear();
    m_defaultState = "Idle";
    m_data = json::object();
    m_data["states"] = json::array();
    m_data["transitions"] = json::array();
    m_data["events"] = json::array();
    m_isDirty = false;
}

void AnimationGraphDocument::MarkDefaultContent(const std::string& name)
{
    Clear();
    m_name = name;
    m_data["schema_version"] = 1;
    m_data["blueprintType"] = "AnimationGraph";
    m_data["name"] = name;
    m_data["description"] = "";
    m_data["animationBankRef"] = "";
    m_data["prefabRef"] = "";
    m_data["animationGraphRef"] = "";
    m_data["defaultState"] = "Idle";
}

void AnimationGraphDocument::SetAnimationBankPath(const std::string& path)
{
    m_bankRef = path;
    m_data["animationBankRef"] = path;
    SYSTEM_LOG << "[AnimationGraphDocument] SetAnimationBankPath: " << path << "\n";
    const bool ok = ReloadAnimationBankMetadata();
    SYSTEM_LOG << "[AnimationGraphDocument] ReloadAnimationBankMetadata: " << (ok ? "OK" : "EMPTY") << " clips=" << m_availableAnimations.size() << "\n";
    m_isDirty = true;
}

bool AnimationGraphDocument::LoadAnimationBankFromFile(const std::string& path)
{
    SetAnimationBankPath(path);
    return !m_availableAnimations.empty();
}

void AnimationGraphDocument::SetPrefabPath(const std::string& path)
{
    m_prefabRef = path;
    m_data["prefabRef"] = path;
    m_isDirty = true;
}

void AnimationGraphDocument::SetAnimationGraphPath(const std::string& path)
{
    m_animationGraphRef = path;
    m_data["animationGraphRef"] = path;
    m_isDirty = true;
}

bool AnimationGraphDocument::LoadAnimationGraphRuntimeFromFile(const std::string& path)
{
    SetAnimationGraphPath(path);
    return true;
}

void AnimationGraphDocument::SetDefaultState(const std::string& stateName)
{
    m_defaultState = stateName.empty() ? "Idle" : stateName;
    m_data["defaultState"] = m_defaultState;
    m_isDirty = true;
}

bool AnimationGraphDocument::AddStateFromClip(const std::string& stateName, const std::string& animationName)
{
    if (stateName.empty() || animationName.empty())
        return false;

    json state = json::object();
    state["name"] = stateName;
    state["animation"] = animationName;
    state["loop"] = true;
    state["speed"] = 1.0;
    state["priority"] = 0;
    state["x"] = 0.0;
    state["y"] = 0.0;

    m_data["states"].push_back(state);
    if (m_defaultState.empty())
        SetDefaultState(stateName);
    m_isDirty = true;
    return true;
}

bool AnimationGraphDocument::AddStateNode(const std::string& stateName, const std::string& animationName, float x, float y)
{
    if (!AddStateFromClip(stateName, animationName))
        return false;
    if (m_data.contains("states") && m_data["states"].is_array() && !m_data["states"].empty())
    {
        nlohmann::json& state = m_data["states"][m_data["states"].size() - 1];
        state["x"] = x;
        state["y"] = y;
        state["nodeId"] = m_nextNodeId++;
    }
    return true;
}

std::string AnimationGraphDocument::MakeUniqueStateName(const std::string& baseName) const
{
    if (!m_data.contains("states") || !m_data["states"].is_array())
        return baseName;

    std::string candidate = baseName;
    int suffix = 1;
    while (true)
    {
        bool exists = false;
        for (size_t i = 0; i < m_data["states"].size(); ++i)
        {
            const nlohmann::json& state = m_data["states"][i];
            if (state.contains("name") && state["name"].is_string() && state["name"].get<std::string>() == candidate)
            {
                exists = true;
                break;
            }
        }

        if (!exists)
            break;

        candidate = baseName + "_" + std::to_string(suffix++);
    }

    return candidate;
}

bool AnimationGraphDocument::RemoveState(const std::string& stateName)
{
    if (stateName.empty() || !m_data.contains("states") || !m_data["states"].is_array())
        return false;

    json& states = m_data["states"];
    json filteredStates = json::array();
    for (size_t i = 0; i < states.size(); ++i)
    {
        if (states[i].contains("name") && states[i]["name"].is_string() && states[i]["name"].get<std::string>() == stateName)
            continue;
        filteredStates.push_back(states[i]);
    }
    states = filteredStates;

    if (m_data.contains("transitions") && m_data["transitions"].is_array())
    {
        json& transitions = m_data["transitions"];
        json filteredTransitions = json::array();
        for (size_t i = 0; i < transitions.size(); ++i)
        {
            bool remove = false;
            if (transitions[i].contains("from") && transitions[i]["from"].is_string() && transitions[i]["from"].get<std::string>() == stateName)
                remove = true;
            if (transitions[i].contains("to") && transitions[i]["to"].is_string() && transitions[i]["to"].get<std::string>() == stateName)
                remove = true;
            if (!remove)
                filteredTransitions.push_back(transitions[i]);
        }
        transitions = filteredTransitions;
    }

    if (m_defaultState == stateName)
        m_defaultState = "Idle";

    m_isDirty = true;
    return true;
}

bool AnimationGraphDocument::AddTransition(const std::string& fromState, const std::string& toState, float transitionTime)
{
    if (fromState.empty() || toState.empty())
        return false;

    json transition = json::object();
    transition["from"] = fromState;
    transition["to"] = toState;
    transition["transitionTime"] = transitionTime;
    transition["conditions"] = json::array();

    m_data["transitions"].push_back(transition);
    m_isDirty = true;
    return true;
}

bool AnimationGraphDocument::AddTransitionByIndex(size_t fromIndex, size_t toIndex, float transitionTime)
{
    if (!m_data.contains("states") || !m_data["states"].is_array())
        return false;
    const nlohmann::json& states = m_data["states"];
    if (fromIndex >= states.size() || toIndex >= states.size())
        return false;
    return AddTransition(GetStateNameAt(fromIndex), GetStateNameAt(toIndex), transitionTime);
}

bool AnimationGraphDocument::RemoveTransition(size_t index)
{
    if (!m_data.contains("transitions") || !m_data["transitions"].is_array())
        return false;
    json& transitions = m_data["transitions"];
    if (index >= transitions.size())
        return false;
    json filteredTransitions = json::array();
    for (size_t i = 0; i < transitions.size(); ++i)
    {
        if (i != index)
            filteredTransitions.push_back(transitions[i]);
    }
    transitions = filteredTransitions;
    m_isDirty = true;
    return true;
}

std::string AnimationGraphDocument::GetStateNameAt(size_t index) const
{
    if (!m_data.contains("states") || !m_data["states"].is_array() || index >= m_data["states"].size())
        return "";
    return m_data["states"][index].value("name", "");
}

bool AnimationGraphDocument::AddEvent(const std::string& stateName, const std::string& eventName, float normalizedTime)
{
    if (stateName.empty() || eventName.empty())
        return false;

    json evt = json::object();
    evt["state"] = stateName;
    evt["name"] = eventName;
    evt["time"] = normalizedTime;
    m_data["events"].push_back(evt);
    m_isDirty = true;
    return true;
}

bool AnimationGraphDocument::RemoveEvent(size_t index)
{
    if (!m_data.contains("events") || !m_data["events"].is_array())
        return false;
    json& events = m_data["events"];
    if (index >= events.size())
        return false;
    json filteredEvents = json::array();
    for (size_t i = 0; i < events.size(); ++i)
    {
        if (i != index)
            filteredEvents.push_back(events[i]);
    }
    events = filteredEvents;
    m_isDirty = true;
    return true;
}

bool AnimationGraphDocument::GenerateDefaultTransitions(float transitionTime)
{
    if (!m_data.contains("states") || !m_data["states"].is_array())
        return false;

    const nlohmann::json& states = m_data["states"];
    for (size_t i = 0; i < states.size(); ++i)
    {
        std::string stateName = "";
        if (states[i].contains("name") && states[i]["name"].is_string())
            stateName = states[i]["name"].get<std::string>();
        if (stateName.empty() || stateName == m_defaultState)
            continue;

        AddTransition(m_defaultState, stateName, transitionTime);
        AddTransition(stateName, m_defaultState, transitionTime);
    }
    return true;
}

bool AnimationGraphDocument::GenerateStatesFromBank()
{
    if (!ReloadAnimationBankMetadata())
        return false;
    return !m_availableAnimations.empty();
}

bool AnimationGraphDocument::AutoLayoutStates()
{
    if (!m_data.contains("states") || !m_data["states"].is_array())
        return false;

    const size_t count = m_data["states"].size();
    for (size_t i = 0; i < count; ++i)
    {
        if (!m_data["states"][i].is_object())
            continue;

        double x = 80.0 + static_cast<double>(i % 4) * 220.0;
        double y = 120.0 + static_cast<double>(i / 4) * 140.0;
        m_data["states"][i]["x"] = x;
        m_data["states"][i]["y"] = y;
    }

    m_isDirty = true;
    return true;
}

bool AnimationGraphDocument::ReloadAnimationBankMetadata()
{
    m_availableAnimations.clear();
    if (m_bankRef.empty())
    {
        SYSTEM_LOG << "[AnimationGraphDocument] ReloadAnimationBankMetadata: empty bank ref\n";
        return false;
    }

    std::string resolvedPath = m_bankRef;
    std::replace(resolvedPath.begin(), resolvedPath.end(), '\\', '/');
    if (resolvedPath.rfind("./", 0) == 0)
        resolvedPath.erase(0, 2);
    std::ifstream ifs(resolvedPath.c_str());
    if (!ifs.good())
    {
        SYSTEM_LOG << "[AnimationGraphDocument] ReloadAnimationBankMetadata: cannot open " << resolvedPath << " (from " << m_bankRef << ")\n";
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (content.empty())
        return false;

    size_t pos = 0;
    while (true)
    {
        size_t tilePos = content.find("<tile", pos);
        if (tilePos == std::string::npos)
            break;

        size_t tagEnd = content.find('>', tilePos);
        if (tagEnd == std::string::npos)
            break;

        size_t typePos = content.find("type=\"", tilePos);
        if (typePos != std::string::npos && typePos < tagEnd)
        {
            typePos += 6;
            size_t typeEnd = content.find('"', typePos);
            if (typeEnd != std::string::npos && typeEnd < tagEnd + 128)
            {
                std::string clipName = content.substr(typePos, typeEnd - typePos);
                if (!clipName.empty())
                    m_availableAnimations.push_back(clipName);
            }
        }

        pos = tagEnd + 1;
    }

    SYSTEM_LOG << "[AnimationGraphDocument] ReloadAnimationBankMetadata: parsed " << m_availableAnimations.size() << " clip(s)\n";
    return !m_availableAnimations.empty();
}

std::vector<std::string> AnimationGraphDocument::ValidateBinding() const
{
    std::vector<std::string> warnings;

    if (m_bankRef.empty())
        warnings.push_back("Animation bank is not linked.");
    if (m_prefabRef.empty())
        warnings.push_back("Prefab is not linked.");
    if (m_animationGraphRef.empty())
        warnings.push_back("Animation graph reference is not linked.");
    if (m_defaultState.empty())
        warnings.push_back("Default state is not set.");

    if (!m_data.contains("states") || !m_data["states"].is_array() || m_data["states"].empty())
        warnings.push_back("No animation states defined.");
    if (!m_data.contains("transitions") || !m_data["transitions"].is_array())
        warnings.push_back("Transition list is missing.");
    if (!m_data.contains("events") || !m_data["events"].is_array())
        warnings.push_back("Event list is missing.");

    if (m_data.contains("states") && m_data["states"].is_array())
    {
        const json& states = m_data["states"];
        for (size_t i = 0; i < states.size(); ++i)
        {
            if (!states[i].contains("animation") || !states[i]["animation"].is_string() || states[i]["animation"].get<std::string>().empty())
                warnings.push_back("A state is missing its animation clip reference.");
        }
    }

    if (m_data.contains("events") && m_data["events"].is_array())
    {
        const json& events = m_data["events"];
        for (size_t i = 0; i < events.size(); ++i)
        {
            if (!events[i].contains("state") || !events[i]["state"].is_string() || events[i]["state"].get<std::string>().empty())
                warnings.push_back("An event is missing its owning state.");
        }
    }

    return warnings;
}

nlohmann::json AnimationGraphDocument::BuildRuntimeJson() const
{
    json root = json::object();
    root["graphName"] = m_name;
    root["animationBankRef"] = m_bankRef;
    root["prefabRef"] = m_prefabRef;
    root["defaultState"] = m_defaultState;
    root["states"] = m_data.contains("states") ? m_data["states"] : json::array();
    root["transitions"] = m_data.contains("transitions") ? m_data["transitions"] : json::array();
    root["events"] = m_data.contains("events") ? m_data["events"] : json::array();
    return root;
}

bool AnimationGraphDocument::ExportRuntimeJson(const std::string& filePath) const
{
    if (filePath.empty())
        return false;
    std::ofstream ofs(filePath.c_str());
    if (!ofs.good())
        return false;
    ofs << BuildRuntimeJson().dump(2);
    return true;
}

std::string AnimationGraphDocument::GetName() const
{
    return m_name.empty() ? "Untitled Animation Graph" : m_name;
}

bool AnimationGraphDocument::LoadFromJson(const json& root)
{
    if (!root.is_object())
        return false;

    m_name = JsonHelper::GetString(root, "name", "Untitled Animation Graph");
    m_description = JsonHelper::GetString(root, "description", "");
    m_bankRef = JsonHelper::GetString(root, "animationBankRef", "");
    m_prefabRef = JsonHelper::GetString(root, "prefabRef", "");
    m_animationGraphRef = JsonHelper::GetString(root, "animationGraphRef", "");
    m_defaultState = JsonHelper::GetString(root, "defaultState", "Idle");
    m_data = root;
    if (!m_data.contains("states")) m_data["states"] = json::array();
    if (!m_data.contains("transitions")) m_data["transitions"] = json::array();
    if (!m_data.contains("events")) m_data["events"] = json::array();
    m_availableAnimations.clear();
    if (!m_bankRef.empty())
    {
        ReloadAnimationBankMetadata();
    }
    m_isDirty = false;
    return true;
}

bool AnimationGraphDocument::Load(const std::string& filePath)
{
    json root;
    if (!JsonHelper::LoadJsonFromFile(filePath, root))
        return false;
    if (!LoadFromJson(root))
        return false;
    m_filePath = filePath;
    SYSTEM_LOG << "[AnimationGraphDocument] Loaded graph: " << filePath << " bankRef=" << m_bankRef << " clips=" << m_availableAnimations.size() << "\n";
    return true;
}

json AnimationGraphDocument::BuildJson() const
{
    json root = m_data;
    root["schema_version"] = 1;
    root["blueprintType"] = "AnimationGraph";
    root["name"] = m_name;
    root["description"] = m_description;
    root["animationBankRef"] = m_bankRef;
    root["prefabRef"] = m_prefabRef;
    root["animationGraphRef"] = m_animationGraphRef;
    root["defaultState"] = m_defaultState;
    return root;
}

bool AnimationGraphDocument::Save(const std::string& filePath)
{
    const std::string targetPath = filePath.empty() ? m_filePath : filePath;
    if (targetPath.empty())
        return false;

    json root = BuildJson();
    std::ofstream ofs(targetPath.c_str());
    if (!ofs.good())
        return false;
    ofs << root.dump(2);
    m_filePath = targetPath;
    m_isDirty = false;
    SYSTEM_LOG << "[AnimationGraphDocument] Saved graph: " << targetPath << " bankRef=" << m_bankRef << " states=" << (m_data.contains("states") ? m_data["states"].size() : 0) << "\n";
    return true;
}

} // namespace Olympe
