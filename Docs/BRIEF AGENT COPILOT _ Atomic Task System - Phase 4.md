<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" style="height:64px;margin-right:32px"/>

# BRIEF AGENT COPILOT : Atomic Task System - Phase 4 Corrections \& Éditeur UX

**Date :** 24 Février 2026
**Auteur :** Atlasbruce (Lead Olympe Engine)
**Agent :** Copilot C++/ImGui Expert

***

## 1. CONTEXTE \& OBJECTIFS

### 1.1 Contexte Olympe Engine

Olympe est un moteur 2D/3D ECS avec Blueprint Editor (ImGui/SDL3). L'**Atomic Task System** remplace les anciens Behavior Trees par un système de **graphes visuels polyvalents** :

- **IA NPC** : Patrouilles, détection, combat.
- **Level Design** : Séquence, cinématiques, puzzles.
- **Objets interactifs** : Portes, terminaux, règles de jeu.

**Architecture** :

```
Éditeur Blueprint → TaskGraph.json → TaskGraphTemplate (Asset) → TaskRunnerComponent (ECS) → TaskSystem::Update() → Atomic Tasks C++
```


### 1.2 État Actuel (Phase 3)

- **OK** : TaskSystem, TaskRunner, LocalBlackboard, Registry, 5 Atomic Tasks (MoveTo, Wait, SetVar, Compare, Pathfinding).
- **Tests E2E** : Fonctionnels (MoveTo → Wait → SetVar).
- **Éditeur** : Supporte déjà les graphes (mais rendu brut).


### 1.3 Objectifs Phase 4

1. **Corriger les écarts techniques** identifiés dans Phase 3.
2. **Refondre l'UX de l'éditeur** pour atteindre le niveau "Unreal Blueprint".
3. **Valider E2E** : Créer un graphe complexe "Guard v2" (Patrouille + Détection + Combat).

**Critère de Succès** :

- Un designer crée un graphe Guard AI en < 5 min.
- TaskSystem exécute 100 entités @ 60 FPS.
- Debug runtime visuel (nœud actif, Blackboard live).

***

## 2. CORRECTIONS TECHNIQUES (Phase 3 → Phase 4)

### 2.1 Task_SetVariable (bug logique)

**Problème** :

```cpp
// Actuel (incorrect)
auto nameIt = params.find("Name");
auto valueIt = params.find("Value");
try { name = valueIt->first; } catch(...) {}  // ← valueIt->first = "Value", pas la vraie valeur
try { name = nameIt->second.AsString(); }     // ← écrase la ligne précédente
```

**Correction** :

```cpp
TaskStatus Task_SetVariable::ExecuteWithContext(const AtomicTaskContext& ctx, const ParameterMap& params) {
    if (!ctx.LocalBB) return TaskStatus::Failure;
    
    auto nameIt = params.find("Name");
    auto valueIt = params.find("Value");
    if (nameIt == params.end() || valueIt == params.end()) {
        SYSTEM_LOG << "[Task_SetVariable] Missing Name or Value param\n";
        return TaskStatus::Failure;
    }
    
    std::string name = nameIt->second.AsString();  // ← LIGNE CORRIGÉE
    ctx.LocalBB->SetValue(name, valueIt->second);
    
    SYSTEM_LOG << "[Task_SetVariable] Entity " << ctx.Entity << " set '" << name << "'\n";
    return TaskStatus::Success;
}
```


### 2.2 Registry IDs (lisibilité éditeur/JSON)

**Problème** : IDs trop verbeux (`"Task_MoveToLocation"`) → JSON illisible.

**Correction** : Modifier tous les enregistrements :

```cpp
// Actuel
AtomicTaskRegistry::Get().Register("Task_MoveToLocation", ... );

// Nouveau (propre)
AtomicTaskRegistry::Get().Register("MoveToLocation", ... );
AtomicTaskRegistry::Get().Register("Wait", ... );
AtomicTaskRegistry::Get().Register("SetVariable", ... );
AtomicTaskRegistry::Get().Register("Compare", ... );
AtomicTaskRegistry::Get().Register("RequestPathfinding", ... );
```

