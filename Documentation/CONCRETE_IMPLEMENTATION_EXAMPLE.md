# Exemple Concret : Refactoring ComponentNodeRenderer avec SelectionEffectRenderer

## Avant (Code actuel)

```cpp
// ComponentNodeRenderer.cpp - Fonction RenderNodeBox()

void ComponentNodeRenderer::RenderNodeBox(const ComponentNode& node)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList == nullptr) { return; }

    Vector screenCenter = CanvasToScreen(node.position);
    float scaledWidth = node.size.x * 0.5f * m_nodeScale * m_canvasZoom;
    float scaledHeight = node.size.y * 0.5f * m_nodeScale * m_canvasZoom;

    Vector min = screenCenter;
    min.x -= scaledWidth;
    min.y -= scaledHeight;

    Vector max = screenCenter;
    max.x += scaledWidth;
    max.y += scaledHeight;

    Vector color = GetNodeColor(node);
    ImU32 bgColor = ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, 1.0f));
    ImU32 borderColor = ImGui::GetColorU32(ImVec4(
        (color.x * 1.3f > 1.0f) ? 1.0f : color.x * 1.3f,
        (color.y * 1.3f > 1.0f) ? 1.0f : color.y * 1.3f,
        (color.z * 1.3f > 1.0f) ? 1.0f : color.z * 1.3f,
        1.0f
    ));

    // ===== SELECTION GLOW (Inline Code) =====
    if (node.selected)
    {
        ImU32 glowColor = ImGui::GetColorU32(ImVec4(0.0f, 0.8f, 1.0f, 0.3f));
        float glowSize = 4.0f * m_nodeScale * m_canvasZoom;
        drawList->AddRectFilled(
            ImVec2(min.x - glowSize, min.y - glowSize),
            ImVec2(max.x + glowSize, max.y + glowSize),
            glowColor,
            m_style.cornerRadius + glowSize
        );
    }
    // ===== END SELECTION GLOW =====

    drawList->AddRectFilled(
        ImVec2(min.x, min.y),
        ImVec2(max.x, max.y),
        bgColor,
        m_style.cornerRadius
    );

    float borderWidth = node.selected ? m_style.borderWidth * 2.0f : m_style.borderWidth;
    drawList->AddRect(
        ImVec2(min.x, min.y),
        ImVec2(max.x, max.y),
        borderColor,
        m_style.cornerRadius,
        ImDrawFlags_RoundCornersAll,
        borderWidth
    );

    // ... reste du rendu (titre, labels, ports) ...
}
```

---

## Après (Code refactorisé)

### Étape 1 : Ajouter le membre dans le header

**ComponentNodeRenderer.h**
```cpp
#pragma once

#include "./../../vector.h"
#include "ComponentNodeData.h"
#include "./../../third_party/imgui/imgui.h"
#include "../SelectionEffectRenderer.h"  // ← ADD THIS

namespace Olympe
{
    class EntityPrefabGraphDocument;

    class ComponentNodeRenderer
    {
    public:
        // ... existing methods ...

    private:
        ComponentNodeStyle m_style;
        SelectionEffectRenderer m_selectionRenderer;  // ← ADD THIS
        bool m_showLabels = true;
        // ... reste des membres ...
    };
}
```

### Étape 2 : Refactoriser RenderNodeBox()

**ComponentNodeRenderer.cpp**
```cpp
void ComponentNodeRenderer::RenderNodeBox(const ComponentNode& node)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList == nullptr) { return; }

    Vector screenCenter = CanvasToScreen(node.position);
    float scaledWidth = node.size.x * 0.5f * m_nodeScale * m_canvasZoom;
    float scaledHeight = node.size.y * 0.5f * m_nodeScale * m_canvasZoom;

    Vector min = screenCenter;
    min.x -= scaledWidth;
    min.y -= scaledHeight;

    Vector max = screenCenter;
    max.x += scaledWidth;
    max.y += scaledHeight;

    Vector color = GetNodeColor(node);
    ImU32 bgColor = ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, 1.0f));
    ImU32 borderColor = ImGui::GetColorU32(ImVec4(
        (color.x * 1.3f > 1.0f) ? 1.0f : color.x * 1.3f,
        (color.y * 1.3f > 1.0f) ? 1.0f : color.y * 1.3f,
        (color.z * 1.3f > 1.0f) ? 1.0f : color.z * 1.3f,
        1.0f
    ));

    // ===== REFACTORED: Utiliser SelectionEffectRenderer =====
    if (node.selected)
    {
        m_selectionRenderer.RenderSelectionGlow(
            ImVec2(min.x, min.y),
            ImVec2(max.x, max.y),
            m_canvasZoom,
            m_nodeScale,
            m_style.cornerRadius
        );
    }
    // ===== END REFACTORED =====

    drawList->AddRectFilled(
        ImVec2(min.x, min.y),
        ImVec2(max.x, max.y),
        bgColor,
        m_style.cornerRadius
    );

    // Utiliser SelectionEffectRenderer pour la bordure aussi (optionnel)
    if (node.selected)
    {
        m_selectionRenderer.RenderSelectionBorder(
            ImVec2(min.x, min.y),
            ImVec2(max.x, max.y),
            borderColor,
            m_style.borderWidth,
            m_canvasZoom,
            m_style.cornerRadius
        );
    }
    else
    {
        drawList->AddRect(
            ImVec2(min.x, min.y),
            ImVec2(max.x, max.y),
            borderColor,
            m_style.cornerRadius,
            ImDrawFlags_RoundCornersAll,
            m_style.borderWidth
        );
    }

    // ... reste du rendu (titre, labels, ports) ...
}
```

