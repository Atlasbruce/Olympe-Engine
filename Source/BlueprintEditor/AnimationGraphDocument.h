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
    AnimationGraphDocument();
    ~AnimationGraphDocument() override;

    bool Load(const std::string& filePath) override;
    bool Save(const std::string& filePath) override;
    bool IsDirty() const override { return m_isDirty; }
    std::string GetName() const override;
    DocumentType GetType() const override { return DocumentType::UNKNOWN; }
    std::string GetFilePath() const override { return m_filePath; }
    void SetFilePath(const std::string& path) override { m_filePath = path; }
    IGraphRenderer* GetRenderer() override;
    const IGraphRenderer* GetRenderer() const override;
    void OnDocumentModified() override { m_isDirty = true; }

    void SetRenderer(IGraphRenderer* renderer);
    void Clear();
    const nlohmann::json& GetData() const { return m_data; }
    nlohmann::json& GetDataMutable() { return m_data; }
    void MarkDefaultContent(const std::string& name);
    void SetAnimationBankPath(const std::string& path);
    const std::string& GetAnimationBankPath() const { return m_bankRef; }
    void SetDefaultState(const std::string& stateName);
    const std::string& GetDefaultState() const { return m_defaultState; }
    bool AddStateFromClip(const std::string& stateName, const std::string& animationName);
    bool RemoveState(const std::string& stateName);
    bool AddTransition(const std::string& fromState, const std::string& toState, float transitionTime);
    bool RemoveTransition(size_t index);
    bool GenerateDefaultTransitions(float transitionTime = 0.1f);
    bool GenerateStatesFromBank();
    bool AutoLayoutStates();
    bool ReloadAnimationBankMetadata();
    const std::vector<std::string>& GetAvailableAnimations() const { return m_availableAnimations; }

private:
    bool LoadFromJson(const nlohmann::json& root);
    nlohmann::json BuildJson() const;

    std::string m_name;
    std::string m_description;
    std::string m_bankRef;
    std::string m_defaultState;
    std::string m_filePath;
    bool m_isDirty;
    nlohmann::json m_data;
    IGraphRenderer* m_renderer;
    std::vector<std::string> m_availableAnimations;
};

} // namespace Olympe
