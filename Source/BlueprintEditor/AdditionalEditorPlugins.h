/*
 * Olympe Blueprint Editor - HFSM Plugin
 * 
 * Plugin for editing Hierarchical Finite State Machine blueprints
 */

#pragma once

#include "BlueprintEditorPlugin.h"

namespace Olympe
{
    class HFSMEditorPlugin : public BlueprintEditorPlugin
    {
    public:
        HFSMEditorPlugin() {}
        virtual ~HFSMEditorPlugin() {}
        
        std::string GetBlueprintType() const override { return "HFSM"; }
        std::string GetDisplayName() const override { return "Hierarchical FSM"; }
        std::string GetDescription() const override { return "Hierarchical Finite State Machine for AI"; }
        std::string GetIconPath() const override { return "Resources/Icons/hfsm.png"; }
        std::string GetDefaultFolder() const override { return "Blueprints/AI/"; }
        
        nlohmann::json CreateNew(const std::string& name) override;
        bool CanHandle(const nlohmann::json& blueprint) const override;
        std::vector<ValidationError> Validate(const nlohmann::json& blueprint) override;
        
        void RenderEditor(nlohmann::json& blueprintData, EditorContext& ctx) override;
        void RenderProperties(const nlohmann::json& blueprintData) override;
        void RenderToolbar(nlohmann::json& blueprintData) override;
    };
    
    class AnimationGraphEditorPlugin : public BlueprintEditorPlugin
    {
    public:
        AnimationGraphEditorPlugin() {}
        virtual ~AnimationGraphEditorPlugin() {}
        
        std::string GetBlueprintType() const override { return "AnimationGraph"; }
        std::string GetDisplayName() const override { return "Animation Graph"; }
        std::string GetDescription() const override { return "Animation state machine with transitions"; }
        std::string GetIconPath() const override { return "Resources/Icons/animation.png"; }
        std::string GetDefaultFolder() const override { return "Blueprints/Animations/"; }
        
        nlohmann::json CreateNew(const std::string& name) override;
        bool CanHandle(const nlohmann::json& blueprint) const override;
        std::vector<ValidationError> Validate(const nlohmann::json& blueprint) override;
        
        void RenderEditor(nlohmann::json& blueprintData, EditorContext& ctx) override;
        void RenderProperties(const nlohmann::json& blueprintData) override;
        void RenderToolbar(nlohmann::json& blueprintData) override;
    };
    
    class ScriptedEventEditorPlugin : public BlueprintEditorPlugin
    {
    public:
        ScriptedEventEditorPlugin() {}
        virtual ~ScriptedEventEditorPlugin() {}
        
        std::string GetBlueprintType() const override { return "ScriptedEvent"; }
        std::string GetDisplayName() const override { return "Scripted Event"; }
        std::string GetDescription() const override { return "Scripted event sequence"; }
        std::string GetIconPath() const override { return "Resources/Icons/event.png"; }
        std::string GetDefaultFolder() const override { return "Blueprints/ScriptedEvents/"; }
        
        nlohmann::json CreateNew(const std::string& name) override;
        bool CanHandle(const nlohmann::json& blueprint) const override;
        std::vector<ValidationError> Validate(const nlohmann::json& blueprint) override;
        
        void RenderEditor(nlohmann::json& blueprintData, EditorContext& ctx) override;
        void RenderProperties(const nlohmann::json& blueprintData) override;
        void RenderToolbar(nlohmann::json& blueprintData) override;
    };
    
    class LevelDefinitionEditorPlugin : public BlueprintEditorPlugin
    {
    public:
        LevelDefinitionEditorPlugin() {}
        virtual ~LevelDefinitionEditorPlugin() {}
        
        std::string GetBlueprintType() const override { return "LevelDefinition"; }
        std::string GetDisplayName() const override { return "Level Definition"; }
        std::string GetDescription() const override { return "Level layout and entity placement"; }
        std::string GetIconPath() const override { return "Resources/Icons/level.png"; }
        std::string GetDefaultFolder() const override { return "Blueprints/Levels/"; }
        
        nlohmann::json CreateNew(const std::string& name) override;
        bool CanHandle(const nlohmann::json& blueprint) const override;
        std::vector<ValidationError> Validate(const nlohmann::json& blueprint) override;
        
        void RenderEditor(nlohmann::json& blueprintData, EditorContext& ctx) override;
        void RenderProperties(const nlohmann::json& blueprintData) override;
        void RenderToolbar(nlohmann::json& blueprintData) override;
    };
    
    class UIMenuEditorPlugin : public BlueprintEditorPlugin
    {
    public:
        UIMenuEditorPlugin() {}
        virtual ~UIMenuEditorPlugin() {}
        
        std::string GetBlueprintType() const override { return "UIMenu"; }
        std::string GetDisplayName() const override { return "UI Menu"; }
        std::string GetDescription() const override { return "User interface menu layout"; }
        std::string GetIconPath() const override { return "Resources/Icons/ui_menu.png"; }
        std::string GetDefaultFolder() const override { return "Blueprints/UI/"; }
        
        nlohmann::json CreateNew(const std::string& name) override;
        bool CanHandle(const nlohmann::json& blueprint) const override;
        std::vector<ValidationError> Validate(const nlohmann::json& blueprint) override;
        
        void RenderEditor(nlohmann::json& blueprintData, EditorContext& ctx) override;
        void RenderProperties(const nlohmann::json& blueprintData) override;
        void RenderToolbar(nlohmann::json& blueprintData) override;
    };
}
