<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" style="height:64px;margin-right:32px"/>

# CAHIER DES CHARGES : ÉDITEUR NODE GRAPH 2.0

## Atomic Task System - Olympe Engine

**Version :** 2.0
**Date :** 21 Février 2026
**Auteur :** Expert C++/GameDev (Perplexity)
**Contexte :** Refonte UX de l'éditeur Behavior Tree existant pour supporter Atomic Tasks avec un rendu et une ergonomie "Unreal Blueprint Grade".

***

## 1. VISION ET OBJECTIFS

### 1.1 Vision Produit

L'éditeur doit devenir un outil **créatif** pour les Game Designers et Level Designers, pas seulement un **débugger** pour les programmeurs.

**Paragraphe Vision** :
> "L'éditeur Atomic Task doit offrir une expérience fluide et intuitive où le designer peut visualiser immédiatement la logique du graphe, itérer rapidement, et comprendre l'état d'exécution en temps réel. L'ergonomie doit rivaliser avec Unreal Blueprint, Unity Visual Scripting et Godot Visual Script, tout en restant parfaitement aligné avec l'architecture ECS et Atomic Task d'Olympe."

### 1.2 Objectifs Métriques

- **Temps de création** : Un designer expérimenté crée un graphe "Guard Patrol" en < 5 minutes.
- **Lisibilité** : Un nouveau designer comprend un graphe existant en < 30 secondes.
- **Debug Runtime** : État d'exécution visible en temps réel (nœud actif, flux, variables).
- **Performance** : 60 FPS avec 200 nœuds + liens (ImGui SDL3 optimisé).

***

## 2. MODÈLE CONCEPTUEL

### 2.1 Architecture en 4 Couches (UX)

```
┌─────────────────────────────────────────────────────────┐
│ 1. EDITOR UI LAYER (ImGui Frontend)                     │
│   - Asset Browser / Node Graph / Inspector              │
│   - Reactif sur Backend state                           │
└─────────────────────────────────────────────────────────┘
                    ↓ (reactive reads)
┌─────────────────────────────────────────────────────────┐
│ 2. EDITOR BACKEND LAYER (BlueprintEditor)               │
│   - TaskGraphModel (JSON-like in memory)                │
│   - NodeRegistry, VariableManager                       │
└─────────────────────────────────────────────────────────┘
                    ↓ (load/save)
┌─────────────────────────────────────────────────────────┐
│ 3. ASSET LAYER (TaskGraphTemplate)                      │
│   - Read-Only optimized structures                      │
└─────────────────────────────────────────────────────────┘
                    ↓ (runtime)
┌─────────────────────────────────────────────────────────┐
│ 4. RUNTIME LAYER (TaskRunnerComponent)                  │
│   - LocalBlackboard, CurrentNodeIndex                   │
└─────────────────────────────────────────────────────────┘
```

**Justification pour Olympe** :

- **Réutilise ton Backend existant** (`BlueprintEditor Singleton`, `NodeGraphManager`).[^1]
- **Séparation stricte Frontend/Backend** (ton principe actuel : Backend = logique, Frontend = ImGui).
- **Runtime Editor** (F2) peut lire le même Backend que Standalone.


### 2.2 Modèle de Données (Editor Backend)

```cpp
// Extension de ton modèle actuel
struct TaskNodeModel {
    int32 NodeID;
    std::string DisplayName;     // "Move To Waypoint" (user-friendly)
    TaskNodeType BackendType;    // Sequence, AtomicTask, etc.
    
    // Visual properties
    NodeStyle Style;             // Couleur, icône, forme
    ImVec2 Position;
    bool IsSelected;
    bool IsActiveRuntime;        // Pour debug runtime
    
    // Data flow
    std::vector<NodePin> InputPins;
    std::vector<NodePin> OutputPins;
    
    // Exec flow (pour BT/Atomic Tasks)
    std::vector<int32> ChildrenIDs;
    int32 NextSuccessID;
    int32 NextFailureID;
};

struct NodePin {
    std::string Name;            // "Target", "Speed"
    PinType Type;                // Exec, Bool, Float, Vector...
    PinKind Kind;                // Input, Output
    bool IsConnected;
};

struct NodeStyle {
    ImVec4 HeaderColor;          // Par type de nœud
    ImVec4 BodyColor;
    ImVec4 SelectedColor;
    std::string Icon;            // FontAwesome ou IconFontCppHeaders
};
```


***

## 3. FONCTIONNALITÉS UX (Fit Gap vs Unreal Blueprint)

### 3.1 CRUD des Nœuds et Graphes

