#include "CanvasGridRenderer.h"
#include <cmath>

namespace Olympe
{
    CanvasGridRenderer::GridConfig CanvasGridRenderer::GetStylePreset(GridStylePreset preset)
    {
        GridConfig config;

        switch (preset)
        {
            case Style_VisualScript:
                // Professional dark blue background with subtle gray grid (imnodes StyleColorsDark defaults)
                // Verified against native imnodes grid appearance (Phase 5)
                config.majorSpacing = 24.0f;
                config.minorDivisor = 1.0f;  // No minor lines
                config.backgroundColor = ImVec4(38.0f/255.0f, 38.0f/255.0f, 47.0f/255.0f, 1.0f);  // #26262FFF (imnodes native)
                config.majorLineColor = ImVec4(63.0f/255.0f, 63.0f/255.0f, 71.0f/255.0f, 1.0f);  // #3F3F47FF (imnodes native grid line)
                config.minorLineColor = ImVec4(63.0f/255.0f, 63.0f/255.0f, 71.0f/255.0f, 0.5f); // Same as major with reduced alpha
                config.majorLineThickness = 1.0f;
                config.minorLineThickness = 0.5f;
                break;

            case Style_Compact:
                // Tight grid for dense layouts
                config.majorSpacing = 16.0f;
                config.minorDivisor = 2.0f;
                config.backgroundColor = ImVec4(16.0f/255.0f, 16.0f/255.0f, 16.0f/255.0f, 224.0f/255.0f);   // Darker
                config.majorLineColor = ImVec4(160.0f/255.0f, 160.0f/255.0f, 160.0f/255.0f, 80.0f/255.0f);
                config.minorLineColor = ImVec4(80.0f/255.0f, 80.0f/255.0f, 80.0f/255.0f, 30.0f/255.0f);
                config.majorLineThickness = 1.5f;
                config.minorLineThickness = 0.5f;
                break;

            case Style_Spacious:
                // Wide grid for spacious layouts
                config.majorSpacing = 50.0f;
                config.minorDivisor = 5.0f;
                config.backgroundColor = ImVec4(32.0f/255.0f, 32.0f/255.0f, 48.0f/255.0f, 176.0f/255.0f);  // Softer
                config.majorLineColor = ImVec4(96.0f/255.0f, 96.0f/255.0f, 96.0f/255.0f, 48.0f/255.0f);
                config.minorLineColor = ImVec4(64.0f/255.0f, 64.0f/255.0f, 64.0f/255.0f, 24.0f/255.0f);
                config.majorLineThickness = 1.0f;
                config.minorLineThickness = 0.5f;
                break;
        }

        return config;
    }

    void CanvasGridRenderer::RenderGrid(ImDrawList* drawList, const GridConfig& config)
    {
        if (drawList == nullptr)
            return;

        ImVec2 canvasEnd(config.canvasPos.x + config.canvasSize.x, 
                         config.canvasPos.y + config.canvasSize.y);

        // Draw canvas background
        ImU32 backgroundColor = ImGui::GetColorU32(config.backgroundColor);
        drawList->AddRectFilled(config.canvasPos, canvasEnd, backgroundColor);

        // Convert colors to ImU32
        ImU32 majorGridColor = ImGui::GetColorU32(config.majorLineColor);
        ImU32 minorGridColor = ImGui::GetColorU32(config.minorLineColor);

        // ========== Grid Calculation ==========
        // Scale grid spacing by zoom level to maintain consistent grid density
        float scaledGridSpacing = config.majorSpacing * config.zoom;
        float scaledMinorSpacing = config.minorDivisor > 0.0f ? scaledGridSpacing / config.minorDivisor : scaledGridSpacing;

        // Calculate grid offset in screen space
        // FIX #3: Pan offset is already in screen space, don't multiply by zoom
        // Grid origin = canvasPos + offset (NOT offset * zoom)
        // The zoom scaling is applied to the grid spacing, not the offset
        float gridStartX = config.canvasPos.x + config.offsetX;
        float gridStartY = config.canvasPos.y + config.offsetY;

        // Calculate which grid lines should be visible and their screen positions
        float gridOffsetX = fmod(gridStartX, scaledGridSpacing);
        float gridOffsetY = fmod(gridStartY, scaledGridSpacing);

        // Handle negative modulos (important for negative offsets)
        if (gridOffsetX < 0) gridOffsetX += scaledGridSpacing;
        if (gridOffsetY < 0) gridOffsetY += scaledGridSpacing;

        // ========== Draw Minor Grid Lines (subtle background pattern) ==========
        if (config.minorDivisor > 1.0f)
        {
            // Vertical minor lines
            for (float x = config.canvasPos.x + gridOffsetX - scaledGridSpacing; 
                 x < canvasEnd.x + scaledMinorSpacing; 
                 x += scaledMinorSpacing)
            {
                drawList->AddLine(
                    ImVec2(x, config.canvasPos.y), 
                    ImVec2(x, canvasEnd.y), 
                    minorGridColor, 
                    config.minorLineThickness
                );
            }

            // Horizontal minor lines
            for (float y = config.canvasPos.y + gridOffsetY - scaledGridSpacing; 
                 y < canvasEnd.y + scaledMinorSpacing; 
                 y += scaledMinorSpacing)
            {
                drawList->AddLine(
                    ImVec2(config.canvasPos.x, y), 
                    ImVec2(canvasEnd.x, y), 
                    minorGridColor, 
                    config.minorLineThickness
                );
            }
        }

        // ========== Draw Major Grid Lines (prominent pattern) ==========
        // Vertical major lines
        for (float x = config.canvasPos.x + gridOffsetX - scaledGridSpacing; 
             x < canvasEnd.x + scaledGridSpacing; 
             x += scaledGridSpacing)
        {
            drawList->AddLine(
                ImVec2(x, config.canvasPos.y), 
                ImVec2(x, canvasEnd.y), 
                majorGridColor, 
                config.majorLineThickness
            );
        }

        // Horizontal major lines
        for (float y = config.canvasPos.y + gridOffsetY - scaledGridSpacing; 
             y < canvasEnd.y + scaledGridSpacing; 
             y += scaledGridSpacing)
        {
            drawList->AddLine(
                ImVec2(config.canvasPos.x, y), 
                ImVec2(canvasEnd.x, y), 
                majorGridColor, 
                config.majorLineThickness
            );
        }
    }

    void CanvasGridRenderer::RenderGrid(const GridConfig& config)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        RenderGrid(drawList, config);
    }
}
