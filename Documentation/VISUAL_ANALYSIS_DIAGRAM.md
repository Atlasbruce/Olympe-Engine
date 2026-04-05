# Analyse Visuelle : Effet de Sélection des Nodes

## 1. Couches de Rendu (Z-Order)

```
┌────────────────────────────────────────────────────────┐
│                    LAYER 4 (TOP)                       │
│              Border (Épaisseur ×2 si sélectionné)     │
│              ImDrawFlags_RoundCornersAll              │
│              borderWidth × borderMultiplier            │
└────────────────────────────────────────────────────────┘
                          ↑
┌────────────────────────────────────────────────────────┐
│                    LAYER 3                             │
│           Node Main Box (Fond coloré)                 │
│           bgColor = GetNodeColor(node)                 │
│           - Normal: (0.3, 0.3, 0.3) gris             │
│           - Selected: (0.0, 0.7, 1.0) cyan           │
│           - Disabled: (0.2, 0.2, 0.2) très gris      │
└────────────────────────────────────────────────────────┘
                          ↑
┌────────────────────────────────────────────────────────┐
│                    LAYER 2                             │
│           Title Bar (teinte plus sombre)              │
│           darkColor = baseColor × 0.8                  │
│           Hauteur: 25.0f pixels                        │
└────────────────────────────────────────────────────────┘
                          ↑
┌────────────────────────────────────────────────────────┐
│                  LAYER 1 (BOTTOM)                       │
│        Selection Glow (si node.selected = true)        │
│        Couleur: (0.0, 0.8, 1.0, 0.3) CYAN semi-trans  │
│        Taille: 4.0f pixels (adapté zoom × scale)       │
│        Coin: cornerRadius + glowSize                   │
│        Rendu AVANT le main box (pour voir la glow)    │
└────────────────────────────────────────────────────────┘
```

---

## 2. État Visual : Node Sélectionné vs Non Sélectionné

### Node NON SÉLECTIONNÉ
```
┌─────────────────────────────────────┐
│  Gris foncé (0.3, 0.3, 0.3)        │
│  Bordure fine (2.0f pixels)         │
│  ┌───────────────────────────────┐  │  ← Titre (teinte sombre)
│  │ AudioSource_data             │  │
│  ├───────────────────────────────┤  │
│  │ soundEffectID: volume: 1      │  │
│  │                               │  │
│  └───────────────────────────────┘  │
│  Port ○               Port ●         │
└─────────────────────────────────────┘
```

### Node SÉLECTIONNÉ (Avec Glow)
```
        ╭─────────────────────────────────────╮  ← Glow cyan
        │  ┌─────────────────────────────────┐│  (4px padding)
        │  │ Cyan vif (0.0, 0.7, 1.0)       ││
        │  │ Bordure épaisse (4.0f pixels)  ││
        │  │ ┌───────────────────────────┐  ││  ← Titre (teinte cyan)
        │  │ │ AudioSource_data          │  ││
        │  │ ├───────────────────────────┤  ││
        │  │ │ soundEffectID: volume: 1  │  ││
        │  │ │                           │  ││
        │  │ └───────────────────────────┘  ││
        │  │ Port ○               Port ●     ││
        │  └─────────────────────────────────┘│
        ╰─────────────────────────────────────╯
```

---

## 3. Flux de Rendu (RenderNodeBox)

```
START
  │
  ├─ Transformer position canvas → screen
  │  screenCenter = CanvasToScreen(node.position)
  │
  ├─ Calculer bounding box
  │  min = screenCenter - (scaledWidth, scaledHeight)
  │  max = screenCenter + (scaledWidth, scaledHeight)
  │
  ├─ Déterminer couleurs
  │  bgColor = GetNodeColor(node)
  │  borderColor = brighten(bgColor, 1.3×)
  │
  └─ RENDU EN ORDRE (z-ordering) :
      │
      ├─ [1] SI node.selected → RenderSelectionGlow()
      │         glowColor = (0.0, 0.8, 1.0, 0.3)
      │         glowSize = 4.0 × zoom × scale
      │         AddRectFilled(min-glow, max+glow)
      │
      ├─ [2] AddRectFilled(min, max) ← Main box
      │         color = bgColor
      │         cornerRadius = 5.0
      │
      ├─ [3] AddRect(min, max) ← Border
      │         borderWidth = selected ? 4.0 : 2.0
      │
      ├─ [4] SI m_showLabels → RenderNodeLabel()
      │
      ├─ [5] RenderNodePorts()
      │
      └─ [6] SI m_showProperties → RenderProperties()

END
```

---

## 4. Paramètres d'Ajustement

### Glow Customization Matrix

