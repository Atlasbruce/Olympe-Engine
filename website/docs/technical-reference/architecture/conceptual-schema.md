---
id: conceptual-schema
title: "Schéma Conceptuel de l'Architecture"
sidebar_label: "Schéma Conceptuel"
---

# Schéma Conceptuel de l'Architecture

Ce document présente l'organisation conceptuelle d'Olympe Engine en quatre couches indépendantes qui communiquent à travers des interfaces bien définies.

---

## Vue d'ensemble — Les 4 couches

```mermaid
graph TB
    subgraph L1["Couche 1 — Moteur Runtime (SDL3)"]
        GE["GameEngine<br/>(Singleton, boucle principale)"]
        W["World<br/>(Registre ECS, chargement niveau)"]
        VG["VideoGame<br/>(État jeu, joueurs)"]
        DM["DataManager<br/>(Ressources, cache)"]
        GE --> W
        GE --> VG
        GE --> DM
    end

    subgraph L2["Couche 2 — Task / Visual Script Runtime"]
        TS["TaskSystem<br/>(Système ECS)"]
        TGT["TaskGraphTemplate<br/>(Asset immuable partagé)"]
        VSE["VSGraphExecutor<br/>(Exécution frame-by-frame)"]
        LBB["LocalBlackboard<br/>(Variables par entité)"]
        CPE["ConditionPresetEvaluator<br/>(Évaluation conditions)"]
        TS --> VSE
        VSE --> TGT
        VSE --> LBB
        VSE --> CPE
    end

    subgraph L3["Couche 3 — Blueprint Editor"]
        BGUI["BlueprintEditorGUI<br/>(Interface principale)"]
        TM["TabManager<br/>(Onglets multi-graphes)"]
        IGR["IGraphRenderer<br/>(Interface polymorphe)"]
        VSP["VisualScriptEditorPanel"]
        BTR["BehaviorTreeRenderer"]
        EPR["EntityPrefabRenderer"]
        BGUI --> TM
        TM --> IGR
        IGR --> VSP
        IGR --> BTR
        IGR --> EPR
    end

    subgraph L4["Couche 4 — NodeGraphCore (Générique)"]
        GD["GraphDocument<br/>(Modèle de données)"]
        NGM["NodeGraphManager<br/>(Multi-graphes)"]
        BS["BlackboardSystem<br/>(Variables de graphe)"]
        NGM --> GD
        GD --> BS
    end

    W --> TS
    VSP --> TGT
    BTR --> GD
    EPR --> GD
```

---

## Flux principal : Boucle de jeu

```mermaid
sequenceDiagram
    participant SDL as SDL3 Event Loop
    participant GE as GameEngine
    participant W as World
    participant TS as TaskSystem
    participant VSE as VSGraphExecutor
    participant LBB as LocalBlackboard

    SDL->>GE: Process(deltaTime)
    GE->>W: Update(deltaTime)
    W->>TS: Process()
    loop Pour chaque entité avec TaskRunnerComponent
        TS->>VSE: ExecuteFrame(entity, template, blackboard)
        VSE->>LBB: GetValue / SetValue
        VSE-->>TS: BTStatus (Running / Success / Failure)
    end
    W-->>GE: OK
    GE-->>SDL: Frame terminée
```

---

## Flux éditeur : Ouverture d'un graphe

```mermaid
sequenceDiagram
    participant User as Utilisateur
    participant GUI as BlueprintEditorGUI
    participant TM as TabManager
    participant IGR as IGraphRenderer
    participant TGL as TaskGraphLoader
    participant TGT as TaskGraphTemplate

    User->>GUI: Ouvre un fichier .json
    GUI->>TM: OpenFile(path)
    TM->>TM: Détecte graphType (VisualScript / BehaviorTree / EntityPrefab)
    TM->>IGR: new VisualScriptEditorPanel() ou BehaviorTreeRenderer()
    IGR->>TGL: Load(path)
    TGL->>TGT: ParseSchemaV4(json)
    TGT-->>IGR: TaskGraphTemplate chargé
    IGR-->>TM: renderer prêt
    TM-->>GUI: Onglet affiché
```

---

## Sous-système : Condition Preset (Phase 24)

