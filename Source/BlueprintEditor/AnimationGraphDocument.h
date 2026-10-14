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

    struct EventDefinition
    {
        std::string type = "Custom";
        // Stable across direction variants with different frame counts.
        // The editor displays this phase as frames and seconds.
        float phase = 0.0f;
        std::string payload;
    };

    struct StateDefinition
    {
        std::string name;
        std::string defaultClip;
        std::vector<DirectionClipMapping> directionClips;
        std::vector<EventDefinition> events;
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
        struct FrameDefinition
        {
            int tileId = 0;
            int durationMs = 0;
        };

        struct ClipDefinition
        {
            std::string id;
            std::string group;
            std::string direction;
            bool loop = true;
            int sourceTileId = -1;
            std::vector<FrameDefinition> frames;
        };

        std::string filePath;
        std::string sourceName;
        // Atlas metadata is preserved from TSX so the editor preview and the
        // runtime can address an exact tile/frame without reparsing the TSX.
        std::string imagePath;
        int imageWidth = 0;
        int imageHeight = 0;
        int tileWidth = 0;
        int tileHeight = 0;
        int columns = 0;
        int spacing = 0;
        int margin = 0;
        std::vector<ClipDefinition> clips;
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
    // Instantiates one semantic state from a TSX animation bank.  Directional
    // clips remain variants of this state and are selected by the runtime.
    bool AddStateFromSource(const std::string& stateName, size_t sourceIndex);
    bool RenameState(const std::string& oldName, const std::string& newName);
    bool RemoveState(const std::string& stateName);
    // Removes a selection as one model mutation.  This is intentionally not
    // implemented by repeatedly calling RemoveState from a renderer: all
    // incident transitions and the default-state fallback must be resolved
    // against the same pre-edit model for an undoable multi-selection delete.
    bool RemoveStates(const std::vector<std::string>& stateNames);
    bool SetStatePosition(const std::string& stateName, double x, double y);
    bool SetStateDefaultClip(const std::string& stateName, const std::string& clipName);
    bool SetStateDirectionClip(const std::string& stateName, Direction direction, const std::string& clipName);
    bool AddStateEvent(const std::string& stateName, const std::string& eventName);
    bool RemoveStateEvent(const std::string& stateName, const std::string& eventName);
    bool AddStateEventMarker(const std::string& stateName, const EventDefinition& event);
    bool UpdateStateEventMarker(const std::string& stateName, size_t eventIndex, const EventDefinition& event);
    bool RemoveStateEventMarker(const std::string& stateName, size_t eventIndex);

    bool AddTransition(const std::string& fromState, const std::string& toState, const std::string& condition, double duration);
    bool UpdateTransition(size_t index, const std::string& condition, double duration, int priority, bool interruptible);
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

    /** Complete document snapshots are used by the shared command history. */
    nlohmann::json CreateSnapshot() const;
    bool RestoreSnapshot(const nlohmann::json& snapshot);

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
