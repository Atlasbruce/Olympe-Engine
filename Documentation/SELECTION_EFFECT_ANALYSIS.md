# Analyse : Système de Sélection des Nodes (Entity Prefab Editor)

## Vue d'ensemble
L'effet de sélection des nodes dans l'Entity Prefab Editor est implémenté via un système **multi-couches** combinant :
1. **État de sélection** (propriété `bool selected` dans ComponentNode)
2. **Rendering en couches** (glow → fond → bordure)
3. **Interactions utilisateur** (clic, rectangle de sélection, Ctrl+clic)
4. **Style configurable** (couleurs, épaisseurs, rayons de coin)

---

## Architecture Détaillée

### 1. **Modèle de Données (ComponentNode)**
```cpp
struct ComponentNode
{
    NodeId nodeId;
    bool selected = false;  // ← État de sélection
    Vector position;
    Vector size;
    std::string componentType;
    std::string componentName;
    bool enabled;
    std::map<std::string, std::string> properties;
    std::vector<NodePort> ports;
};
```
**Point clé** : La sélection est un simple `bool` stocké dans le node lui-même.

### 2. **Système de Rendu (ComponentNodeRenderer)**

#### 2.1 Rendu de la Boîte du Node
**Fonction** : `RenderNodeBox(const ComponentNode& node)`

**Ordre de rendu (du plus profond au plus superficiel)** :

```
┌─────────────────────────────────────┐
│   ÉTAPE 4: Bordure (border)         │ ← Épaissie si sélectionné (×2)
├─────────────────────────────────────┤
│   ÉTAPE 3: Boîte principale         │ ← Couleur basée sur l'état
├─────────────────────────────────────┤
│   ÉTAPE 2: Barre de titre (darker)  │ ← Teinte plus sombre
├─────────────────────────────────────┤
│   ÉTAPE 1: GLOW (si sélectionné)    │ ← Cyan semi-transparent (★)
└─────────────────────────────────────┘
```

#### 2.2 L'Effet de GLOW (Le Cœur de l'UX)

**Code** (ComponentNodeRenderer.cpp, ligne ~165-173) :
```cpp
// Draw selection glow if node is selected
if (node.selected)
{
    ImU32 glowColor = ImGui::GetColorU32(ImVec4(0.0f, 0.8f, 1.0f, 0.3f));
    //                                                        ↑ CYAN VIVE
    float glowSize = 4.0f * m_nodeScale * m_canvasZoom;
    drawList->AddRectFilled(
        ImVec2(min.x - glowSize, min.y - glowSize),
        ImVec2(max.x + glowSize, max.y + glowSize),
        glowColor,
        m_style.cornerRadius + glowSize
    );
}
```