```mermaid
graph LR
    subgraph Definition["Définition (Éditeur)"]
        CP["ConditionPreset<br/>{id, name, left, op, right}"]
        OP["Operand<br/>{mode, stringValue, constValue}"]
        NCR["NodeConditionRef<br/>{presetID, leftPinID, rightPinID, logicalOp}"]
        DDP["DynamicDataPin<br/>{uuid, label, type}"]
        CP --> OP
        NCR --> CP
        NCR --> DDP
    end

    subgraph Storage["Stockage (Phase 24 — Self-contained)"]
        TGT2["TaskGraphTemplate<br/>ConditionPreset[]<br/>(embarqués dans le JSON v4)"]
        CPR["ConditionPresetRegistry<br/>(mémoire + fichier JSON)"]
    end

    subgraph Runtime["Évaluation (Runtime)"]
        VSE2["VSGraphExecutor<br/>HandleBranch()"]
        CPEv["ConditionPresetEvaluator<br/>(stateless)"]
        RE["RuntimeEnvironment<br/>(Blackboard + DynamicPins)"]
        VSE2 --> CPEv
        CPEv --> RE
    end

    CP --> TGT2
    CP --> CPR
    NCR --> VSE2
```

---

## Sous-système : Entity Prefab Editor (Phases 27–29b)

```mermaid
graph TB
    subgraph Editor["Éditeur"]
        EPR2["EntityPrefabRenderer<br/>(IGraphRenderer)"]
        PC["PrefabCanvas<br/>(ImGui + input)"]
        CPP["ComponentPalettePanel<br/>(recherche, drag-drop)"]
        CNR["ComponentNodeRenderer<br/>(dessin des nœuds)"]
        EPR2 --> PC
        EPR2 --> CPP
        PC --> CNR
    end

    subgraph Data["Modèle de données"]
        EPGD["EntityPrefabGraphDocument<br/>(nœuds + connexions)"]
        CN["ComponentNode<br/>(type, position, propriétés)"]
        EPGD --> CN
    end

    subgraph Schema["Schéma de paramètres"]
        PSR["ParameterSchemaRegistry (Prefab)<br/>(composant → paramètres)"]
        CJSON["ComponentsParameters.json<br/>(Gamedata/PrefabEntities/)"]
        PSR --> CJSON
    end

    PC --> EPGD
    CPP --> PSR
    CNR --> EPGD
```

---

## Sous-système : BehaviorTree (Runtime + Éditeur)

```mermaid
graph TB
    subgraph Runtime["Runtime AI"]
        BT["BehaviorTree<br/>(BTNode[], hiérarchique)"]
        BTTypes["BTNodeType<br/>Selector, Sequence, Condition<br/>Action, Inverter, Repeater<br/>Root, OnEvent, SubGraph"]
        BT --> BTTypes
    end

    subgraph Editor2["Éditeur BT (dans BlueprintEditor)"]
        BTRend["BehaviorTreeRenderer<br/>(IGraphRenderer)"]
        BTNGM["BTNodeGraphManager<br/>(états éditeur)"]
        BTRend --> BTNGM
    end

    subgraph Shared["Couche Partagée (NodeGraphShared)"]
        BTGA["BehaviorTreeGraphAdapter<br/>(BT → TaskGraphTemplate)"]
        BTGDC["BTGraphDocumentConverter<br/>(BT → GraphDocument)"]
        BTDA["BTDebugAdapter<br/>(debug générique)"]
    end

    subgraph Simulation["Simulation / Validation"]
        GES["GraphExecutionSimulator<br/>(dry-run, validation)"]
        GET["GraphExecutionTracer<br/>(trace d'exécution)"]
        GES --> GET
    end

    BT --> BTGA
    BT --> BTGDC
    BTGA --> GES
    BTRend --> BTGA
```

---

## Canvas standardisé (Phase 37)

```mermaid
graph TB
    subgraph Interface["Interface abstraite"]
        ICE["ICanvasEditor<br/>(BeginRender/EndRender, pan/zoom, grille)"]
    end

    subgraph Impl["Implémentations"]
        INCE["ImNodesCanvasEditor<br/>(wraps imnodes — VisualScript)"]
        CCE["CustomCanvasEditor<br/>(pan/zoom manuel — EntityPrefab, BT)"]
    end

    subgraph Utils["Utilitaires"]
        CGR["CanvasGridRenderer<br/>(grille, zoom/pan)"]
        CMR["CanvasMinimapRenderer<br/>(minimap)"]
    end

    ICE --> INCE
    ICE --> CCE
    INCE --> CGR
    INCE --> CMR
    CCE --> CGR
```
