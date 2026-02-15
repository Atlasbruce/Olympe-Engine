/**
 * @file RenderSort.h
 * @brief Rendering sort key helpers for ECS sprite sorting
 * @author Olympe Engine Team
 * @date 2025
 * 
 * Purpose: Provides helper functions to generate sort keys for sprite rendering.
 * Supports layer-based sorting, isometric depth sorting, and hexagonal grid sorting.
 * 
 * Sort keys are used to order sprites for correct depth rendering in 2D games.
 * Lower keys are rendered first (background), higher keys are rendered last (foreground).
 */

#pragma once

#include "vector.h"
#include "ECS_Components.h"

/**
 * @namespace RenderSort
 * @brief Rendering sort key utilities
 * 
 * This namespace provides functions to calculate sort keys for sprite rendering.
 * Sort keys determine the rendering order of sprites to achieve correct depth.
 */
namespace RenderSort
{
    // Constants for sort key calculation
    constexpr float LAYER_SCALE = 10000.0f;      // Scale factor for layer separation
    constexpr float DIAGONAL_SCALE = 100.0f;     // Scale for isometric X+Y diagonal
    constexpr float X_SCALE = 0.1f;              // Tie-breaker for X position
    constexpr float Y_SCALE = 1.0f;              // Primary scale for Y position
    constexpr float HEX_ROW_SCALE = 100.0f;      // Scale for hexagonal row sorting
    constexpr float HEX_COL_SCALE = 0.1f;        // Tie-breaker for hexagonal column
    
    /**
     * @brief Calculate sort key based on render layer only
     * @param layer Render layer enumeration
     * @return Sort key (float)
     * 
     * Simple layer-based sorting. Sprites in higher layers render on top.
     * No position-based sorting within the same layer.
     */
    inline float GetSortKeyLayer(RenderLayer layer)
    {
        return static_cast<float>(static_cast<int>(layer)) * LAYER_SCALE;
    }
    
    /**
     * @brief Calculate sort key for layer + Y position (standard 2D)
     * @param layer Render layer enumeration
     * @param worldPos World position of sprite
     * @return Sort key (float)
     * 
     * Standard 2D sorting: Layer first, then Y position (lower Y = further back).
     * Suitable for top-down or side-scrolling games with Y-axis depth.
     */
    inline float GetSortKeyLayerY(RenderLayer layer, const Vector& worldPos)
    {
        float layerKey = GetSortKeyLayer(layer);
        return layerKey + (worldPos.y * Y_SCALE);
    }
    
    /**
     * @brief Calculate sort key for isometric rendering
     * @param layer Render layer enumeration
     * @param worldPos World position of sprite
     * @return Sort key (float)
     * 
     * Isometric sorting: Layer first, then diagonal (X+Y) sum for depth.
     * Objects with higher X+Y sum render in front (closer to camera).
     * X coordinate used as tie-breaker for sprites at same diagonal.
     */
    inline float GetSortKeyIsometric(RenderLayer layer, const Vector& worldPos)
    {
        float layerKey = GetSortKeyLayer(layer);
        float diagonal = (worldPos.x + worldPos.y) * DIAGONAL_SCALE;
        float tieBreaker = worldPos.x * X_SCALE;
        return layerKey + diagonal + tieBreaker;
    }
    
    /**
     * @brief Calculate sort key for hexagonal axial grid rendering
     * @param layer Render layer enumeration
     * @param worldPos World position of sprite
     * @param hexRadius Radius of hexagonal cells
     * @return Sort key (float)
     * 
     * Hexagonal sorting: Layer first, then row-major order in hex grid.
     * Converts world position to hexagonal axial coordinates (q, r).
     * Sprites in higher rows (larger r) render in front.
     * Column (q) used as tie-breaker within same row.
     */
    inline float GetSortKeyHexagonal(RenderLayer layer, const Vector& worldPos, float hexRadius)
    {
        // Convert world position to hexagonal axial coordinates (q, r)
        // Pointy-top hexagon layout
        float q = ((worldPos.x * 0.866025404f) / hexRadius) - ((worldPos.y * 0.5f) / hexRadius);
        float r = worldPos.y / hexRadius;
        
        // Sort by row first (r), then column (q) as tie-breaker
        float layerKey = GetSortKeyLayer(layer);
        float rowKey = r * HEX_ROW_SCALE;
        float colKey = q * HEX_COL_SCALE;
        return layerKey + rowKey + colKey;
    }
    
    /**
     * @brief Calculate sort key using automatic projection detection
     * @param layer Render layer enumeration
     * @param worldPos World position of sprite
     * @param projection Grid projection type (Ortho/Iso/HexAxial)
     * @param hexRadius Hexagon radius (only used for hexagonal projection)
     * @return Sort key (float)
     * 
     * Convenience function that selects the appropriate sorting method
     * based on the current grid projection type. Falls back to layer-only
     * sorting if projection type is unknown.
     */
    inline float GetSortKeyAuto(RenderLayer layer, const Vector& worldPos, 
                               GridProjection projection, float hexRadius = 16.0f)
    {
        switch (projection)
        {
            case GridProjection::Ortho:
                return GetSortKeyLayerY(layer, worldPos);
                
            case GridProjection::Isometric:
                return GetSortKeyIsometric(layer, worldPos);
                
            case GridProjection::HexAxial:
                return GetSortKeyHexagonal(layer, worldPos, hexRadius);
                
            default:
                return GetSortKeyLayer(layer);
        }
    }
    
    /**
     * @brief Calculate sort key from sprite component data
     * @param sprite VisualSprite_data component containing layer and position
     * @param worldPos World position of sprite
     * @param projection Grid projection type
     * @param hexRadius Hexagon radius (only used for hexagonal projection)
     * @return Sort key (float)
     * 
     * Helper that extracts layer from sprite component and calculates
     * sort key using automatic projection detection.
     */
    inline float GetSortKeyFromSprite(const VisualSprite_data& sprite, const Vector& worldPos,
                                     GridProjection projection, float hexRadius = 16.0f)
    {
        RenderLayer layer = ZToLayer(sprite.layerZ);
        return GetSortKeyAuto(layer, worldPos, projection, hexRadius);
    }
}
