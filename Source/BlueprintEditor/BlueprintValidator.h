/*
 * Olympe Blueprint Editor - Blueprint Validator
 * 
 * Validates blueprint integrity and enforces catalog consistency
 * Provides error reporting for invalid node types and parameters
 */

#pragma once

#include <string>
#include <vector>
#include "NodeGraphManager.h"
#include "../../Source/third_party/imgui/imgui.h"

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
     */
    class BlueprintValidator
    {
    public:
        BlueprintValidator();
        ~BlueprintValidator();

        // Validate a complete graph
        std::vector<ValidationError> ValidateGraph(const NodeGraph* graph);
        
        // Validate a single node
        std::vector<ValidationError> ValidateNode(const NodeGraph* graph, int nodeId);
        
        // Quick validation check (returns true if no errors)
        bool IsGraphValid(const NodeGraph* graph);
        
        // Get error count by severity
        int GetErrorCount(const std::vector<ValidationError>& errors, ErrorSeverity severity) const;
        
        // Severity to string conversion
        static const char* SeverityToString(ErrorSeverity severity);
        static ImVec4 SeverityToColor(ErrorSeverity severity);

    private:
        // Validation helpers
        void ValidateNodeType(const NodeGraph* graph, const GraphNode* node, 
                             std::vector<ValidationError>& errors);
        void ValidateNodeParameters(const NodeGraph* graph, const GraphNode* node, 
                                   std::vector<ValidationError>& errors);
        void ValidateNodeLinks(const NodeGraph* graph, const GraphNode* node, 
                              std::vector<ValidationError>& errors);
    };
}
