# Guide d'Intégration du SelectionEffectRenderer

## Vue d'ensemble
Le `SelectionEffectRenderer` est un wrapper réutilisable qui encapsule l'effet de sélection des nodes (glow + bordure épaisse). Il peut être intégré dans **n'importe quel canvas** de l'éditeur.

---

## Architecture Générale

### Before (Code dupliqué dans chaque renderer)
```cpp
// Dans ComponentNodeRenderer.cpp
void ComponentNodeRenderer::RenderNodeBox(const ComponentNode& node)
{
    // ... calcul des positions ...
    
    // Glow + Bordure épaisse = code copié/collé partout
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
    // ... reste du rendu ...
}
```

### After (Avec SelectionEffectRenderer)
```cpp
// Dans ComponentNodeRenderer.cpp
void ComponentNodeRenderer::RenderNodeBox(const ComponentNode& node)
{
    // ... calcul des positions ...
    
    // Une seule ligne !
    if (node.selected)
    {
        m_selectionRenderer.RenderSelectionGlow(
            ImVec2(min.x, min.y), ImVec2(max.x, max.y),
            m_canvasZoom, m_nodeScale, m_style.cornerRadius
        );
    }
    
    // ... reste du rendu ...
}
```

---

## Patterns d'Intégration

### Pattern 1 : Intégration Minimal (Copy-Paste Easy)

Pour **ajouter le glow à un canvas existant** sans refactoring majeur :

```cpp
// Dans YourCanvasRenderer.h
#include "SelectionEffectRenderer.h"

class YourCanvasRenderer
{
private:
    SelectionEffectRenderer m_selectionRenderer;  // ← Ajouter
};

// Dans YourCanvasRenderer.cpp
void YourCanvasRenderer::RenderNodeBox(const YourNodeType& node)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Positions écran calculées
    ImVec2 minScreen = ...; // Your calculation
    ImVec2 maxScreen = ...;
    
    // ===== NOUVEAU CODE (3 lignes) =====
    if (node.selected)
    {
        m_selectionRenderer.RenderSelectionGlow(
            minScreen, maxScreen, m_canvasZoom, m_nodeScale, m_cornerRadius);
    }
    // ===== FIN NOUVEAU CODE =====
    
    // Rendu de la boîte normal
    drawList->AddRectFilled(minScreen, maxScreen, bgColor, cornerRadius);
    
    // Bordure (adapté)
    float borderWidth = node.selected ? baseWidth * 2.0f : baseWidth;
    drawList->AddRect(minScreen, maxScreen, borderColor, cornerRadius, 
                      ImDrawFlags_RoundCornersAll, borderWidth);
}
```

**Avantages** :
- ✅ Minimal code changes
- ✅ Peut être appliqué immédiatement
- ✅ Pas de dépendances externes

---

### Pattern 2 : Refactoring Propre (Recommandé à long terme)

Pour **intégrer complètement** et utiliser l'interface unifiée :

```cpp
// Dans INodeRenderer.h (nouvelle abstraction)
class INodeRenderer
{
public:
    virtual ~INodeRenderer() = default;
    
    /// Rend un node unique avec tous ses effets
    virtual void RenderNode(const INodeData* node) = 0;
    
    /// Configure le style de sélection
    virtual void SetSelectionStyle(const SelectionEffectRenderer& style) = 0;
    
protected:
    SelectionEffectRenderer m_selectionRenderer;
};

// Dans ComponentNodeRenderer.cpp (hérite de INodeRenderer)
class ComponentNodeRenderer : public INodeRenderer
{
public:
    void RenderNode(const INodeData* node) override
    {
        const ComponentNode* compNode = static_cast<const ComponentNode*>(node);
        
        // Calcul des positions
        ImVec2 minScreen = ..., maxScreen = ...;
        
        // Rendu du glow si sélectionné
        if (compNode->selected)
        {
            m_selectionRenderer.RenderSelectionGlow(
                minScreen, maxScreen, m_canvasZoom, m_nodeScale, m_cornerRadius);
        }
        
        // Rendu de la boîte
        drawList->AddRectFilled(minScreen, maxScreen, bgColor, m_cornerRadius);
        
        // Rendu de la bordure
        float borderWidth = compNode->selected 
            ? m_baseWidth * m_selectionRenderer.GetBorderWidthMultiplier()
            : m_baseWidth;
        m_selectionRenderer.RenderSelectionBorder(
            minScreen, maxScreen, borderColor, m_baseWidth, m_canvasZoom, m_cornerRadius);
        
        // Rendu des labels, ports, etc.
        RenderNodeLabel(*compNode);
        RenderNodePorts(*compNode);
    }
};
```

---

## Exemples Spécifiques par Canvas

### 1. Entity Prefab Editor (Déjà Implémenté)

