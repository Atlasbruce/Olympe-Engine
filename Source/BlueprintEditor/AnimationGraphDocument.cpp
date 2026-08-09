#include "AnimationGraphDocument.h"
#include "../../Source/json_helper.h"
#include "../../Source/system/system_utils.h"
#include <fstream>

using json = nlohmann::json;

namespace Olympe {

AnimationGraphDocument::AnimationGraphDocument()
    : m_isDirty(false)
    , m_renderer(nullptr)
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
    m_data["defaultState"] = "Idle";
}

void AnimationGraphDocument::SetAnimationBankPath(const std::string& path)
{
    m_bankRef = path;
    m_data["animationBankRef"] = path;
    ReloadAnimationBankMetadata();
    m_isDirty = true;
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

    m_data["states"].push_back(state);
    if (m_defaultState.empty())
        SetDefaultState(stateName);
    m_isDirty = true;
    return true;
}

bool AnimationGraphDocument::ReloadAnimationBankMetadata()
{
    m_availableAnimations.clear();
    if (m_bankRef.empty())
        return false;

    json root;
    if (!JsonHelper::LoadJsonFromFile(m_bankRef, root))
        return false;

    if (!root.contains("animations") || !root["animations"].is_array())
        return false;

    const nlohmann::json& animations = root["animations"];
    for (size_t i = 0; i < animations.size(); ++i)
    {
        if (animations[i].contains("name") && animations[i]["name"].is_string())
            m_availableAnimations.push_back(animations[i]["name"].get<std::string>());
    }

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
    m_defaultState = JsonHelper::GetString(root, "defaultState", "Idle");
    m_data = root;
    if (!m_data.contains("states")) m_data["states"] = json::array();
    if (!m_data.contains("transitions")) m_data["transitions"] = json::array();
    if (!m_data.contains("events")) m_data["events"] = json::array();
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
    return true;
}

} // namespace Olympe
