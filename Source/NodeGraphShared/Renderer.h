#pragma once

#include "NodeGraphShared.h"
#include "../AI/BehaviorTree.h"
#include "../BlueprintEditor/NodeStyleRegistry.h"
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <string>

namespace Olympe
{
    namespace NodeGraphShared
    {
        inline NodeType MapBTToEditor(BTNodeType t)
        {
            switch (t)
            {
            case BTNodeType::Selector: return NodeType::BT_Selector;
            case BTNodeType::Sequence: return NodeType::BT_Sequence;
            case BTNodeType::Condition: return NodeType::BT_Condition;
            case BTNodeType::Action: return NodeType::BT_Action;
            case BTNodeType::Inverter: return NodeType::BT_Decorator;
            case BTNodeType::Repeater: return NodeType::BT_Decorator;
            default: return NodeType::BT_Action;
            }
        }

        inline void RenderNodeVisual(int nodeId, const std::string& title, NodeType editorType,
                                     float posX, float posY, float width, float height,
                                     bool isCurrentNode, float currentZoom, float pulseTimer,
                                     float pinRadius, float pinOutline, float pinHeaderHeight,
                                     std::unordered_set<uint32_t>& positionedNodes)
        {
            if (positionedNodes.find(nodeId) == positionedNodes.end())
            {
                ImNodes::SetNodeGridSpacePos(nodeId, ImVec2(posX, posY));
                positionedNodes.insert(nodeId);
            }

            const NodeStyle& style = NodeStyleRegistry::Get().GetStyle(editorType);

            ImNodes::BeginNode(nodeId);
            ImNodes::BeginNodeTitleBar();
            NodeGraphShared::RenderNodeHeader(nodeId, style, style.icon, title, isCurrentNode, pulseTimer);
            ImNodes::EndNodeTitleBar();

            ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(0,0,0,0));
            ImNodes::PushColorStyle(ImNodesCol_PinHovered, IM_COL32(0,0,0,0));
            ImNodes::BeginInputAttribute(nodeId * 10000);
            ImGui::TextUnformatted("In");
            ImNodes::EndInputAttribute();
            ImNodes::PopColorStyle(); ImNodes::PopColorStyle();

            ImGui::TextDisabled(" ");
            ImGui::Text("Type: %s", NodeTypeToString(editorType));
            ImGui::Text("ID: %d", nodeId);

            ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(0,0,0,0));
            ImNodes::PushColorStyle(ImNodesCol_PinHovered, IM_COL32(0,0,0,0));
            ImNodes::BeginOutputAttribute(nodeId * 10000 + 1);
            ImGui::TextUnformatted("Out");
            ImNodes::EndOutputAttribute();
            ImNodes::PopColorStyle(); ImNodes::PopColorStyle();

            if (isCurrentNode)
            {
                ImU32 highlightColor = IM_COL32(255, 200, 50, 180);
                ImNodes::PushColorStyle(ImNodesCol_NodeOutline, highlightColor);
            }

            ImNodes::EndNode();

            if (isCurrentNode)
                ImNodes::PopColorStyle();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            if (drawList)
            {
                ImVec2 inputCenter = ComputePinCenterScreen(nodeId, nullptr, false, ImNodes::GetStyle().PinOffset, pinHeaderHeight, currentZoom);
                ImVec2 outputCenter = ComputePinCenterScreen(nodeId, nullptr, true, ImNodes::GetStyle().PinOffset, pinHeaderHeight, currentZoom);
                ImU32 fill = IM_COL32(66,133,244,255);
                ImU32 outline = IM_COL32(40,40,40,255);
                DrawPinCircle(drawList, inputCenter, pinRadius, fill, outline, pinOutline);
                DrawPinCircle(drawList, outputCenter, pinRadius, fill, outline, pinOutline);
            }
        }

        inline void RenderBTNode(const BTNode* node, const BTNodeLayout* layout, bool isCurrentNode,
                                 float currentZoom, float pulseTimer, const BTConfig& config,
                                 std::unordered_set<uint32_t>& positionedNodes)
        {
            if (!node || !layout) return;
            RenderNodeVisual(static_cast<int>(node->id), node->name, MapBTToEditor(node->type),
                             layout->position.x, layout->position.y, layout->width, layout->height,
                             isCurrentNode, currentZoom, pulseTimer,
                             static_cast<float>(config.pinRadius), static_cast<float>(config.pinOutlineThickness), static_cast<float>(config.pinHeaderHeight),
                             positionedNodes);
        }

        inline void RenderBTNodeConnections(const BehaviorTreeAsset* tree, uint32_t activeNodeId)
        {
            if (!tree) return;
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            if (!drawList) return;

            for (const auto& node : tree->nodes)
            {
                if (node.type == BTNodeType::Selector || node.type == BTNodeType::Sequence)
                {
                    for (uint32_t childId : node.childIds)
                    {
                        ImVec2 pPos = ImNodes::GetNodeScreenSpacePos(static_cast<int>(node.id));
                        ImVec2 pDim = ImNodes::GetNodeDimensions(static_cast<int>(node.id));
                        ImVec2 cPos = ImNodes::GetNodeScreenSpacePos(static_cast<int>(childId));
                        ImVec2 cDim = ImNodes::GetNodeDimensions(static_cast<int>(childId));

                        const float po = ImNodes::GetStyle().PinOffset;
                        ImVec2 p1(pPos.x + pDim.x - po, pPos.y + pDim.y * 0.5f);
                        ImVec2 p4(cPos.x + po, cPos.y + cDim.y * 0.5f);

                        float curve = std::max(50.0f, std::abs(p4.x - p1.x) * 0.4f);
                        ImVec2 p2(p1.x + curve, p1.y);
                        ImVec2 p3(p4.x - curve, p4.y);

                        ImU32 col = IM_COL32(100, 160, 240, 255);
                        drawList->AddBezierCubic(p1, p2, p3, p4, col, 3.0f);
                    }
                }
                else if ((node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater) && node.decoratorChildId != 0)
                {
                    ImVec2 pPos = ImNodes::GetNodeScreenSpacePos(static_cast<int>(node.id));
                    ImVec2 pDim = ImNodes::GetNodeDimensions(static_cast<int>(node.id));
                    ImVec2 cPos = ImNodes::GetNodeScreenSpacePos(static_cast<int>(node.decoratorChildId));
                    ImVec2 cDim = ImNodes::GetNodeDimensions(static_cast<int>(node.decoratorChildId));

                    const float po = ImNodes::GetStyle().PinOffset;
                    ImVec2 p1(pPos.x + pDim.x - po, pPos.y + pDim.y * 0.5f);
                    ImVec2 p4(cPos.x + po, cPos.y + cDim.y * 0.5f);

                    float curve = std::max(50.0f, std::abs(p4.x - p1.x) * 0.4f);
                    ImVec2 p2(p1.x + curve, p1.y);
                    ImVec2 p3(p4.x - curve, p4.y);

                    ImU32 col = IM_COL32(100, 160, 240, 255);
                    drawList->AddBezierCubic(p1, p2, p3, p4, col, 3.0f);
                }
            }
        }

    }
}