**Fichiers à modifier** :
- `ComponentNodeRenderer.h` - Ajouter `SelectionEffectRenderer m_selectionRenderer`
- `ComponentNodeRenderer.cpp` - Appeler la méthode glow

**Changement minimal** :
```cpp
// ComponentNodeRenderer.cpp - RenderNodeBox()

// Avant
if (node.selected)
{
    ImU32 glowColor = ImGui::GetColorU32(ImVec4(0.0f, 0.8f, 1.0f, 0.3f));
    float glowSize = 4.0f * m_nodeScale * m_canvasZoom;
    drawList->AddRectFilled(...);
}

// Après
if (node.selected)
{
    m_selectionRenderer.RenderSelectionGlow(
        ImVec2(min.x, min.y), ImVec2(max.x, max.y),
        m_canvasZoom, m_nodeScale, m_style.cornerRadius);
}
```

---

### 2. Visual Script Editor (VisualScriptNodeRenderer)

**Intégration** :
```cpp
// VisualScriptNodeRenderer.h
#include "SelectionEffectRenderer.h"

class VisualScriptNodeRenderer
{
private:
    SelectionEffectRenderer m_selectionRenderer;
    float m_baseGlowSize = 4.0f;
};

// VisualScriptNodeRenderer.cpp
void VisualScriptNodeRenderer::RenderNode(const VisualScriptNode& node)
{
    // Déterminer si le node est sélectionné (imnodes API)
    bool isNodeSelected = ImNodes::IsNodeSelected(node.nodeId);
    
    if (isNodeSelected)
    {
        m_selectionRenderer.SetGlowColor({0.2f, 0.8f, 1.0f});  // Cyan un peu bleu
        ImVec2 nodeMin = ImGui::GetItemRectMin();
        ImVec2 nodeMax = ImGui::GetItemRectMax();
        
        m_selectionRenderer.RenderSelectionGlow(
            nodeMin, nodeMax, 
            1.0f,  // VisualScript n'a pas de zoom custom
            1.0f,  // et pas de node scale
            6.0f   // coins plus arrondis pour imnodes style
        );
    }
    
    // Rendre le node avec imnodes
    ImNodes::BeginNode(node.nodeId);
    // ... content ...
    ImNodes::EndNode();
}
```

---

### 3. AI Editor (AIEditorNodeRenderer)

**Intégration** :
```cpp
// AIEditorNodeRenderer.cpp
void AIEditorNodeRenderer::RenderNode(const AINode& node, bool isHovered)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 nodePos = GetNodeScreenPos(node.id);
    ImVec2 nodeSize = GetNodeScreenSize(node.id);
    
    ImVec2 minScreen = nodePos;
    ImVec2 maxScreen = ImVec2(nodePos.x + nodeSize.x, nodePos.y + nodeSize.y);
    
    // Glow doré pour les nodes sélectionnées
    if (m_selectedNodeIds.find(node.id) != m_selectedNodeIds.end())
    {
        m_selectionRenderer.ApplyStyle_GoldAccent();  // Style or
        m_selectionRenderer.RenderSelectionGlow(
            minScreen, maxScreen, GetCanvasZoom(), 1.0f, 4.0f);
    }
    
    // Glow rouge pour les erreurs
    if (node.hasError)
    {
        m_selectionRenderer.ApplyStyle_RedAlert();
        m_selectionRenderer.RenderSelectionGlow(
            minScreen, maxScreen, GetCanvasZoom(), 1.0f, 4.0f);
    }
    
    // Rendu normal de la boîte
    drawList->AddRectFilled(minScreen, maxScreen, GetNodeColor(node), 4.0f);
    drawList->AddRect(minScreen, maxScreen, GetBorderColor(node), 4.0f, 
                      ImDrawFlags_RoundCornersAll, 1.5f);
}
```

---

## Configuration du Style

### Option 1 : Presets Prédéfinis

```cpp
// Au démarrage du canvas
m_selectionRenderer.ApplyStyle_OlympeBlue();   // Cyan (défaut)
// ou
m_selectionRenderer.ApplyStyle_GoldAccent();   // Or
// ou
m_selectionRenderer.ApplyStyle_GreenEnergy();  // Vert
// ou
m_selectionRenderer.ApplyStyle_PurpleMystery(); // Violet
// ou
m_selectionRenderer.ApplyStyle_RedAlert();     // Rouge
```

### Option 2 : Configuration Personnalisée

```cpp
// Custom style
SelectionEffectRenderer customRenderer;
customRenderer.SetGlowColor({0.5f, 0.2f, 0.8f});  // Purple custom
customRenderer.SetGlowAlpha(0.25f);                // Plus transparent
customRenderer.SetBaseGlowSize(6.0f);              // Glow plus grand
customRenderer.SetBorderWidthMultiplier(3.0f);    // Bordure plus épaisse
```

### Option 3 : Configuration par ImGui

