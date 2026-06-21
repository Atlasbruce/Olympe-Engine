/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

Behavior Tree implementation: JSON loading and built-in node execution.

*/

#include "BehaviorTree.h"
#include "BehaviorTreeDependencyScanner.h"
#include "../ECS_Components_AI.h"
#include "../ECS_Components.h"
#include "../World.h"
// Avoid including full debug window header here to keep compilation units independent.
// Use a tiny API header to interact with the optional debugger (weak dependency).
#include "BehaviorTreeDebugAPI.h"
#include "../system/system_utils.h"
#include "../system/EventQueue.h"
#include "../json_helper.h"
#include "../NodeGraphCore/GraphDocument.h"
#include "../NodeGraphCore/GraphMigrator.h"
#include "../CollisionMap.h"
#include "../GameEngine.h"
#include <cmath>
#include <functional>
#include <set>
#include <fstream>
#include <ctime>

using json = nlohmann::json;

// --- BehaviorTreeManager Implementation ---

bool BehaviorTreeManager::LoadTreeFromFile(const std::string& filepath, uint32_t treeId)
{
    std::cout << "\n[BehaviorTreeManager] ========================================" << std::endl;
    std::cout << "[BehaviorTreeManager] Loading: " << filepath << std::endl;
    
    try
    {
        // 1. Load JSON file
        std::cout << "[BehaviorTreeManager] Step 1: Loading JSON file..." << std::endl;
        json j;
        if (!JsonHelper::LoadJsonFromFile(filepath, j))
        {
            std::cerr << "[BehaviorTreeManager] ERROR: Failed to load file: " << filepath << std::endl;
            std::cout << "[BehaviorTreeManager] ========================================+n" << std::endl;
            return false;
        }
        std::cout << "[BehaviorTreeManager] JSON loaded successfully" << std::endl;
        
        // 2. Detect version (accept both snake_case and camelCase keys and nested schemaVersion)
        std::cout << "[BehaviorTreeManager] Step 2: Detecting format version..." << std::endl;
        bool isV2 = false;
        if (j.contains("schema_version") && j["schema_version"].is_number())
            isV2 = (j["schema_version"].get<int>() == 2);
        if (!isV2 && j.contains("schemaVersion") && j["schemaVersion"].is_number())
            isV2 = (j["schemaVersion"].get<int>() == 2);
        // Also support schemaVersion inside data (some exports nest it)
        if (!isV2 && j.contains("data") && j["data"].is_object() && j["data"].contains("schemaVersion") && j["data"]["schemaVersion"].is_number())
            isV2 = (j["data"]["schemaVersion"].get<int>() == 2);

        std::cout << "[BehaviorTreeManager] Version: " << (isV2 ? "v2" : "v1") << std::endl;
        
        // 3. Extract tree metadata and data section
        std::cout << "[BehaviorTreeManager] Step 3: Extracting tree metadata..." << std::endl;
        BehaviorTreeAsset tree;
        tree.id = treeId;
        
        const json* dataSection = &j;
        
        if (isV2)
        {
            tree.name = JsonHelper::GetString(j, "name", "Unnamed Tree");

            // If the asset has no explicit name, derive it from the filepath
            if (tree.name.empty() || tree.name == "Unnamed Tree")
            {
                auto extract = [&](const std::string& p)->std::string {
                    size_t lastSlash = p.find_last_of("/\\");
                    std::string fname = (lastSlash == std::string::npos) ? p : p.substr(lastSlash + 1);
                    // Prefer stripping ".bt.json" specifically, otherwise remove final extension
                    if (fname.size() > 8 && fname.substr(fname.size() - 8) == ".bt.json")
                        fname = fname.substr(0, fname.size() - 8);
                    else
                    {
                        size_t dot = fname.find_last_of('.');
                        if (dot != std::string::npos) fname = fname.substr(0, dot);
                    }
                    return fname;
                };

                tree.name = extract(filepath);
            }
            
            if (!j.contains("data"))
            {
                std::cerr << "[BehaviorTreeManager] ERROR: v2 format but no 'data' section" << std::endl;
                std::cout << "[BehaviorTreeManager] ========================================+n" << std::endl;
                return false;
            }
            
            dataSection = &j["data"];
            std::cout << "[BehaviorTreeManager] Extracted 'data' section from v2 format" << std::endl;
        }
        else
        {
            tree.name = JsonHelper::GetString(j, "name", "Unnamed Tree");
            std::cout << "[BehaviorTreeManager] Using root as data section (v1 format)" << std::endl;

            if (tree.name.empty() || tree.name == "Unnamed Tree")
            {
                size_t lastSlash = filepath.find_last_of("/\\");
                std::string fname = (lastSlash == std::string::npos) ? filepath : filepath.substr(lastSlash + 1);
                size_t dot = fname.find_last_of('.');
                if (dot != std::string::npos) fname = fname.substr(0, dot);
                tree.name = fname;
            }
        }
        
        tree.rootNodeId = JsonHelper::GetUInt(*dataSection, "rootNodeId", 0);
        std::cout << "[BehaviorTreeManager] Tree name: " << tree.name << std::endl;
        std::cout << "[BehaviorTreeManager] Root node ID: " << tree.rootNodeId << std::endl;
        
        // 4. Parse nodes - prefer using GraphDocument::FromJson for v2 assets (editor parity)
        std::cout << "[BehaviorTreeManager] Step 4: Parsing nodes..." << std::endl;
        // docLinks holds GraphDocument links so reconstruction can run after parsing
        std::vector<Olympe::NodeGraphTypes::LinkData> docLinks;
        if (isV2)
        {
            try
            {
                // Use the migrator/parser used by the editor/debugger to ensure identical interpretation
                Olympe::NodeGraphTypes::GraphDocument doc = Olympe::GraphMigrator::LoadWithMigration(j);

                // Convert GraphDocument nodes to BTNode entries
                for (const auto& nd : doc.GetNodes())
                {
                    BTNode bn;
                    bn.id = nd.id.value;
                    bn.name = nd.name;
                    bn.editorPosX = nd.position.x;
                    bn.editorPosY = nd.position.y;

                    std::string typeStr = nd.type;
                    if (typeStr.find("Selector") != std::string::npos) bn.type = BTNodeType::Selector;
                    else if (typeStr.find("Sequence") != std::string::npos) bn.type = BTNodeType::Sequence;
                    else if (typeStr.find("Root") != std::string::npos) bn.type = BTNodeType::Root;
                    else if (typeStr.find("Inverter") != std::string::npos) bn.type = BTNodeType::Inverter;
                    else if (typeStr.find("Repeater") != std::string::npos) bn.type = BTNodeType::Repeater;
                    else if (typeStr.find("OnEvent") != std::string::npos) bn.type = BTNodeType::OnEvent;
                    else if (typeStr.find("BT_") != std::string::npos && typeStr.find("BT_") == 0)
                    {
                        if (typeStr.find("Has") != std::string::npos || typeStr.find("Is") != std::string::npos || typeStr.find("Can") != std::string::npos)
                            bn.type = BTNodeType::Condition;
                        else
                            bn.type = BTNodeType::Action;
                    }
                    else bn.type = BTNodeType::Action;

                    // Copy decorator child if present
                    if (nd.decoratorChild.value != 0)
                        bn.decoratorChildId = nd.decoratorChild.value;

                    // Copy simple string parameters
                    for (const auto& kv : nd.parameters)
                    {
                        bn.stringParams[kv.first] = kv.second;
                    }

                    tree.nodes.push_back(bn);
                }

                // Use document root if present
                if (doc.rootNodeId.value != 0)
                    tree.rootNodeId = doc.rootNodeId.value;

                // Copy GraphDocument links into docLinks so the reconstruction step below can use them
                docLinks = doc.GetLinks();

                // COPY: transfer GraphDocument node children (already reconstructed in GraphDocument) to BTNode.childIds
                // Build id index for tree.nodes
                std::unordered_map<uint32_t, size_t> tmpIndex;
                for (size_t i = 0; i < tree.nodes.size(); ++i) tmpIndex[tree.nodes[i].id] = i;
                // For each source node in document, copy children into corresponding BTNode
                for (const auto& ndSrc : doc.GetNodes())
                {
                    auto itIdx = tmpIndex.find(ndSrc.id.value);
                    if (itIdx == tmpIndex.end()) continue;
                    auto& targetNode = tree.nodes[itIdx->second];
                    targetNode.childIds.clear();
                    for (const auto& cid : ndSrc.children)
                    {
                        targetNode.childIds.push_back(cid.value);
                    }
                }

                std::cout << "[BehaviorTreeManager] Parsed V2 BehaviorTree asset: nodes=" << tree.nodes.size() << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "[BehaviorTreeManager] ERROR: V2 parsing failed: " << e.what() << std::endl;
                return false;
            }
        }
        else
        {
            // legacy parsing for v1 format
            if (!JsonHelper::IsArray(*dataSection, "nodes"))
            {
                std::cerr << "[BehaviorTreeManager] ERROR: No 'nodes' array in data section" << std::endl;
                std::cerr << "[BehaviorTreeManager] This may be an empty or invalid tree" << std::endl;
                std::cout << "[BehaviorTreeManager] JSON structure:" << std::endl;
                std::cout << j.dump(2) << std::endl;
                return false;
            }

            static const std::unordered_map<std::string, BTNodeType> btTypeMap = {
                {"Root", BTNodeType::Root},
                {"RootSelector", BTNodeType::Root},
                {"Selector", BTNodeType::Selector},
                {"Sequence", BTNodeType::Sequence},
                {"Inverter", BTNodeType::Inverter},
                {"Repeater", BTNodeType::Repeater},
                {"SubGraph", BTNodeType::SubGraph}
            };

            static const std::unordered_map<std::string, BTConditionType> btCondMap = {
                {"HasTarget", BTConditionType::TargetVisible},
                {"Has Target", BTConditionType::TargetVisible},
                {"HasTarget?", BTConditionType::TargetVisible},
                {"TargetVisible", BTConditionType::TargetVisible},
                {"IsTargetInRange", BTConditionType::TargetInRange},
                {"Is Target In Range?", BTConditionType::TargetInRange},
                {"HealthBelow", BTConditionType::HealthBelow},
                {"HasMoveGoal", BTConditionType::HasMoveGoal},
                {"CanAttack", BTConditionType::CanAttack},
                {"HeardNoise", BTConditionType::HeardNoise},
                {"IsWaitTimerExpired", BTConditionType::IsWaitTimerExpired},
                {"HasNavigableDestination", BTConditionType::HasNavigableDestination},
                {"HasValidPath", BTConditionType::HasValidPath},
                {"HasReachedDestination", BTConditionType::HasReachedDestination}
            };

            static const std::unordered_map<std::string, BTActionType> btActionMap = {
                {"SetMoveGoalToLastKnownTargetPos", BTActionType::SetMoveGoalToLastKnownTargetPos},
                {"SetMoveGoalToTarget", BTActionType::SetMoveGoalToTarget},
                {"SetMoveGoalToPatrolPoint", BTActionType::SetMoveGoalToPatrolPoint},
                {"MoveToGoal", BTActionType::MoveToGoal},
                {"MoveTo", BTActionType::MoveToGoal},
                {"MoveToTarget", BTActionType::MoveToGoal},
                {"AttackIfClose", BTActionType::AttackIfClose},
                {"AttackMelee", BTActionType::AttackIfClose},
                {"AttackTarget", BTActionType::AttackIfClose},
                {"PatrolPickNextPoint", BTActionType::PatrolPickNextPoint},
                {"ClearTarget", BTActionType::ClearTarget},
                {"Idle", BTActionType::Idle},
                {"WaitRandomTime", BTActionType::WaitRandomTime},
                {"ChooseRandomNavigablePoint", BTActionType::ChooseRandomNavigablePoint},
                {"RequestPathfinding", BTActionType::RequestPathfinding},
                {"FollowPath", BTActionType::FollowPath},
                {"SendMessage", BTActionType::SendMessage}
            };

            // Count nodes first
            int nodeCount = 0;
            JsonHelper::ForEachInArray(*dataSection, "nodes", [&nodeCount](const json& nodeJson, size_t i) { nodeCount++; });
            std::cout << "[BehaviorTreeManager] Found " << nodeCount << " nodes to parse" << std::endl;

            JsonHelper::ForEachInArray(*dataSection, "nodes", [&tree, isV2](const json& nodeJson, size_t i)
            {
                BTNode node;
                node.id = JsonHelper::GetInt(nodeJson, "id", 0);
                node.name = JsonHelper::GetString(nodeJson, "name", "");
                node.editorPosX = JsonHelper::GetFloat(nodeJson, "editorPosX", 0.0f);
                node.editorPosY = JsonHelper::GetFloat(nodeJson, "editorPosY", 0.0f);
                if (nodeJson.contains("position") && nodeJson["position"].is_object())
                {
                    node.editorPosX = JsonHelper::GetFloat(nodeJson["position"], "x", node.editorPosX);
                    node.editorPosY = JsonHelper::GetFloat(nodeJson["position"], "y", node.editorPosY);
                }

                std::string typeStr = JsonHelper::GetString(nodeJson, "type", "");
                std::string base = typeStr;
                if (base.rfind("BT_", 0) == 0 && base.size() > 3)
                    base = base.substr(3);

                auto itType = btTypeMap.find(base);
                if (itType != btTypeMap.end())
                {
                    node.type = itType->second;
                }
                else
                {
                    auto itCond = btCondMap.find(base);
                    if (itCond != btCondMap.end())
                    {
                        node.type = BTNodeType::Condition;
                        node.conditionType = itCond->second;
                        node.conditionTypeString = base;
                    }
                    else
                    {
                        auto itAct = btActionMap.find(base);
                        if (itAct != btActionMap.end())
                        {
                            node.type = BTNodeType::Action;
                            node.actionType = itAct->second;
                        }
                        else if (base.find("Selector") != std::string::npos)
                        {
                            node.type = BTNodeType::Selector;
                        }
                        else if (base.find("Sequence") != std::string::npos)
                        {
                            node.type = BTNodeType::Sequence;
                        }
                        else if (base.find("Inverter") != std::string::npos)
                        {
                            node.type = BTNodeType::Inverter;
                        }
                        else if (base.find("Repeater") != std::string::npos)
                        {
                            node.type = BTNodeType::Repeater;
                        }
                        else if (base.find("SubGraph") != std::string::npos)
                        {
                            node.type = BTNodeType::SubGraph;
                        }
                        else if (base.find("Is") != std::string::npos || base.find("Has") != std::string::npos || base.find("Can") != std::string::npos || base.find("Heard") != std::string::npos)
                        {
                            node.type = BTNodeType::Condition;
                            node.conditionTypeString = base;
                        }
                        else
                        {
                            node.type = BTNodeType::Action;
                        }
                    }
                }

                if (JsonHelper::IsArray(nodeJson, "children"))
                {
                    JsonHelper::ForEachInArray(nodeJson, "children", [&node](const json& childId, size_t j)
                    {
                        node.childIds.push_back(childId.get<uint32_t>());
                    });
                }

                if (node.type == BTNodeType::Condition)
                {
                    std::string condStr = JsonHelper::GetString(nodeJson, "conditionType", node.conditionTypeString);
                    auto itCond = btCondMap.find(condStr);
                    if (itCond != btCondMap.end())
                    {
                        node.conditionType = itCond->second;
                    }
                    if (!condStr.empty())
                    {
                        node.conditionTypeString = condStr;
                    }

                    node.conditionParam = JsonHelper::GetFloat(nodeJson, "param", 0.0f);
                    if (nodeJson.contains("parameters") && nodeJson["parameters"].is_object())
                    {
                        const json& params = nodeJson["parameters"];
                        node.conditionParam = JsonHelper::GetFloat(params, "param", node.conditionParam);
                        for (auto it = params.begin(); it != params.end(); ++it)
                        {
                            if (it.value().is_string())
                                node.stringParams[it.key()] = it.value().get<std::string>();
                            else if (it.value().is_number_integer())
                                node.intParams[it.key()] = it.value().get<int>();
                            else if (it.value().is_number_float())
                                node.floatParams[it.key()] = it.value().get<float>();
                        }
                    }
                }

                if (node.type == BTNodeType::Action)
                {
                    std::string actStr = JsonHelper::GetString(nodeJson, "actionType", "");
                    auto itAct = btActionMap.find(actStr);
                    if (itAct != btActionMap.end())
                    {
                        node.actionType = itAct->second;
                    }

                    node.actionParam1 = JsonHelper::GetFloat(nodeJson, "param1", 0.0f);
                    node.actionParam2 = JsonHelper::GetFloat(nodeJson, "param2", 0.0f);
                    if (nodeJson.contains("parameters") && nodeJson["parameters"].is_object())
                    {
                        const json& params = nodeJson["parameters"];
                        node.actionParam1 = JsonHelper::GetFloat(params, "param1", node.actionParam1);
                        node.actionParam2 = JsonHelper::GetFloat(params, "param2", node.actionParam2);
                    }
                }

                if (node.type == BTNodeType::SubGraph || typeStr == "SubGraph" || base == "SubGraph")
                {
                    node.type = BTNodeType::SubGraph;
                    node.subgraphPath = JsonHelper::GetString(nodeJson, "subgraphPath", "");
                    if (nodeJson.contains("subgraphInputs") && nodeJson["subgraphInputs"].is_object())
                    {
                        const json& inputs = nodeJson["subgraphInputs"];
                        for (auto it = inputs.begin(); it != inputs.end(); ++it)
                            node.subgraphInputs[it.key()] = it.value().get<std::string>();
                    }
                    if (nodeJson.contains("subgraphOutputs") && nodeJson["subgraphOutputs"].is_object())
                    {
                        const json& outputs = nodeJson["subgraphOutputs"];
                        for (auto it = outputs.begin(); it != outputs.end(); ++it)
                            node.subgraphOutputs[it.key()] = it.value().get<std::string>();
                    }
                }

                if (node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater)
                {
                    node.decoratorChildId = JsonHelper::GetInt(nodeJson, "decoratorChildId", 0);
                }
                if (node.type == BTNodeType::Repeater)
                {
                    node.repeatCount = JsonHelper::GetInt(nodeJson, "repeatCount", 0);
                }

                tree.nodes.push_back(node);
                std::cout << "[BehaviorTreeManager]   Node " << node.id << ": " << node.name
                          << " (" << typeStr << ") children: " << node.childIds.size()
                          << " pos=(" << node.editorPosX << "," << node.editorPosY << ")" << std::endl;
            });
            std::cout << "[BehaviorTreeManager] Parsed " << tree.nodes.size() << " nodes successfully" << std::endl;
        }

        // Reconstruct child relations from the authoritative data.links exported by the editor
        // NOTE: For v2 assets we already used GraphMigrator::LoadWithMigration() and copied
        // the GraphDocument node children into BTNode.childIds above. Re-running an independent
        // adjacency reconstruction can produce a different interpretation (pin heuristics,
        // positional tie-breakers) and lead to rendering differences between editor and runtime.
        // Therefore: skip the link-based reconstruction for v2 assets and trust the GraphDocument
        // children produced by the editor. For legacy v1 assets, perform reconstruction from
        // the raw "links" array as before.
        if (!isV2)
        {
            try
            {
            // Build lookup index
            std::unordered_map<uint32_t, size_t> idIndex;
            for (size_t i = 0; i < tree.nodes.size(); ++i) idIndex[tree.nodes[i].id] = i;

            std::unordered_map<uint32_t, std::vector<uint32_t>> forwardAdj;

            // Prefer direct data.links from the v2 JSON (canonical format)
            if (isV2 && j.contains("data") && j["data"].is_object() && j["data"].contains("links") && j["data"]["links"].is_array())
            {
                const json& linksArr = j["data"]["links"];
                std::cout << "[BehaviorTreeManager] Building adjacency from data.links (" << linksArr.size() << " entries)" << std::endl;

                auto isComposite = [](const BTNode* n){ return n && (n->type == BTNodeType::Selector || n->type == BTNodeType::Sequence || n->type == BTNodeType::Root); };

                // If GraphDocument produced link structures, prefer those (they preserve pin names and resolution rules)
                if (!docLinks.empty())
                {
                    std::cout << "[BehaviorTreeManager] Using GraphDocument-parsed links (docLinks.size=" << docLinks.size() << ")" << std::endl;
                    for (const auto& link : docLinks)
                    {
                        uint32_t fromNode = link.fromPin.value;
                        uint32_t toNode = link.toPin.value;
                        if (fromNode == 0 || toNode == 0) continue;

                        // Diagnostic: preserve link mapping information for runtime diagnosis
                        SYSTEM_LOG << "[BehaviorTreeManager] docLink id=" << link.id.value
                                   << " from=" << fromNode << "(pin='" << link.fromPinName << "')"
                                   << " to=" << toNode << "(pin='" << link.toPinName << "')" << std::endl;

                        BTNode* fromBn = tree.GetNode(fromNode);
                        BTNode* toBn = tree.GetNode(toNode);

                        std::string fromPinId = link.fromPinName;
                        std::string toPinId = link.toPinName;

                        bool applied = false;

                        if (isComposite(fromBn) && !isComposite(toBn))
                        {
                            forwardAdj[fromNode].push_back(toNode);
                            applied = true;
                        }
                        else if (!isComposite(fromBn) && isComposite(toBn))
                        {
                            forwardAdj[toNode].push_back(fromNode);
                            applied = true;
                        }

                        if (!applied)
                        {
                            if (!fromPinId.empty() && !toPinId.empty())
                            {
                                if ((fromPinId == "output" && toPinId == "input") || (fromPinId == "out" && toPinId == "in"))
                                {
                                    forwardAdj[fromNode].push_back(toNode);
                                    applied = true;
                                }
                                else if ((toPinId == "output" && fromPinId == "input") || (toPinId == "out" && fromPinId == "in"))
                                {
                                    forwardAdj[toNode].push_back(fromNode);
                                    applied = true;
                                }
                            }
                        }

                        if (!applied)
                        {
                            float fx = (fromBn) ? fromBn->editorPosX : 0.0f;
                            float tx = (toBn) ? toBn->editorPosX : 0.0f;
                            if (fx <= tx)
                                forwardAdj[fromNode].push_back(toNode);
                            else
                                forwardAdj[toNode].push_back(fromNode);
                        }
                    }
                }
                else
                {
                    // Fallback: interpret raw JSON links when docLinks not available
                    for (const auto& linkJson : linksArr)
                    {
                        if (!linkJson.contains("fromPin") || !linkJson.contains("toPin")) continue;
                        const json& fromPin = linkJson["fromPin"];
                        const json& toPin = linkJson["toPin"];
                        if (!fromPin.is_object() || !toPin.is_object()) continue;

                        uint32_t fromNode = JsonHelper::GetUInt(fromPin, "nodeId", 0);
                        uint32_t toNode = JsonHelper::GetUInt(toPin, "nodeId", 0);
                        if (fromNode == 0 || toNode == 0) continue;

                        BTNode* fromBn = tree.GetNode(fromNode);
                        BTNode* toBn = tree.GetNode(toNode);

                        std::string fromPinId = JsonHelper::GetString(fromPin, "pinId", "");
                        std::string toPinId = JsonHelper::GetString(toPin, "pinId", "");

                        bool applied = false;

                        if (isComposite(fromBn) && !isComposite(toBn))
                        {
                            forwardAdj[fromNode].push_back(toNode);
                            applied = true;
                        }
                        else if (!isComposite(fromBn) && isComposite(toBn))
                        {
                            forwardAdj[toNode].push_back(fromNode);
                            applied = true;
                        }

                        if (!applied)
                        {
                            if (!fromPinId.empty() && !toPinId.empty())
                            {
                                if ((fromPinId == "output" && toPinId == "input") || (fromPinId == "out" && toPinId == "in"))
                                {
                                    forwardAdj[fromNode].push_back(toNode);
                                    applied = true;
                                }
                                else if ((toPinId == "output" && fromPinId == "input") || (toPinId == "out" && fromPinId == "in"))
                                {
                                    forwardAdj[toNode].push_back(fromNode);
                                    applied = true;
                                }
                            }
                        }

                        if (!applied)
                        {
                            float fx = (fromBn) ? fromBn->editorPosX : 0.0f;
                            float tx = (toBn) ? toBn->editorPosX : 0.0f;
                            if (fx <= tx)
                                forwardAdj[fromNode].push_back(toNode);
                            else
                                forwardAdj[toNode].push_back(fromNode);
                        }
                    }
                }
            }

            // Apply adjacency to tree.nodes
            for (auto& n : tree.nodes) n.childIds.clear();
            for (const auto& kv : forwardAdj)
            {
                auto it = idIndex.find(kv.first);
                if (it == idIndex.end()) continue;
                auto& childVec = tree.nodes[it->second].childIds;
                for (uint32_t cid : kv.second)
                {
                    if (tree.GetNode(cid) == nullptr) continue; // skip unresolved
                    if (std::find(childVec.begin(), childVec.end(), cid) == childVec.end()) childVec.push_back(cid);
                }
            }

            // Diagnostic summary of adjacency
            std::cout << "[BehaviorTreeManager] Adjacency summary (parent -> [children])" << std::endl;
            for (const auto& kv : forwardAdj)
            {
                std::cout << "  " << kv.first << " -> [";
                for (size_t i = 0; i < kv.second.size(); ++i) { if (i) std::cout << ","; std::cout << kv.second[i]; }
                std::cout << "]" << std::endl;
            }

            // Sort children by editor Y position
            for (auto& parent : tree.nodes)
            {
                if (parent.childIds.size() <= 1) continue;
                std::sort(parent.childIds.begin(), parent.childIds.end(), [&](uint32_t a, uint32_t b)
                {
                    const BTNode* na = tree.GetNode(a); const BTNode* nb = tree.GetNode(b);
                    if (!na || !nb) return a < b;
                    return na->editorPosY < nb->editorPosY;
                });
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[BehaviorTreeManager] Warning: failed to reconstruct children from links: " << e.what() << std::endl;
        }
        }

            // Final dump
            std::cout << "[BehaviorTreeManager] Final reconstructed BT graph:" << std::endl;
            for (const auto& n : tree.nodes)
            {
                std::cout << "  Node " << n.id << ": " << n.name << " (type=" << static_cast<int>(n.type) << ")";
                if (n.decoratorChildId != 0) std::cout << " decoratorChildId=" << n.decoratorChildId;
                std::cout << " children=[";
                for (size_t i = 0; i < n.childIds.size(); ++i) { if (i) std::cout << ","; std::cout << n.childIds[i]; }
                std::cout << "]" << std::endl;
            }

        // 5. Validate tree structure
        std::cout << "[BehaviorTreeManager] Step 5: Validating tree structure..." << std::endl;
        std::string validationError;
        bool valid = ValidateTree(tree, validationError);
        if (!valid)
        {
            std::cerr << "[BehaviorTreeManager] WARNING: Tree validation failed: " << validationError << std::endl;
            // Don't fail loading - allow hot-reload to fix issues
        }
        else
        {
            std::cout << "[BehaviorTreeManager] Tree validation: OK" << std::endl;
        }

        // Phase 38b: Step 6: Ensure Root node exists (auto-create if missing)
        std::cout << "[BehaviorTreeManager] Step 6: Ensuring Root node exists..." << std::endl;
        tree.EnsureRootNodeExists();

        // 7. Store the tree
        std::cout << "[BehaviorTreeManager] Step 7: Registering tree..." << std::endl;
        m_trees.push_back(tree);
        
        // Register the path -> ID mapping
        m_pathToIdMap[filepath] = treeId;
        
        std::cout << "[BehaviorTreeManager] SUCCESS: Loaded '" << tree.name << "' (ID=" << treeId << ") with " 
                  << tree.nodes.size() << " nodes" << std::endl;
        std::cout << "[BehaviorTreeManager] Registered path mapping: " << filepath << " -> ID " << treeId << "\n";
        std::cout << "[BehaviorTreeManager] ========================================+n" << std::endl;
        
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n[BehaviorTreeManager] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        std::cerr << "[BehaviorTreeManager] EXCEPTION: " << e.what() << std::endl;
        std::cerr << "[BehaviorTreeManager] !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" << std::endl;
        std::cout << "[BehaviorTreeManager] ========================================+n" << std::endl;
        return false;
    }
}

const BehaviorTreeAsset* BehaviorTreeManager::GetTree(uint32_t treeId) const
{
    for (const auto& tree : m_trees)
    {
        if (tree.id == treeId)
            return &tree;
    }
    return nullptr;
}

void BehaviorTreeManager::Clear()
{
    m_trees.clear();
    m_pathToIdMap.clear();
}

bool BehaviorTreeManager::ReloadTree(uint32_t treeId)
{
    // Find the tree
    for (auto& tree : m_trees)
    {
        if (tree.id == treeId)
        {
            // Get the original filepath (we need to store it)
            // For now, reconstruct it from tree name
            std::string filepath = "Blueprints/AI/" + tree.name + ".json";
            
            // Remove old tree
            m_trees.erase(std::remove_if(m_trees.begin(), m_trees.end(),
                [treeId](const BehaviorTreeAsset& t) { return t.id == treeId; }), 
                m_trees.end());
            
            // Load new version
            bool success = LoadTreeFromFile(filepath, treeId);
            if (success)
            {
                SYSTEM_LOG << "BehaviorTreeManager: Hot-reloaded tree ID=" << treeId << "\n";
            }
            return success;
        }
    }
    
    SYSTEM_LOG << "BehaviorTreeManager: Cannot reload tree ID=" << treeId << " (not found)\n";
    return false;
}

bool BehaviorTreeManager::ValidateTree(const BehaviorTreeAsset& tree, std::string& errorMessage) const
{
    errorMessage.clear();
    
    // Check if tree has nodes
    if (tree.nodes.empty())
    {
        errorMessage = "Tree has no nodes";
        return false;
    }
    
    // Check if root node exists
    const BTNode* root = tree.GetNode(tree.rootNodeId);
    if (!root)
    {
        errorMessage = "Root node ID " + std::to_string(tree.rootNodeId) + " not found";
        return false;
    }
    
    // Validate each node
    for (const auto& node : tree.nodes)
    {
        // Check composite nodes have children
        if (node.type == BTNodeType::Selector || node.type == BTNodeType::Sequence)
        {
            if (node.childIds.empty())
            {
                errorMessage = "Composite node '" + node.name + "' (ID=" + std::to_string(node.id) + ") has no children";
                return false;
            }
            
            // Validate all children exist
            for (uint32_t childId : node.childIds)
            {
                if (!tree.GetNode(childId))
                {
                    errorMessage = "Node '" + node.name + "' references missing child ID " + std::to_string(childId);
                    return false;
                }
            }
        }
        
        // Check decorator nodes have a child
        if (node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater)
        {
            if (!tree.GetNode(node.decoratorChildId))
            {
                errorMessage = "Decorator node '" + node.name + "' references missing child ID " + std::to_string(node.decoratorChildId);
                return false;
            }
        }
        
        // Check for duplicate node IDs
        int count = 0;
        for (const auto& other : tree.nodes)
        {
            if (other.id == node.id)
                count++;
        }
        if (count > 1)
        {
            errorMessage = "Duplicate node ID " + std::to_string(node.id);
            return false;
        }
    }
    
    return true;
}

// --- Behavior Tree Execution ---

// Optional: debugger interaction now routed through BehaviorTreeDebugAPI.h

BTStatus ExecuteBTNode(const BTNode& node, EntityID entity, AIBlackboard_data& blackboard, const BehaviorTreeAsset& tree)
{
    BTStatus finalStatus = BTStatus::Failure;

    // Recursion / cycle protection: use a thread-local call stack to detect repeated visits
    // This prevents infinite recursion / stack overflow when a tree contains cycles at runtime.
    static thread_local std::vector<uint32_t> s_execCallStack;
    struct CallStackGuard
    {
        std::vector<uint32_t>& stack;
        uint32_t id;
        bool wasCycle;
        CallStackGuard(std::vector<uint32_t>& s, uint32_t i) : stack(s), id(i), wasCycle(false)
        {
            if (std::find(stack.begin(), stack.end(), id) != stack.end())
            {
                wasCycle = true;
            }
            else
            {
                stack.push_back(id);
            }
        }
        ~CallStackGuard()
        {
            if (!wasCycle)
            {
                if (!stack.empty() && stack.back() == id)
                    stack.pop_back();
                else
                {
                    // Fallback: remove first occurrence if back() doesn't match (defensive)
                    auto it = std::find(stack.begin(), stack.end(), id);
                    if (it != stack.end()) stack.erase(it);
                }
            }
        }
    } guard(s_execCallStack, node.id);

    if (guard.wasCycle)
    {
        // Detected cycle during traversal - bail out to avoid stack overflow
        std::cerr << "[ExecuteBTNode] Cycle detected at node ID " << node.id << " - aborting branch" << std::endl;
        return BTStatus::Failure;
    }

    // Update per-entity runtime current node so debugger can highlight traversal in realtime
    try
    {
        if (World::Get().HasComponent<BehaviorTreeRuntime_data>(entity))
        {
            BehaviorTreeRuntime_data& rt = World::Get().GetComponent<BehaviorTreeRuntime_data>(entity);
            rt.AICurrentNodeIndex = node.id;
        }
    }
    catch (...) { /* ignore failures to avoid breaking BT execution */ }

    switch (node.type)
    {
        case BTNodeType::Root:
        {
            // Root node: delegate to children (treat as a selector over children to find active branch)
            if (node.childIds.empty()) { finalStatus = BTStatus::Failure; break; }
            for (uint32_t childId : node.childIds)
            {
                const BTNode* child = tree.GetNode(childId);
                if (!child) continue;

                BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
                if (status == BTStatus::Success)
                {
                    finalStatus = BTStatus::Success;
                    break;
                }
                if (status == BTStatus::Running)
                {
                    finalStatus = BTStatus::Running;
                    break;
                }
            }
            break;
        }
        case BTNodeType::Selector:
        {
            // OR node: succeeds if any child succeeds
            for (uint32_t childId : node.childIds)
            {
                const BTNode* child = tree.GetNode(childId);
                if (!child) continue;

                BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
                if (status == BTStatus::Success)
                {
                    finalStatus = BTStatus::Success;
                    break;
                }
                if (status == BTStatus::Running)
                {
                    finalStatus = BTStatus::Running;
                    break;
                }
            }
            if (finalStatus == BTStatus::Failure) finalStatus = BTStatus::Failure;
            break;
        }

        case BTNodeType::Sequence:
        {
            // AND node: succeeds if all children succeed
            for (uint32_t childId : node.childIds)
            {
                const BTNode* child = tree.GetNode(childId);
                if (!child) continue;

                BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
                if (status == BTStatus::Failure)
                {
                    finalStatus = BTStatus::Failure;
                    break;
                }
                if (status == BTStatus::Running)
                {
                    finalStatus = BTStatus::Running;
                    break;
                }
            }
            if (finalStatus != BTStatus::Failure && finalStatus != BTStatus::Running)
                finalStatus = BTStatus::Success;
            break;
        }

        case BTNodeType::Condition:
        {
            // Check if this is a string-based condition (like CheckBlackboardValue)
            if (!node.conditionTypeString.empty() && node.conditionTypeString == "CheckBlackboardValue")
            {
                // Execute CheckBlackboardValue condition
                std::string key = node.GetParameterString("key");
                std::string op = node.GetParameterString("operator");
                int expectedValue = node.GetParameterInt("value");

                int actualValue = 0;

                // Get value from blackboard
                if (key == "AIMode")
                {
                    actualValue = blackboard.AIMode;
                }
                else
                {
                    // Future: support other blackboard integer fields
                    finalStatus = BTStatus::Failure;  // Key not found
                    break;
                }

                // Perform comparison
                if (op == "Equals" || op == "equals" || op == "==")
                    finalStatus = (actualValue == expectedValue) ? BTStatus::Success : BTStatus::Failure;
                else if (op == "NotEquals" || op == "notequals" || op == "!=")
                    finalStatus = (actualValue != expectedValue) ? BTStatus::Success : BTStatus::Failure;
                else if (op == "GreaterThan" || op == "greaterthan" || op == ">")
                    finalStatus = (actualValue > expectedValue) ? BTStatus::Success : BTStatus::Failure;
                else if (op == "LessThan" || op == "lessthan" || op == "<")
                    finalStatus = (actualValue < expectedValue) ? BTStatus::Success : BTStatus::Failure;
                else if (op == "GreaterOrEqual" || op == "greaterorequal" || op == ">=")
                    finalStatus = (actualValue >= expectedValue) ? BTStatus::Success : BTStatus::Failure;
                else if (op == "LessOrEqual" || op == "lessorequal" || op == "<=")
                    finalStatus = (actualValue <= expectedValue) ? BTStatus::Success : BTStatus::Failure;
                else
                    finalStatus = BTStatus::Failure;
            }
            else
            {
                // Execute enum-based condition (legacy)
                finalStatus = ExecuteBTCondition(node.conditionType, node.conditionParam, entity, blackboard);
            }
            break;
        }

        case BTNodeType::Action:
        {
            finalStatus = ExecuteBTAction(node.actionType, node.actionParam1, node.actionParam2, entity, blackboard);
            break;
        }

        case BTNodeType::Inverter:
        {
            const BTNode* child = tree.GetNode(node.decoratorChildId);
            if (!child) { finalStatus = BTStatus::Failure; break; }

            BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
            if (status == BTStatus::Success)
                finalStatus = BTStatus::Failure;
            else if (status == BTStatus::Failure)
                finalStatus = BTStatus::Success;
            else
                finalStatus = status;
            break;
        }

        case BTNodeType::Repeater:
        {
            // Simplified repeater: just execute once per tick
            const BTNode* child = tree.GetNode(node.decoratorChildId);
            if (!child) { finalStatus = BTStatus::Failure; break; }

            finalStatus = ExecuteBTNode(*child, entity, blackboard, tree);
            break;
        }

        default:
            finalStatus = BTStatus::Failure;
            break;
    }

    // Enhanced runtime debugging: emit rich JSON execution entry (via small API)
    try
    {
        json j;
        // Use doubles for JSON numeric fields to avoid MSVC overload ambiguity
        j["ts"] = static_cast<double>(static_cast<unsigned long long>(std::time(nullptr)) * 1000ULL);
        j["treeId"] = static_cast<double>(tree.id);
        j["entity"] = static_cast<double>(entity);
        j["nodeId"] = static_cast<double>(node.id);
        j["nodeName"] = node.name;
        j["status"] = (finalStatus == BTStatus::Success) ? "Success"
                    : (finalStatus == BTStatus::Failure) ? "Failure"
                    : "Running";

        if (World::Get().HasComponent<Identity_data>(entity))
        {
            const Identity_data& identity = World::Get().GetComponent<Identity_data>(entity);
            j["entityName"] = identity.name;
        }

        if (node.type == BTNodeType::Condition)
        {
            json details;
            if (!node.conditionTypeString.empty())
                details["conditionType"] = node.conditionTypeString;
            else
                details["conditionType"] = static_cast<int>(node.conditionType);
            details["conditionParam"] = node.conditionParam;
            details["hasTarget"] = blackboard.hasTarget;
            details["distanceToTarget"] = blackboard.distanceToTarget;

            if (blackboard.targetEntity != INVALID_ENTITY_ID &&
                World::Get().HasComponent<Identity_data>(blackboard.targetEntity))
            {
                const Identity_data& targetIdentity = World::Get().GetComponent<Identity_data>(blackboard.targetEntity);
                details["targetName"] = targetIdentity.name;
            }

            j["details"] = details;
        }

        const std::string line = j.dump();
        BTDebug_AddExecutionJson(line.c_str());
    }
    catch (...) { /* don't let debug logging break runtime */ }

    return finalStatus;
}

BTStatus ExecuteBTCondition(BTConditionType condType, float param, EntityID entity, const AIBlackboard_data& blackboard)
{
    switch (condType)
    {
        case BTConditionType::TargetVisible:
            return blackboard.targetVisible ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::TargetInRange:
            if (!blackboard.hasTarget) return BTStatus::Failure;
            return (blackboard.distanceToTarget <= param) ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::HealthBelow:
            if (World::Get().HasComponent<Health_data>(entity))
            {
                const Health_data& health = World::Get().GetComponent<Health_data>(entity);
                float healthPercent = static_cast<float>(health.currentHealth) / static_cast<float>(health.maxHealth);
                return (healthPercent < param) ? BTStatus::Success : BTStatus::Failure;
            }
            return BTStatus::Failure;
        
        case BTConditionType::HasMoveGoal:
            return blackboard.hasMoveGoal ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::CanAttack:
            return blackboard.canAttack ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::HeardNoise:
            return blackboard.heardNoise ? BTStatus::Success : BTStatus::Failure;
        
        // NEW: Wander behavior conditions
        case BTConditionType::IsWaitTimerExpired:
            return (blackboard.wanderWaitTimer >= blackboard.wanderTargetWaitTime) ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::HasNavigableDestination:
            return blackboard.hasWanderDestination ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::HasValidPath:
            // Check if NavigationAgent has a valid path
            if (World::Get().HasComponent<NavigationAgent_data>(entity))
            {
                const NavigationAgent_data& navAgent = World::Get().GetComponent<NavigationAgent_data>(entity);
                return (!navAgent.currentPath.empty()) ? BTStatus::Success : BTStatus::Failure;
            }
            return BTStatus::Failure;
        
        case BTConditionType::HasReachedDestination:
            if (!blackboard.hasWanderDestination) return BTStatus::Failure;
            
            if (World::Get().HasComponent<Position_data>(entity))
            {
                const Position_data& pos = World::Get().GetComponent<Position_data>(entity);
                Vector vDest = pos.position;
                vDest -= blackboard.wanderDestination;
                float dist = vDest.Magnitude();
                
                // Use arrival threshold from MoveIntent if available, otherwise use default
                float threshold = 5.0f;
                if (World::Get().HasComponent<MoveIntent_data>(entity))
                {
                    const MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
                    threshold = intent.arrivalThreshold;
                }
                
                return (dist < threshold) ? BTStatus::Success : BTStatus::Failure;
            }
            return BTStatus::Failure;
    }
    
    return BTStatus::Failure;
}

BTStatus ExecuteBTAction(BTActionType actionType, float param1, float param2, EntityID entity, AIBlackboard_data& blackboard)
{
    switch (actionType)
    {
        case BTActionType::SetMoveGoalToLastKnownTargetPos:
            blackboard.moveGoal = blackboard.lastKnownTargetPosition;
            blackboard.hasMoveGoal = true;
            return BTStatus::Success;
        
        case BTActionType::SetMoveGoalToTarget:
            if (blackboard.hasTarget && blackboard.targetEntity != INVALID_ENTITY_ID)
            {
                if (World::Get().HasComponent<Position_data>(blackboard.targetEntity))
                {
                    const Position_data& targetPos = World::Get().GetComponent<Position_data>(blackboard.targetEntity);
                    blackboard.moveGoal = targetPos.position;
                    blackboard.hasMoveGoal = true;
                    return BTStatus::Success;
                }
            }
            return BTStatus::Failure;
        
        case BTActionType::SetMoveGoalToPatrolPoint:
            if (blackboard.patrolPointCount > 0)
            {
                int index = static_cast<int>(param1);
                if (index < 0 || index >= blackboard.patrolPointCount)
                    index = blackboard.currentPatrolPoint;
                
                blackboard.moveGoal = blackboard.patrolPoints[index];
                blackboard.hasMoveGoal = true;
                return BTStatus::Success;
            }
            return BTStatus::Failure;
        
        case BTActionType::MoveToGoal:
            if (!blackboard.hasMoveGoal) return BTStatus::Failure;
            
            // Set MoveIntent if component exists
            if (World::Get().HasComponent<MoveIntent_data>(entity))
            {
                MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
                intent.targetPosition = blackboard.moveGoal;
                intent.desiredSpeed = (param1 > 0.0f) ? param1 : 1.0f;
                intent.hasIntent = true;
                
                // Check if we've arrived
                if (World::Get().HasComponent<Position_data>(entity))
                {
                    Position_data& pos = World::Get().GetComponent<Position_data>(entity);
                    float dist = (pos.position - blackboard.moveGoal).Magnitude();
                    if (dist < intent.arrivalThreshold)
                    {
                        blackboard.hasMoveGoal = false;
                        intent.hasIntent = false;
                        return BTStatus::Success;
                    }
                }
                
                return BTStatus::Running;
            }
            return BTStatus::Failure;
        
        case BTActionType::AttackIfClose:
        {
            float range = (param1 > 0.0f) ? param1 : 50.0f;
            if (blackboard.hasTarget && blackboard.distanceToTarget <= range && blackboard.canAttack)
            {
                // Set AttackIntent if component exists
                if (World::Get().HasComponent<AttackIntent_data>(entity))
                {
                    AttackIntent_data& intent = World::Get().GetComponent<AttackIntent_data>(entity);
                    intent.targetEntity = blackboard.targetEntity;
                    intent.targetPosition = blackboard.lastKnownTargetPosition;
                    intent.range = range;
                    intent.damage = (param2 > 0.0f) ? param2 : 10.0f;
                    intent.hasIntent = true;
                    
                    blackboard.canAttack = false;
                    return BTStatus::Success;
                }
            }
            return BTStatus::Failure;
        }
        
        case BTActionType::PatrolPickNextPoint:
            if (blackboard.patrolPointCount > 0)
            {
                blackboard.currentPatrolPoint = (blackboard.currentPatrolPoint + 1) % blackboard.patrolPointCount;
                blackboard.moveGoal = blackboard.patrolPoints[blackboard.currentPatrolPoint];
                blackboard.hasMoveGoal = true;
                return BTStatus::Success;
            }
            return BTStatus::Failure;
        
        case BTActionType::ClearTarget:
            blackboard.hasTarget = false;
            blackboard.targetEntity = INVALID_ENTITY_ID;
            blackboard.targetVisible = false;
            return BTStatus::Success;
        
        case BTActionType::Idle:
            // Do nothing
            return BTStatus::Success;
        
        // NEW: Wander behavior actions
        case BTActionType::WaitRandomTime:
        {
            // If timer not initialized, create a random time
            if (blackboard.wanderTargetWaitTime == 0.0f)
            {
                float minWait = (param1 > 0.0f) ? param1 : 2.0f;
                float maxWait = (param2 > 0.0f) ? param2 : 6.0f;
                
                // Random generation between min and max
                float randomFactor = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                blackboard.wanderTargetWaitTime = minWait + randomFactor * (maxWait - minWait);
                blackboard.wanderWaitTimer = 0.0f;
            }
            
            // Increment timer with engine deltaTime
            blackboard.wanderWaitTimer += GameEngine::fDt;
            
            // Check if timer expired
            if (blackboard.wanderWaitTimer >= blackboard.wanderTargetWaitTime)
            {
                // Reset for next cycle
                blackboard.wanderTargetWaitTime = 0.0f;
                blackboard.wanderWaitTimer = 0.0f;
                return BTStatus::Success;
            }
            
            return BTStatus::Running;
        }
        
        case BTActionType::ChooseRandomNavigablePoint:
        {
            // Get current position
            if (!World::Get().HasComponent<Position_data>(entity))
                return BTStatus::Failure;
            
            const Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            
            // Parameters
            float searchRadius = (param1 > 0.0f) ? param1 : blackboard.wanderSearchRadius;
            int maxAttempts = (param2 > 0.0f) ? static_cast<int>(param2) : blackboard.wanderMaxSearchAttempts;
            
            // Search for a random navigable point
            float destX, destY;
            bool found = NavigationMap::Get().GetRandomNavigablePoint(
                pos.position.x, pos.position.y, searchRadius, maxAttempts, destX, destY
            );
            
            if (found)
            {
                blackboard.wanderDestination = Vector(destX, destY);
                blackboard.hasWanderDestination = true;
                blackboard.moveGoal = blackboard.wanderDestination;
                blackboard.hasMoveGoal = true;
                return BTStatus::Success;
            }
            
            return BTStatus::Failure;
        }
        
        case BTActionType::RequestPathfinding:
        {
            if (!blackboard.hasMoveGoal) return BTStatus::Failure;
            
            // Go through MoveIntent_data with pathfinding enabled
            if (World::Get().HasComponent<MoveIntent_data>(entity))
            {
                MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
                intent.targetPosition = blackboard.moveGoal;
                intent.desiredSpeed = 1.0f;
                intent.hasIntent = true;
                intent.usePathfinding = true;  // Enable pathfinding
                intent.avoidObstacles = true;
                
                return BTStatus::Success;
            }
            
            return BTStatus::Failure;
        }
        
                case BTActionType::FollowPath:
                {
                    if (!blackboard.hasWanderDestination) return BTStatus::Failure;

                    // Check if we have an active MoveIntent
                    if (World::Get().HasComponent<MoveIntent_data>(entity))
                    {
                        MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);

                        // Maintain the active intent
                        if (!intent.hasIntent)
                        {
                            intent.targetPosition = blackboard.wanderDestination;
                            intent.desiredSpeed = 1.0f;
                            intent.hasIntent = true;
                            intent.usePathfinding = true;
                        }

                        // Check if we have arrived
                        if (World::Get().HasComponent<Position_data>(entity))
                        {
                            const Position_data& pos = World::Get().GetComponent<Position_data>(entity);
                            Vector vDest = pos.position;
                            vDest -= blackboard.wanderDestination;
                            float dist = vDest.Magnitude();

                            if (dist < intent.arrivalThreshold)
                            {
                                // Arrived at destination
                                blackboard.hasWanderDestination = false;
                                blackboard.hasMoveGoal = false;
                                intent.hasIntent = false;
                                return BTStatus::Success;
                            }
                        }

                        return BTStatus::Running;
                    }

                    return BTStatus::Failure;
                }

                        case BTActionType::SendMessage:
                        {
                            // Phase 38b: Emit event to EventQueue for OnEvent node processing
                            // param1 encoded as EventType enum value

                            EventType eventType = static_cast<EventType>(static_cast<int>(param1));

                            Message msg;
                            msg.msg_type = eventType;
                            msg.domain = EventDomain::Gameplay;
                            msg.targetUid = entity;
                            msg.param1 = 0;
                            msg.param2 = 0;
                            msg.state = 0;

                            EventQueue::Get().Push(msg);

                            return BTStatus::Success;
                        }
                    }

                    return BTStatus::Failure;
                }