| Fonctionnalité | État Actuel | Niveau Attendu Unreal | Implémentation |
| :-- | :-- | :-- | :-- |
| **Create Node** | Drag depuis palette ? | Clic droit + SearchBox + Preview | Menu contextuel avec fuzzy search + tooltip preview |
| **Duplicate** | Non visible | Ctrl+D | Menu contextuel + raccourci |
| **Delete** | Non visible | Del | Menu contextuel + raccourci + confirmation |
| **Copy/Paste** | Non | Ctrl+C/V | Sélection multiple + clipboard JSON |
| **Multi-Select** | ? | Ctrl+clic / Lasso | Lasso rectangulaire + Ctrl+clic |
| **Group/Comment** | Non | Boîtes de commentaire | Nœud "CommentBox" draggable/redimensionnable |

### 3.2 Navigation et Zoom

| Fonctionnalité | Implémentation |
| :-- | :-- |
| **Pan / Zoom** | Molette = zoom, Maj+Drag = pan (standard ImGui) |
| **Minimap** | Coin inférieur droit : aperçu du graphe, clic = jump |
| **Search Node** | Ctrl+F : cherche nœud par nom, focus + zoom |
| **Fit View** | Ctrl+0 : centre et zoom sur tous les nœuds |
| **Snap to Grid** | Option + grille magnétique (1.0f step) |

### 3.3 Wiring et Data Flow

| Fonctionnalité | Implémentation |
| :-- | :-- |
| **Pin Alignment** | Pins d’entrée alignés à gauche, sortie à droite |
| **Pin Types** | Exec = triangle, Data = cercle coloré (Rouge=Bool, Bleu=Float...) |
| **Reroute Nodes** | Double-clic sur lien = crée point de passage |
| **Link Colors** | Exec=Gris, Data=couleur du type |
| **Link Thickness** | 2.5f + glow sur lien actif |


***

## 4. IMPLEM TECHNICAL (C++ Instructions pour Copilot)

### 4.1 Refonte Rendu des Nœuds (ImNodes)

**Fichier cible** : `Source/BlueprintEditor/NodeGraphManager.cpp::RenderNode()`

```cpp
// 1. NOUVEAU : Style Registry (singleton)
class NodeStyleRegistry {
public:
    static NodeStyleRegistry& Get();
    NodeStyle GetStyle(TaskNodeType type) const;
};

NodeStyle NodeStyleRegistry::GetStyle(TaskNodeType type) {
    switch (type) {
        case TaskNodeType::EventStart: return { ImVec4(0.8f,0.2f,0.2f,1.0f), /*...*/ };
        case TaskNodeType::Sequence: return { ImVec4(0.2f,0.8f,0.2f,1.0f), /*...*/ };
        case TaskNodeType::AtomicTask: return { ImVec4(0.2f,0.2f,0.8f,1.0f), /*...*/ };
    }
}

// 2. RENDU HEADER COLORÉ
void NodeGraphManager::RenderNode(const TaskNodeModel& node) {
    NodeStyle style = NodeStyleRegistry::Get().GetStyle(node.BackendType);
    
    // Mesurer la taille
    ImVec2 size = ImGui::CalcTextSize(node.DisplayName.c_str()) + ImVec2(40, 20);
    
    // HEADER (avant ImNode::BeginNode)
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 header_tl = ImGui::GetCursorScreenPos();
    ImVec2 header_br = header_tl + ImVec2(size.x, 28.0f);
    
    draw_list->AddRectFilled(header_tl, header_br, 
        ImGui::ColorConvertFloat4ToU32(style.HeaderColor), 6.0f);
    
    // Icône + titre
    ImGui::SetCursorScreenPos(header_tl + ImVec2(8, 4));
    RenderIcon(style.Icon);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1,1,1,1), "%s", node.DisplayName.c_str());
    
    // CORPS DU NŒUD (standard ImNode::BeginNode)
    ImNode::BeginNode(node.NodeID);
    
    // Contenu : Pins, paramètres...
    RenderNodeContent(node);
    
    ImNode::EndNode();
}
```


### 4.2 Pins Typés et Alignés

**Fichier cible** : `NodeGraphManager.cpp::RenderNodeContent()`

```cpp
void RenderNodeContent(const TaskNodeModel& node) {
    // Pins d'entrée (gauche)
    ImGui::BeginGroup();
    for (const auto& pin : node.InputPins) {
        RenderTypedPin(pin, PinKind::Input);
    }
    ImGui::EndGroup();
    
    ImGui::SameLine(0, 20);  // Espacement fixe
    
    // Contenu central (paramètres)
    ImGui::BeginGroup();
    ImGui::TextWrapped("Paramètres...");
    // ...
    ImGui::EndGroup();
    
    ImGui::SameLine(0, 20);
    
    // Pins de sortie (droite)
    ImGui::BeginGroup();
    for (const auto& pin : node.OutputPins) {
        RenderTypedPin(pin, PinKind::Output);
    }
    ImGui::EndGroup();
}
```

**Rendu Pin Typé** :

