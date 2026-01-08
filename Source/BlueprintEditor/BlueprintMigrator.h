/*
 * Olympe Blueprint Editor - Blueprint Migrator
 * 
 * Migrates v1 blueprints to v2 format with:
 * - schema_version and blueprintType fields
 * - Calculated node positions
 * - Unified parameters structure
 * - Metadata and editor state
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <queue>
#include "../../Source/third_party/nlohmann/json.hpp"

// Forward declare ImVec2
struct ImVec2;

namespace Olympe
{
    /**
     * BlueprintMigrator - Converts v1 blueprints to v2 format
     * Handles automatic position calculation and structure migration
     */
    class BlueprintMigrator
    {
    public:
        struct NodeLayout
        {
            int nodeId;
            int depth;
            int siblingIndex;
            float posX;
            float posY;
        };
        
        BlueprintMigrator();
        ~BlueprintMigrator();
        
        // Main migration function
        nlohmann::json MigrateToV2(const nlohmann::json& v1Blueprint);
        
        // Check if blueprint is already v2
        bool IsV2(const nlohmann::json& blueprint) const;
        
    private:
        // Type detection
        std::string DetectBlueprintType(const nlohmann::json& blueprint);
        
        // Type-specific migration
        void MigrateBehaviorTree(const nlohmann::json& v1, nlohmann::json& v2Data);
        void MigrateHFSM(const nlohmann::json& v1, nlohmann::json& v2Data);
        void MigrateEntityPrefab(const nlohmann::json& v1, nlohmann::json& v2Data);
        
        // Position calculation
        std::map<int, NodeLayout> CalculateHierarchicalLayout(
            const nlohmann::json& nodes,
            const std::map<int, std::vector<int>>& childrenMap,
            int rootId);
        
        // Utilities
        std::string GetCurrentTimestamp();
        void MigrateParameters(const nlohmann::json& v1Node, nlohmann::json& v2Node);
        
        // Layout constants
        static constexpr float HORIZONTAL_SPACING = 350.0f;
        static constexpr float VERTICAL_SPACING = 250.0f;
        static constexpr float START_X = 200.0f;
        static constexpr float START_Y = 300.0f;
    };
}