// --- Path-to-ID Registry Methods ---

uint32_t BehaviorTreeManager::GetTreeIdFromPath(const std::string& treePath) const
{
    auto it = m_pathToIdMap.find(treePath);
    if (it != m_pathToIdMap.end())
        return it->second;
    
    // Fallback: generate ID from path if not in registry
    // This allows forward compatibility with paths that aren't loaded yet
    return BehaviorTreeDependencyScanner::GenerateTreeIdFromPath(treePath);
}

bool BehaviorTreeManager::IsTreeLoadedByPath(const std::string& treePath) const
{
    return m_pathToIdMap.find(treePath) != m_pathToIdMap.end();
}

const BehaviorTreeAsset* BehaviorTreeManager::GetTreeByPath(const std::string& treePath) const
{
    uint32_t treeId = GetTreeIdFromPath(treePath);
    return GetTree(treeId);
}

const BehaviorTreeAsset* BehaviorTreeManager::GetTreeByAnyId(uint32_t treeId) const
{
    // Strategy 1: Direct ID lookup
    for (const auto& tree : m_trees)
    {
        if (tree.id == treeId)
            return &tree;
    }
    
    // No additional strategies currently implemented
    // Future enhancement: Could add tree structure matching for corrupted prefabs
    
    return nullptr;
}