**Dans l'éditeur** : Toolbox affiche `"MoveToLocation"` (nom court), mais le code C++ garde le nom complet de classe pour debug.

### 2.3 MoveToLocation : intégration réelle ECS

**Problème** : Logique LocalBlackboard uniquement, pas de vraie intégration ECS.

**Correction** : Implémenter la branche `ctx.WorldPtr` :

```cpp
if (ctx.WorldPtr) {
    try {
        // 1. Récupérer PositionComponent
        PositionComponent* pos = ctx.WorldPtr->GetComponent<PositionComponent>(ctx.Entity);
        MovementComponent* move = ctx.WorldPtr->GetComponent<MovementComponent>(ctx.Entity);
        
        if (pos && move) {
            // Logique réelle ECS
            Vector toTarget = target - pos->Position;
            float dist = toTarget.Length();
            
            if (dist <= acceptRadius) {
                move->Velocity = Vector3::Zero;
                return TaskStatus::Success;
            }
            
            Vector dir = toTarget.Normalized();
            move->Velocity = dir * speed;
            return TaskStatus::Running;
        }
    } catch (...) { }
}

// Fallback LocalBlackboard (actuel)
```

**À implémenter** : Ajouter les composants `PositionComponent` et `MovementComponent` dans ton ECS (si pas déjà fait).

***

## 3. AMÉLIORATIONS ÉDITEUR (UX Unreal Blueprint Grade)

### 3.1 Rendu des Nœuds (Header Coloré + Icônes)

**Fichier cible** : `Source/BlueprintEditor/NodeGraphManager.cpp`

```cpp
// NOUVEAU : NodeStyleRegistry.h
struct NodeStyle {
    ImVec4 HeaderColor;
    ImVec4 BodyColor;
    std::string Icon;
};

class NodeStyleRegistry {
public:
    NodeStyle GetStyle(TaskNodeType type);
};

NodeStyle NodeStyleRegistry::GetStyle(TaskNodeType type) {
    switch (type) {
        case TaskNodeType::EventStart: return {ImVec4(0.8f,0.2f,0.2f,1.0f), ImVec4(0.2f,0.0f,0.0f,0.3f), ICON_FA_PLAY};
        case TaskNodeType::Sequence: return {ImVec4(0.2f,0.8f,0.2f,1.0f), ImVec4(0.0f,0.4f,0.0f,0.3f), ICON_FA_LIST_OL};
        case TaskNodeType::Selector: return {ImVec4(0.8f,0.6f,0.2f,1.0f), ImVec4(0.4f,0.3f,0.0f,0.3f), ICON_FA_RANDOM};
        case TaskNodeType::AtomicTask: return {ImVec4(0.2f,0.4f,0.8f,1.0f), ImVec4(0.0f,0.2f,0.4f,0.3f), ICON_FA_BOLT};
        case TaskNodeType::Branch: return {ImVec4(0.6f,0.4f,0.8f,1.0f), ImVec4(0.3f,0.2f,0.4f,0.3f), ICON_FA_CODE_BRANCH};
        default: return {ImVec4(0.5f,0.5f,0.5f,1.0f), ImVec4(0.2f,0.2f,0.2f,0.3f), ICON_FA_QUESTION};
    }
}

// MODIFIER : RenderNode() dans NodeGraphManager.cpp
void NodeGraphManager::RenderNode(const TaskNodeModel& node) {
    NodeStyle style = NodeStyleRegistry::Get().GetStyle(node.BackendType);
    
    // 1. HEADER COLORÉ (avant ImNode::BeginNode)
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 header_tl = ImGui::GetCursorScreenPos();
    ImVec2 header_size = ImVec2(200, 28);  // Largeur fixe + hauteur header
    ImVec2 header_br = header_tl + header_size;
    
    draw_list->AddRectFilled(header_tl, header_br, 
        ImGui::ColorConvertFloat4ToU32(style.HeaderColor), 6.0f);
    draw_list->AddRect(header_tl, header_br, 
        IM_COL32(255,255,255,100), 6.0f);  // Bordure subtile
    
    // Icône + titre
    ImGui::SetCursorScreenPos(header_tl + ImVec2(8, 4));
    RenderIcon(style.Icon.c_str(), ImVec4(1,1,1,1));
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1,1,1,1), "%s", node.DisplayName.c_str());
    
    // 2. CORPS DU NŒUD
    ImNode::BeginNode(node.NodeID);
    
    ImGui::Dummy(ImVec2(0, 4));  // Padding top
    RenderNodePinsAndContent(node);
    ImGui::Dummy(ImVec2(0, 4));  // Padding bottom
    
    ImNode::EndNode();
}
```