**Caractéristiques** :
- **Couleur** : Cyan `(0.0f, 0.8f, 1.0f)` à 30% d'opacité
- **Taille** : `4.0f` pixels (mise à l'échelle par zoom et nodeScale)
- **Extension** : Rectangle "gonflé" de `glowSize` pixels de tous côtés
- **Coins arrondis** : Respecte le rayon de coin du node

#### 2.3 Couleurs Adaptatives du Node

```cpp
Vector GetNodeColor(const ComponentNode& node) const
{
    if (!node.enabled) {
        return m_style.disabledColor;
    }
    if (node.selected) {
        return m_style.selectedColor;  // Cyan (0.0f, 0.7f, 1.0f)
    }
    return m_style.normalColor;  // Gris (0.3f, 0.3f, 0.3f)
}
```

**Palette par défaut** :
| État | Couleur RGB | Cas d'usage |
|------|-----------|-----------|
| **Normal** | (0.3, 0.3, 0.3) | Node non sélectionné |
| **Sélectionné** | (0.0, 0.7, 1.0) | Node avec focus |
| **Hover** | (0.5, 0.5, 0.5) | (réservé pour interactions futures) |
| **Désactivé** | (0.2, 0.2, 0.2) | Node avec `enabled=false` |

#### 2.4 Épaisseur de Bordure Adaptative

```cpp
float borderWidth = node.selected ? m_style.borderWidth * 2.0f : m_style.borderWidth;
drawList->AddRect(..., borderWidth);
```

**Effet** : La bordure **double d'épaisseur** quand sélectionné (normallement 2.0f → 4.0f)

---

### 3. **Gestion de la Sélection (PrefabCanvas)**

#### 3.1 Interaction à un Node

```cpp
void PrefabCanvas::SelectNodeAt(float x, float y, bool addToSelection)
{
    NodeId nodeId = GetNodeAtPosition(x, y);
    if (nodeId != InvalidNodeId)
    {
        if (!addToSelection)
        {
            m_document->DeselectAll();  // Effacer sélection précédente
        }
        m_document->SelectNode(nodeId);  // Ajouter à la sélection
    }
}
```

#### 3.2 Sélection par Rectangle

```cpp
void PrefabCanvas::SelectNodesInRectangle(const Vector& rectStart, const Vector& rectEnd, bool addToSelection)
{
    // ... normaliser rectangle ...
    
    for (const auto& node : m_document->GetAllNodes())
    {
        // Test AABB intersection
        if (NodeIntersectsRectangle(node, minX, maxX, minY, maxY))
        {
            if (!addToSelection) m_document->DeselectAll();
            m_document->SelectNode(node.nodeId);
        }
    }
}
```

#### 3.3 Flux d'Interaction Utilisateur

```
Clic souris
    ↓
[Ctrl+Clic ?]
    ├─ OUI → addToSelection = true  (multi-sélection)
    └─ NON → addToSelection = false (sélection unique)
    ↓
GetNodeAtPosition(x, y)  ← Hit detection
    ↓
IsPointInNode() [AABB test]
    ↓
SelectNodeAt() → SelectNode() → [node.selected = true]
    ↓
RenderNodeBox() détecte node.selected = true
    ↓
Dessine GLOW + couleur cyan + bordure épaisse
```

---

## Paramètres de Style

### ComponentNodeStyle
```cpp
struct ComponentNodeStyle
{
    Vector normalColor;      // (0.3f, 0.3f, 0.3f)
    Vector selectedColor;    // (0.0f, 0.7f, 1.0f) ← CYAN
    Vector hoverColor;       // (0.5f, 0.5f, 0.5f)
    Vector disabledColor;    // (0.2f, 0.2f, 0.2f)
    Vector textColor;        // (1.0f, 1.0f, 1.0f)
    float borderWidth;       // 2.0f
    float cornerRadius;      // 5.0f
};
```

### Paramètres Dynamiques

| Paramètre | Formule | Effet |
|-----------|---------|-------|
| **Glow Size** | `4.0f * nodeScale * canvasZoom` | Adapté au zoom et échelle du node |
| **Border Width** | `borderWidth * (selected ? 2.0f : 1.0f)` | Double si sélectionné |
| **Text Color** | Brightened 1.3× de la couleur du node | Plus lumineux pour contraste |

---

## Transformation des Coordonnées

### Canvas Space → Screen Space
```cpp
Vector ComponentNodeRenderer::CanvasToScreen(const Vector& canvasPos) const
{
    Vector screenPos = canvasPos;
    screenPos.x = screenPos.x * m_canvasZoom + m_canvasOffset.x + m_canvasScreenPos.x;
    screenPos.y = screenPos.y * m_canvasZoom + m_canvasOffset.y + m_canvasScreenPos.y;
    return screenPos;
}
```

**Cela assure** : Le glow se redimensionne correctement au zoom

---

## Avantages de Cette Implémentation

### ✅ Réutilisabilité
- **Style configurable** : Toutes les couleurs/épaisseurs sont des champs modifiables
- **Canvas-agnostique** : Fonctionne avec n'importe quel système de zoom/pan
- **Scalable** : Tous les effets s'adaptent au zoom

### ✅ Performance
- **Pas d'animation complexe** : Juste un rendu multi-couches immédia
- **Pas de shader GPU** : Utilise ImGui DrawList API (CPU-friendly)
- **Hit detection rapide** : AABB simple, pas d'octree complexe

### ✅ UX Claire
- **4 signaux visuels** en même temps :
  1. Glow bleu lumineux (le plus visible)
  2. Couleur du node changée en cyan
  3. Bordure doublée d'épaisseur
  4. Les panels adjacents affichent les propriétés
  
- **Hiérarchie visuelle** : Impossible de manquer un node sélectionné

---

## Blueprint pour Wrapper Réutilisable

### Classe Abstraite : INodeSelectionRenderer

```cpp
class INodeSelectionRenderer
{
public:
    virtual ~INodeSelectionRenderer() = default;
    
    // Style configuration
    virtual void SetSelectionGlowColor(const Vector& color) = 0;
    virtual void SetSelectionBorderMultiplier(float multiplier) = 0;
    virtual void SetGlowSize(float baseSize) = 0;
    
    // Rendering
    virtual void RenderSelectionGlow(const ImVec2& min, const ImVec2& max, 
                                     float zoom, float scale) = 0;
    virtual void RenderSelectionBorder(const ImVec2& min, const ImVec2& max,
                                       ImU32 color, float baseWidth, float zoom) = 0;
};
```

### Implémentation Concrète : SelectionEffectRenderer

```cpp
class SelectionEffectRenderer : public INodeSelectionRenderer
{
private:
    Vector m_glowColor = {0.0f, 0.8f, 1.0f};  // Cyan
    float m_glowAlpha = 0.3f;
    float m_borderMultiplier = 2.0f;
    float m_baseGlowSize = 4.0f;
    
public:
    void RenderSelectionGlow(...) override
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float glowSize = m_baseGlowSize * scale * zoom;
        ImU32 glowColor = ImGui::GetColorU32(ImVec4(
            m_glowColor.x, m_glowColor.y, m_glowColor.z, m_glowAlpha
        ));
        drawList->AddRectFilled(
            ImVec2(min.x - glowSize, min.y - glowSize),
            ImVec2(max.x + glowSize, max.y + glowSize),
            glowColor,
            cornerRadius + glowSize
        );
    }
};
```

---

## Points d'Intégration pour Autres Canvas

Pour intégrer cette UX dans d'autres canvas (VisualScript, AI Editor, etc.) :

1. **Ajouter** `bool selected` au node data structure
2. **Implémenter** `GetNodeColor()` adaptée (palette propre ou génériques)
3. **Appeler** le wrapper SelectionEffectRenderer avant de dessiner la boîte du node
4. **Wirer** les interactions (clic, Ctrl+clic, rectangle) via InputHandler
5. **Configurer** le style via SetSelectionGlowColor(), SetBorderMultiplier(), etc.

---

## Résumé Visuel

```
State Machine de la Sélection:

    [Node Unselected]
           ↑
           │ Clic               Clic
           │                  (dans vide)
           └──────────────────────┘
           
    [Node Selected]
           ↑
           │ (node.selected = true)
           │
    Rendu:
    • GLOW bleu (0,0,0,1,0,0,3)
    • Couleur CYAN (0,0,7,1,0)  
    • Bordure ×2 épaisseur
    • Labels + Properties
```