std::string BehaviorTreeManager::GetTreePathFromId(uint32_t treeId) const
{
    // Check path-to-ID registry
    for (const auto& entry : m_pathToIdMap)
    {
        if (entry.second == treeId)
            return entry.first;
    }
    
    // Check if a tree with this ID exists (might be loaded with different path)
    for (const auto& tree : m_trees)
    {
        if (tree.id == treeId)
            return "TreeName:" + tree.name; // Prefix to distinguish from actual file path
    }
    
    return "";
}

void BehaviorTreeManager::DebugPrintLoadedTrees() const
{
    std::cout << "[BehaviorTreeManager] Loaded trees (" << m_trees.size() << "):" << std::endl;
    for (const auto& tree : m_trees)
    {
        std::cout << "  - ID=" << tree.id << " Name='" << tree.name << "' Nodes=" << tree.nodes.size() << std::endl;
    }
    
    std::cout << "[BehaviorTreeManager] Path-to-ID registry (" << m_pathToIdMap.size() << "):" << std::endl;
    for (const auto& entry : m_pathToIdMap)
    {
        std::cout << "  - '" << entry.first << "' -> ID=" << entry.second << std::endl;
    }
}

// =============================================================================
// BehaviorTreeAsset - Validation Methods
// =============================================================================

std::vector<BTValidationMessage> BehaviorTreeAsset::ValidateTreeFull() const
{
    std::vector<BTValidationMessage> messages;
    
    // Rule 1: Exactly one root node
    int rootCount = 0;
    const BTNode* rootNode = nullptr;
    for (const auto& node : nodes)
    {
        if (node.id == rootNodeId)
        {
            rootNode = &node;
            rootCount++;
        }
    }
    
    if (rootCount == 0)
    {
        BTValidationMessage msg;
        msg.severity = BTValidationMessage::Severity::Error;
        msg.nodeId = 0;
        msg.message = "No root node found (rootNodeId=" + std::to_string(rootNodeId) + ")";
        messages.push_back(msg);
        return messages; // Cannot continue without root
    }
    else if (rootCount > 1)
    {
        BTValidationMessage msg;
        msg.severity = BTValidationMessage::Severity::Error;
        msg.nodeId = 0;
        msg.message = "Multiple nodes with root ID found";
        messages.push_back(msg);
    }
    
    // Build parent count map
    std::map<uint32_t, int> parentCounts;
    for (const auto& node : nodes)
    {
        parentCounts[node.id] = 0;
    }
    
    // Count parents for each node
    for (const auto& node : nodes)
    {
        // Check childIds for composites
        if (node.type == BTNodeType::Selector || node.type == BTNodeType::Sequence)
        {
            for (uint32_t childId : node.childIds)
            {
                if (parentCounts.find(childId) != parentCounts.end())
                {
                    parentCounts[childId]++;
                }
                else
                {
                    BTValidationMessage msg;
                    msg.severity = BTValidationMessage::Severity::Error;
                    msg.nodeId = node.id;
                    msg.message = "Node references non-existent child ID " + std::to_string(childId);
                    messages.push_back(msg);
                }
            }
        }
        
        // Check decoratorChildId for decorators
        if (node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater)
        {
            if (node.decoratorChildId != 0)
            {
                if (parentCounts.find(node.decoratorChildId) != parentCounts.end())
                {
                    parentCounts[node.decoratorChildId]++;
                }
                else
                {
                    BTValidationMessage msg;
                    msg.severity = BTValidationMessage::Severity::Error;
                    msg.nodeId = node.id;
                    msg.message = "Decorator references non-existent child ID " + std::to_string(node.decoratorChildId);
                    messages.push_back(msg);
                }
            }
        }
    }
    
    // Rule 2: No node should have multiple parents
    for (const auto& entry : parentCounts)
    {
        if (entry.second > 1)
        {
            BTValidationMessage msg;
            msg.severity = BTValidationMessage::Severity::Error;
            msg.nodeId = entry.first;
            msg.message = "Node has multiple parents (count=" + std::to_string(entry.second) + ")";
            messages.push_back(msg);
        }
    }
    
    // Rule 3: Root should have no parent
    if (rootNode && parentCounts[rootNode->id] > 0)
    {
        BTValidationMessage msg;
        msg.severity = BTValidationMessage::Severity::Error;
        msg.nodeId = rootNode->id;
        msg.message = "Root node has parent(s)";
        messages.push_back(msg);
    }
    
    // Rule 4: No orphan nodes (except root)
    for (const auto& entry : parentCounts)
    {
        if (entry.second == 0 && entry.first != rootNodeId)
        {
            BTValidationMessage msg;
            msg.severity = BTValidationMessage::Severity::Warning;
            msg.nodeId = entry.first;
            msg.message = "Orphan node (no parent, not root)";
            messages.push_back(msg);
        }
    }
    
    // Rule 5: Validate node type-specific constraints
    for (const auto& node : nodes)
    {
        if (node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater)
        {
            // Decorators must have exactly 1 child
            if (node.decoratorChildId == 0)
            {
                BTValidationMessage msg;
                msg.severity = BTValidationMessage::Severity::Error;
                msg.nodeId = node.id;
                msg.message = "Decorator has no child (decoratorChildId=0)";
                messages.push_back(msg);
            }
        }
        else if (node.type == BTNodeType::Selector || node.type == BTNodeType::Sequence)
        {
            // Composites should have at least 1 child (warning if 0)
            if (node.childIds.empty())
            {
                BTValidationMessage msg;
                msg.severity = BTValidationMessage::Severity::Warning;
                msg.nodeId = node.id;
                msg.message = "Composite has no children";
                messages.push_back(msg);
            }
        }
    }
    
    // Rule 6: Detect cycles
    for (const auto& node : nodes)
    {
        if (DetectCycle(node.id))
        {
            BTValidationMessage msg;
            msg.severity = BTValidationMessage::Severity::Error;
            msg.nodeId = node.id;
            msg.message = "Cycle detected starting from this node";
            messages.push_back(msg);
        }
    }
    
    return messages;
}