```cpp
// Dans RenderSettings()
if (ImGui::CollapsingHeader("Selection Effect"))
{
    float glowColor[3] = {
        m_selectionRenderer.GetGlowColor().x,
        m_selectionRenderer.GetGlowColor().y,
        m_selectionRenderer.GetGlowColor().z
    };
    if (ImGui::ColorEdit3("Glow Color", glowColor))
    {
        m_selectionRenderer.SetGlowColor({glowColor[0], glowColor[1], glowColor[2]});
    }
    
    float glowAlpha = m_selectionRenderer.GetGlowAlpha();
    if (ImGui::SliderFloat("Glow Alpha", &glowAlpha, 0.0f, 1.0f))
    {
        m_selectionRenderer.SetGlowAlpha(glowAlpha);
    }
    
    float baseGlowSize = m_selectionRenderer.GetBaseGlowSize();
    if (ImGui::SliderFloat("Glow Size", &baseGlowSize, 0.0f, 10.0f))
    {
        m_selectionRenderer.SetBaseGlowSize(baseGlowSize);
    }
    
    float borderMult = m_selectionRenderer.GetBorderWidthMultiplier();
    if (ImGui::SliderFloat("Border Multiplier", &borderMult, 0.5f, 4.0f))
    {
        m_selectionRenderer.SetBorderWidthMultiplier(borderMult);
    }
}
```

---

## Checklist d'Intégration

Pour intégrer SelectionEffectRenderer dans un nouveau canvas :

- [ ] Ajouter `#include "SelectionEffectRenderer.h"` dans le header du renderer
- [ ] Ajouter `SelectionEffectRenderer m_selectionRenderer;` comme membre
- [ ] Dans la fonction de rendu du node, ajouter appel `RenderSelectionGlow()` après calcul des positions
- [ ] Adapter l'épaisseur de bordure pour utiliser le multiplicateur (ou `RenderSelectionBorder()`)
- [ ] Tester la sélection de nodes et vérifier que le glow apparaît
- [ ] Configurer le style approprié pour le canvas (ou laisser le défaut Olympe Blue)
- [ ] (Optionnel) Ajouter UI de configuration si nécessaire

---

## Cas Spéciaux

### A. Nodes Hover (Pas Sélectionnés)

Vous pouvez ajouter un effet hover distinct :

```cpp
if (isNodeHovered && !node.selected)
{
    SelectionEffectRenderer hoverRenderer;
    hoverRenderer.SetGlowColor({0.7f, 0.7f, 0.7f});  // Gris
    hoverRenderer.SetGlowAlpha(0.15f);                 // Très transparent
    hoverRenderer.RenderSelectionGlow(
        minScreen, maxScreen, zoom, scale, cornerRadius);
}
```

### B. Multi-Sélection avec Groupes

```cpp
// Mettre en évidence les nodes du groupe sélectionné
const std::vector<NodeId>& selectedGroup = GetSelectedGroup();

for (const auto& node : allNodes)
{
    bool isInGroup = std::find(selectedGroup.begin(), selectedGroup.end(), node.id) 
                   != selectedGroup.end();
    
    if (isInGroup)
    {
        m_selectionRenderer.SetGlowColor(GetGroupColor(node.groupId));
        m_selectionRenderer.RenderSelectionGlow(
            nodeMin, nodeMax, zoom, scale, cornerRadius);
    }
}
```

### C. Performance : Désactiver le Glow Pour Grandes Graphs

```cpp
// Si le graph a 1000+ nodes, désactiver le glow
if (totalNodeCount > 1000)
{
    m_selectionRenderer.SetGlowEnabled(false);
}
else
{
    m_selectionRenderer.SetGlowEnabled(true);
}
```

---

## Dépannage

| Problème | Cause | Solution |
|----------|-------|----------|
| Glow ne s'affiche pas | Appelé APRÈS la boîte du node | Appeler AVANT la boîte (il faut un ordre de rendu) |
| Glow ne s'adapte pas au zoom | Zoom non passé en paramètre | Vérifier `canvasZoom` est != 1.0f |
| Bordure trop épaisse | Multiplicateur trop élevé | Diminuer `SetBorderWidthMultiplier()` |
| Couleur glow étrange | Alpha trop bas ou haut | Ajuster `SetGlowAlpha()` (0.25-0.4 est standard) |
| Performance baisse | Glow recalculé chaque frame | Utiliser `SetGlowEnabled(false)` pour grands graphs |

---

## Summary

Le `SelectionEffectRenderer` fournit :
- ✅ **Réutilisabilité** : Une seule implémentation pour tous les canvas
- ✅ **Flexibilité** : Configurable couleur/alpha/taille
- ✅ **Performance** : Zéro overhead, juste des appels ImGui
- ✅ **Consistency** : Même style UX partout dans l'éditeur
- ✅ **Extensibilité** : Presets et options de personnalisation

