# Faisabilité : Appliquer SelectionEffectRenderer au Visual Script Editor

## 🎯 Executive Summary

**Status**: ✅ **POSSIBLE MAIS AVEC CONTRAINTES**

Visual Script Editor utilise **imnodes** (composant tiers), pas un système de rendu custom. L'intégration de SelectionEffectRenderer nécessite une approche différente de Entity Prefab Editor, mais reste **faisable et bénéfique**.

**Effort estimé**: 2-3h (refactoring moyen)
**Risque technique**: Faible (imnodes API bien stable)
**Impact visuel**: Haut (améliorera l'UX significativement)

---

## 📊 Analyse Comparative

### Visual Script Editor (Actuellement)

```
Architecture: imnodes native
  ├─ ImNodes::BeginNodeEditor()
  ├─ ImNodes::BeginNode(nodeUID)
  ├─ ImNodes::BeginNodeTitleBar()
  │   └─ ImGui::TextUnformatted(nodeName)
  ├─ ImNodes::EndNodeTitleBar()
  ├─ ImNodes::BeginInputAttribute() / ImNodes::EndInputAttribute()
  ├─ ImNodes::EndNode()
  └─ ImNodes::EndNodeEditor()

Sélection actuelle (de VisualScriptNodeRenderer.cpp ligne 339-354):
  ├─ titleSelectedCol = IM_COL32(255, 220, 50, 255)  // JAUNE
  │   └─ Appliqué via ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, ...)
  ├─ isActive → ImNodesCol_NodeOutline = IM_COL32(80, 255, 80, 255)  // VERT
  └─ Aucun glow custom actuellement
```

### Problème Actuel

```
Quand un node est sélectionné:
  ✓ Titre devient JAUNE (255, 220, 50)
  ✗ Pas de glow derrière le node
  ✗ Pas de bordure épaissie
  ✗ Peu visible sur fond gris
```

### Avec SelectionEffectRenderer

```
Quand un node est sélectionné:
  ✓ Titre devient JAUNE (conservé)
  ✓ GLOW CYAN derrière le node (NOUVEAU)
  ✓ Bordure plus visible (via ImNodes)
  ✓ Beaucoup plus visible et moderne
```

---

## 🔍 Analyse Technique

### 1. Où Est Rendu la Sélection Actuelle ?

**File**: `Source/BlueprintEditor/VisualScriptNodeRenderer.cpp`
**Lines**: 339-354 (première surcharge) et 506-525 (deuxième surcharge)

```cpp
unsigned int titleSelectedCol = IM_COL32(255, 220, 50, 255);  // JAUNE

ImNodes::PushColorStyle(ImNodesCol_TitleBar,         titleCol);
ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered,  titleHoveredCol);
ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, titleSelectedCol);

if (isActive)
    ImNodes::PushColorStyle(ImNodesCol_NodeOutline, IM_COL32(80, 255, 80, 255));

ImNodes::BeginNode(nodeUID);
// ... rendu du node ...
ImNodes::EndNode();
```

**Point clé** : Les couleurs sont appliquées via imnodes **avant** ImNodes::BeginNode()
- ✅ C'est l'approche correcte pour imnodes
- ❌ On ne peut pas directement appeler SelectionEffectRenderer ici

### 2. Où est Appelée la Sélection ?

**File**: `Source/BlueprintEditor/VisualScriptEditorPanel.cpp`
**Search**: Comment savoir si un node est sélectionné ?

imnodes utilise:
```cpp
ImNodes::IsNodeSelected(nodeUID)  // Retourne bool
```

### 3. Où Est la Boucle de Rendu ?

**Expected location**: `VisualScriptEditorPanel.cpp` dans la fonction de rendu du canvas (probablement `RenderContent()` ou similaire)

```cpp
ImNodes::BeginNodeEditor();

// Pour chaque node:
for (const auto& node : graphNodes)
{
    int nodeUID = ...;
    bool isSelected = ImNodes::IsNodeSelected(nodeUID);
    
    VisualScriptNodeRenderer::RenderNode(...);
    
    // ← C'EST ICI QU'ON DOIT INJECTER LE GLOW
}

ImNodes::EndNodeEditor();
```

---

## 🚧 Contraintes et Défis

### Défi 1: imnodes Gère la Boucle de Rendu

**Problème**:
```
imnodes utilise BeginNodeEditor() / EndNodeEditor() qui gère toute la logique
On n'a pas accès direct aux ImDrawList pendant le rendu du node
```

**Solution**:
```cpp
// imnodes reçoit les appels ImNodes::BeginNode/EndNode DANS sa scope
// Les ImGui::* et ImDrawList sont APRÈS EndNodeEditor()

ImNodes::BeginNodeEditor();
for (node : nodes)
{
    ImNodes::BeginNode(uid);
    // On peut utiliser ImGui::GetWindowDrawList() ici ✓
    // Et ajouter du custom rendering avant ImNodes::EndNode() ✓
    ImNodes::EndNode();
}
ImNodes::EndNodeEditor();

// Après EndNodeEditor(), on peut utiliser ImGui::GetWindowDrawList()
// pour dessiner par-dessus les nodes ✓ (ce qui fait le glow)
```

**Verdict** : ✅ Possible en deux approches

---

## 🎯 3 Approches Possibles

### Approche 1: Glow Post-Render (Recommandée - Simple)

**Concept**: Dessiner le glow **APRÈS** que tous les nodes soient rendus

```cpp
// Dans VisualScriptEditorPanel::RenderContent()

ImNodes::BeginNodeEditor();

std::vector<int> selectedNodeUIDs;

for (const auto& node : m_graphNodes)
{
    int nodeUID = MakeNodeUID(node);
    
    if (ImNodes::IsNodeSelected(nodeUID))
    {
        selectedNodeUIDs.push_back(nodeUID);
    }
    
    VisualScriptNodeRenderer::RenderNode(...);  // Normal rendering
}

ImNodes::EndNodeEditor();

// ← APRÈS EndNodeEditor(), dessiner les glows
for (int nodeUID : selectedNodeUIDs)
{
    ImVec2 nodeMin = ImNodes::GetNodeGridSpacePos(nodeUID);
    ImVec2 nodeMax = nodeMin + ImNodes::GetNodeDimensions(nodeUID);
    
    // Adapter au screen space
    ImVec2 screenMin = ImGui::GetCursorScreenPos() + nodeMin;
    ImVec2 screenMax = screenMin + (nodeMax - nodeMin);
    
    m_selectionRenderer.RenderSelectionGlow(
        screenMin, screenMax,
        1.0f,  // imnodes n'a pas de zoom custom (zoom = 1.0)
        1.0f,  // pas de node scale
        5.0f   // corner radius
    );
}
```

**Avantages**:
- ✅ Pas de modification au VisualScriptNodeRenderer
- ✅ Glow s'affiche par-dessus les nodes (z-order parfait)
- ✅ Pas de conflit avec imnodes rendering
- ✅ Facile à implémenter

**Inconvénients**:
- ❌ Faut gérer la transformation de coordonnées (grid → screen)
- ❌ imnodes API pour GetNodeGridSpacePos() peut être limitée

---

### Approche 2: Glow In-Node (Avant EndNode)

**Concept**: Dessiner le glow AVANT que imnodes termine le rendu du node

```cpp
// Dans VisualScriptNodeRenderer::RenderNode() (nouvelle implémentation)

SelectionEffectRenderer& glowRenderer = GetGlobalSelectionRenderer();

ImNodes::BeginNode(nodeUID);

// ← NOUVEAU: Dessiner le glow ici
ImVec2 nodeMin = ImGui::GetItemRectMin();
ImVec2 nodeMax = ImGui::GetItemRectMax();

if (/* somehow detect isSelected */)  // ❌ PROBLÈME: on ne sait pas si selected ici
{
    glowRenderer.RenderSelectionGlow(nodeMin, nodeMax, 1.0f, 1.0f, 5.0f);
}

// Normal imnodes rendering
ImNodes::BeginNodeTitleBar();
ImGui::TextUnformatted(nodeName);
ImNodes::EndNodeTitleBar();
// ...

ImNodes::EndNode();
```

**Avantages**:
- ✅ Pas besoin de transformer coordonnées

**Inconvénients**:
- ❌ Impossible de détecter isSelected AVANT ImNodes::EndNode()
- ❌ Z-order problématique (glow au mauvais niveau)
- ❌ Complexe à implémenter correctement

**Verdict**: ❌ **NON RECOMMANDÉ**

---

### Approche 3: Modifier VisualScriptNodeRenderer + Callback

**Concept**: Passer un callback de rendering custom à RenderNode()

```cpp
// Dans VisualScriptNodeRenderer.h

typedef std::function<void(int nodeUID, ImVec2 min, ImVec2 max, bool isSelected)> SelectionRendererCallback;

static void RenderNode(
    ... /* tous les paramètres existants */,
    SelectionRendererCallback* glowCallback = nullptr  // ← NOUVEAU
);

// Dans VisualScriptNodeRenderer.cpp

void VisualScriptNodeRenderer::RenderNode(
    ... params ...,
    SelectionRendererCallback* glowCallback)
{
    // Déterminer si selected (imnodes fournit ça maintenant)
    bool isSelected = ImNodes::IsNodeSelected(nodeUID);
    
    if (glowCallback && isSelected)
    {
        // Calculer positions
        ImVec2 nodeMin = ...; // peut être calculé avant BeginNode()
        ImVec2 nodeMax = ...;
        
        // Appeler le callback
        (*glowCallback)(nodeUID, nodeMin, nodeMax, true);
    }
    
    // Normal rendering
    ImNodes::BeginNode(nodeUID);
    // ...
    ImNodes::EndNode();
}
```

**Avantages**:
- ✅ Flexible et extensible
- ✅ Pas de modification de l'API publique de VisualScriptEditorPanel

**Inconvénients**:
- ❌ Complique VisualScriptNodeRenderer (ajoute param callback)
- ❌ Déterminer les coordonnées du node avant BeginNode() est difficile

**Verdict**: ⚠️ **POSSIBLE MAIS COMPLEXE**

---

## 🏆 Recommandation : Approche 1 (Post-Render)

Je recommande **l'Approche 1 (Post-Render)** pour ces raisons:

1. **Simplicité** : Pas de modification au VisualScriptNodeRenderer
2. **Sécurité** : Pas de conflit avec imnodes internals
3. **Z-order Perfect** : Glow par-dessus les nodes
4. **Réutilisable** : Pattern que vous pourrez utiliser partout
5. **Maintenance** : Code centralisé dans VisualScriptEditorPanel

---

## 🛠️ Implémentation Détaillée (Approche 1)

### Étape 1: Ajouter SelectionEffectRenderer à VisualScriptEditorPanel

**File**: `Source/BlueprintEditor/VisualScriptEditorPanel.h`

```cpp
#include "SelectionEffectRenderer.h"

class VisualScriptEditorPanel : public IGraphRenderer
{
private:
    SelectionEffectRenderer m_selectionRenderer;  // ← ADD THIS
    
    // ... existing members ...
};
```

### Étape 2: Initialiser le Style

**File**: `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` (Constructor)

```cpp
VisualScriptEditorPanel::VisualScriptEditorPanel()
{
    m_selectionRenderer.ApplyStyle_OlympeBlue();  // ← ADD THIS
    // ... existing init ...
}
```

### Étape 3: Tracker les Nodes Sélectionnés

**File**: `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` (RenderContent() ou similaire)

```cpp
void VisualScriptEditorPanel::RenderContent()
{
    // ... existing code ...
    
    ImNodes::BeginNodeEditor();
    
    std::vector<int> selectedNodeUIDs;  // ← ADD THIS
    
    // Render all nodes
    for (const auto& node : m_graphNodes)
    {
        int nodeUID = MakeNodeUID(node);
        
        if (ImNodes::IsNodeSelected(nodeUID))  // ← ADD THIS
        {
            selectedNodeUIDs.push_back(nodeUID);
        }
        
        VisualScriptNodeRenderer::RenderNode(...);
    }
    
    ImNodes::EndNodeEditor();
    
    // ← ADD THIS SECTION: Draw glows for selected nodes
    DrawSelectionGlows(selectedNodeUIDs);
}
```

### Étape 4: Implémenter DrawSelectionGlows()

**File**: `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` (New Method)

```cpp
void VisualScriptEditorPanel::DrawSelectionGlows(const std::vector<int>& selectedNodeUIDs)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (drawList == nullptr) { return; }
    
    for (int nodeUID : selectedNodeUIDs)
    {
        // Get node position and size from imnodes
        ImVec2 gridPos = ImNodes::GetNodeGridSpacePos(nodeUID);
        ImVec2 nodeSize = ImNodes::GetNodeDimensions(nodeUID);
        
        // Convert to screen space
        // imnodes uses canvas offset internally
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 screenMin = canvasPos + gridPos;  // Simplified; may need adjustment
        ImVec2 screenMax = screenMin + nodeSize;
        
        // Draw glow
        m_selectionRenderer.RenderSelectionGlow(
            screenMin,
            screenMax,
            1.0f,       // imnodes n'a pas de zoom custom
            1.0f,       // pas de node scale
            5.0f        // corner radius for imnodes style
        );
    }
}
```

### Étape 5: Ajouter Getter (dans VisualScriptEditorPanel.h)

```cpp
private:
    void DrawSelectionGlows(const std::vector<int>& selectedNodeUIDs);
```

---

## 🧪 Test & Validation

### Checklist

- [ ] Compiler sans erreurs
- [ ] Ouvrir Visual Script Editor
- [ ] Sélectionner un node
  - ✓ Titre doit rester JAUNE (imnodes style)
  - ✓ **NOUVEAU**: Glow CYAN doit apparaître derrière
  - ✓ Sélection doit être **très visible maintenant**
- [ ] Sélectionner plusieurs nodes
  - ✓ Tous les nodes sélectionnés doivent avoir le glow
- [ ] Hover sur node non-sélectionné
  - ✓ Pas de glow (correct)
- [ ] Pan/Zoom le canvas
  - ✓ Glow doit rester attaché au node
- [ ] Performance check
  - ✓ Pas de baisse de FPS (ajout = 1 draw call par node sélectionné)

---

## 📊 Comparaison Visuelle

### Avant (Actuel)
```
Node jaune (juste le titre)
┌─────────────────┐
│ SetBBValue      │ ← Titre JAUNE, peu visible
├─────────────────┤
│ Variable │ Value│
└─────────────────┘
Pas de glow = facile à manquer quand zoom out
```

### Après (Avec SelectionEffectRenderer)
```
Node avec glow cyan
      ╭─────────────────╮
      │ ┌─────────────┐ │
      │ │SetBBValue   │ │ ← Titre JAUNE (conservé)
      │ │─────────────│ │ + GLOW CYAN lumineux
      │ │ Var │ Value│ │
      │ └─────────────┘ │
      ╰─────────────────╯

BEAUCOUP plus visible, même zoom out
```

---

## 🎨 Personnalisation (Optionnel)

Si vous voulez changer la couleur du glow pour Visual Script:

```cpp
// Dans VisualScriptEditorPanel::VisualScriptEditorPanel()

// Option 1: Utiliser un preset
m_selectionRenderer.ApplyStyle_OlympeBlue();  // Cyan (défaut)

// Option 2: Custom color
m_selectionRenderer.SetGlowColor({1.0f, 0.84f, 0.0f});  // Or
m_selectionRenderer.SetGlowAlpha(0.25f);

// Option 3: Match le jaune existant
m_selectionRenderer.SetGlowColor({1.0f, 1.0f, 0.0f});  // Jaune lumineux
m_selectionRenderer.SetGlowAlpha(0.2f);
```

---

## ⚠️ Pièges à Éviter

### Piège 1: Transformer les Coordonnées
**❌ ERREUR**: 
```cpp
ImVec2 screenPos = gridPos * zoom + canvasOffset;  // FAUX
```

**✅ CORRECT**: 
```cpp
ImVec2 screenPos = canvasPos + gridPos;  // imnodes gère le zoom
```

### Piège 2: Appeler GetNodeGridSpacePos() Avant EndNodeEditor()
**❌ ERREUR**:
```cpp
for (node : nodes)
{
    ImVec2 pos = ImNodes::GetNodeGridSpacePos(nodeUID);  // NULL
    ImNodes::RenderNode(...);
}
```

**✅ CORRECT**:
```cpp
ImNodes::EndNodeEditor();
for (nodeUID : selectedNodeUIDs)
{
    ImVec2 pos = ImNodes::GetNodeGridSpacePos(nodeUID);  // ✓ Valide
}
```

### Piège 3: Z-Order (Glow Doit Être Derrière)
**❌ ERREUR**: Dessiner le glow APRÈS ImGui::EndNodeEditor()
- Glow apparaît par-dessus tout (mauvais z-order)

**✅ CORRECT**: Dessiner dans le même DrawList IMMÉDIATEMENT après EndNodeEditor()
- ImGui::GetWindowDrawList() continue dans la même couche

---

## 📈 Impact Estimé

### Effort
| Tâche | Temps |
|-------|-------|
| Ajouter SelectionEffectRenderer member | 5 min |
| Implémenter DrawSelectionGlows() | 30 min |
| Intégrer dans boucle de rendu | 15 min |
| Tester et valider | 30 min |
| **Total** | **~1.5h** |

### Risk
| Risque | Probabilité | Impact | Mitigation |
|--------|------------|--------|-----------|
| Conflit imnodes API | Bas | Moyen | Bien documenté |
| Coordonnées invalides | Bas | Moyen | Tester sys coords |
| Performance baisse | Bas | Faible | +1 call par node |
| Glow incorrect z-order | Moyen | Haut | Dessiner post-EndNodeEditor |

---

## ✅ Conclusion

**Status**: ✅ **Hautement Faisable**

- Visual Script Editor peut adopter SelectionEffectRenderer
- imnodes n'est pas un obstacle (post-render pattern simple)
- Effort minimal (~1.5h)
- Amélioration visuelle significative
- Risques techniques faibles

**Recommended Next Step**: Implémenter Approche 1 (Post-Render)

---

## 📚 References

**Files to examine**:
- `Source/BlueprintEditor/VisualScriptNodeRenderer.cpp` (lines 506-525)
- `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` (render loop)
- `Source/third_party/imnodes.h` (for ImNodes API)

**Related Documentation**:
- `EXECUTIVE_SUMMARY.md`
- `SELECTION_EFFECT_INTEGRATION_GUIDE.md` (Pattern 2 for external API)
- `CONCRETE_IMPLEMENTATION_EXAMPLE.md`

