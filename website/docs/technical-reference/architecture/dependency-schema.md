---
id: dependency-schema
title: "Schéma de Dépendances des Modules"
sidebar_label: "Schéma de Dépendances"
---

# Schéma de Dépendances des Modules

Ce document recense les dépendances entre les modules C++ du projet. Les flèches indiquent la direction de l'inclusion (`A → B` signifie que A dépend de B / inclut B).

:::note
Le projet cible le standard **C++14** partout. Il n'utilise pas `std::optional`, `std::variant`, les liaisons structurées ni `std::filesystem`.
:::

---

## Dépendances entre modules Source/

```mermaid
graph TD
    subgraph Core["Noyau du moteur (Source/)"]
        EE["ECS_Entity.h<br/>(EntityID, ComponentTypeID, IComponentPool)"]
        EC["ECS_Components.h<br/>(structures de composants)"]
        ES["ECS_Systems.h<br/>(ECS_System, systèmes de base)"]
        ER["ECS_Register.h<br/>(ComponentPool<T>)"]
        W["World<br/>(gestionnaire ECS)"]
        GE["GameEngine<br/>(singleton principal)"]
        DM["DataManager<br/>(ressources, cache)"]
        VG["VideoGame"]
        CD["ComponentDefinition.h<br/>(ComponentParameter)"]
        PS["ParameterSchema.h<br/>(ParameterSchemaEntry, ComponentSchema)"]
        PR["ParameterResolver"]

        EC --> EE
        ES --> EE
        ES --> EC
        ER --> EE
        ER --> EC
        ER --> ES
        W --> ER
        W --> EC
        W --> ES
        W --> PS
        W --> PR
        GE --> W
        GE --> DM
        GE --> VG
        PS --> CD
    end
```

---

## Dépendances du TaskSystem (Couche 2)

```mermaid
graph TD
    subgraph TaskSys["Source/TaskSystem/"]
        TGTypes["TaskGraphTypes.h<br/>(TaskValue, VariableType, NodeType…)"]
        TGT["TaskGraphTemplate.h<br/>(TaskNodeDefinition, VariableDefinition…)"]
        TGL["TaskGraphLoader.h<br/>(ParseSchemaV4)"]
        TGM["TaskGraphMigrator_v3_to_v4"]
        LBB["LocalBlackboard.h"]
        EBB["EntityBlackboard.h"]
        TRC["TaskRunnerComponent.h"]
        TS["TaskSystem.h<br/>(ECS System)"]
        VSE["VSGraphExecutor.h"]
        TEB["TaskExecutionBridge.h"]

        TGT --> TGTypes
        TGL --> TGT
        TGM --> TGT
        LBB --> TGT
        TS --> TRC
        TS --> TGT
        TS --> VSE
        VSE --> TGT
        VSE --> LBB
        VSE --> EBB
        TEB --> TS
    end

    subgraph CondSys["Source/Editor/ConditionPreset/"]
        CP["ConditionPreset.h"]
        OP["Operand.h"]
        NCR["NodeConditionRef.h"]
        DDP["DynamicDataPin.h"]
        CPR["ConditionPresetRegistry.h"]
        DDPM["DynamicDataPinManager.h"]
        CP --> OP
        NCR --> DDP
        CPR --> CP
        DDPM --> DDP
    end

    TGT --> CP
    TGT --> NCR
    TGT --> DDP
    VSE --> CP
```

---

## Dépendances du Runtime (Source/Runtime/)

```mermaid
graph LR
    subgraph Runtime["Source/Runtime/"]
        RE["RuntimeEnvironment.h<br/>(Blackboard + DynamicPins)"]
        CPEv["ConditionPresetEvaluator.h<br/>(stateless)"]
        GRI["GraphRuntimeInstance.h<br/>(exécution step-by-step)"]
    end

    subgraph Deps["Dépendances"]
        CP2["ConditionPreset.h"]
        NCR2["NodeConditionRef.h"]
        TGT2["TaskGraphTemplate.h"]
        OP2["Operand.h"]
    end

    CPEv --> RE
    CPEv --> CP2
    CPEv --> OP2
    GRI --> TGT2
    GRI --> RE
    GRI --> NCR2
```

---

## Dépendances du BlueprintEditor (Couche 3)

