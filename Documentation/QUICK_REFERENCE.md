# SelectionEffectRenderer - Quick Reference Card

## One-Page Cheat Sheet

### Inclusion
```cpp
#include "BlueprintEditor/SelectionEffectRenderer.h"

// Dans votre classe renderer
SelectionEffectRenderer m_selectionRenderer;
```

### Basic Usage
```cpp
// Appel simple
if (node.selected)
{
    m_selectionRenderer.RenderSelectionGlow(
        minScreen, maxScreen,      // ImVec2 positions
        canvasZoom,                // float, typically 0.1 - 3.0
        nodeScale,                 // float, typically 1.0
        cornerRadius               // float, typically 5.0
    );
}
```

### Complete Effect
```cpp
if (node.selected)
{
    m_selectionRenderer.RenderCompleteSelection(
        minScreen, maxScreen, borderColor, baseWidth,
        canvasZoom, nodeScale, cornerRadius
    );
}
```

---

## Configuration

### Color
```cpp
m_selectionRenderer.SetGlowColor({0.0f, 0.8f, 1.0f});  // RGB
```

### Transparency (0.0 = invisible, 1.0 = opaque)
```cpp
m_selectionRenderer.SetGlowAlpha(0.3f);  // 30% alpha
```

### Glow Size (pixels before zoom)
```cpp
m_selectionRenderer.SetBaseGlowSize(4.0f);
```

### Border Thickness Multiplier
```cpp
m_selectionRenderer.SetBorderWidthMultiplier(2.0f);  // ×2 épaisseur
```

### Enable/Disable (useful for performance)
```cpp
m_selectionRenderer.SetGlowEnabled(true);
```

---

## Presets

### Apply in Constructor
```cpp
m_selectionRenderer.ApplyStyle_OlympeBlue();      // Cyan (défaut)
m_selectionRenderer.ApplyStyle_GoldAccent();      // Or
m_selectionRenderer.ApplyStyle_GreenEnergy();     // Vert
m_selectionRenderer.ApplyStyle_PurpleMystery();   // Violet
m_selectionRenderer.ApplyStyle_RedAlert();        // Rouge
```

---

## Common Patterns

### Pattern 1: Minimal Integration
```cpp
class YourRenderer
{
private:
    SelectionEffectRenderer m_selectionRenderer;
    
public:
    void RenderNode(const YourNode& node)
    {
        // ... calculate minScreen, maxScreen ...
        
        if (node.selected)
        {
            m_selectionRenderer.RenderSelectionGlow(
                minScreen, maxScreen, zoom, scale, cornerRadius);
        }
        
        // ... render box normally ...
    }
};
```

### Pattern 2: With Border
```cpp
if (node.selected)
{
    m_selectionRenderer.RenderCompleteSelection(
        minScreen, maxScreen, borderColor, 
        2.0f,     // baseWidth
        zoom, scale, cornerRadius
    );
}
else
{
    // Normal border
    drawList->AddRect(..., 1 borderWidth);
}
```

### Pattern 3: Custom Color per State
```cpp
if (node.selected)
{
    m_selectionRenderer.ApplyStyle_OlympeBlue();
    m_selectionRenderer.RenderSelectionGlow(...);
}
else if (node.hasError)
{
    m_selectionRenderer.ApplyStyle_RedAlert();
    m_selectionRenderer.RenderSelectionGlow(...);
}
else if (node.isHovered)
{
    m_selectionRenderer.SetGlowAlpha(0.15f);
    m_selectionRenderer.RenderSelectionGlow(...);
}
```

---

## API Reference

### Setters
```cpp
SetGlowColor(const Vector& color)
SetGlowAlpha(float alpha)                    // 0.0 - 1.0
SetBaseGlowSize(float size)                  // pixels
SetBorderWidthMultiplier(float multiplier)   // > 0.1
SetGlowEnabled(bool enabled)
```

### Getters
```cpp
Vector GetGlowColor() const
float GetGlowAlpha() const
float GetBaseGlowSize() const
float GetBorderWidthMultiplier() const
bool IsGlowEnabled() const
```