// =============================================================================
// BehaviorTreeAsset - Phase 38b: Root Node Auto-Creation
// =============================================================================

void BehaviorTreeAsset::EnsureRootNodeExists()
{
    // Check if Root node already exists
    if (rootNodeId != 0)
    {
        const BTNode* rootNode = GetNode(rootNodeId);
        if (rootNode != nullptr)
        {
            return; // Root node exists and is valid
        }

        // Root ID is set but node doesn't exist - invalid, will recreate
        std::cerr << "[BehaviorTreeAsset::EnsureRootNodeExists] "
                  << "Root node ID " << rootNodeId << " not found in nodes list, creating new Root" << std::endl;
    }

    // Create default Root node if missing
    std::cout << "[BehaviorTreeAsset::EnsureRootNodeExists] Creating default Root node" << std::endl;

    BTNode rootNode;
    rootNode.id = GenerateNextNodeId();
    rootNode.type = BTNodeType::Selector;  // Default to Selector as root composite
    rootNode.name = "Root";
    rootNode.editorPosX = 0.0f;
    rootNode.editorPosY = 0.0f;

    nodes.push_back(rootNode);
    rootNodeId = rootNode.id;

    std::cout << "[BehaviorTreeAsset::EnsureRootNodeExists] "
              << "Created Root node with ID=" << rootNode.id << ", total nodes=" << nodes.size() << std::endl;
}

