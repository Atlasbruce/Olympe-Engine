#pragma once

#include "../AI/BehaviorTree.h"
#include "../AI/BTGraphLayoutEngine.h"
#include "../third_party/nlohmann/json.hpp"
#include <string>

namespace Olympe
{
    namespace NodeGraphShared
    {
        using json = nlohmann::json;

        // Serialize a BehaviorTreeAsset to JSON using layout positions from the
        // provided layout engine (may be null).
        inline json SerializeBehaviorTreeToJson(const BehaviorTreeAsset& tree, const BTGraphLayoutEngine* layoutEngine)
        {
            json j = json::object();

            j["schema_version"] = 2;
            j["type"] = "BehaviorTree";
            j["blueprintType"] = "BehaviorTree";
            j["name"] = tree.name;
            j["description"] = "";

            json metadata = json::object();
            metadata["author"] = "BT Editor";
            // Simple timestamp
            time_t now = time(nullptr);
            struct tm timeinfo;
        #ifdef _WIN32
            localtime_s(&timeinfo, &now);
        #else
            localtime_r(&now, &timeinfo);
        #endif
            char buffer[32];
            strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
            metadata["created"] = std::string(buffer);
            metadata["lastModified"] = std::string(buffer);

            json tags = json::array();
            tags.push_back("AI");
            tags.push_back("BehaviorTree");
            metadata["tags"] = tags;

            j["metadata"] = metadata;

            json editorState = json::object();
            editorState["zoom"] = 1.0;
            json scrollOffset = json::object();
            scrollOffset["x"] = 0.0;
            scrollOffset["y"] = 0.0;
            editorState["scrollOffset"] = scrollOffset;
            j["editorState"] = editorState;

            json data = json::object();
            data["rootNodeId"] = static_cast<int>(tree.rootNodeId);

            json nodesArray = json::array();
            for (const auto& node : tree.nodes)
            {
                json nodeJson = json::object();
                nodeJson["id"] = static_cast<int>(node.id);
                nodeJson["name"] = node.name;

                const char* typeStr = "";
                switch (node.type)
                {
                case BTNodeType::Selector: typeStr = "Selector"; break;
                case BTNodeType::Sequence: typeStr = "Sequence"; break;
                case BTNodeType::Condition: typeStr = "Condition"; break;
                case BTNodeType::Action: typeStr = "Action"; break;
                case BTNodeType::Inverter: typeStr = "Inverter"; break;
                case BTNodeType::Repeater: typeStr = "Repeater"; break;
                }
                nodeJson["type"] = typeStr;

                json pos = json::object();

                // Phase 38: Use stored editorPosX/Y if available, otherwise try layoutEngine, else auto-layout
                if (node.editorPosX != 0.0f || node.editorPosY != 0.0f)
                {
                    // Use stored positions (from editor canvas)
                    pos["x"] = node.editorPosX;
                    pos["y"] = node.editorPosY;
                }
                else if (layoutEngine)
                {
                    const BTNodeLayout* nodeLayout = layoutEngine->GetNodeLayout(node.id);
                    if (nodeLayout)
                    {
                        pos["x"] = nodeLayout->position.x;
                        pos["y"] = nodeLayout->position.y;
                    }
                    else
                    {
                        pos["x"] = 200.0f;
                        pos["y"] = 100.0f * static_cast<float>(node.id);
                    }
                }
                else
                {
                    pos["x"] = 200.0f;
                    pos["y"] = 100.0f * static_cast<float>(node.id);
                }
                nodeJson["position"] = pos;

                if (node.type == BTNodeType::Selector || node.type == BTNodeType::Sequence)
                {
                    json children = json::array();
                    for (uint32_t cid : node.childIds)
                        children.push_back(static_cast<int>(cid));
                    nodeJson["children"] = children;
                }

                if (node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater)
                {
                    if (node.decoratorChildId != 0)
                        nodeJson["decoratorChildId"] = static_cast<int>(node.decoratorChildId);
                    if (node.type == BTNodeType::Repeater)
                        nodeJson["repeatCount"] = node.repeatCount;
                }

                if (node.type == BTNodeType::Action)
                {
                    const char* actionTypeStr = "";
                    switch (node.actionType)
                    {
                    case BTActionType::SetMoveGoalToLastKnownTargetPos: actionTypeStr = "SetMoveGoalToLastKnownTargetPos"; break;
                    case BTActionType::SetMoveGoalToTarget: actionTypeStr = "SetMoveGoalToTarget"; break;
                    case BTActionType::SetMoveGoalToPatrolPoint: actionTypeStr = "SetMoveGoalToPatrolPoint"; break;
                    case BTActionType::MoveToGoal: actionTypeStr = "MoveToGoal"; break;
                    case BTActionType::AttackIfClose: actionTypeStr = "AttackIfClose"; break;
                    case BTActionType::PatrolPickNextPoint: actionTypeStr = "PatrolPickNextPoint"; break;
                    case BTActionType::ClearTarget: actionTypeStr = "ClearTarget"; break;
                    case BTActionType::Idle: actionTypeStr = "Idle"; break;
                    case BTActionType::WaitRandomTime: actionTypeStr = "WaitRandomTime"; break;
                    case BTActionType::ChooseRandomNavigablePoint: actionTypeStr = "ChooseRandomNavigablePoint"; break;
                    case BTActionType::RequestPathfinding: actionTypeStr = "RequestPathfinding"; break;
                    case BTActionType::FollowPath: actionTypeStr = "FollowPath"; break;
                    }
                    nodeJson["actionType"] = actionTypeStr;

                    json params = json::object();
                    params["param1"] = node.actionParam1;
                    params["param2"] = node.actionParam2;
                    nodeJson["parameters"] = params;
                }

                if (node.type == BTNodeType::Condition)
                {
                    const char* conditionTypeStr = "";
                    switch (node.conditionType)
                    {
                    case BTConditionType::TargetVisible: conditionTypeStr = "TargetVisible"; break;
                    case BTConditionType::TargetInRange: conditionTypeStr = "TargetInRange"; break;
                    case BTConditionType::HealthBelow: conditionTypeStr = "HealthBelow"; break;
                    case BTConditionType::HasMoveGoal: conditionTypeStr = "HasMoveGoal"; break;
                    case BTConditionType::CanAttack: conditionTypeStr = "CanAttack"; break;
                    case BTConditionType::HeardNoise: conditionTypeStr = "HeardNoise"; break;
                    case BTConditionType::IsWaitTimerExpired: conditionTypeStr = "IsWaitTimerExpired"; break;
                    case BTConditionType::HasNavigableDestination: conditionTypeStr = "HasNavigableDestination"; break;
                    case BTConditionType::HasValidPath: conditionTypeStr = "HasValidPath"; break;
                    case BTConditionType::HasReachedDestination: conditionTypeStr = "HasReachedDestination"; break;
                    }
                    nodeJson["conditionType"] = conditionTypeStr;

                    json params = json::object();
                    params["param"] = node.conditionParam;
                    nodeJson["parameters"] = params;
                }

                if (!nodeJson.contains("parameters"))
                    nodeJson["parameters"] = json::object();

                nodesArray.push_back(nodeJson);
            }

            data["nodes"] = nodesArray;
            j["data"] = data;

            return j;
        }
    }
}
