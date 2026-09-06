#pragma once

#include "Framework/IGraphDocument.h"
#include "IGraphRenderer.h"
#include "../../Source/third_party/nlohmann/json.hpp"
#include <string>
#include <vector>

namespace Olympe {

class AnimationGraphDocument : public IGraphDocument
{
public:
    static const size_t kDirectionCount = 8;

    enum class Direction : int
    {
        North = 0,
        NorthEast = 1,
        East = 2,
        SouthEast = 3,
        South = 4,
        SouthWest = 5,
        West = 6,
        NorthWest = 7
    };

    struct DirectionClipMapping
    {
        std::string direction;
        std::string clip;
    };

    struct StateDefinition
    {
        std::string name;
        std::string defaultClip;
        std::vector<DirectionClipMapping> directionClips;
        std::vector<std::string> events;
        double blendTime = 0.1;
        bool loop = true;
        int priority = 0;
        double x = 0.0;
        double y = 0.0;
    };

    struct TransitionDefinition
    {
        std::string fromState;
        std::string toState;
        std::string condition;
        double duration = 0.1;
        int priority = 0;
        bool interruptible = true;
    };

    struct SourceDefinition
    {
        std::string filePath;
        std::string sourceName;
        std::vector<std::string> clips;
    };

    AnimationGraphDocument();
    ~AnimationGraphDocument() override;

    bool Load(const std::string& filePath) override;
    bool Save(const std::string& filePath) override;
    bool IsDirty() const override { return m_isDirty; }
    std::string GetName() const override;
    DocumentType GetType() const override { return DocumentType::ANIMATION_GRAPH; }
    std::string GetFilePath() const override { return m_filePath; }
    void SetFilePath(const std::string& path) override { m_filePath = path; }
    IGraphRenderer* GetRenderer() override;
    const IGraphRenderer* GetRenderer() const override;
    void OnDocumentModified() override { m_isDirty = true; }

    void SetRenderer(IGraphRenderer* renderer);
    void Clear();
    const nlohmann::json& GetData() const { return m_data; }
    nlohmann::json& GetDataMutable() { return m_data; }

    void SetName(const std::string& name);
    void SetDescription(const std::string& description);
    void SetDefaultState(const std::string& stateName);
    const std::string& GetDefaultState() const { return m_defaultState; }

    void AddSource(const std::string& filePath);
    void RemoveSource(size_t index);
    bool HasSource(const std::string& filePath) const;
    const std::vector<SourceDefinition>& GetSources() const { return m_sources; }
    std::vector<std::string> GetSourceNames() const;
    std::vector<std::string> GetSourceClipNames(size_t index) const;

    std::string MakeUniqueStateName(const std::string& baseName) const;
    bool AddState(const std::string& stateName);
    bool AddStateFromClip(const std::string& stateName, const std::string& clipName);
    bool RenameState(const std::string& oldName, const std::string& newName);
    bool RemoveState(const std::string& stateName);
    bool SetStatePosition(const std::string& stateName, double x, double y);
    bool SetStateDefaultClip(const std::string& stateName, const std::string& clipName);
    bool SetStateDirectionClip(const std::string& stateName, Direction direction, const std::string& clipName);
    bool AddStateEvent(const std::string& stateName, const std::string& eventName);
    bool RemoveStateEvent(const std::string& stateName, const std::string& eventName);

    bool AddTransition(const std::string& fromState, const std::string& toState, const std::string& condition, double duration);
    bool RemoveTransition(size_t index);

    int FindStateIndex(const std::string& stateName) const;
    std::string GetStateNameAt(size_t index) const;
    const StateDefinition* GetState(const std::string& stateName) const;
    StateDefinition* GetState(const std::string& stateName);

    std::string ResolveClipForState(const std::string& stateName, Direction direction) const;
    bool IsDirectionValid(Direction direction) const;
    static std::string DirectionToString(Direction direction);
    static Direction StringToDirection(const std::string& text, bool* ok = nullptr);

    std::vector<std::string> ValidateBinding() const;
    nlohmann::json BuildRuntimeJson() const;
    bool ExportRuntimeJson(const std::string& filePath) const;

    bool ImportTSXSource(const std::string& filePath, std::vector<std::string>* errors = nullptr);
    bool ImportTSXDirectory(const std::string& directoryPath, std::vector<std::string>* errors = nullptr);
    bool ImportTSXFile(const std::string& filePath, std::vector<std::string>* errors = nullptr);
    std::vector<std::string> ValidateImportedSources() const;
    std::vector<std::string> GetAllKnownClips() const;

private:
    bool LoadFromJson(const nlohmann::json& root);
    nlohmann::json BuildJson() const;
    void UpdateStateNamesInReferences(const std::string& oldName, const std::string& newName);
    void RebuildAvailableClips();
    static std::string NormalizePath(const std::string& path);
    static std::string ExtractClipNameFromTsx(const nlohmann::json& root, const std::string& fallbackName);

    std::string m_name;
    std::string m_description;
    std::string m_filePath;
    std::string m_defaultState;
    bool m_isDirty;
    nlohmann::json m_data;
    IGraphRenderer* m_renderer;

    std::vector<SourceDefinition> m_sources;
    std::vector<StateDefinition> m_states;
    std::vector<TransitionDefinition> m_transitions;
    std::vector<std::string> m_availableClips;
    bool m_suppressDirtyNotifications;
    int m_nextNodeId;
};

} // namespace Olympe