// =============================================================================
// BehaviorTreeAsset - Validation Methods (continued)
// =============================================================================

bool BehaviorTreeAsset::DetectCycle(uint32_t startNodeId) const
{
    std::set<uint32_t> visited;
    std::set<uint32_t> recursionStack;
    
    // DFS helper function
    std::function<bool(uint32_t)> dfs = [&](uint32_t nodeId) -> bool
    {
        if (recursionStack.find(nodeId) != recursionStack.end())
        {
            return true; // Cycle detected
        }
        
        if (visited.find(nodeId) != visited.end())
        {
            return false; // Already checked this node
        }
        
        visited.insert(nodeId);
        recursionStack.insert(nodeId);
        
        const BTNode* node = GetNode(nodeId);
        if (!node)
        {
            recursionStack.erase(nodeId);
            return false;
        }
        
        // Check children in childIds (composites)
        for (uint32_t childId : node->childIds)
        {
            if (dfs(childId))
            {
                return true;
            }
        }
        
        // Check decoratorChildId (decorators)
        if ((node->type == BTNodeType::Inverter || node->type == BTNodeType::Repeater) &&
            node->decoratorChildId != 0)
        {
            if (dfs(node->decoratorChildId))
            {
                return true;
            }
        }
        
        recursionStack.erase(nodeId);
        return false;
    };
    
    return dfs(startNodeId);
}