### 3.2 Pins Typés et Alignés

```cpp
enum class PinType { Exec, Bool, Int, Float, Vector2, Vector3, EntityID, String };
enum class PinKind { Input, Output };

struct NodePin {
    std::string Name;
    PinType Type;
    PinKind Kind;
    int32 ConnectedToNodeID;
    int32 ConnectedToPinIndex;
};

void RenderNodePinsAndContent(const TaskNodeModel& node) {
    // Layout 3 colonnes : Input | Content | Output
    ImGui::BeginGroup();
    ImGui::Text("Entrées");
    for (int i = 0; i < node.InputPins.size(); ++i) {
        RenderTypedPin(node.InputPins[i], i, PinKind::Input);
    }
    ImGui::EndGroup();
    
    ImGui::SameLine(0, 16);
    
    ImGui::BeginGroup();
    ImGui::Text("Paramètres");
    for (const auto& param : node.Parameters) {
        RenderParameterEditor(param.first.c_str(), param.second);
    }
    ImGui::EndGroup();
    
    ImGui::SameLine(0, 16);
    
    ImGui::BeginGroup();
    ImGui::Text("Sorties");
    for (int i = 0; i < node.OutputPins.size(); ++i) {
        RenderTypedPin(node.OutputPins[i], i, PinKind::Output);
    }
    ImGui::EndGroup();
}

void RenderTypedPin(const NodePin& pin, int pinIndex, PinKind kind) {
    ImVec4 pinColor = GetPinColor(pin.Type);  // Implémenter GetPinColor
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pinCenter = ImGui::GetCursorScreenPos() + ImVec2(kind == PinKind::Input ? 8 : ImGui::GetContentRegionAvail().x - 8, 8);
    
    if (pin.Type == PinType::Exec) {
        // Triangle pour Execution pins
        ImVec2 p1 = pinCenter;
        ImVec2 p2 = pinCenter + (kind == PinKind::Input ? ImVec2(-6, 0) : ImVec2(6, 0));
        ImVec2 p3 = pinCenter + ImVec2(0, 12);
        draw_list->AddTriangleFilled(p1, p2, p3, 
            ImGui::ColorConvertFloat4ToU32(pinColor));
    } else {
        // Cercle pour Data pins
        draw_list->AddCircleFilled(pinCenter, 6.0f, 
            ImGui::ColorConvertFloat4ToU32(pinColor));
    }
    
    ImGui::InvisibleButton(("Pin##" + std::to_string(pinIndex)).c_str(), ImVec2(20, 16));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s (%s)", pin.Name.c_str(), PinTypeToString(pin.Type).c_str());
    }
}

ImVec4 GetPinColor(PinType type) {
    switch (type) {
        case PinType::Exec: return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        case PinType::Bool: return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        case PinType::Int: return ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
        case PinType::Float: return ImVec4(0.3f, 0.3f, 1.0f, 1.0f);
        case PinType::Vector2: return ImVec4(1.0f, 0.7f, 0.3f, 1.0f);
        case PinType::Vector3: return ImVec4(0.7f, 0.3f, 1.0f, 1.0f);
        default: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}
```


### 3.3 Menu Contextuel Intelligent

