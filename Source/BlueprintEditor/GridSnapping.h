/**
 * @file GridSnapping.h
 * @brief Grid-snapping helper for the VS graph canvas (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * GridSnapping is a lightweight singleton that aligns node positions to a
 * configurable grid.  When disabled, Snap() / SnapX() / SnapY() return the
 * input value unchanged so callers need no extra branch.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

namespace Olympe {

// ============================================================================
// GridSnapping
// ============================================================================

/**
 * @class GridSnapping
 * @brief Singleton grid-snapping helper.
 *
 * Typical usage:
 * @code
 *   auto& gs = GridSnapping::Get();
 *   gs.SetEnabled(true);
 *   gs.SetGridSize(16);
 *   float x = 123.7f, y = 88.2f;
 *   gs.Snap(x, y);   // x == 128.0f, y == 80.0f
 * @endcode
 */
class GridSnapping {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the single shared instance.
     */
    static GridSnapping& Get();

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    bool IsEnabled()  const;
    void SetEnabled(bool enabled);

    /**
     * @brief Toggles snapping on/off.
     */
    void Toggle();

    // -----------------------------------------------------------------------
    // Grid size
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the current grid cell size in pixels (default: 16).
     */
    int  GetGridSize() const;

    /**
     * @brief Sets the grid cell size.  Values <= 0 are ignored.
     */
    void SetGridSize(int size);

    // -----------------------------------------------------------------------
    // Snapping
    // -----------------------------------------------------------------------

    /**
     * @brief Snaps a single x coordinate to the nearest grid line.
     */
    float SnapX(float x) const;

    /**
     * @brief Snaps a single y coordinate to the nearest grid line.
     */
    float SnapY(float y) const;

    /**
     * @brief Snaps both x and y coordinates in place.
     */
    void Snap(float& x, float& y) const;

private:

    GridSnapping();

    bool m_Enabled;
    int  m_GridSize;
};

} // namespace Olympe
