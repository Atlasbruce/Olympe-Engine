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
#include "BTNodeGraphManager.h"
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
        BlueprintValidator()
        {
        }

        ~BlueprintValidator()
        {
        }

        // Node graph validation (deprecated - Phase 50.3)
        // TODO: Reimplement with NodeGraphTypes::GraphDocument
        std::vector<ValidationError> ValidateGraph(const Olympe::NodeGraphTypes::GraphDocument* graph);
        std::vector<ValidationError> ValidateNode(const Olympe::NodeGraphTypes::GraphDocument* graph, Olympe::NodeGraphTypes::NodeId nodeId);
        bool IsGraphValid(const Olympe::NodeGraphTypes::GraphDocument* graph);
        int GetErrorCount(const std::vector<ValidationError>& errors, ErrorSeverity severity) const
        {
            int count = 0;
            for (const auto& error : errors)
            {
                if (error.severity == severity)
                    count++;
            }
            return count;
        }

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
        static const char* SeverityToString(ErrorSeverity severity)
        {
            switch (severity)
            {
                case ErrorSeverity::Info:     return "Info";
                case ErrorSeverity::Warning:  return "Warning";
                case ErrorSeverity::Error:    return "Error";
                case ErrorSeverity::Critical: return "Critical";
                default:                      return "Unknown";
            }
        }

        static ImVec4 SeverityToColor(ErrorSeverity severity)
        {
            switch (severity)
            {
                case ErrorSeverity::Info:     return ImVec4(0.2f, 0.8f, 1.0f, 1.0f);
                case ErrorSeverity::Warning:  return ImVec4(1.0f, 1.0f, 0.2f, 1.0f);
                case ErrorSeverity::Error:    return ImVec4(1.0f, 0.4f, 0.2f, 1.0f);
                case ErrorSeverity::Critical: return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
                default:                      return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }

    private:
        // Validation helpers (existing)
        void ValidateNodeType(const Olympe::NodeGraphTypes::GraphDocument* graph, const Olympe::NodeGraphTypes::NodeData* node, 
                             std::vector<ValidationError>& errors);
        void ValidateNodeParameters(const Olympe::NodeGraphTypes::GraphDocument* graph, const Olympe::NodeGraphTypes::NodeData* node, 
                                   std::vector<ValidationError>& errors);
        void ValidateNodeLinks(const Olympe::NodeGraphTypes::GraphDocument* graph, const Olympe::NodeGraphTypes::NodeData* node, 
                              std::vector<ValidationError>& errors);
        
        // JSON validation helpers (new)
        bool ValidateBehaviorTree(const nlohmann::json& blueprint, std::string& errors);
        bool ValidateHFSM(const nlohmann::json& blueprint, std::string& errors);
        bool ValidateEntityPrefab(const nlohmann::json& blueprint, std::string& errors);
        bool ValidateUIBlueprint(const nlohmann::json& blueprint, std::string& errors);
        bool ValidateLevel(const nlohmann::json& blueprint, std::string& errors);
    };
}