// =============================================================================
// BehaviorTreeAsset - Editor CRUD Operations
// =============================================================================

uint32_t BehaviorTreeAsset::GenerateNextNodeId() const
{
    uint32_t maxId = 0;
    for (const auto& node : nodes)
    {
        if (node.id > maxId)
        {
            maxId = node.id;
        }
    }
    return maxId + 1;
}

uint32_t BehaviorTreeAsset::AddNode(BTNodeType type, const std::string& name, const Vector& position)
{
    BTNode newNode;
    newNode.type = type;
    newNode.id = GenerateNextNodeId();
    newNode.name = name;
    newNode.editorPosX = position.x;
    newNode.editorPosY = position.y;

    // Set default parameters based on type
    if (type == BTNodeType::Action)
    {
        newNode.actionType = BTActionType::Idle;
        newNode.actionParam1 = 0.0f;
        newNode.actionParam2 = 0.0f;
    }
    else if (type == BTNodeType::Condition)
    {
        newNode.conditionType = BTConditionType::TargetVisible;
        newNode.conditionParam = 0.0f;
    }
    else if (type == BTNodeType::Repeater)
    {
        newNode.repeatCount = 1;
        newNode.decoratorChildId = 0;
    }
    else if (type == BTNodeType::Inverter)
    {
        newNode.decoratorChildId = 0;
    }

    nodes.push_back(newNode);

    std::cout << "[BehaviorTreeAsset] Added node ID=" << newNode.id
              << " type=" << static_cast<int>(type) << " name='" << name << "'"
              << " pos=(" << position.x << "," << position.y << ")" << std::endl;

    return newNode.id;
}

