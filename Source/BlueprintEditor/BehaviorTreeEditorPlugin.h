/*
 * Olympe Blueprint Editor - Behavior Tree Plugin
 * 
 * Plugin for editing AI Behavior Tree blueprints
 */

#pragma once

#include "BlueprintEditorPlugin.h"
#include <map>

namespace Olympe
{
    class BehaviorTreeEditorPlugin : public BlueprintEditorPlugin
    {
    public:
        BehaviorTreeEditorPlugin();
        virtual ~BehaviorTreeEditorPlugin();
        
        // Identification
        std::string GetBlueprintType() const override { return "BehaviorTree"; }
        std::string GetDisplayName() const override { return "Behavior Tree"; }
        std::string GetDescription() const override { return "AI Behavior Tree with nodes and decorators"; }
        std::string GetIconPath() const override { return "Resources/Icons/behavior_tree.png"; }
        std::string GetDefaultFolder() const override { return "Blueprints/AI/"; }
        
        // Lifecycle
        nlohmann::json CreateNew(const std::string& name) override;
        bool CanHandle(const nlohmann::json& blueprint) const override;
        std::vector<ValidationError> Validate(const nlohmann::json& blueprint) override;
        
        // Rendering
        void RenderEditor(nlohmann::json& blueprintData, EditorContext_st& ctx) override;
        void RenderProperties(const nlohmann::json& blueprintData) override;
        void RenderToolbar(nlohmann::json& blueprintData) override;
        
    private:
        void RenderBehaviorTreeGraph(nlohmann::json& data, EditorContext_st& ctx);
        std::string GetCurrentTimestamp();
    };
}
