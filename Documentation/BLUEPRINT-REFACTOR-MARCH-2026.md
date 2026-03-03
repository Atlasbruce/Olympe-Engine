# Journal de Refactoring — Debugger NodeGraph Runtime
## Mission : Unification du Debug F10 avec le pipeline Blueprint Editor
**Date de début :** 2026-03-03
**Auteur :** Atlasbruce / GitHub Copilot
**Repo :** Atlasbruce/Olympe-Engine

---

## Table des matières
1. [Objectifs de la mission](#1-objectifs-de-la-mission)
2. [Phase 1 — Analyse du Blueprint Editor Standalone](#2-phase-1--analyse-du-blueprint-editor-standalone)
3. [Phase 2 — Analyse du Debugger Runtime F10](#3-phase-2--analyse-du-debugger-runtime-f10)
4. [Phase 3 — Inventaire des dépendances et spécificités](#4-phase-3--inventaire-des-dépendances-et-spécificités)
5. [Phase 4 — Plan de re-implémentation](#5-phase-4--plan-de-re-implémentation)
6. [Phase 5 — Nettoyage et archivage legacy](#6-phase-5--nettoyage-et-archivage-legacy)
7. [Décisions d'architecture](#7-décisions-darchitecture)
8. [Risques et points de vigilance](#8-risques-et-points-de-vigilance)
9. [Checklist de validation](#9-checklist-de-validation)

---

## 1. Objectifs de la mission

| # | Objectif |
|---|----------|
| O1 | Le Debugger runtime (F10) doit utiliser **exclusivement** les APIs, classes et services du Blueprint Editor pipeline |
| O2 | Lister toutes les entités runtime avec une IA (BT) dans un panneau gauche |
| O3 | Sélection d'une entité → affichage de son node graph via les routines du Blueprint Editor |
| O4 | Rendu visuel identique au Blueprint Editor standalone (couleurs NodeStyleRegistry, pins, connexions) |
| O5 | Highlighting runtime du nœud actif (animation pulse amber/jaune) |
| O6 | Aucune régression sur le Blueprint Editor standalone |
| O7 | Nettoyage et archivage du code legacy dans `Source/_deprecated/` après validation |

---

## 2. Phase 1 — Analyse du Blueprint Editor Standalone

> **Statut : COMPLETEE — 2026-03-03**

### 2.1 Projets Visual Studio

Deux exécutables distincts dans la solution :

| Projet | Point d'entrée |
|--------|----------------|
| Olympe Engine | `Source/GameEngine.cpp` |
| Olympe Blueprint Editor | `Source/BlueprintEditorStandalone/BlueprintEditorStandaloneMain.cpp` |

### 2.2 Pipeline complet du Blueprint Editor

#### 2.2.1 Couche données — NodeGraphCore

| Classe | Fichier | Rôle |
|--------|---------|------|
| `GraphDocument` | `Source/NodeGraphCore/GraphDocument.h/cpp` | Document principal d'un graphe (nodes, links, metadata, validation, serialization) |
| `NodeGraphManager` | `Source/NodeGraphCore/NodeGraphManager.cpp` | Singleton gérant N graphes ouverts, tabs, load/save/close |
| `CommandSystem` | `Source/NodeGraphCore/CommandSystem.cpp` | Undo/Redo pattern via `ICommand` + `CommandStack` |
| `GraphMigrator` | `Source/NodeGraphCore/GraphMigrator.cpp` | Migration automatique JSON v0/v1 → v2 |

**Schema JSON v2 (format actuel) :**

```json
{
  "schema_version": 2,
  "name": "MyBT",
  "blueprintType": "BehaviorTree",
  "data": {
    "rootNodeId": 1,
    "nodes": [{"id":1,"type":"BT_Sequence","name":"Root","position":{"x":100,"y":100},"children":[2,3]}],
    "links": []
  },
  "editorState": {"zoom":1.0,"scrollOffset":{"x":0,"y":0}}
}
```

#### 2.2.2 Parsing et chargement (BTNodeGraphManager)

Fichier : `Source/BlueprintEditor/BTNodeGraphManager.cpp`

Pipeline en 6 étapes :
1. Ouverture et lecture fichier
2. Vérification non-vide
3. Parse JSON via `nlohmann::json`
4. Détection version (v1 vs v2 via `schema_version`)
5. `NodeGraph::FromJson()` → `NodeGraph`
6. Migration v1→v2 si nécessaire (`GraphMigrator::LoadWithMigration`)

**Détection de version :**
```cpp
bool isV2 = j.contains("schema_version") && j["schema_version"].get<int>() == 2;
bool isV1 = !isV2 && (j.contains("nodes") || j.contains("rootNodeId"));
```

#### 2.2.3 Rendu des nodes — NodeGraphPanel

Fichier : `Source/BlueprintEditor/NodeGraphPanel.cpp`

**Pipeline de rendu :**
```cpp
void NodeGraphPanel::RenderGraph()
{
    ImNodes::BeginNodeEditor();
    for (GraphNode* node : graph->GetAllNodes())
    {
        // 1. UID global : graphID * GRAPH_ID_MULTIPLIER + node->id
        // 2. SetNodeGridSpacePos (une seule fois par node)
        // 3. Lookup style via NodeStyleRegistry
        const NodeStyle& style = NodeStyleRegistry::Get().GetStyle(node->type);
        // 4. Debug highlight (noeud actif = amber pulse)
        if (s_ActiveDebugNodeId == node->id) { headerColor = IM_COL32(200, 180, 20, 255); }
        // 5. Push colors + BeginNode
        ImNodes::BeginNode(globalNodeUID);
        RenderNodePinsAndContent(node, globalNodeUID, graphID);
        ImNodes::EndNode();
        ImNodes::PopColorStyle(); // x3
    }
    // 6. Liens baseline
    // 7. MiniMap si activée
    if (m_ShowMinimap) ImNodes::MiniMap(0.15f, ImNodesMiniMapLocation_BottomRight);
    ImNodes::EndNodeEditor();
    // 8. Bezier glow APRES EndNodeEditor
    RenderActiveLinks(graph, graphID);
}
```

**API de debug overlay :**
```cpp
static int NodeGraphPanel::s_ActiveDebugNodeId = -1;
static void NodeGraphPanel::SetActiveDebugNode(int localNodeId);
```

#### 2.2.4 Styles visuels — NodeStyleRegistry

Fichier : `Source/BlueprintEditor/NodeStyleRegistry.h/cpp`

| NodeType | Couleur header | Icône |
|----------|---------------|-------|
| BT_Sequence | Bleu (41,74,122) | → |
| BT_Selector | Vert (41,122,74) | ? |
| BT_Action | Orange (150,80,20) | A |
| BT_Condition | Jaune (140,120,20) | C |
| BT_Decorator | Violet (100,40,140) | D |
| HFSM_State | Teal (30,110,120) | S |
| HFSM_Transition | Rouge (140,40,40) | T |
| Comment | Gris foncé (60,60,60) | // |

Méthode clé : `NodeStyleRegistry::Get().GetStyle(NodeType type)`

#### 2.2.5 Shared helpers — NodeGraphShared

Fichiers : `Source/NodeGraphShared/`

```cpp
namespace NodeGraphShared {
    ImVec2 ComputePinCenterScreen(int nodeId, const BTNodeLayout* layout,
                                   bool isOutput, float pinOffset,
                                   float headerPx, float currentZoom);
    void DrawPinCircle(ImDrawList* drawList, const ImVec2& center,
                       float radius, ImU32 fillColor,
                       ImU32 outlineColor, float outlineThickness);
}
```
`Renderer.h` contient `RenderBTNode()` — wrapper complet (BeginNode → header → pins → EndNode).

#### 2.2.6 Conversion BT <-> GraphDocument

Fichier : `Source/NodeGraphShared/BTGraphDocumentConverter.h/cpp`

```cpp
namespace Olympe::NodeGraphShared {
    class BTGraphDocumentConverter {
        static GraphDocument* FromBehaviorTree(const BehaviorTreeAsset* tree);
        static BehaviorTreeAsset* ToBehaviorTree(const GraphDocument* doc);
        static void SyncRuntimeToDocument(const BehaviorTreeAsset* tree, GraphDocument* doc);
    };
}
```

#### 2.2.7 EditorContext — gestion des modes

Fichier : `Source/NodeGraphCore/EditorContext.cpp`

```cpp
EditorContext::CreateEditor()     // Full CRUD + save
EditorContext::CreateVisualizer() // Read-only
EditorContext::CreateDebugger()   // Read-only + showRuntimeState = true
```

#### 2.2.8 Résumé du pipeline Blueprint Editor standalone

```
[Fichier JSON]
     | GraphMigrator::LoadWithMigration()
[GraphDocument]       <- NodeGraphManager (singleton, N docs ouverts)
     |                <- CommandSystem (undo/redo)
[NodeGraphPanel]      -> ImNodes::BeginNodeEditor
     |                -> NodeStyleRegistry (couleurs)
     |                -> NodeGraphShared::DrawPinCircle / ComputePinCenterScreen
     |                -> ImNodes::Link (liens)
     |                -> ImNodes::MiniMap
     |                -> RenderActiveLinks (glow Bezier post-EndNodeEditor)
[ImGui/SDL Render]
```

---

## 3. Phase 2 — Analyse du Debugger Runtime F10

> **Statut : COMPLETEE — 2026-03-03**

### 3.1 Localisation et taille

| Fichier | LOC | Type |
|---------|-----|------|
| `Source/AI/BehaviorTreeDebugWindow.h` | ~400 | Header complet |
| `Source/AI/BehaviorTreeDebugWindow.cpp` | ~3606 | Implémentation |

### 3.2 Architecture actuelle (F10 trigger)

```
GameEngine -> input F10 -> BehaviorTreeDebugWindow::ToggleVisibility()
                              |
                    CreateSeparateWindow()  <- Fenêtre SDL3 native indépendante
                    ImGui::CreateContext()  <- ImGui context séparé
                    ImNodes::CreateContext()
                              |
                    Render() [chaque frame]
                              |
                    RenderInSeparateWindow()
                       |-- RenderEntityListPanel()   [col gauche]
                       |-- RenderNodeGraphPanel()    [col centre]
                       +-- RenderInspectorPanel()    [col droite]
```

### 3.3 Fenêtre SDL3 séparée — spécificité majeure

La debug window crée sa propre fenêtre SDL3 avec son propre ImGui context :
```cpp
SDL_CreateWindowAndRenderer(..., &m_separateWindow, &m_separateRenderer);
m_separateImGuiContext = ImGui::CreateContext();
ImGui_ImplSDL3_InitForSDLRenderer(m_separateWindow, m_separateRenderer);
ImGui_ImplSDLRenderer3_Init(m_separateRenderer);
```
Peut être déplacée sur un 2e moniteur.

### 3.4 Panel Entités (RenderEntityListPanel)

Source de données : `World::Get().GetAllEntities()` filtré par `HasComponent<BehaviorTreeRuntime_data>`.

- Status icon (actif/inactif) + lastStatus icon
- Filtres : texte, Active Only, Has Target
- Tri : Name / TreeName / LastUpdate / AIMode
- À la sélection → `m_layoutEngine.ComputeLayout(tree, ...)` recalcule le layout

### 3.5 Panel NodeGraph (RenderNodeGraphPanel) — état actuel HYBRIDE

Le panneau utilise **partiellement** les APIs du Blueprint Editor :

| API | Status |
|-----|--------|
| `NodeStyleRegistry::Get().GetStyle(BTNodeTypeToEditorNodeType(type))` | ✅ Utilisé |
| `NodeGraphShared::DrawPinCircle()` | ✅ Utilisé |
| `NodeGraphShared::ComputePinCenterScreen()` | ✅ Utilisé |
| `BTGraphDocumentConverter::FromBehaviorTree()` | ❌ Non utilisé |
| `NodeGraphPanel::Render()` | ❌ Non utilisé |
| `GraphDocument` comme source de vérité | ❌ Non utilisé (`BehaviorTreeAsset*` à la place) |
| `CommandSystem` NodeGraphCore | ❌ Non utilisé (`BTCommandStack` local à la place) |

### 3.6 Panel Inspecteur

Sections :
- **Node Properties** (double-click sur nœud)
- **Validation** (mode éditeur, `m_editingTree.ValidateTreeFull()`)
- **Runtime Info** : treeId, nodeName, lastStatus, AIMode, timeInCurrentMode
- **Blackboard** : target, movement, patrol, combat, stimuli, wander
- **Execution Log** : buffer circulaire 100 entrées (deque)

### 3.7 Highlighting runtime

```cpp
if (isCurrentNode) {
    float t = 0.5f + 0.5f * sinf(m_pulseTimer * 2*PI * 2.0f);
    headerColor = IM_COL32(180 + t*75, 140 + t*115, 10, 255);
}
// + glow Bezier post-EndNodeEditor
RenderActiveLinkGlow(start, end, tangent);
```

### 3.8 Dépendances ECS du debugger

```cpp
World::Get().GetComponent<BehaviorTreeRuntime_data>(entity)  // AITreeAssetId, AICurrentNodeIndex, isActive, lastStatus
World::Get().GetComponent<Identity_data>(entity)             // name
World::Get().GetComponent<AIState_data>(entity)              // currentMode, timeInCurrentMode
World::Get().GetComponent<AIBlackboard_data>(entity)         // hasTarget, targetEntity, distanceToTarget, etc.
```

---

## 4. Phase 3 — Inventaire des dépendances et spécificités

> **Statut : COMPLETEE — 2026-03-03**

### 4.1 APIs Blueprint Editor disponibles et réutilisables

| API | Fichier | Statut |
|-----|---------|--------|
| `NodeStyleRegistry::GetStyle(NodeType)` | `BlueprintEditor/NodeStyleRegistry.h` | Déjà utilisé |
| `NodeGraphShared::RenderBTNode()` | `NodeGraphShared/Renderer.h` | À utiliser |
| `NodeGraphShared::DrawPinCircle()` | `NodeGraphShared/NodeGraphShared.h` | Déjà utilisé |
| `NodeGraphShared::ComputePinCenterScreen()` | `NodeGraphShared/NodeGraphShared.h` | Déjà utilisé |
| `BTGraphDocumentConverter::FromBehaviorTree()` | `NodeGraphShared/BTGraphDocumentConverter.h` | À adopter |
| `EditorContext::CreateDebugger()` | `NodeGraphCore/EditorContext.cpp` | À utiliser |
| `GraphDocument` (source de vérité) | `NodeGraphCore/GraphDocument.h` | À adopter |
| `NodeGraphPanel::SetActiveDebugNode()` + `Render()` | `BlueprintEditor/NodeGraphPanel.h` | À utiliser |

### 4.2 Spécificités du debugger à préserver

| Spécificité | Raison | Décision |
|-------------|--------|----------|
| Fenêtre SDL3 séparée | 2e moniteur | CONSERVER |
| ImGui/ImNodes context séparé | Isolation | CONSERVER avec push/pop |
| Source de données ECS runtime | Accès live | CONSERVER |
| Filtres/tri entités | UX debugger | CONSERVER |
| Execution Log (deque) | Historique runtime | CONSERVER |
| Blackboard panel | Inspection ECS live | CONSERVER |
| `m_pulseTimer` animation | Animation runtime | CONSERVER |
| `BTNodeTypeToEditorNodeType()` mapping | Conversion enum | CONSERVER |

### 4.3 Code legacy à supprimer après migration

| Méthode | LOC est. | Remplacement |
|---------|---------|--------------|
| `RenderBehaviorTreeGraph()` | ~30 | `NodeGraphPanel::Render()` |
| `RenderNode()` | ~150 | `NodeGraphShared::Renderer.h::RenderBTNode()` |
| `RenderNodeConnections()` | ~60 | `ImNodes::Link` dans NodeGraphPanel |
| `GetNodeColor()`, `GetNodeIcon()` | ~40 | `NodeStyleRegistry` |
| `m_editingTree` (BehaviorTreeAsset local) | — | `GraphDocument` |
| `BTCommandStack m_commandStack` | — | `CommandSystem` NodeGraphCore |
| Custom `RenderMinimap()` | ~30 | `ImNodes::MiniMap()` |
| `ApplyZoomToStyle()` | ~10 | ImNodes natif |
| `FitGraphToView()`, `CenterViewOnGraph()`, `ResetZoom()` | ~60 | `NodeGraphPanel` |

**Total estimé supprimé : ~380 LOC** (sur ~3606 — le reste étant ECS/Inspector qui reste).

---

## 5. Phase 4 — Plan de re-implémentation

> **Statut : PLANIFIEE — 2026-03-03 | VALIDATION REQUISE**

### 5.1 Architecture cible

```
[F10 Input]
     |
BehaviorTreeDebugWindow::ToggleVisibility()
     | (Fenêtre SDL3 séparée inchangée)
RenderInSeparateWindow()
     |
     |-- [LEFT]   RenderEntityListPanel()            <-- INCHANGÉ (scan ECS)
     |
     |-- [CENTER] RenderNodeGraphPanel() (nouvelle)  <-- REFACTORÉ
     |                |
     |           EnsureGraphDocument(tree)
     |               BTGraphDocumentConverter::FromBehaviorTree(tree) -> GraphDocument* (cache par treeId)
     |                |
     |           NodeGraphManager::Get().SetActiveGraph(m_cachedGraphMgrId)
     |                |
     |           NodeGraphPanel::SetActiveDebugNode(btRuntime.AICurrentNodeIndex)
     |                |
     |           m_nodeGraphPanel.Render()  <-- PIPELINE COMPLET BP EDITOR
     |
     +-- [RIGHT]  RenderInspectorPanel()             <-- INCHANGÉ (données ECS)
```

### 5.2 Étapes d'implémentation détaillées

#### Étape 4.1 — Modifications de BehaviorTreeDebugWindow.h

```cpp
// Nouveaux includes
#include "../NodeGraphShared/BTGraphDocumentConverter.h"
#include "../NodeGraphCore/GraphDocument.h"
#include "../BlueprintEditor/NodeGraphPanel.h"
#include "../NodeGraphCore/EditorContext.h"

// Nouveaux membres (à ajouter dans la classe)
NodeGraphPanel   m_nodeGraphPanel;
EditorContext    m_editorContext;          // = EditorContext::CreateDebugger()
GraphDocument*   m_cachedGraphDoc = nullptr;
uint32_t         m_cachedTreeId = 0;      // Cache invalidation
int              m_cachedGraphMgrId = -1; // ID dans NodeGraphManager

// Membres à supprimer après validation
// BehaviorTreeAsset m_editingTree;      -> remplacé par GraphDocument
// BTCommandStack    m_commandStack;     -> remplacé par CommandSystem
// BTGraphLayoutEngine m_layoutEngine;   -> remplacé par AutoLayout GraphDocument (ou conservé pour init)
```

#### Étape 4.2 — EnsureGraphDocument() (nouveau)

```cpp
void BehaviorTreeDebugWindow::EnsureGraphDocument(const BehaviorTreeAsset* tree)
{
    if (tree == nullptr) return;
    if (m_cachedTreeId == tree->id && m_cachedGraphDoc != nullptr)
        return; // Cache valide, rien à faire

    // Invalider cache et reconvertir
    if (m_cachedGraphMgrId >= 0)
        NodeGraphManager::Get().CloseGraph(m_cachedGraphMgrId);

    delete m_cachedGraphDoc;
    m_cachedGraphDoc = BTGraphDocumentConverter::FromBehaviorTree(tree);
    m_cachedTreeId = tree->id;

    // Enregistrer dans NodeGraphManager pour que NodeGraphPanel puisse le trouver
    m_cachedGraphMgrId = NodeGraphManager::Get().LoadFromDocument(m_cachedGraphDoc, tree->name);
    NodeGraphManager::Get().SetActiveGraph(m_cachedGraphMgrId);

    SYSTEM_LOG << "[BTDebugWindow] GraphDocument refreshed for tree: " << tree->name << std::endl;
}
```

#### Étape 4.3 — RenderNodeGraphPanel() réécrit

```cpp
void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
{
    if (m_selectedEntity == 0)
    {
        ImGui::Text("Select an entity from the list to view its behavior tree");
        return;
    }

    auto& world = World::Get();
    if (!world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
    {
        ImGui::Text("Selected entity no longer has a behavior tree");
        m_selectedEntity = 0;
        return;
    }

    const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
    const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);

    if (!tree)
    {
        ImGui::TextColored(ImVec4(1,0.5f,0,1), "Behavior Tree asset not found (ID: %u)", btRuntime.AITreeAssetId);
        if (ImGui::Button("Show All Loaded Trees"))
            BehaviorTreeManager::Get().DebugPrintLoadedTrees();
        return;
    }

    // 1. Convertir/mettre à jour le GraphDocument (cache par treeId)
    EnsureGraphDocument(tree);

    // 2. Notifier le NodeGraphPanel du nœud actif courant
    uint32_t activeNodeId = btRuntime.isActive ? btRuntime.AICurrentNodeIndex : (uint32_t)-1;
    NodeGraphPanel::SetActiveDebugNode(static_cast<int>(activeNodeId));

    // 3. Rendre via le pipeline complet du Blueprint Editor
    m_nodeGraphPanel.Render();
}
```

#### Étape 4.4 — Gestion contexte ImGui/ImNodes (CRITIQUE)

```cpp
void BehaviorTreeDebugWindow::Render()
{
    if (!m_isVisible || !m_windowCreated) return;

    ImGuiContext* prevImGuiCtx = ImGui::GetCurrentContext();
    // ImNodesContext* prevImNodesCtx = ImNodes::GetCurrentContext(); // si API disponible

    ImGui::SetCurrentContext(m_separateImGuiContext);
    // ImNodes::SetCurrentContext(m_separateImNodesCtx); // si API disponible

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    RenderInSeparateWindow();

    ImGui::Render();
    SDL_SetRenderDrawColor(m_separateRenderer, 18, 18, 20, 255);
    SDL_RenderClear(m_separateRenderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_separateRenderer);
    SDL_RenderPresent(m_separateRenderer);

    ImGui::SetCurrentContext(prevImGuiCtx);
    // ImNodes::SetCurrentContext(prevImNodesCtx); // si API disponible
}
```

---

## 6. Phase 5 — Nettoyage et archivage legacy

> **Statut : EN ATTENTE DE VALIDATION (étape 4)**

### 6.1 Fichiers à archiver dans `Source/_deprecated/`

| Source | Archive |
|--------|---------|
| Méthodes legacy de `BehaviorTreeDebugWindow.cpp` | `Source/_deprecated/BTDebugWindow_LegacyRenderer_2026-03.cpp` |

### 6.2 Méthodes à supprimer de BehaviorTreeDebugWindow

```
RenderBehaviorTreeGraph()    (~30 LOC)
RenderNode()                 (~150 LOC)
RenderNodeConnections()      (~60 LOC)
GetNodeColor()               (~20 LOC)
GetNodeIcon()                (~20 LOC)
RenderMinimap() custom       (~30 LOC)
ApplyZoomToStyle()           (~10 LOC)
GetGraphBounds()             (~20 LOC)
FitGraphToView()             (~30 LOC)
CenterViewOnGraph()          (~20 LOC)
ResetZoom()                  (~10 LOC)
```

**Total estimé supprimé : ~400 LOC**

---

## 7. Décisions d'architecture

| # | Décision | Justification |
|---|----------|---------------|
| D1 | Conserver la fenêtre SDL3 séparée | Fonctionnalité 2e moniteur, aucun équivalent BP Editor |
| D2 | `BTGraphDocumentConverter::FromBehaviorTree()` | Conversion standard, testée, réutilise le même code que l'éditeur |
| D3 | Source de vérité → `GraphDocument` | Unification avec pipeline BP Editor |
| D4 | `NodeStyleRegistry` pour tous les styles | Cohérence visuelle entre les deux éditeurs |
| D5 | `EditorContext::CreateDebugger()` | Read-only + `showRuntimeState = true` |
| D6 | `NodeGraphPanel::SetActiveDebugNode()` | Réutilise logique existante amber pulse, zéro duplication |
| D7 | Cache `GraphDocument` invalidé par `treeId` | Évite reconversion chaque frame, conserve les positions |
| D8 | Conserver Execution Log, Blackboard, Runtime Info | Données exclusivement ECS runtime — hors scope Blueprint Editor |
| D9 | Nettoyage conditionnel à validation étape 4 | Sécurité : ne pas supprimer avant validation fonctionnelle |

---

## 8. Risques et points de vigilance

| # | Risque | Sévérité | Mitigation |
|---|--------|----------|------------|
| R1 | Conflit ImGui context entre main window et debug window | HAUTE | Push/pop context systématique ; vérifier si `ImNodes::SetCurrentContext()` existe dans la version utilisée |
| R2 | `NodeGraphPanel` assume `NodeGraphManager` comme source de vérité | MOYENNE | Appeler `NodeGraphManager::LoadFromDocument()` avant `Render()` |
| R3 | `BTGraphDocumentConverter` ne préserve pas les positions `BTGraphLayoutEngine` | MOYENNE | Injecter le layout calculé dans le document converti |
| R4 | `EditorContext::Get()` singleton global — conflit mode éditeur standalone | HAUTE | Vérifier si EditorContext est singleton ou instance locale ; isoler |
| R5 | `NodeGraphPanel::s_ActiveDebugNodeId` static partagé | MOYENNE | OK si une seule instance NodeGraphPanel debug active |
| R6 | `BTGraphDocumentConverter` peut être un stub non implémenté | MOYENNE | Vérifier l'implémentation réelle avant utilisation |
| R7 | `NodeGraphManager::LoadFromDocument()` peut ne pas exister | HAUTE | Vérifier l'API réelle de NodeGraphManager |

---

## 9. Checklist de validation

### Phases 1-3 — Analyse (COMPLETÉES)
- [x] Architecture Blueprint Editor documentée
- [x] Pipeline de rendu NodeGraphPanel analysé
- [x] Shared helpers NodeGraphShared documentés
- [x] NodeStyleRegistry documenté
- [x] BTGraphDocumentConverter identifié
- [x] EditorContext modes documentés
- [x] Architecture BehaviorTreeDebugWindow documentée
- [x] Pipeline de rendu custom identifié et analysé
- [x] Fenêtre SDL3 séparée documentée
- [x] Dépendances ECS listées
- [x] Code legacy à supprimer identifié

### Phase 4 — Implémentation (EN ATTENTE)
- [ ] `BTGraphDocumentConverter::FromBehaviorTree()` vérifié (implémentation réelle)
- [ ] `NodeGraphManager::LoadFromDocument()` vérifié ou alternative identifiée
- [ ] `EditorContext` mode isolation vérifiée
- [ ] `EnsureGraphDocument()` implémenté
- [ ] `RenderNodeGraphPanel()` réécrit
- [ ] Contexte ImGui/ImNodes géré correctement
- [ ] Compilation sans erreurs ni warnings
- [ ] **⚠️ VALIDATION ATLASBRUCE REQUISE**

### Phase 5 — Nettoyage (EN ATTENTE)
- [ ] Backup des méthodes legacy dans `Source/_deprecated/`
- [ ] Suppression des méthodes obsolètes
- [ ] Compilation finale sans erreurs ni warnings
- [ ] F10 ouvre la fenêtre debugger
- [ ] Sélection entité affiche le graphe BT
- [ ] Nœud actif highlighted (pulse amber)
- [ ] Minimap visible et fonctionnelle
- [ ] Zoom/Pan fonctionne
- [ ] Blackboard affiche les données live
- [ ] **⚠️ VALIDATION ATLASBRUCE REQUISE**

---

*Dernière mise à jour : 2026-03-03 — Phases 1-3 complétées, Phase 4 planifiée, en attente de validation.*