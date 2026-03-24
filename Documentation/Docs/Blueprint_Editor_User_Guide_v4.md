# Olympe Blueprint Editor - Guide Utilisateur Complet v4

**Version** : 4.0 (Mars 2026)  
**Format principal** : ATS Visual Script (Schema v4)  
**Auteur** : Olympe Engine Team

---

## 📚 Table des matières

1. [Introduction](#introduction)
2. [Architecture système](#architecture-système)
3. [Créer un graphe](#créer-un-graphe)
4. [Éditer un graphe](#éditer-un-graphe)
5. [Opérations CRUD](#opérations-crud)
6. [Blackboard (Variables)](#blackboard-variables)
7. [Débogage](#débogage)
8. [Raccourcis clavier](#raccourcis-clavier)
9. [Formats de fichiers](#formats-de-fichiers)
10. [Migration BT v2 → VS v4](#migration)

---

## Introduction

### Qu'est-ce que le Blueprint Editor ?

L'**Olympe Blueprint Editor** est un éditeur visuel multi-format permettant de créer :
- ✅ **Graphes IA** : Comportements d'agents, pathfinding, décisions tactiques
- ✅ **Level Scripting** : Triggers, events, séquences de niveau
- ✅ **Dialogues** : Arbres de conversation, choix de réponse
- ✅ **Systèmes de jeu** : Crafting, inventory, quêtes

### Formats supportés

| Format | Schema Version | Éditeur | Status | Usage |
|--------|---------------|---------|--------|-------|
| **ATS Visual Script v4** | 4 | VisualScriptEditorPanel | ✅ **ACTUEL** | Tous nouveaux graphes |
| **Behavior Tree v2** | 2 | NodeGraphPanel | ⚠️ **DEPRECATED** | Debug uniquement |
| **Entity Prefab** | 2 | EntityPrefabEditor | ✅ Active | Entités réutilisables |
| **HFSM** | 2 | HFSMEditor | ✅ Active | Machines à états |

**⚠️ IMPORTANT** : **NodeGraphPanel** (BT v2) est **déprécié pour l'édition** depuis **Phase 7**.  
Utiliser **VisualScriptEditorPanel** pour tous nouveaux graphes.

### Modes d'utilisation

**Mode Standalone** :
- Lancé via `OlympeBlueprintEditor.exe`
- CRUD complet : Créer, Éditer, Supprimer, Sauvegarder
- Permissions complètes via `EditorContext::InitializeStandalone()`

**Mode Runtime** :
- Intégré au jeu pour visualisation en temps réel
- Read-only : Pas de modifications
- Permissions limitées via `EditorContext::InitializeRuntime()`

---

## Architecture système

### Backend (Business Logic)

**`BlueprintEditor`** (Singleton)
- **Rôle** : Gestion centralisée de l'état, assets, et données
- **Fichier** : `Source/BlueprintEditor/blueprinteditor.h/cpp`
- **Méthodes clés** :
  ```cpp
  BlueprintEditor::Get().Initialize();
  BlueprintEditor::Get().InitializeStandaloneEditor();
  BlueprintEditor::Get().OpenGraphInEditor("path/to/graph.ats");
  BlueprintEditor::Get().RefreshAssets();
  BlueprintEditor::Get().PreloadATSGraphs();
  ```
- **Responsabilités** :
  - Asset management : Scan de `Blueprints/` et `Gamedata/`
  - Entity tracking : Liste des entités runtime via `NotifyEntityCreated()`
  - Selection : Synchronisation entre panneaux (asset sélectionné, entité sélectionnée)
  - Command stack : Undo/Redo system
  - Plugin registry : Gestion des plugins éditeur

### Frontend (UI)

**`BlueprintEditorGUI`**
- **Rôle** : Interface ImGui, layout des panneaux, menu bar
- **Fichier** : `Source/BlueprintEditor/BlueprintEditorGUI.h/cpp`
- **Panneaux gérés** :
  - `m_AssetBrowser` : Navigation fichiers + runtime entities
  - `m_NodeGraphPanel` : Legacy BT v2 (deprecated)
  - `m_VSEditorPanel` : **Éditeur principal v4**
  - `m_InspectorPanel` : Propriétés et inspection
  - `m_DebugPanel` : Débogueur runtime
  - `m_ProfilerPanel` : Profiling des graphes
  - `m_HistoryPanel` : Historique Undo/Redo
  - `m_TemplateBrowserPanel` : Templates réutilisables

### Core Systems

#### NodeGraphCore (Abstraction générique)

**`GraphDocument`** : Modèle de données abstrait
- **Format** : Schema v2 (compatible BT, HFSM, custom)
- **CRUD** :
  ```cpp
  NodeId CreateNode(const std::string& nodeType, Vector2 pos);
  bool DeleteNode(NodeId id);
  LinkId ConnectPins(PinId fromPin, PinId toPin);
  bool DisconnectLink(LinkId id);
  bool UpdateNodePosition(NodeId id, Vector2 newPos);
  ```
- **Validation** :
  ```cpp
  bool ValidateGraph(std::string& errorMessage) const;
  bool HasCycles() const;
  ```
- **Serialization** :
  ```cpp
  json ToJson() const;
  static GraphDocument FromJson(const json& j);
  ```

**`NodeGraphManager`** : Gestionnaire multi-graphes
- **Tabs** : Gère plusieurs graphes ouverts simultanément
- **Lifecycle** :
  ```cpp
  GraphId CreateGraph(const std::string& graphType, const std::string& graphKind);
  GraphId LoadGraph(const std::string& filepath);
  bool SaveGraph(GraphId id, const std::string& filepath);
  bool CloseGraph(GraphId id);
  ```
- **Active graph** :
  ```cpp
  void SetActiveGraph(GraphId id);
  GraphDocument* GetActiveGraph();
  ```

#### TaskGraphLoader (Chargeur universel)

**Responsabilité** : Charger et migrer tous formats de graphes
- **Schema v4** : Format principal (flat ATS Visual Script)
  ```cpp
  TaskGraphTemplate* ParseSchemaV4(const json& data, std::vector<std::string>& outErrors);
  ```
- **Schema v3** : Migration automatique vers v4
  ```cpp
  json v4data = TaskGraphMigrator_v3_to_v4::MigrateJson(data, errors);
  ```
- **Schema v2** : Migration BT legacy via `BTtoVSMigrator`
  ```cpp
  TaskGraphTemplate migrated = BTtoVSMigrator::Convert(data, errors);
  ```

**API** :
```cpp
TaskGraphTemplate* TaskGraphLoader::LoadFromFile(
    const std::string& path,
    std::vector<std::string>& outErrors
);
```

#### Command System (Undo/Redo)

**`CommandStack`** : Pile d'historique
- **Commandes disponibles** :
  - `CreateNodeCommand` : Création de nœud
  - `DeleteNodeCommand` : Suppression de nœud
  - `ConnectPinsCommand` : Connexion de pins
  - `DeleteLinkCommand` : Suppression de lien
  - `MoveNodeCommand` : Déplacement de nœud
  - `ToggleNodeBreakpointCommand` : Toggle breakpoint

**Usage** :
```cpp
auto& stack = *BlueprintEditor::Get().GetCommandStack();
auto cmd = std::make_unique<CreateNodeCommand>(graph, "BT_Action", Vector2{100, 200});
stack.ExecuteCommand(std::move(cmd));

// Undo/Redo
BlueprintEditor::Get().Undo();
BlueprintEditor::Get().Redo();
```

### Plugin System

**`BlueprintEditorPlugin`** : Interface extensible
- **Méthodes virtuelles** :
  ```cpp
  virtual json CreateNew(const std::string& name) = 0;
  virtual bool CanHandle(const json& blueprint) const = 0;
  virtual std::vector<ValidationError> Validate(const json& blueprint) = 0;
  ```

**Plugins enregistrés** :
- `BehaviorTreeEditorPlugin` : BT v2 legacy
- `EntityPrefabEditorPlugin` : Entity Prefabs
- `HFSMEditorPlugin` : Machines à états
- `DialogueEditorPlugin`, `ScriptedEventPlugin`, `AnimationGraphPlugin`

**Enregistrement** :
```cpp
void BlueprintEditor::InitializePlugins()
{
    RegisterPlugin(std::make_unique<BehaviorTreeEditorPlugin>());
    RegisterPlugin(std::make_unique<EntityPrefabEditorPlugin>());
    // ...
}
```

---

## Créer un graphe

### ✅ Méthode 1 : Nouveau graphe ATS VS v4 (RECOMMANDÉ)

**Via VS Graph Editor** :
1. Menu **View → VS Graph Editor** (ouvre `VisualScriptEditorPanel`)
2. Dans le panneau : Bouton **"New Graph"**
3. Graphe vide créé :
   ```json
   {
     "schema_version": 4,
     "name": "Untitled VS Graph",
     "graphType": "VisualScript",
     "blackboard": [],
     "nodes": [],
     "execConnections": [],
     "dataConnections": []
   }
   ```
4. **Ajouter EntryPoint** (obligatoire) :
   - Clic droit → **Flow Control → EntryPoint**
5. **Ajouter nœuds** :
   - Clic droit → Choisir catégorie (Flow, Actions, Data, SubGraph)
6. **Connecter nœuds** :
   - Glisser depuis pin exec out (▶) vers pin exec in (◀)
   - Glisser depuis pin data out (○) vers pin data in (○)
7. **Sauvegarder** :
   - Bouton **"Save"** ou `Ctrl+S`
   - Choisir chemin (ex: `Blueprints/AI/guard_ai.ats`)

**Via Menu File** :
1. Menu **File → New Blueprint → AI → Behavior Tree**
   (⚠️ Crée un graphe v4, pas un BT v2 legacy !)
2. Entrer le nom du graphe
3. Le graphe s'ouvre automatiquement dans VS Graph Editor
4. Suivre étapes 4-7 ci-dessus

### ✅ Méthode 2 : Ouvrir un graphe existant

**Via Asset Browser (PRINCIPAL)** :
1. Panneau **Asset Browser** → onglet **"Blueprint Files"**
2. Naviguer dans l'arborescence :
   - `Blueprints/` : Graphes éditables
   - `Gamedata/` : Assets en lecture seule
3. **Double-clic** sur un fichier `.ats` ou `.json`
4. Détection automatique du type via `DetectAssetType()` :
   ```cpp
   std::string type = DetectAssetType(filepath);
   // Priorité : champ "type" > "blueprintType" > structure JSON
   ```
5. Ouverture dans l'éditeur approprié :
   - `"VisualScript"` → `VisualScriptEditorPanel` ✅
   - `"BehaviorTree"` → `NodeGraphPanel` (debug, pas d'édition) ⚠️
   - `"EntityPrefab"` → `InspectorPanel`
   - `"HFSM"` → `HFSMEditor`

**Via Menu** :
1. Menu **File → Open Blueprint...** (`Ctrl+O`)
2. Dialogue de fichiers (natif)
3. Sélectionner `.ats` ou `.json`
4. Détection automatique + ouverture

**Via Code** :
```cpp
BlueprintEditor::Get().OpenGraphInEditor("Blueprints/AI/guard_patrol.ats");
```

### ✅ Méthode 3 : Depuis un template

1. Menu **File → Template Browser** ou `View → Template Browser`
2. Liste des templates disponibles (catégories : AI, Level, Dialogue)
3. Sélectionner un template
4. **"Apply Template"** → remplace le graphe actuel
5. Éditer selon besoins
6. **Save As...** pour créer un nouveau fichier

---

## Éditer un graphe

### Layout de l'éditeur VS Graph Editor

```
┌───────────────────────────────────────────────────────────┐
│ VS Graph Editor                                     [X]   │
├───────────────────────────────────────────────────────────┤
│ guard_ai.ats *           [Save]  [New Graph]             │
├─────────────────────────────────────┬─────────────────────┤
│                                     │ Properties          │
│        Canvas (ImNodes)             │ ┌─────────────────┐ │
│                                     │ │ Selected Node   │ │
│   [EntryPoint] ──▶ [Branch]        │ │ Name: CheckHP   │ │
│                      │ │            │ │ Type: Branch    │ │
│                    True False       │ │ ConditionID:    │ │
│                      ↓   ↓          │ │ lowHealth       │ │
│                  [Heal] [Attack]    │ │                 │ │
│                                     │ │ [x] Breakpoint  │ │
│  Clic droit = Palette de nœuds     │ └─────────────────┘ │
│                                     │                     │
│                                     │ Local Blackboard    │
│                                     │ ┌─────────────────┐ │
│                                     │ │ [+] Add key     │ │
│                                     │ │ speed    Float  │ │
│                                     │ │ target   Entity │ │
│                                     │ │ lowHealth Bool  │ │
│                                     │ └─────────────────┘ │
└─────────────────────────────────────┴─────────────────────┘
```

### Panneaux principaux

1. **Asset Browser** :
   - Onglet **"Blueprint Files"** : Arborescence `Blueprints/` et `Gamedata/`
   - Onglet **"Runtime Entities"** : Entités actives dans le monde
   - Onglet **"Nodes"** : Palette de nœuds (drag & drop)

2. **VS Graph Editor** :
   - **Canvas** (gauche) : Rendu ImNodes du graphe
   - **Properties** (droite haut) : Édition du nœud sélectionné
   - **Blackboard** (droite bas) : Variables locales/globales

3. **Inspector** :
   - Inspection contextuelle (entité ou asset)
   - Composants, métadonnées, validation

4. **Debugger** :
   - Breakpoints, step-by-step (F9, F5, F10)
   - État du Blackboard en temps réel
   - Call stack d'exécution

---

## Créer un graphe

### Workflow complet (ATS Visual Script v4)

#### Étape 1 : Nouveau graphe

**Option A : Via VS Graph Editor**
1. Menu **View → VS Graph Editor**
2. Bouton **"New Graph"**
3. Template vide créé en mémoire

**Option B : Via Menu File**
1. **File → New Blueprint → AI → Behavior Tree**
   (⚠️ Crée un graphe v4, pas BT v2)
2. Entrer nom : `guard_ai`
3. Graphe ouvert dans VS Graph Editor

#### Étape 2 : Ajouter EntryPoint (OBLIGATOIRE)

1. **Clic droit** sur le canvas
2. **Flow Control → EntryPoint**
3. Nœud créé avec pin exec out **"Out"**

**Règle** : **1 seul EntryPoint par graphe** (point de départ de l'exécution)

#### Étape 3 : Ajouter une logique conditionnelle

1. **Clic droit** → **Flow Control → Branch**
2. Nœud **Branch** créé avec :
   - Pin exec in : **"In"**
   - Pins exec out : **"True"**, **"False"**
3. **Connecter** EntryPoint.Out → Branch.In :
   - Cliquer sur ▶ de EntryPoint
   - Glisser vers ◀ de Branch
   - Relâcher

#### Étape 4 : Configurer la condition

1. **Sélectionner** le nœud Branch (clic simple)
2. Dans le panneau **Properties** :
   - **Name** : `"Check Low Health"`
   - **ConditionID** : `"lowHealth"` (clé Blackboard)
3. Créer la variable Blackboard :
   - Panneau **Local Blackboard** → **[+] Add key**
   - Nom : `lowHealth`
   - Type : **Bool**
   - Valeur par défaut : `false`

#### Étape 5 : Ajouter des actions

**Action si True (Health basse)** :
1. Clic droit → **Actions → AtomicTask**
2. Nœud créé nommé `AtomicTask`
3. Sélectionner le nœud
4. Dans Properties :
   - **Name** : `"Heal Self"`
   - **TaskType** : `"HealAction"` (doit exister dans `AtomicTaskRegistry`)
5. Connecter Branch.True → HealSelf.In

**Action si False (Health OK)** :
1. Répéter avec TaskType : `"AttackTarget"`
2. Connecter Branch.False → AttackTarget.In

#### Étape 6 : Sauvegarder

1. Bouton **"Save"** ou `Ctrl+S`
2. Dialogue de sauvegarde :
   - Chemin : `Blueprints/AI/guard_ai.ats`
   - Format : JSON v4
3. **Confirmation** : Astérisque `*` disparaît du titre

**Résultat JSON** :
```json
{
  "schema_version": 4,
  "name": "guard_ai",
  "graphType": "VisualScript",
  "blackboard": [
    {
      "key": "lowHealth",
      "type": "Bool",
      "value": false,
      "isGlobal": false
    }
  ],
  "nodes": [
    {
      "id": 1,
      "label": "Start",
      "type": "EntryPoint",
      "position": {"x": 100, "y": 100}
    },
    {
      "id": 2,
      "label": "Check Low Health",
      "type": "Branch",
      "conditionKey": "lowHealth",
      "position": {"x": 300, "y": 100}
    },
    {
      "id": 3,
      "label": "Heal Self",
      "type": "AtomicTask",
      "taskType": "HealAction",
      "position": {"x": 500, "y": 50}
    },
    {
      "id": 4,
      "label": "Attack Target",
      "type": "AtomicTask",
      "taskType": "AttackTarget",
      "position": {"x": 500, "y": 150}
    }
  ],
  "execConnections": [
    {"fromNode": 1, "fromPin": "Out", "toNode": 2, "toPin": "In"},
    {"fromNode": 2, "fromPin": "Then", "toNode": 3, "toPin": "In"},
    {"fromNode": 2, "fromPin": "Else", "toNode": 4, "toPin": "In"}
  ],
  "dataConnections": []
}
```

---

## Éditer un graphe

### Ouvrir un graphe existant

**Via Asset Browser** (méthode principale) :
1. Panneau **Asset Browser** → onglet **"Blueprint Files"**
2. Naviguer dans `Blueprints/AI/`
3. **Double-clic** sur `guard_ai.ats`
4. Code appelé :
   ```cpp
   // Dans AssetBrowser.cpp
   if (node->type == "BehaviorTree" || node->type == "HFSM")
   {
       BlueprintEditor::Get().OpenGraphInEditor(node->fullPath);
   }
   ```
5. Détection du type :
   ```cpp
   std::string assetType = DetectAssetType(assetPath);
   // Lit champ "type" ou "graphType" ou "blueprintType"
   ```
6. Chargement via `TaskGraphLoader` :
   ```cpp
   std::vector<std::string> errors;
   TaskGraphTemplate* tmpl = TaskGraphLoader::LoadFromFile(assetPath, errors);
   m_VSEditorPanel->LoadTemplate(tmpl, assetPath);
   ```

---

## Opérations CRUD

### CREATE : Créer un nœud

**Méthode : Menu contextuel (clic droit)**
1. **Clic droit** sur zone vide du canvas
2. Popup **"Add Node"** avec catégories :
   ```
   Add Node
   ─────────────────
   Flow Control  ▶   ← EntryPoint, Branch, Sequence, While, ForEach, DoOnce, Delay
   Actions       ▶   ← AtomicTask
   Data          ▶   ← GetBBValue, SetBBValue, MathOp
   SubGraph      ▶   ← SubGraph
   ```
3. Sélectionner type → nœud créé à la position de la souris

**Code** :
```cpp
// VisualScriptEditorPanel::AddNode()
int VisualScriptEditorPanel::AddNode(TaskNodeType type, float x, float y)
{
    int newID = AllocNodeID();
    
    VSEditorNode eNode;
    eNode.nodeID = newID;
    eNode.posX = x;
    eNode.posY = y;
    
    eNode.def.NodeID = newID;
    eNode.def.Type = type;
    eNode.def.NodeName = GetNodeTypeLabel(type);
    
    m_editorNodes.push_back(eNode);
    m_template.Nodes.push_back(eNode.def);
    m_dirty = true;
    
    return newID;
}
```

**Types de nœuds disponibles** :

| Catégorie | Type | Description | Pins Exec | Pins Data |
|-----------|------|-------------|-----------|-----------|
| **Flow Control** | EntryPoint | Point d'entrée unique | Out | - |
| | Branch | Branchement conditionnel | In → True/False | ConditionID |
| | Sequence | Exécute enfants séquentiellement | In, Child0-N, Out | - |
| | While | Boucle conditionnelle | In → Loop/Completed | Condition |
| | ForEach | Itération sur collection | In → Loop Body/Completed | Collection |
| | DoOnce | Exécute une seule fois | In, Out | Reset |
| | Delay | Délai temporel | In, Completed | DelaySeconds |
| **Actions** | AtomicTask | Tâche atomique custom | In, Completed | Params |
| **Data** | GetBBValue | Lecture Blackboard | In, Out | Value (out) |
| | SetBBValue | Écriture Blackboard | In, Out | Value (in) |
| | MathOp | Opération mathématique | In, Out | A, B → Result |
| **SubGraph** | SubGraph | Appel sous-graphe | In, Completed | Params |

### READ : Visualiser un nœud

**Sélection** :
- **Simple clic** sur le nœud
- Surbrillance ImNodes
- `m_selectedNodeID` mis à jour

**Affichage visuel** (via `VisualScriptNodeRenderer::RenderNode()`) :
- **Barre de titre** colorée selon le type
- **Nom** du nœud (centre)
- **Pins Exec** : Triangles blancs/bleus (◀▶)
- **Pins Data** : Cercles colorés selon le type
  - Bool : vert
  - Int : orange
  - Float : jaune
  - String : magenta
  - EntityID : cyan
- **Indicateurs** :
  - **Rouge** : Breakpoint actif
  - **Vert** : Nœud en cours d'exécution (debug)

**Inspection détaillée** :
- Panneau **Properties** affiche :
  - **NodeID**, **Name**, **Type**
  - **Paramètres éditables** (voir UPDATE)

### UPDATE : Modifier un nœud

**Via Properties Panel** :
1. **Sélectionner** le nœud (clic simple)
2. Panneau **Properties** (droite) affiche les champs :

**Champs communs** :
- **Name** : InputText (modifiable en temps réel)

**Champs spécifiques par type** :

| Type de nœud | Paramètres éditables | Format |
|--------------|---------------------|--------|
| **AtomicTask** | `taskType` | InputText (ex: "MoveToLocation") |
| **Delay** | `delaySeconds` | InputFloat (ex: 2.5) |
| **Branch** / **While** | `conditionKey` | InputText (clé BB bool) |
| **GetBBValue** / **SetBBValue** | `bbKey` | InputText (clé BB) |
| **SubGraph** | `subGraphPath` | InputText (chemin .ats) |
| **MathOp** | `mathOp` | InputText (+, -, *, /) |

**Code** :
```cpp
// RenderProperties() - édition en temps réel
TaskNodeDefinition& def = eNode->def;

char nameBuf[128];
strncpy_s(nameBuf, sizeof(nameBuf), def.NodeName.c_str(), _TRUNCATE);
if (ImGui::InputText("Name##vsname", nameBuf, sizeof(nameBuf)))
{
    def.NodeName = nameBuf;
    // Sync back to template
    for (TaskNodeDefinition& tNode : m_template.Nodes)
    {
        if (tNode.NodeID == m_selectedNodeID)
        {
            tNode.NodeName = def.NodeName;
            break;
        }
    }
    m_dirty = true;
}
```

**Déplacement** :
- **Glisser-déposer** le nœud sur le canvas
- Position récupérée via `ImNodes::GetNodeEditorSpacePos(nodeID)`
- Sauvegardée dans `VSEditorNode.posX/posY`
- Écrite dans JSON lors du Save :
  ```json
  "position": {"x": 500.0, "y": 150.0}
  ```

**Snap-to-Grid** :
- Activer : `Ctrl+G`
- Arrondit positions à la grille 16px

### DELETE : Supprimer un nœud

**Méthode** :
1. **Sélectionner** le nœud (clic)
2. **Delete** ou **Del**
3. Suppression via `RemoveNode(nodeID)` :

**Code** :
```cpp
void VisualScriptEditorPanel::RemoveNode(int nodeID)
{
    // 1. Remove editor node
    m_editorNodes.erase(
        std::remove_if(m_editorNodes.begin(), m_editorNodes.end(),
            [nodeID](const VSEditorNode& n) { return n.nodeID == nodeID; }),
        m_editorNodes.end());
    
    // 2. Remove template node
    m_template.Nodes.erase(
        std::remove_if(m_template.Nodes.begin(), m_template.Nodes.end(),
            [nodeID](const TaskNodeDefinition& n) { return n.NodeID == nodeID; }),
        m_template.Nodes.end());
    
    // 3. Remove exec connections
    m_template.ExecConnections.erase(
        std::remove_if(m_template.ExecConnections.begin(), m_template.ExecConnections.end(),
            [nodeID](const ExecPinConnection& c) {
                return c.SourceNodeID == nodeID || c.TargetNodeID == nodeID;
            }),
        m_template.ExecConnections.end());
    
    // 4. Remove data connections
    m_template.DataConnections.erase(
        std::remove_if(m_template.DataConnections.begin(), m_template.DataConnections.end(),
            [nodeID](const DataPinConnection& c) {
                return c.SourceNodeID == nodeID || c.TargetNodeID == nodeID;
            }),
        m_template.DataConnections.end());
    
    m_template.BuildLookupCache();
    RebuildLinks();
    m_dirty = true;
}
```

⚠️ **Cascade** : Supprime automatiquement **toutes les connexions** (exec + data) attachées au nœud.

---

## Opérations sur les connexions

### Types de connexions

#### 1. Exec Connections (Flux d'exécution)

**Visuel** :
- Lignes **blanches/bleues épaisses**
- Pins **triangles** : ◀ (input) ▶ (output)

**Rôle** : Définit l'**ordre d'exécution** des nœuds (flowchart)

**Exemple** :
```
EntryPoint ──▶ Branch ──▶ True ──▶ AttackTask
                      │
                      └──▶ False ──▶ PatrolTask
```

**Format JSON** :
```json
"execConnections": [
  {"fromNode": 1, "fromPin": "Out", "toNode": 2, "toPin": "In"},
  {"fromNode": 2, "fromPin": "Then", "toNode": 3, "toPin": "In"},
  {"fromNode": 2, "fromPin": "Else", "toNode": 4, "toPin": "In"}
]
```

#### 2. Data Connections (Flux de données)

**Visuel** :
- Lignes **bleues fines** (`IM_COL32(160, 160, 255, 200)`)
- Pins **cercles** colorés selon le type

**Rôle** : Transmet **valeurs typées** entre nœuds

**Types supportés** :
- `Bool` : Booléen (vert)
- `Int` : Entier (orange)
- `Float` : Flottant (jaune)
- `String` : Texte (magenta)
- `Vector` : Vecteur 2D/3D (bleu)
- `EntityID` : Référence d'entité (cyan)

**Exemple** :
```
[GetBBValue "speed"] ──○──▶ [MathOp *2] ──○──▶ [SetBBValue "finalSpeed"]
     Float out              Float in→out          Float in
```

**Format JSON** :
```json
"dataConnections": [
  {"fromNode": 5, "fromPin": "Value", "toNode": 6, "toPin": "InputA"},
  {"fromNode": 6, "fromPin": "Result", "toNode": 7, "toPin": "Value"}
]
```

### Créer une connexion

**Méthode : Drag & Drop**
1. **Cliquer** sur pin de **sortie** (droite du nœud source)
2. **Glisser** vers pin d'**entrée** (gauche du nœud destination)
3. **Relâcher**
4. Validation automatique :
   - **Exec** : Toujours valide (pas de vérification de type)
   - **Data** : Vérifie compatibilité des types de pins
   - **Cycles** : Détecte et empêche les cycles interdits

**Détection ImNodes** :
```cpp
// Dans VisualScriptEditorPanel::RenderCanvas()
int startAttr = -1, endAttr = -1;
if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
{
    // Extraire nodeID depuis attributeUID
    // Formule : attrUID = nodeID * 10000 + offset
    int srcNodeID = startAttr / 10000;
    int dstNodeID = endAttr / 10000;
    
    // Déterminer pin names (simplification : "Out" et "In")
    ConnectExec(srcNodeID, "Out", dstNodeID, "In");
    m_dirty = true;
}
```

**Création de la connexion** :
```cpp
void VisualScriptEditorPanel::ConnectExec(
    int srcNodeID, const std::string& srcPinName,
    int dstNodeID, const std::string& dstPinName)
{
    ExecPinConnection conn;
    conn.SourceNodeID = srcNodeID;
    conn.SourcePinName = srcPinName;
    conn.TargetNodeID = dstNodeID;
    conn.TargetPinName = dstPinName;
    
    m_template.ExecConnections.push_back(conn);
    RebuildLinks();  // Reconstruit VSEditorLink pour ImNodes
    m_dirty = true;
}
```

### Supprimer une connexion

**Méthode** :
1. **Sélectionner** la connexion (clic sur le lien)
2. **Delete** ou **Del**
3. ImNodes détecte la destruction

**Code** :
```cpp
int destroyedLink = -1;
if (ImNodes::IsLinkDestroyed(&destroyedLink))
{
    // Remove from m_editorLinks
    m_editorLinks.erase(
        std::remove_if(m_editorLinks.begin(), m_editorLinks.end(),
            [destroyedLink](const VSEditorLink& l) {
                return l.linkID == destroyedLink;
            }),
        m_editorLinks.end());
    
    // Sync back to template connections
    // (Simplification : RebuildLinks() inverse, mais ici on supprime directement)
    m_dirty = true;
}
```

---

## Blackboard (Variables)

### Créer une variable Blackboard

**Dans le panneau Local Blackboard** (bas droit du VS Graph Editor) :
1. Bouton **"[+] Add key"**
2. Nouvelle entrée créée :
   ```cpp
   BlackboardEntry entry;
   entry.Key = "newKey";
   entry.Type = VariableType::Float;
   entry.IsGlobal = false;
   m_template.Blackboard.push_back(entry);
   ```
3. **Éditer** :
   - **Nom** : Input text (ex: `playerHealth`)
   - **Type** : Combo (None/Bool/Int/Float/Vector/EntityID/String)
   - **G** : Checkbox "Global" (partagée entre graphes)
4. **Supprimer** : Bouton **"x"**

**Format JSON** :
```json
"blackboard": [
  {
    "key": "playerHealth",
    "type": "Float",
    "value": 100.0,
    "isGlobal": false
  },
  {
    "key": "targetEntity",
    "type": "EntityID",
    "value": "0",
    "isGlobal": false
  }
]
```

### Utiliser les variables

**Lecture : GetBBValue**
1. Créer nœud **GetBBValue** (Data → GetBBValue)
2. Paramètre `bbKey` : Nom de la variable (ex: `"playerHealth"`)
3. Pin data out **"Value"** : Valeur lue (type automatique selon BB)
4. Connecter vers pin data in d'un autre nœud (ex: MathOp, Branch)

**Écriture : SetBBValue**
1. Créer nœud **SetBBValue** (Data → SetBBValue)
2. Paramètre `bbKey` : Nom de la variable
3. Pin data in **"Value"** : Nouvelle valeur à écrire
4. Connecter depuis pin data out d'un autre nœud

**Exemple complet : Système de vie**
```
[GetBBValue "playerHealth"] ──○──▶ [MathOp -10] ──○──▶ [SetBBValue "playerHealth"]
         Float out                 Float→Float           Float in

                                      ↓ exec out
                            [Branch conditionKey="isDead"]
                                   / \
                               True   False
                                 ↓     ↓
                            [GameOver] [Continue]
```

**Variables globales** :
- Cocher **"G"** lors de la création
- Accessible depuis **tous les graphes** de l'entité
- Utile pour partager état entre AI et Level Scripting

---

## Débogage

### Breakpoints (F9)

**Ajouter** :
1. Sélectionner un nœud dans VS Graph Editor
2. **F9** ou cocher **"Breakpoint (F9)"** dans Properties
3. Appel :
   ```cpp
   DebugController::Get().ToggleBreakpoint(
       0 /* graphID */,
       m_selectedNodeID,
       m_template.Name,
       def.NodeName
   );
   ```
4. **Indicateur visuel** : Cercle rouge sur le nœud

**Runtime** :
- Exécution s'arrête au nœud avec breakpoint
- Panneau **Debugger** (View → Debugger) :
  - État du Blackboard (clés + valeurs actuelles)
  - Nœud actif (surbrillance verte via `RenderActiveNodeGlow()`)
  - Call stack (si SubGraphs imbriqués)
- **Contrôles** :
  - **F5** : Continue (reprendre exécution)
  - **F10** : Step Over (nœud suivant)
  - **F11** : Step Into (entrer dans SubGraph)

**Supprimer breakpoint** : Re-appuyer **F9** ou décocher dans Properties

### Validation en temps réel

Le système valide le graphe pendant l'édition via `RenderValidationOverlay()`.

**Erreurs bloquantes (rouges)** :
- ❌ **Nœud sans exec-in** (sauf EntryPoint) :
  ```
  "Node 5 (AttackTask): no exec-in connection"
  ```
- ❌ **Cycle dans le graphe** (détecté par `GraphDocument::HasCycles()`)
- ❌ **Types incompatibles** sur connexions data (ex: Int → String)

**Warnings non-bloquants (jaunes)** :
- ⚠️ **SubGraph sans chemin** :
  ```
  "Node 8 (SubGraph): SubGraphPath is empty"
  ```
- ⚠️ **AtomicTask sans taskType**
- ⚠️ **Nœuds orphelins** (non-atteignables depuis EntryPoint)

**Affichage** :
- Overlay semi-transparent en bas du canvas
- Liste des messages avec NodeID
- Sauvegarde bloquée si erreurs critiques

---

## Raccourcis clavier

### VS Graph Editor (VisualScriptEditorPanel)

| Raccourci | Action | Description |
|-----------|--------|-------------|
| `Ctrl+S` | **Sauvegarder** | Sauvegarde le graphe au format v4 |
| `Ctrl+N` | **Nouveau** | Crée un graphe vide |
| `Clic droit` | **Palette de nœuds** | Ouvre le menu de création |
| `Delete` | **Supprimer** | Supprime nœud/lien sélectionné |
| `F9` | **Toggle Breakpoint** | Ajoute/retire un breakpoint |
| `Clic simple` | **Sélectionner** | Sélectionne un nœud |
| `Glisser-Déposer` | **Déplacer** | Déplace un nœud |
| `Ctrl+G` | **Snap-to-Grid** | Active/désactive alignement grille |

### Debugger (Runtime)

| Raccourci | Action | Description |
|-----------|--------|-------------|
| `F5` | **Continue** | Reprend l'exécution après breakpoint |
| `F10` | **Step Over** | Exécute le nœud suivant |
| `F11` | **Step Into** | Entre dans un SubGraph |
| `Shift+F11` | **Step Out** | Sort du SubGraph actuel |

### NodeGraphPanel (Legacy BT v2 - Deprecated)

⚠️ **Uniquement pour debug BT v2** (plus d'édition)

| Raccourci | Action |
|-----------|--------|
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+D` | Dupliquer nœud |
| `Ctrl+C` | Copier nœuds |
| `Ctrl+V` | Coller nœuds |
| `Ctrl+0` | Reset panning |
| `Ctrl+M` | Toggle minimap |

### Navigation canvas (ImNodes)

| Action | Input |
|--------|-------|
| **Pan** | Middle-mouse drag OU `Alt+LMB drag` |
| **Zoom** | Mouse wheel |
| **Zoom to Fit** | `F` |
| **Reset View** | `Ctrl+0` |
| **Multi-select** | `Shift+Click` OU rubber-band drag |

---

## Formats de fichiers

### Schema v4 : ATS Visual Script (ACTUEL)

**Extension** : `.ats` ou `.json`

**Structure complète** :
```json
{
  "schema_version": 4,
  "name": "guard_ai",
  "graphType": "VisualScript",

  "blackboard": [
    {
      "key": "playerHealth",
      "type": "Float",
      "value": 100.0,
      "isGlobal": false
    },
    {
      "key": "targetEntity",
      "type": "EntityID",
      "value": "0",
      "isGlobal": false
    }
  ],

  "nodes": [
    {
      "id": 1,
      "label": "Start",
      "type": "EntryPoint",
      "position": {"x": 100.0, "y": 100.0}
    },
    {
      "id": 2,
      "label": "Check Low Health",
      "type": "Branch",
      "conditionKey": "lowHealth",
      "position": {"x": 300.0, "y": 100.0}
    },
    {
      "id": 3,
      "label": "Heal",
      "type": "AtomicTask",
      "taskType": "HealAction",
      "position": {"x": 500.0, "y": 50.0}
    },
    {
      "id": 4,
      "label": "Attack",
      "type": "AtomicTask",
      "taskType": "AttackTarget",
      "position": {"x": 500.0, "y": 150.0}
    }
  ],

  "execConnections": [
    {"fromNode": 1, "fromPin": "Out", "toNode": 2, "toPin": "In"},
    {"fromNode": 2, "fromPin": "Then", "toNode": 3, "toPin": "In"},
    {"fromNode": 2, "fromPin": "Else", "toNode": 4, "toPin": "In"}
  ],

  "dataConnections": []
}
```

**Champs obligatoires** :
- `schema_version` : **4** (entier)
- `nodes` : Array de TaskNodeDefinition (au moins 1 EntryPoint)
- `execConnections` : Array de connexions exec

**Champs optionnels** :
- `name` : Nom du graphe (défaut: "Unnamed")
- `graphType` : "VisualScript" (défaut si absent)
- `blackboard` : Array de variables (défaut: `[]`)
- `dataConnections` : Array de connexions data (défaut: `[]`)
- `isSubGraph` : Boolean (Phase 3 - SubGraphs)
- `inputParameters` / `outputParameters` : Pour SubGraphs

### Schema v2 : Behavior Tree Legacy (DEPRECATED)

**Extension** : `.json`

**Structure (doubly-nested)** :
```json
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "data": {
    "data": {
      "rootNodeId": 1,
      "nodes": [
        {
          "id": 1,
          "type": "Selector",
          "name": "Root",
          "position": {"x": 100, "y": 100},
          "children": [2, 3]
        }
      ]
    }
  }
}
```

**⚠️ NE PLUS UTILISER** : Migrer vers v4 avec `BTtoVSMigrator` ou script Python.

---

## Migration

### Migrer BT v2 → VS v4

**Via script Python** :
```bash
cd Tools
python migrate_bt_to_vs.py ../Blueprints/AI/guard_patrol.json

# Crée : guard_patrol_v4.ats
```

**Via éditeur** :
1. Menu **Tools → Migrate Blueprints v1 → v2**
2. Dialogue de migration avec liste de fichiers
3. Sélectionner fichiers à migrer
4. **"Migrate Selected"**
5. Fichiers v4 créés avec suffixe `_v4.ats`

**Via code** :
```cpp
#include "BTtoVSMigrator.h"

json btData; // Charger BT v2 JSON
std::vector<std::string> errors;

TaskGraphTemplate v4Template = BTtoVSMigrator::Convert(btData, errors);

// Sauvegarder v4Template
json v4Json;
v4Json["schema_version"] = 4;
v4Json["graphType"] = "VisualScript";
// ... sérialiser template
```

### Différences BT v2 vs VS v4

| Aspect | BT v2 | VS v4 |
|--------|-------|-------|
| **Structure** | Hiérarchie parent→children | Graphe avec connexions exec/data explicites |
| **Nœuds composites** | Selector, Sequence (implicite) | Sequence node explicite avec pins |
| **Conditions** | Nœuds Condition (feuilles) | Branch node avec conditionKey |
| **Actions** | Nœuds Action | AtomicTask avec taskType |
| **Decorators** | Nœuds Decorator | Pas d'équivalent direct (utiliser While/DoOnce) |
| **Connexions** | Implicites (children array) | Explicites (execConnections) |
| **Variables** | Pas de Blackboard | Blackboard intégré |
| **SubGraphs** | Pas supporté | SubGraph node (Phase 3) |

---

## Bonnes pratiques

### Organisation du graphe

1. **Nommer clairement** : `"Check Player Nearby"` plutôt que `"Branch 2"`
2. **EntryPoint en haut à gauche** : Convention visuelle (flux top→down, left→right)
3. **Grouper logiquement** : Branches True/False alignées verticalement
4. **Snap-to-Grid** (`Ctrl+G`) : Alignement propre (grille 16px)
5. **Espacement** : Laisser 100-200px entre nœuds pour lisibilité

### Blackboard

1. **Noms descriptifs** : `playerHealth` plutôt que `hp` ou `val`
2. **Typage strict** : Définir le type correct (Float pour santé, Bool pour flags)
3. **Variables globales** : Uniquement pour états partagés (éviter abus)
4. **Documentation** : Utiliser noms auto-documentés (`isPlayerDetected` vs `flag1`)

### Performance

1. **Éviter cycles infinis** : Utiliser While avec condition de sortie
2. **Limiter profondeur SubGraphs** : Max 3-4 niveaux d'imbrication
3. **AtomicTasks légers** : Tasks rapides (< 1ms) pour réactivité IA
4. **Blackboard minimal** : Seulement variables nécessaires (mémoire)

### Débogage

1. **Breakpoints stratégiques** : Aux décisions clés (Branch, While)
2. **Noms explicites** : Facilite identification dans Debugger
3. **Validation continue** : Corriger warnings avant sauvegarde
4. **Tester en Runtime** : Mode Runtime pour voir exécution en temps réel

---

## Pipeline complet : De la création à l'exécution

### 1. Création (Standalone Editor)

```cpp
// Lancé via OlympeBlueprintEditor.exe
BlueprintEditor::Get().InitializeStandaloneEditor();
BlueprintEditorGUI gui;
gui.Initialize();

// User: View → VS Graph Editor → New Graph
// User: Ajoute EntryPoint, Branch, AtomicTask
// User: Connecte nœuds, configure Blackboard
// User: Ctrl+S → Sauvegarde "Blueprints/AI/guard_ai.ats"

VisualScriptEditorPanel::Save();
// → Appelle SerializeAndWrite()
// → Écrit JSON v4 sur disque
```

### 2. Chargement (AssetManager)

```cpp
// Au démarrage du jeu
AssetManager::Get().PreloadAssets();

// Pour un asset spécifique
std::vector<std::string> errors;
AssetID id = AssetManager::Get().LoadTaskGraph("Blueprints/AI/guard_ai.ats", errors);

// Récupération
TaskGraphTemplate* tmpl = AssetManager::Get().GetTaskGraph(id);
```

### 3. Instanciation (World/Entity)

```cpp
// Création entité avec graphe AI
uint64_t guardEntity = World::Get().CreateEntity();

// Attachement du graphe (via component AI ou script)
AIComponent* aiComp = World::Get().GetComponent<AIComponent>(guardEntity);
aiComp->SetTaskGraph("Blueprints/AI/guard_ai.ats");

// Le graphe est instancié avec son propre LocalBlackboard
```

### 4. Exécution (VSGraphExecutor)

```cpp
// Chaque frame (si entité active)
void VSGraphExecutor::Tick(float deltaTime)
{
    // 1. Event Tick déclenché
    // 2. Exécution depuis EntryPoint
    // 3. Parcourt ExecConnections
    // 4. Évalue Branch conditions (lit Blackboard)
    // 5. Exécute AtomicTasks
    // 6. Termine quand plus de connexions
    // 7. Reboucle au frame suivant
}
```

### 5. Débogage (Runtime Editor)

```cpp
// Lancé en parallèle du jeu
BlueprintEditor::Get().InitializeRuntimeEditor();

// User: View → Debugger
// User: F9 sur nœud → Breakpoint
// Exécution pause → Inspect Blackboard
// F10 pour Step Over
```

---

## Dépannage

### Erreur : "No exec-in connection"

**Cause** : Nœud non-connecté (orphelin)  
**Solution** : Connecter un pin exec depuis un nœud parent

### Warning : "SubGraph path is empty"

**Cause** : Nœud SubGraph sans `subGraphPath`  
**Solution** : Sélectionner nœud → Properties → SubGraph Path → Entrer chemin `.ats`

### Graphe ne s'ouvre pas dans VS Graph Editor

**Cause** : Format BT v2 legacy détecté  
**Solution** :
1. Migrer avec `Tools/migrate_bt_to_vs.py`
2. Ou vérifier champ `"type"` / `"graphType"` dans le JSON

### Connexion refusée (data pins)

**Cause** : Types incompatibles (ex: Int → String)  
**Solution** : Utiliser nœud **MathOp** ou **GetBBValue** pour conversion

---

## Ressources additionnelles

### Documentation

- **Guide ATS complet** : `Documentation/Olympe_ATS_VisualScript_Complete_Doc.md`
- **User Manual ATS** : `Documentation/UserManual_ATS_VisualScripting.md`
- **Architecture NodeGraph** : `Docs/Developer/NodeGraph_Unified_Architecture.md`

### Exemples

- **Graphes AI** : `Blueprints/AI/*.ats`
- **Level Scripting** : `Gamedata/Levels/*.ats`
- **Exemples simples** : `Gamedata/TaskGraph/Examples/*.ats`

### Code source

- **Backend** : `Source/BlueprintEditor/blueprinteditor.h/cpp`
- **VS Editor** : `Source/BlueprintEditor/VisualScriptEditorPanel.h/cpp`
- **TaskGraph Loader** : `Source/TaskSystem/TaskGraphLoader.h/cpp`
- **Node Renderer** : `Source/BlueprintEditor/VisualScriptNodeRenderer.h/cpp`

---

## Résumé : Création et édition d'un graphe (Quick Start)

### 🚀 Quick Start (5 minutes)

**1. Créer un graphe** :
```
View → VS Graph Editor → New Graph
```

**2. Ajouter nœuds** :
```
Clic droit → Flow Control → EntryPoint
Clic droit → Flow Control → Branch
Clic droit → Actions → AtomicTask (x2)
```

**3. Connecter** :
```
EntryPoint.Out ──→ Branch.In
Branch.True    ──→ Task1.In
Branch.False   ──→ Task2.In
```

**4. Configurer** :
```
- Branch : conditionKey = "lowHealth"
- Task1 : taskType = "HealAction"
- Task2 : taskType = "AttackTarget"
- Blackboard : Ajouter "lowHealth" (Bool)
```

**5. Sauvegarder** :
```
Ctrl+S → Blueprints/AI/myGraph.ats
```

**6. Tester** :
```
Lancer jeu → Attacher graphe à entité → Observer exécution
View → Debugger → F9 sur Branch → Runtime pause → Inspect
```

---

**Dernière mise à jour** : Mars 2026  
**Version Blueprint Editor** : 4.0 (Phase 7)  
**Contact** : Olympe Engine Team