bool BehaviorTreeAsset::RemoveNode(uint32_t nodeId)
{
    // Find and remove the node
    auto it = nodes.begin();
    while (it != nodes.end())
    {
        if (it->id == nodeId)
        {
            nodes.erase(it);
            break;
        }
        ++it;
    }
    
    // Clean up connections to/from this node
    for (auto& node : nodes)
    {
        // Remove from childIds
        auto childIt = node.childIds.begin();
        while (childIt != node.childIds.end())
        {
            if (*childIt == nodeId)
            {
                childIt = node.childIds.erase(childIt);
            }
            else
            {
                ++childIt;
            }
        }
        
        // Remove from decoratorChildId
        if (node.decoratorChildId == nodeId)
        {
            node.decoratorChildId = 0;
        }
    }
    
    std::cout << "[BehaviorTreeAsset] Removed node ID=" << nodeId << std::endl;
    return true;
}

bool BehaviorTreeAsset::ConnectNodes(uint32_t parentId, uint32_t childId)
{
    BTNode* parent = GetNode(parentId);
    const BTNode* child = GetNode(childId);
    
    if (!parent || !child)
    {
        std::cerr << "[BehaviorTreeAsset] ConnectNodes failed: invalid node IDs" << std::endl;
        return false;
    }
    
    // Add connection based on parent type
    if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
    {
        // Check if already connected
        for (uint32_t id : parent->childIds)
        {
            if (id == childId)
            {
                std::cerr << "[BehaviorTreeAsset] Connection already exists" << std::endl;
                return false;
            }
        }
        
        parent->childIds.push_back(childId);
        std::cout << "[BehaviorTreeAsset] Connected composite node " << parentId << " -> " << childId << std::endl;
        return true;
    }
    else if (parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater)
    {
        if (parent->decoratorChildId != 0)
        {
            std::cerr << "[BehaviorTreeAsset] Decorator already has a child" << std::endl;
            return false;
        }
        
        parent->decoratorChildId = childId;
        std::cout << "[BehaviorTreeAsset] Connected decorator node " << parentId << " -> " << childId << std::endl;
        return true;
    }
    else
    {
        std::cerr << "[BehaviorTreeAsset] Cannot connect from leaf node" << std::endl;
        return false;
    }
}