```mermaid
graph TD
    subgraph BE["Source/BlueprintEditor/"]
        BGUI["BlueprintEditorGUI"]
        TM["TabManager"]
        IGR["IGraphRenderer.h<br/>(interface pure)"]
        VSP["VisualScriptEditorPanel"]
        BTRend["BehaviorTreeRenderer"]
        EPRend["EntityPrefabRenderer"]
        GES["GraphExecutionSimulator"]
        GET["GraphExecutionTracer"]
        SER["SelectionEffectRenderer"]
        URStack["UndoRedoStack"]
        VSConnV["VSConnectionValidator"]
        VSGVer["VSGraphVerifier"]
        BP["BlueprintEditorPlugin.h<br/>(interface plugin)"]
        BTPlugin["BehaviorTreeEditorPlugin"]
        EPPlugin["EntityPrefabEditorPlugin"]
        HFSM["HFSMEditorPlugin"]
        ANPLUGIN["AnimationGraphEditorPlugin"]

        BGUI --> TM
        TM --> IGR
        IGR --> VSP
        IGR --> BTRend
        IGR --> EPRend
        BGUI --> BP
        BP --> BTPlugin
        BP --> EPPlugin
        BP --> HFSM
        BP --> ANPLUGIN
        VSP --> SER
        VSP --> URStack
        VSP --> VSConnV
        VSP --> VSGVer
        GES --> GET
    end

    subgraph Utils["Source/BlueprintEditor/Utilities/"]
        ICE["ICanvasEditor.h"]
        INCE["ImNodesCanvasEditor"]
        CCE["CustomCanvasEditor"]
        CGR2["CanvasGridRenderer"]
        CMR2["CanvasMinimapRenderer"]
        ICE --> INCE
        ICE --> CCE
        INCE --> CGR2
        INCE --> CMR2
        CCE --> CGR2
    end

    VSP --> INCE
    BTRend --> CCE
    EPRend --> CCE
```

---

## Dépendances du sous-système Entity Prefab Editor

```mermaid
graph TD
    subgraph EPE["Source/BlueprintEditor/EntityPrefabEditor/"]
        EPR3["EntityPrefabRenderer<br/>(IGraphRenderer)"]
        PC["PrefabCanvas"]
        EPGD["EntityPrefabGraphDocument"]
        CPP3["ComponentPalettePanel"]
        CNR3["ComponentNodeRenderer"]
        CND["ComponentNodeData.h"]
        PL["PrefabLoader.h"]
        PSR3["ParameterSchemaRegistry (Prefab)"]
        NPP["NodePropertiesPanel"]
        PEP["PropertyEditorPanel"]
        PIPr["PropertyInspectorPrefab"]

        EPR3 --> PC
        EPR3 --> CPP3
        PC --> EPGD
        PC --> CNR3
        EPGD --> CND
        EPGD --> PL
        CPP3 --> PSR3
        NPP --> EPGD
        PEP --> EPGD
        PIPr --> EPGD
    end

    PSR3 -.->|"JSON"| CJSON2["Gamedata/EntityPrefab/<br/>ComponentsParameters.json"]
    EPR3 --> IGR2["IGraphRenderer"]
```

---

## Dépendances du NodeGraphCore (Couche 4)

```mermaid
graph TD
    subgraph NGC["Source/NodeGraphCore/"]
        NGC_H["NodeGraphCore.h<br/>(GraphId, NodeId, NodeData…)"]
        GD2["GraphDocument<br/>(CRUD, JSON v2)"]
        NGM2["NodeGraphManager<br/>(multi-graphes)"]
        BS2["BlackboardSystem"]
        GM["GraphMigrator<br/>(v1→v2)"]
        LC["LinkCache"]
        NA["NodeAnnotations"]
        NG["NodeGroup"]
        NV["NodeValidator"]
        GC["GraphComment"]
        GB["GlobalBlackboard"]
        GTB["GlobalTemplateBlackboard"]

        GD2 --> NGC_H
        GD2 --> BS2
        GD2 --> NA
        NGM2 --> GD2
        GM --> GD2
        LC --> NGC_H
        NV --> GD2
        GTB --> GB
    end

    subgraph NGS["Source/NodeGraphShared/"]
        BA["BlueprintAdapter.h"]
        BTGA2["BehaviorTreeGraphAdapter"]
        BTGDC2["BTGraphDocumentConverter"]
        BTDA2["BTDebugAdapter"]
        RH["RenderHelpers"]
        NGSh["NodeGraphShared.h"]

        BTGA2 --> BTDA2
    end

    GD2 --> NGS
    BTGDC2 --> GD2
    BTGA2 --> TGT3["TaskGraphTemplate"]
```

---

## Dépendances de l'éditeur AI / Panels / Modals (Source/Editor/)

