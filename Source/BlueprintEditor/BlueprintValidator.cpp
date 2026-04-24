#include "BlueprintValidator.h"
#include <algorithm>

namespace Olympe
{
    // Phase 50.3: Constructor, destructor, SeverityToString, SeverityToColor, GetErrorCount
    // are inlined in BlueprintValidator.h for symbol generation

    std::vector<ValidationError> BlueprintValidator::ValidateGraph(const Olympe::NodeGraphTypes::GraphDocument* graph)
    {
        std::vector<ValidationError> errors;
        if (!graph)
            errors.push_back(ValidationError(-1, "", "Graph is null", ErrorSeverity::Critical));
        return errors;
    }

    std::vector<ValidationError> BlueprintValidator::ValidateNode(const Olympe::NodeGraphTypes::GraphDocument* graph, Olympe::NodeGraphTypes::NodeId nodeId)
    {
        std::vector<ValidationError> errors;
        return errors;
    }

    bool BlueprintValidator::IsGraphValid(const Olympe::NodeGraphTypes::GraphDocument* graph)
    {
        return graph != nullptr;
    }

    std::string BlueprintValidator::DetectType(const nlohmann::json& blueprint)
    {
        if (blueprint.contains("nodes") && blueprint.contains("links"))
            return "BehaviorTree";
        if (blueprint.contains("nodes") && blueprint.contains("connections"))
            return "EntityPrefab";
        return "Unknown";
    }

    bool BlueprintValidator::Normalize(nlohmann::json& blueprint)
    {
        bool modified = false;
        if (!blueprint.contains("schemaVersion"))
        {
            blueprint["schemaVersion"] = 4;
            modified = true;
        }
        if (!blueprint.contains("graphKind"))
        {
            blueprint["graphKind"] = DetectType(blueprint);
            modified = true;
        }
        return modified;
    }

    bool BlueprintValidator::ValidateJSON(const nlohmann::json& blueprint, std::string& errors)
    {
        std::string type = DetectType(blueprint);
        if (type == "BehaviorTree")
            return ValidateBehaviorTree(blueprint, errors);
        if (type == "EntityPrefab")
            return ValidateEntityPrefab(blueprint, errors);
        return true;
    }

    void BlueprintValidator::ValidateNodeType(const Olympe::NodeGraphTypes::GraphDocument* graph, const Olympe::NodeGraphTypes::NodeData* node, 
                                             std::vector<ValidationError>& errors)
    {
    }

    void BlueprintValidator::ValidateNodeParameters(const Olympe::NodeGraphTypes::GraphDocument* graph, const Olympe::NodeGraphTypes::NodeData* node, 
                                                   std::vector<ValidationError>& errors)
    {
    }

    void BlueprintValidator::ValidateNodeLinks(const Olympe::NodeGraphTypes::GraphDocument* graph, const Olympe::NodeGraphTypes::NodeData* node, 
                                              std::vector<ValidationError>& errors)
    {
    }

    bool BlueprintValidator::ValidateBehaviorTree(const nlohmann::json& blueprint, std::string& errors)
    {
        if (!blueprint.contains("nodes"))
        {
            errors = "Missing 'nodes' field";
            return false;
        }
        return true;
    }

    bool BlueprintValidator::ValidateHFSM(const nlohmann::json& blueprint, std::string& errors)
    {
        return true;
    }

    bool BlueprintValidator::ValidateEntityPrefab(const nlohmann::json& blueprint, std::string& errors)
    {
        if (!blueprint.contains("nodes"))
        {
            errors = "Missing 'nodes' field";
            return false;
        }
        return true;
    }

    bool BlueprintValidator::ValidateUIBlueprint(const nlohmann::json& blueprint, std::string& errors)
    {
        return true;
    }

    bool BlueprintValidator::ValidateLevel(const nlohmann::json& blueprint, std::string& errors)
    {
        return true;
    }
}
