/*
 * Olympe Blueprint Editor - Blueprint Validator
 * 
 * Validates blueprint integrity and enforces catalog consistency
 * Provides error reporting for invalid node types and parameters
 * Also handles JSON schema validation and normalization
 */

#pragma once

#include <string>
#include <vector>
#include "NodeGraphManager.h"
#include "BTConnectionValidator.h"
#include "../../Source/third_party/imgui/imgui.h"
#include "../../Source/third_party/nlohmann/json.hpp"

namespace Olympe
{
    // Validation error severity levels
    enum class ErrorSeverity
    {
        Info,       // Informational message
        Warning,    // Non-critical issue
        Error,      // Critical issue that should be fixed
        Critical    // Blocking issue that prevents execution
    };

    // Validation error structure
    struct ValidationError
    {
        int nodeId = -1;                    // Node ID with the error (-1 for graph-level errors)
        std::string nodeName;               // Node name for display
        std::string message;                // Error message
        ErrorSeverity severity = ErrorSeverity::Error;
        std::string category;               // Error category (e.g., "Type", "Parameter", "Link")
        
        ValidationError() = default;
        ValidationError(int nId, const std::string& nName, const std::string& msg, 
                       ErrorSeverity sev = ErrorSeverity::Error, const std::string& cat = "")
            : nodeId(nId), nodeName(nName), message(msg), severity(sev), category(cat) {}
    };

    /**
     * BlueprintValidator - Validates node graphs against catalogs
     * Ensures type safety and parameter consistency
     * Also provides JSON schema validation and normalization
     */
    class BlueprintValidator
    {
    public:
        BlueprintValidator();
        ~BlueprintValidator();

        // Node graph validation (existing)
        std::vector<ValidationError> ValidateGraph(const NodeGraph* graph);
        std::vector<ValidationError> ValidateNode(const NodeGraph* graph, int nodeId);
        bool IsGraphValid(const NodeGraph* graph);
        int GetErrorCount(const std::vector<ValidationError>& errors, ErrorSeverity severity) const;
        
        // JSON schema validation and normalization (new)
        // Detect blueprint type from JSON structure using heuristics
        std::string DetectType(const nlohmann::json& blueprint);
        
        // Normalize JSON to ensure required fields exist
        // Returns true if changes were made
        bool Normalize(nlohmann::json& blueprint);
        
        // Validate JSON against per-type required fields
        // Returns true if valid, fills errors string if not
        bool ValidateJSON(const nlohmann::json& blueprint, std::string& errors);
        
        // Severity to string conversion
        static const char* SeverityToString(ErrorSeverity severity);
        static ImVec4 SeverityToColor(ErrorSeverity severity);

    private:
        // Validation helpers (existing)
        void ValidateNodeType(const NodeGraph* graph, const GraphNode* node, 
                             std::vector<ValidationError>& errors);
        void ValidateNodeParameters(const NodeGraph* graph, const GraphNode* node, 
                                   std::vector<ValidationError>& errors);
        void ValidateNodeLinks(const NodeGraph* graph, const GraphNode* node, 
                              std::vector<ValidationError>& errors);
        
        // Connection validation helpers (new)
        void ValidateConnectionRules(const NodeGraph* graph, 
                                    std::vector<ValidationError>& errors);
        void ValidateMultipleParents(const NodeGraph* graph, 
                                    std::vector<ValidationError>& errors);
        void ValidateCycles(const NodeGraph* graph, 
                           std::vector<ValidationError>& errors);
        void ValidateOrphanNodes(const NodeGraph* graph, 
                                std::vector<ValidationError>& errors);
        void ValidateRootNodes(const NodeGraph* graph, 
                              std::vector<ValidationError>& errors);
        
        // JSON validation helpers (new)
        bool ValidateBehaviorTree(const nlohmann::json& blueprint, std::string& errors);
        bool ValidateHFSM(const nlohmann::json& blueprint, std::string& errors);
        bool ValidateEntityPrefab(const nlohmann::json& blueprint, std::string& errors);
        bool ValidateUIBlueprint(const nlohmann::json& blueprint, std::string& errors);
        bool ValidateLevel(const nlohmann::json& blueprint, std::string& errors);
        
        BTConnectionValidator m_ConnectionValidator;
    };
}