bool BehaviorTreeAsset::DisconnectNodes(uint32_t parentId, uint32_t childId)
{
    BTNode* parent = GetNode(parentId);
    
    if (!parent)
    {
        return false;
    }
    
    // Remove from childIds
    auto it = parent->childIds.begin();
    while (it != parent->childIds.end())
    {
        if (*it == childId)
        {
            parent->childIds.erase(it);
            std::cout << "[BehaviorTreeAsset] Disconnected " << parentId << " -X-> " << childId << std::endl;
            return true;
        }
        ++it;
    }
    
    // Remove from decoratorChildId
    if (parent->decoratorChildId == childId)
    {
        parent->decoratorChildId = 0;
        std::cout << "[BehaviorTreeAsset] Disconnected decorator " << parentId << " -X-> " << childId << std::endl;
        return true;
    }

    return false;
}

// ============================================================================
// Phase 39c Step 6: SubGraph Validation Methods
// ============================================================================

bool BehaviorTreeManager::ValidateSubGraphPath(const std::string& path) const
{
    // 1. Check if path is empty
    if (path.empty())
    {
        SYSTEM_LOG << "[BehaviorTreeManager] ValidateSubGraphPath: Path is empty\n";
        return false;
    }

    // 2. Check if file exists and is readable
    std::ifstream fileTest(path.c_str());
    if (!fileTest.good())
    {
        SYSTEM_LOG << "[BehaviorTreeManager] ValidateSubGraphPath: File not found or not readable: " << path << "\n";
        return false;
    }
    fileTest.close();

    // 3. Check if it's a .bt.json file
    std::string filename = path;
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        filename = filename.substr(lastSlash + 1);
    }

    if (filename.length() < 8 || filename.substr(filename.length() - 7) != ".bt.json")
    {
        SYSTEM_LOG << "[BehaviorTreeManager] ValidateSubGraphPath: Not a .bt.json file: " << path << "\n";
        return false;
    }

    // 4. Try to parse JSON to verify it's valid
    try
    {
        json j;
        if (!JsonHelper::LoadJsonFromFile(path, j))
        {
            SYSTEM_LOG << "[BehaviorTreeManager] ValidateSubGraphPath: Failed to load JSON: " << path << "\n";
            return false;
        }
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[BehaviorTreeManager] ValidateSubGraphPath: JSON parse error: " << e.what() << "\n";
        return false;
    }

    SYSTEM_LOG << "[BehaviorTreeManager] ValidateSubGraphPath: Valid - " << path << "\n";
    return true;
}

bool BehaviorTreeManager::DetectCircularDependencies(uint32_t graphId, uint32_t nodeId, const BehaviorTreeAsset* parentTree, std::set<std::string>& visited)
{
    if (!parentTree)
    {
        return false;
    }

    const BTNode* node = parentTree->GetNode(nodeId);
    if (!node || node->type != BTNodeType::SubGraph)
    {
        return false;
    }

    // 1. Check if we've already visited this path (cycle detected)
    if (visited.find(node->subgraphPath) != visited.end())
    {
        SYSTEM_LOG << "[BehaviorTreeManager] Circular dependency detected: " << node->subgraphPath << "\n";
        return true;
    }

    // 2. Check if path is valid first
    if (!ValidateSubGraphPath(node->subgraphPath))
    {
        SYSTEM_LOG << "[BehaviorTreeManager] SubGraph path invalid for circular check: " << node->subgraphPath << "\n";
        return false;  // Invalid path is not a circular dependency, just a broken reference
    }

    // 3. Add this path to visited set
    visited.insert(node->subgraphPath);

    // 4. Try to load the SubGraph
    uint32_t subgraphId = std::hash<std::string>{}(node->subgraphPath) & 0x7FFFFFFF;

    // Load if not already loaded
    if (!IsTreeLoadedByPath(node->subgraphPath))
    {
        if (!const_cast<BehaviorTreeManager*>(this)->LoadTreeFromFile(node->subgraphPath, subgraphId))
        {
            SYSTEM_LOG << "[BehaviorTreeManager] Failed to load SubGraph for circular check: " << node->subgraphPath << "\n";
            return false;
        }
    }

    const BehaviorTreeAsset* subgraph = GetTreeByAnyId(subgraphId);
    if (!subgraph)
    {
        return false;
    }

    // 5. Recursively check all SubGraph nodes in the loaded tree
    for (const BTNode& subNode : subgraph->nodes)
    {
        if (subNode.type == BTNodeType::SubGraph)
        {
            if (DetectCircularDependencies(subgraphId, subNode.id, subgraph, visited))
            {
                return true;  // Cycle found deeper in the graph
            }
        }
    }

    // 6. Backtrack: remove from visited set for other branches
    visited.erase(node->subgraphPath);

    return false;  // No cycle in this path
}

std::vector<std::string> BehaviorTreeManager::GetValidationErrors(uint32_t graphId)
{
    std::vector<std::string> errors;

    const BehaviorTreeAsset* tree = GetTree(graphId);
    if (!tree)
    {
        errors.push_back("Graph not found with ID: " + std::to_string(graphId));
        return errors;
    }

    // 1. Check each SubGraph node
    std::set<std::string> visited;

    for (const BTNode& node : tree->nodes)
    {
        if (node.type == BTNodeType::SubGraph)
        {
            // Check 1: Empty path
            if (node.subgraphPath.empty())
            {
                errors.push_back("SubGraph node '" + node.name + "' (ID:" + std::to_string(node.id) + ") has no path specified");
                continue;
            }

            // Check 2: Path validity
            if (!ValidateSubGraphPath(node.subgraphPath))
            {
                errors.push_back("SubGraph node '" + node.name + "' (ID:" + std::to_string(node.id) + ") path invalid: " + node.subgraphPath);
                continue;
            }

            // Check 3: Circular dependency
            std::set<std::string> circularVisited;
            if (DetectCircularDependencies(graphId, node.id, tree, circularVisited))
            {
                errors.push_back("SubGraph node '" + node.name + "' (ID:" + std::to_string(node.id) + ") creates circular reference: " + node.subgraphPath);
            }
        }
    }

    return errors;
}

// ============================================================================
// Phase 38b: OnEvent Root Node Activation via EventQueue
// ============================================================================

void TickEventRoots(EventQueue& eventQueue, const BehaviorTreeAsset& tree, EntityID entity, AIBlackboard_data& blackboard)
{
    // Guard: If tree has no event roots, skip
    if (tree.m_eventRootIds.empty())
        return;

    // Get all events from EventQueue (these are frame N-1 events, now readable in frame N)
    const std::vector<Message>& events = eventQueue.GetEvents();

    // Iterate through all events
    for (const Message& msg : events)
    {
        // Phase 38b: Match event type to OnEvent nodes
        // Find OnEvent root nodes that are listening for this event type
        for (uint32_t eventRootId : tree.m_eventRootIds)
        {
            // Get the OnEvent root node
            const BTNode* onEventRoot = tree.GetNode(eventRootId);
            if (!onEventRoot)
                continue;

            // Check if this OnEvent node is listening for this event type
            // eventType field contains the EventType enum value as string (e.g., "Olympe_EventType_AI_Explosion")
            if (onEventRoot->eventType.empty())
                continue;

            // Convert message type to string for comparison
            // For now, simple enum-to-string mapping
            std::string msgTypeStr;
            switch (msg.msg_type)
            {
                case EventType::Olympe_EventType_AI_Explosion:
                    msgTypeStr = "Olympe_EventType_AI_Explosion";
                    break;
                case EventType::Olympe_EventType_AI_Noise:
                    msgTypeStr = "Olympe_EventType_AI_Noise";
                    break;
                case EventType::Olympe_EventType_AI_DamageDealt:
                    msgTypeStr = "Olympe_EventType_AI_DamageDealt";
                    break;
                case EventType::Olympe_EventType_Object_Create:
                    msgTypeStr = "Olympe_EventType_Object_Create";
                    break;
                case EventType::Olympe_EventType_Object_Destroy:
                    msgTypeStr = "Olympe_EventType_Object_Destroy";
                    break;
                default:
                    msgTypeStr = "";
                    break;
            }

            // Phase 38b: Optional event message filtering (future enhancement)
            // If eventMessage is set, only activate if message content matches
            // For now, simple event type matching

            // Check if this OnEvent node matches the event type
            if (onEventRoot->eventType == msgTypeStr)
            {
                // Execute this OnEvent root node
                std::cout << "[TickEventRoots] Entity " << entity << ": Executing OnEvent root " << eventRootId
                          << " for event '" << msgTypeStr << "'" << std::endl;

                // Execute the OnEvent root (will traverse its child tree)
                ExecuteBTNode(*onEventRoot, entity, blackboard, tree);
            }
        }
    }
}