```mermaid
graph TD
    subgraph EdMod["Source/Editor/Modals/"]
        NCEMod["NodeConditionsEditModal"]
        BTFPMod["BehaviorTreeFilePickerModal"]
        SGFPMod["SubGraphFilePickerModal"]
        SCEMod["SwitchCaseEditorModal"]
    end

    subgraph EdPan["Source/Editor/Panels/"]
        NCP["NodeConditionsPanel<br/>(READ-ONLY)"]
        CPLP["ConditionPresetLibraryPanel"]
        APP["ActionParametersPanel"]
        MOP["MathOpPropertyPanel"]
        GBV["GetBBValuePropertyPanel"]
        SBV["SetBBValuePropertyPanel"]
        VPP["VariablePropertyPanel"]
    end

    subgraph EdCond["Source/Editor/ConditionPreset/"]
        CP3["ConditionPreset"]
        CPR3["ConditionPresetRegistry"]
        OP3["Operand"]
        NCR3["NodeConditionRef"]
        DDP3["DynamicDataPin"]
        DDPM3["DynamicDataPinManager"]
    end

    NCEMod --> NCP
    NCP --> CP3
    CPLP --> CPR3
    MOP --> CP3
    VSP2["VisualScriptEditorPanel"] --> NCEMod
    VSP2 --> NCP
    VSP2 --> CPLP
    VSP2 --> APP
    VSP2 --> MOP
    VSP2 --> GBV
    VSP2 --> SBV
    VSP2 --> VPP
    VSP2 --> SCEMod
    VSP2 --> SGFPMod
```

---

## Dépendances des librairies tierces

```mermaid
graph LR
    subgraph ThirdParty["Source/third_party/"]
        IMGUI["imgui/"]
        IMNODES["imnodes/"]
        NLOHMANN["nlohmann/json.hpp"]
        NFD["nfd/ (Native File Dialog)"]
    end

    subgraph ExternalLibs["Bibliothèques externes"]
        SDL3["SDL3 (fenêtre, rendu, input)"]
        MINIZ["miniz (compression, Tiled)"]
        TINYXML["tinyxml2 (Tiled TMX)"]
    end

    VSP3["VisualScriptEditorPanel"] --> IMGUI
    VSP3 --> IMNODES
    EPGD3["EntityPrefabGraphDocument"] --> NLOHMANN
    TGT4["TaskGraphTemplate"] --> NLOHMANN
    BGU3["BlueprintEditorGUI"] --> NFD
    GE3["GameEngine"] --> SDL3
    TLL["TiledLevelLoader"] --> MINIZ
    TLL --> TINYXML
```

---

## Récapitulatif des dépendances descendantes par couche

| Module | Dépend de | Ne doit PAS dépendre de |
|---|---|---|
| `ECS_Entity.h` | *(rien)* | Tout le reste |
| `ECS_Components.h` | `ECS_Entity.h`, `DataManager`, SDL3 | Editor, Blueprint |
| `TaskGraphTypes.h` | *(rien)* | ECS, Editor, SDL3 |
| `TaskGraphTemplate.h` | `TaskGraphTypes`, `ConditionPreset`, `NodeConditionRef` | Editor UI, ImGui |
| `VSGraphExecutor` | `TaskGraphTemplate`, `LocalBlackboard`, `ConditionPresetEvaluator` | Editor UI, ImGui |
| `IGraphRenderer.h` | *(rien — interface pure)* | Runtime, ECS |
| `VisualScriptEditorPanel` | `TaskGraphTemplate`, `imnodes`, `imgui`, `ConditionPreset`, `ICanvasEditor` | `VSGraphExecutor` direct |
| `NodeGraphCore` | `json_helper.h`, `vector.h` | TaskSystem, ECS, Editor |
| `EntityPrefabEditor` | `imgui`, `nlohmann/json`, `ParameterSchemaRegistry` (prefab) | Runtime TaskSystem |

---

## Règle de dépendance stricte

```mermaid
graph TD
    L4["Couche 4 — NodeGraphCore<br/>(générique, zéro dépendance moteur)"]
    L3["Couche 3 — Blueprint Editor<br/>(dépend de TaskGraphTemplate pour le chargement)"]
    L2["Couche 2 — Task / VS Runtime<br/>(dépend du noyau ECS et ConditionPreset)"]
    L1["Couche 1 — Engine Runtime<br/>(dépend de toutes les couches inférieures)"]

    L4 --> L3
    L3 --> L2
    L2 --> L1

    style L1 fill:#d4edda,color:#000
    style L2 fill:#cce5ff,color:#000
    style L3 fill:#fff3cd,color:#000
    style L4 fill:#f8d7da,color:#000
```

:::caution Règle fondamentale
Les couches inférieures **ne doivent jamais** inclure de headers des couches supérieures. Par exemple, `TaskGraphTemplate.h` n'inclut aucun header ImGui ou éditeur. `VSGraphExecutor` n'appelle pas de fonctions de `VisualScriptEditorPanel`. Cette séparation garantit que la compilation runtime peut se faire sans les dépendances éditeur.
:::