```
┌──────────────────┬──────────────────┬──────────────────┐
│  Aspect Ratio    │  Current Value   │  Recommendation  │
├──────────────────┼──────────────────┼──────────────────┤
│ Glow Color RGB   │ (0.0, 0.8, 1.0)  │ Cyan vivid ✓     │
│ Glow Alpha       │ 0.3 (30%)        │ 0.25-0.4 zone    │
│ Glow Size        │ 4.0 pixels       │ 3-6 pixels       │
│ Border Mult      │ 2.0×             │ 1.5-3.0× zone    │
│ Corner Radius    │ 5.0              │ 4-8 pixels       │
│ Zoom Scaling     │ glowSize×zoom    │ Adaptatif ✓      │
│ Scale Scaling    │ glowSize×scale   │ Per-node ✓       │
└──────────────────┴──────────────────┴──────────────────┘
```

### Impact Visuel des Paramètres

#### Effect 1: Glow Alpha (transparence)
```
Alpha = 0.1 (très transparent)
    │  ┌─────────────┐
    │  │ Node        │  ← Glow quasi invisible
    │  └─────────────┘

Alpha = 0.3 (défaut)
    ╭─┼──────────────┼─╮  ← Glow clairement visible
    │  ┌─────────────┐  │
    │  │ Node        │  │
    │  └─────────────┘  │
    ╰─┼──────────────┼─╯

Alpha = 0.6 (très opaque)
    ╭──────────────────╮  ← Glow opaque, peut occulter
    │  ┌─────────────┐  │
    │  │ Node        │  │
    │  └─────────────┘  │
    ╰──────────────────╯
```

#### Effect 2: Border Multiplier
```
Multiplier = 1.0× (pas d'effet)
    ┌─────────────┐
    │ Node        │  ← Bordure normale
    └─────────────┘

Multiplier = 2.0× (défaut)
    ┃━━━━━━━━━━━━━┃
    ┃ Node        ┃  ← Bordure doublée
    ┃━━━━━━━━━━━━━┃

Multiplier = 3.0× (très épais)
    ┏━━━━━━━━━━━━━━━┓
    ┃ ┌─────────────┐┃
    ┃ │ Node        │┃  ← Bordure très épaisse
    ┃ └─────────────┘┃
    ┗━━━━━━━━━━━━━━━┛
```

#### Effect 3: Glow Size
```
Size = 2.0 pixels (petit glow)
    ╮ ┌─────────────┐ ╭
    ├ │ Node        │ ┤
    ╯ └─────────────┘ ╰

Size = 4.0 pixels (défaut)
    ╭─┼──────────────┼─╮
    │ ┌─────────────┐ │
    │ │ Node        │ │
    │ └─────────────┘ │
    ╰─┼──────────────┼─╯

Size = 8.0 pixels (grand glow)
    ╭────┼──────────────────┼────╮
    │    │ ┌─────────────┐  │    │
    │    │ │ Node        │  │    │
    │    │ └─────────────┘  │    │
    ╰────┼──────────────────┼────╯
```

---

## 5. Transformation des Coordonnées

### Canvas Space → Screen Space (Pour Glow Correctness)

```
Input: node.position (Canvas Logical Space)
  └─ Exemple: (100, 150) - absolu dans le graph

Transform Step 1: Appliquer le zoom
  └─ zoomedPos = logicalPos × zoom
  └─ Exemple: (100, 150) × 1.5 = (150, 225)

Transform Step 2: Appliquer le pan offset
  └─ offsetPos = zoomedPos + canvasOffset
  └─ Exemple: (150, 225) + (-50, 100) = (100, 325)

Transform Step 3: Ajouter la position écran du canvas
  └─ screenPos = offsetPos + canvasScreenPos
  └─ Exemple: (100, 325) + (10, 10) = (110, 335)

Output: screenPos = (110, 335) - coordonnée écran finale

GLOW CALCULATION:
  glowSize = 4.0 × nodeScale × zoom
  └─ Exemple: 4.0 × 1.0 × 1.5 = 6.0 pixels
  
  glowMin = screenPos - glowSize = (104, 329)
  glowMax = screenPos + glowSize = (116, 341)
```

**Résultat** : Le glow se redimensionne automatiquement avec le zoom et le node scale ! ✓

---

## 6. Comparaison Avant/Après Refactoring

### AVANT (Code Inline)
```
ComponentNodeRenderer::RenderNodeBox()
  ├─ 7 lignes de code glow
  │   (hardcodé, non réutilisable)
  └─ Difficile à maintenir
  
VisualScriptNodeRenderer::RenderNode()
  ├─ 7 lignes identiques copiées/collées
  └─ Risque de divergence
  
AIEditorNodeRenderer::RenderNode()
  ├─ 7 lignes légèrement différentes
  └─ Inconsistent visual style
```