```cpp
void NodeGraphManager::OnContextMenu(ImVec2 mousePos) {
    if (ImGui::BeginPopupContextWindow("NodeGraphContext", 1)) {
        // Barre de recherche
        static char searchBuffer[128] = "";
        ImGui::InputTextWithHint("##SearchNodes", ICON_FA_SEARCH " Rechercher...", 
            searchBuffer, sizeof(searchBuffer));
        
        // Résultats filtrés (fuzzy search)
        for (const auto& nodeProto : AtomicTaskRegistry::Get().GetAllNodePrototypes()) {
            if (FuzzyMatch(searchBuffer, nodeProto.Name.c_str())) {
                if (ImGui::Selectable(nodeProto.Name.c_str())) {
                    CreateNodeAtPosition(nodeProto.ID, mousePos);
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        ImGui::Separator();
        if (ImGui::MenuItem(ICON_FA_COMMENT " Comment Box")) {
            CreateCommentBox(mousePos);
        }
        if (ImGui::MenuItem(ICON_FA_CLONE " Nouveau Graphe")) {
            NewTaskGraph();
        }
        
        ImGui::EndPopup();
    }
}
```


### 3.4 Raccourcis Clavier (CRUD)

```cpp
void NodeGraphManager::HandleKeyboardInput() {
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
        if (ImGui::IsKeyPressed(ImGuiKey_D)) {  // Ctrl+D
            DuplicateSelectedNodes();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_V)) {  // Ctrl+V
            PasteNodes();
        }
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        DeleteSelectedNodes();
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_C)) {
        CreateCommentBoxAtMouse();
    }
    
    if (ImGui::IsKeyPressed(ImGuiKey_0, false, ImGuiKeyModFlags_Ctrl)) {
        FitViewToAllNodes();
    }
}
```


### 3.5 Debug Runtime Visuel

```cpp
void NodeGraphManager::RenderRuntimeDebugOverlay(const TaskRunnerComponent& runner) {
    const TaskGraphTemplate* tmpl = AssetManager::Get().GetTaskGraph(runner.GraphTemplateID);
    if (!tmpl) return;
    
    const TaskNodeDefinition* activeNode = tmpl->GetNode(runner.CurrentNodeIndex);
    if (!activeNode) return;
    
    // 1. Bordure nœud actif
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 nodeRect = GetNodeScreenRect(activeNode->NodeID);
    draw_list->AddRect(nodeRect.Min, nodeRect.Max, 
        IM_COL32(255, 200, 0, 255), 8.0f, 0, 4.0f);  // Orange épais arrondi
    
    // 2. Animation des liens traversés
    RenderActiveLinks(runner.CurrentNodeIndex);
    
    // 3. Overlay infos
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::Begin("Runtime Debug", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Entité : %d", runner.EntityID);
    ImGui::Text("Nœud courant : %s", activeNode->NodeName.c_str());
    ImGui::Text("Status : %s", TaskStatusToString(runner.LastStatus).c_str());
    ImGui::Text("Temps : %.2fs", runner.StateTimer);
    ImGui::End();
}
```


***

## 4. PLAN D'IMPLÉMENTATION (Copilot)

```
PHASE 1 (2 jours) : Rendu visuel
├── NodeStyleRegistry + RenderNode() avec header coloré
├── RenderTypedPin() (Exec triangle / Data cercle)
└── Test avec tes 5 graphes existants

PHASE 2 (3 jours) : Wiring & Interactions
├── Animation liens runtime (orange + glow)
├── Menu contextuel + recherche fuzzy
├── Raccourcis Ctrl+D/Del/Ctrl+0
└── Drag & Drop Blackboard → graphe

PHASE 3 (2 jours) : Polish
├── Comment Boxes
├── Minimap
├── Snap to grid
└── Performance profiler ImGui
```

**Fichiers à modifier** (basé sur ton repo) :

- `Source/BlueprintEditor/NodeGraphManager.cpp`
- `Source/BlueprintEditor/NodeGraphManager.h`
- `Source/BlueprintEditor/Panels/InspectorPanel.cpp` (pour Blackboard live)

**Dépendances** :

- `IconFontCppHeaders` (pour icônes FontAwesome).
- `AtomicTaskRegistry::GetAllNodePrototypes()` (à implémenter pour l’introspection).

**Critère de validation** :

- Créer un graphe Guard Patrol en < 2 min.
- Debug runtime : voir nœud actif + Blackboard live.
- 60 FPS avec 100 nœuds + liens.