```cpp
void RenderTypedPin(const NodePin& pin, PinKind kind) {
    ImVec4 color = GetPinColor(pin.Type);  // Rouge=Bool, Bleu=Float...
    
    if (kind == PinKind::Input) {
        // Cercle pour Data, Triangle pour Exec
        if (pin.Type == PinType::Exec) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 center = ImGui::GetCursorScreenPos() + ImVec2(0, 8);
            draw_list->AddTriangleFilled(center, center+ImVec2(6,0), center+ImVec2(0,12), 
                ImGui::ColorConvertFloat4ToU32(color));
        } else {
            // Cercle coloré
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 center = ImGui::GetCursorScreenPos() + ImVec2(8, 8);
            draw_list->AddCircleFilled(center, 6.0f, 
                ImGui::ColorConvertFloat4ToU32(color));
        }
    }
    
    ImGui::Text("%s", pin.Name.c_str());
}
```


### 4.3 Animation des Liens (Runtime Debug)

**Fichier cible** : `NodeGraphManager.cpp::RenderLinks()`

```cpp
void RenderLinks(const std::vector<Link>& links, const TaskRunnerComponent* runner = nullptr) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    for (const auto& link : links) {
        ImVec2 start = GetPinPosition(link.StartPin);
        ImVec2 end = GetPinPosition(link.EndPin);
        
        // Base link color
        ImVec4 linkColor = ImVec4(0.5f, 0.5f, 1.0f, 1.0f);  // Bleu par défaut
        
        // RUNTIME DEBUG : lien actif
        if (runner && IsLinkActive(runner->CurrentNodeIndex, link)) {
            linkColor = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);  // Orange brillant
        }
        
        // Épaisseur + glow
        draw_list->AddBezierCubic(start, start+ImVec2(50,0), 
            end-ImVec2(50,0), end, 
            ImGui::ColorConvertFloat4ToU32(linkColor), 3.0f);
        
        // Glow effect (optionnel, pour liens actifs)
        if (runner && IsLinkActive(runner->CurrentNodeIndex, link)) {
            draw_list->AddBezierCubic(start, start+ImVec2(50,0), 
                end-ImVec2(50,0), end, 
                IM_COL32(255,255,0,100), 6.0f);
        }
    }
}
```


### 4.4 Menu Contextuel (CRUD)

**Fichier cible** : `NodeGraphManager.cpp::OnRightClick()`

```cpp
void OnRightClick(ImVec2 pos) {
    if (ImGui::BeginPopup("NodeGraphContextMenu")) {
        // Recherche de nœuds
        static char searchBuffer[^256] = "";
        ImGui::InputTextWithHint("##Search", "Rechercher un nœud...", searchBuffer, 256);
        
        // Résultats filtrés
        for (const auto& nodeType : NodeStyleRegistry::Get().GetAllNodeTypes()) {
            if (FuzzyMatch(searchBuffer, nodeType.DisplayName)) {
                if (ImGui::MenuItem(nodeType.DisplayName.c_str())) {
                    CreateNodeAt(nodeType, pos);
                }
            }
        }
        
        ImGui::Separator();
        if (ImGui::MenuItem("Comment Box")) CreateCommentBoxAt(pos);
        
        ImGui::EndPopup();
    }
}
```


***

## 5. PLAN D'IMPLÉMENTATION (Pour Copilot)

### Phase 1 (1 semaine) : Rendu Visuel

```
1. Implémenter NodeStyleRegistry
2. Refondre RenderNode() avec header coloré + icône
3. Implémenter RenderTypedPin() (Exec vs Data)
4. Tester avec tes 5 graphes existants (guard_patrol.json etc.)
```


### Phase 2 (1 semaine) : Wiring \& Interactions

```
1. Implémenter animation des liens runtime
2. Menu contextuel avec recherche fuzzy
3. Drag & Drop depuis Blackboard Panel
4. Raccourcis Ctrl+D, Del, Ctrl+0 (fit view)
```


### Phase 3 (1 semaine) : Polish \& Debug

```
1. Comment Boxes
2. Minimap
3. Snap to grid
4. Performance profiling (ImGui Profiler)
```

**Choix Techniques adaptés à Olympe** :

- **ImGui/ImNodes** : Parfait pour SDL3, déjà intégré, performant.
- **Style Registry** : Centralisé, extensible (nouveaux types de nœuds).
- **Reactive Backend** : Ton architecture actuelle est idéale (Backend pousse pas, Frontend pull).
- **Runtime Debug** : S'intègre parfaitement avec ton mode F2 (Runtime Editor).[^2]

Ce cahier des charges transforme ton éditeur en outil **production-ready**. Priorise Phase 1 : le rendu visuel change tout !

<div align="center">⁂</div>

[^1]: ARCHITECTURE.md

[^2]: STANDALONE_BLUEPRINT_EDITOR.md