### APRÈS (Avec SelectionEffectRenderer)
```
ComponentNodeRenderer::RenderNodeBox()
  ├─ m_selectionRenderer.RenderSelectionGlow(...)
  └─ 1 ligne, centalisée
  
VisualScriptNodeRenderer::RenderNode()
  ├─ m_selectionRenderer.RenderSelectionGlow(...)
  └─ 1 ligne identique, réutilisable
  
AIEditorNodeRenderer::RenderNode()
  ├─ m_selectionRenderer.RenderSelectionGlow(...)
  └─ 1 ligne identique, réutilisable
```

**Avantage** : Modification unique → Change partout ✓

---

## 7. Hiérarchie Visuelle Complète

```
ÉCRAN UTILISATEUR

┌──────────────────────────────────────────────────────────┐
│  Canvas (Grille visible)                                │
│                                                          │
│   ┌────────────────────────────────────────────┐        │
│   │  Node 1 (Sélectionné)                      │        │
│   │  Glow CYAN visible                         │        │
│   │  Bordure ÉPAISSE (4.0f)                    │        │
│   │  Couleur CYAN (0.0, 0.7, 1.0)             │        │
│   │  ┌──────────────────────────────────────┐ │        │
│   │  │ AudioSource_data                      │ │        │
│   │  ├──────────────────────────────────────┤ │        │
│   │  │ soundEffectID: | volume: 1           │ │        │
│   │  └──────────────────────────────────────┘ │        │
│   └────────────────────────────────────────────┘        │
│                                                          │
│   ┌────────────────────────────────────────────┐        │
│   │  Node 2 (Non sélectionné)                 │        │
│   │  Pas de glow                              │        │
│   │  Bordure normale (2.0f)                   │        │
│   │  Couleur GRIS (0.3, 0.3, 0.3)            │        │
│   │  ┌──────────────────────────────────────┐ │        │
│   │  │ Movement                              │ │        │
│   │  ├──────────────────────────────────────┤ │        │
│   │  │ velocity: [0, 0, 0]                  │ │        │
│   │  └──────────────────────────────────────┘ │        │
│   └────────────────────────────────────────────┘        │
│                                                          │
│        ◊ Node 1 Properties Panel ◊                       │
│        ID: 1001                                         │
│        Type: AudioSource_data                           │
│        Name: MainAudio                                  │
│        Enabled: [✓]                                     │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

**Key Observations:**
- Node 1 (sélectionné) a glow + couleur cyan + bordure épaisse → TRÈS VISIBLE
- Node 2 (non sélectionné) a couleur normale + bordure fine → moins visible
- Propriétés du Node 1 affichées dans le panneau de droite
- Parfait pour UX car sélection = visibilité maximale

---

## 8. Performance Impact

### Draw Call Analysis

```
Per Frame, Per Node:

WITHOUT Selection:
  └─ AddRectFilled()  [Main box]       ← 1 call
  └─ AddRect()        [Border]         ← 1 call
     Total: 2 calls

WITH Selection:
  └─ AddRectFilled()  [Glow]           ← 1 call (NEW)
  └─ AddRectFilled()  [Main box]       ← 1 call
  └─ AddRect()        [Border]         ← 1 call
     Total: 3 calls (+50%)

Overhead per node: 1 draw call (minimal)
For 100 nodes: +100 calls (typically sub-1ms on GPU)
```

### Memory Impact

```
SelectionEffectRenderer member:
  ├─ Vector m_glowColor           (12 bytes)
  ├─ float m_glowAlpha            (4 bytes)
  ├─ float m_baseGlowSize         (4 bytes)
  ├─ float m_borderWidthMultiplier (4 bytes)
  └─ bool m_glowEnabled           (1 byte + padding 3)
     Total per renderer: 28 bytes

Per renderer instance: +28 bytes
For 5 renderers: +140 bytes (negligible)
```

**Conclusion** : Performance impact is negligible ✓

---

## Summary Table

| Aspect | Detail |
|--------|--------|
| **Glow Color** | Cyan `(0.0, 0.8, 1.0)` vivid |
| **Glow Alpha** | 30% transparency |
| **Glow Size** | 4px (scales with zoom) |
| **Border Multiplier** | 2.0× when selected |
| **Corner Radius** | 5.0 pixels |
| **State Detection** | `node.selected` boolean |
| **Rendering Order** | Glow → Main Box → Border |
| **Coordinate System** | Canvas → Zoom → Pan → Screen |
| **Visual Clarity** | ⭐⭐⭐⭐⭐ (5/5) |
| **Reusability** | ⭐⭐⭐⭐⭐ (5/5) |
| **Performance** | ⭐⭐⭐⭐⭐ (5/5) |