### Rendering
```cpp
void RenderSelectionGlow(
    const ImVec2& minScreen,
    const ImVec2& maxScreen,
    float canvasZoom = 1.0f,
    float nodeScale = 1.0f,
    float cornerRadius = 5.0f
) const;

void RenderSelectionBorder(
    const ImVec2& minScreen,
    const ImVec2& maxScreen,
    ImU32 borderColor,
    float baseWidth,
    float canvasZoom = 1.0f,
    float cornerRadius = 5.0f
) const;

void RenderCompleteSelection(
    const ImVec2& minScreen,
    const ImVec2& maxScreen,
    ImU32 borderColor,
    float baseWidth,
    float canvasZoom = 1.0f,
    float nodeScale = 1.0f,
    float cornerRadius = 5.0f
) const;
```

### Presets
```cpp
void ApplyStyle_OlympeBlue()
void ApplyStyle_GoldAccent()
void ApplyStyle_GreenEnergy()
void ApplyStyle_PurpleMystery()
void ApplyStyle_RedAlert()
```

---

## Preset Specs

| Name | Color RGB | Alpha | Glow Size | Border Mult |
|------|-----------|-------|-----------|-------------|
| OlympeBlue | (0, 0.8, 1) | 0.30 | 4.0 | 2.0 |
| GoldAccent | (1, 0.84, 0) | 0.25 | 5.0 | 2.5 |
| GreenEnergy | (0, 1, 0.5) | 0.35 | 4.0 | 2.0 |
| PurpleMystery | (0.8, 0.2, 1) | 0.30 | 4.5 | 2.0 |
| RedAlert | (1, 0.2, 0.2) | 0.40 | 5.0 | 2.5 |

---

## Default Values

```cpp
glowColor            = (0.0f, 0.8f, 1.0f)     // Cyan
glowAlpha            = 0.3f                   // 30%
baseGlowSize         = 4.0f                   // pixels
borderWidthMultiplier = 2.0f                  // ×2
glowEnabled          = true
```

---

## Troubleshooting

### Glow not visible
- ❌ Called AFTER drawing box (z-order wrong)
- ✅ Call BEFORE drawing box

### Glow not scaling with zoom
- ❌ canvasZoom = 1.0f (not updating)
- ✅ Pass actual zoom value

### Border too thick
- ❌ baseWidth × multiplier too high
- ✅ Reduce SetBorderWidthMultiplier()

### Performance issue
- ❌ Rendering 10000 nodes
- ✅ SetGlowEnabled(false) for large graphs

### Color weird
- ❌ Alpha too low (0.05) or too high (1.0)
- ✅ Use 0.25-0.4 range

---

## Files to Include

**Header**:
```cpp
#include "BlueprintEditor/SelectionEffectRenderer.h"
```

**In your CMakeLists.txt or Visual Studio**:
```
Source/BlueprintEditor/SelectionEffectRenderer.h
Source/BlueprintEditor/SelectionEffectRenderer.cpp
```

---

## Integration Checklist

- [ ] Add `#include "SelectionEffectRenderer.h"`
- [ ] Add `SelectionEffectRenderer m_selectionRenderer;` member
- [ ] Call `RenderSelectionGlow()` or `RenderCompleteSelection()` when node selected
- [ ] Apply style in constructor: `m_selectionRenderer.ApplyStyle_OlympeBlue();`
- [ ] Test: Select node → verify glow appears
- [ ] Test: Zoom/pan → verify glow scales correctly
- [ ] Test: Multi-select → verify all selected nodes show glow

---

## Example: Complete Integration (5 minutes)

```cpp
// YourNodeRenderer.h
#include "SelectionEffectRenderer.h"

class YourNodeRenderer
{
private:
    SelectionEffectRenderer m_selectionRenderer;

public:
    YourNodeRenderer()
    {
        m_selectionRenderer.ApplyStyle_OlympeBlue();
    }

    void RenderNode(const YourNode& node)
    {
        ImVec2 minScreen = ...; // calculate
        ImVec2 maxScreen = ...;
        
        // NEW: 3 lines only
        if (node.selected)
        {
            m_selectionRenderer.RenderSelectionGlow(
                minScreen, maxScreen, m_zoom, 1.0f, 5.0f);
        }
        
        // ... normal render (box, border, labels) ...
    }
};
```

Done ! ✅

---

## Performance Notes

- **Draw Calls**: +1 per selected node per frame (minimal)
- **Memory**: +28 bytes per renderer instance
- **CPU**: Zero overhead on logic (just ImGui API calls)
- **GPU**: Negligible (added 1 AddRectFilled call)

For 100 nodes with 50% selected: +50 calls (< 0.5ms GPU time)

---

## Contact / Questions

See `SELECTION_EFFECT_INTEGRATION_GUIDE.md` for detailed docs.

