/**
 * @file RenderHelpers.h
 * @brief Helper functions for rendering node graph pins and connections
 */

#pragma once

#include "../third_party/imgui/imgui.h"
#include "../vector.h"
#include <cstdint>

namespace Olympe {

// Forward declarations
struct BTNodeLayout;

namespace NodeGraphShared {

/**
 * @brief Compute the screen-space center position of a pin
 * 
 * Calculates where a pin should be rendered on screen, accounting for:
 * - Node screen position and dimensions
 * - Pin type (input/output)
 * - Header offset
 * - Zoom level
 * 
 * @param nodeId The ImNodes node ID
 * @param layout The node's layout information (optional, used for validation)
 * @param isOutput True for output pin (right side), false for input pin (left side)
 * @param pinOffset Horizontal offset from node edge (from ImNodes::GetStyle().PinOffset)
 * @param headerHeight Height of the node header in pixels (pins are positioned below header)
 * @param zoom Current zoom level (affects spacing)
 * @return ImVec2 The screen-space center position of the pin
 */
ImVec2 ComputePinCenterScreen(
    int nodeId,
    const BTNodeLayout* layout,
    bool isOutput,
    float pinOffset,
    float headerHeight,
    float zoom
);

/**
 * @brief Draw a pin circle with outline
 * 
 * Renders a circular pin with two layers:
 * - Outer circle (outline) for contrast
 * - Inner circle (main color)
 * 
 * @param drawList ImGui draw list to render to
 * @param center Center position of the pin in screen space
 * @param radius Inner radius of the pin
 * @param color Inner circle color (RGBA)
 * @param outlineColor Outline color (RGBA)
 * @param outlineThickness Thickness of the outline in pixels
 */
void DrawPinCircle(
    ImDrawList* drawList,
    const ImVec2& center,
    float radius,
    uint32_t color,
    uint32_t outlineColor,
    float outlineThickness
);

} // namespace NodeGraphShared
} // namespace Olympe
