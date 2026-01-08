/*
 * Olympe Blueprint Editor - Entity Prefab Plugin
 * 
 * Plugin for editing Entity Prefab blueprints
 */

#pragma once

#include "BlueprintEditorPlugin.h"

namespace Olympe
{
    class EntityPrefabEditorPlugin : public BlueprintEditorPlugin
    {
    public:
        EntityPrefabEditorPlugin();
        virtual ~EntityPrefabEditorPlugin();
        
        // Identification
        std::string GetBlueprintType() const override { return "EntityPrefab"; }
        std::string GetDisplayName() const override { return "Entity Prefab"; }
        std::string GetDescription() const override { return "Entity template with components"; }
        std::string GetIconPath() const override { return "Resources/Icons/entity_prefab.png"; }
        std::string GetDefaultFolder() const override { return "Blueprints/EntityPrefab/"; }
        
        // Lifecycle
        nlohmann::json CreateNew(const std::string& name) override;
        bool CanHandle(const nlohmann::json& blueprint) const override;
        std::vector<ValidationError> Validate(const nlohmann::json& blueprint) override;
        
        // Rendering
        void RenderEditor(nlohmann::json& blueprintData, EditorContext& ctx) override;
        void RenderProperties(const nlohmann::json& blueprintData) override;
        void RenderToolbar(nlohmann::json& blueprintData) override;
        
    private:
        void RenderComponentPropertiesEditor(nlohmann::json& properties, EditorContext& ctx);
        void AddComponentToBlueprint(nlohmann::json& blueprintData, const std::string& type);
        std::vector<std::string> GetAvailableComponentTypes();
        std::string GetCurrentTimestamp();
    };
}