### Étape 3 : Initialiser le style au démarrage

**ComponentNodeRenderer.cpp - Constructeur**
```cpp
ComponentNodeRenderer::ComponentNodeRenderer() 
    : m_showLabels(true), 
      m_showProperties(true), 
      m_nodeScale(1.0f)
{
    m_style.normalColor = Vector(0.3f, 0.3f, 0.3f);
    m_style.selectedColor = Vector(0.0f, 0.7f, 1.0f);
    m_style.hoverColor = Vector(0.5f, 0.5f, 0.5f);
    m_style.disabledColor = Vector(0.2f, 0.2f, 0.2f);
    m_style.textColor = Vector(1.0f, 1.0f, 1.0f);
    m_style.borderWidth = 2.0f;
    m_style.cornerRadius = 5.0f;
    
    // ← ADD THIS: Appliquer le style par défaut
    m_selectionRenderer.ApplyStyle_OlympeBlue();
}
```

---

## Comparaison Côte à Côte

### Gestion du Code

| Aspect | Avant | Après |
|--------|-------|-------|
| **Lignes glow** | 7 lignes hardcodées | 5 lignes d'appel réutilisable |
| **Maintenance** | Si changement → 10+ fichiers | Si changement → 1 fichier |
| **Testabilité** | Difficile d'isoler | Facile à unit-test |
| **Consistance** | Risque de divergence | Garantie d'uniformité |
| **Flexibilité couleur** | Hardcodée `(0.0, 0.8, 1.0)` | Configurable en runtime |

### Performance

| Métrique | Impact |
|---------|--------|
| **Overhead** | Zéro (juste des appels inline ImGui) |
| **Memory** | +24 bytes par renderer (SelectionEffectRenderer) |
| **Draw calls** | Identique (2 `AddRectFilled` + 1 `AddRect`) |

---

## Avantages Immédiats

### 1. Réutilisabilité
Vous pouvez maintenant faire :

```cpp
// Dans VisualScriptNodeRenderer.cpp
m_selectionRenderer.RenderSelectionGlow(...);

// Dans AIEditorNodeRenderer.cpp
m_selectionRenderer.RenderSelectionGlow(...);

// Dans n'importe quel autre renderer
m_selectionRenderer.RenderSelectionGlow(...);
```

### 2. Configuration Runtime
```cpp
// Changer le style en temps réel
if (userPreference == "Dark Mode")
{
    m_selectionRenderer.ApplyStyle_OlympeBlue();
}
else if (userPreference == "Highlight Mode")
{
    m_selectionRenderer.ApplyStyle_GoldAccent();
}
```

### 3. Debug Amélioré
```cpp
// Ajouter une fonction debug dans SelectionEffectRenderer
void SelectionEffectRenderer::DebugPrintSettings() const
{
    ImGui::Text("Glow Color: (%.2f, %.2f, %.2f)", 
                m_glowColor.x, m_glowColor.y, m_glowColor.z);
    ImGui::Text("Glow Alpha: %.2f", m_glowAlpha);
    ImGui::Text("Border Multiplier: %.2f", m_borderWidthMultiplier);
}
```

---

## Étapes pour Appliquer

1. **Créer les fichiers** :
   - `SelectionEffectRenderer.h` (header)
   - `SelectionEffectRenderer.cpp` (implémentation)

2. **Modifier ComponentNodeRenderer** :
   - Ajouter `#include "SelectionEffectRenderer.h"`
   - Ajouter member `SelectionEffectRenderer m_selectionRenderer`
   - Remplacer code glow dans `RenderNodeBox()`
   - Appeler `ApplyStyle_OlympeBlue()` dans constructeur

3. **Compiler et Tester** :
   ```bash
   # Vérifier compilation
   # Tester sélection de nodes → glow doit apparaître
   # Tester zoom/pan → glow doit se redimensionner
   ```

4. **Documenter** :
   - Ajouter commentaire dans ComponentNodeRenderer.h
   - Lister dans architectural decisions (ADR)

5. **Préparer pour autres canvas** :
   - Créer plan pour VisualScriptNodeRenderer
   - Créer plan pour AIEditorNodeRenderer

---

## Intégration Complète (Futur)

Une fois cette refactorisation appliquée à tous les renderers, on peut créer une interface abstraite :

```cpp
// INodeRenderer.h (nouvelle abstraction)
class INodeRenderer
{
protected:
    SelectionEffectRenderer m_selectionRenderer;
    
public:
    virtual void SetSelectionStyle(const SelectionEffectRenderer& style) final
    {
        m_selectionRenderer = style;
    }
};

// Tous les renderers héritent de INodeRenderer
class ComponentNodeRenderer : public INodeRenderer { ... };
class VisualScriptNodeRenderer : public INodeRenderer { ... };
class AIEditorNodeRenderer : public INodeRenderer { ... };
```

Cela garantit une **interface uniforme** pour tous les canvas.

