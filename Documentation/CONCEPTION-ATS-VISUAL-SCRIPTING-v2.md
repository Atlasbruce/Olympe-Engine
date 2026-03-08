# Conception ATS Visual Scripting - Node Types, Variables et Blackboard
## Atomic Task System - Flowchart Algorithmique

**Date** : 2026-03-08  
**Auteur** : Atlasbruce  
**Version** : 2.0 (Visual Scripting)

---

## Table des matières

1. [Vue d'ensemble et positionnement ATS](#1-vue-densemble-et-positionnement-ats)
2. [Cartographie des types de nodes](#2-cartographie-des-types-de-nodes)
3. [Système de Pins (Exec et Data)](#3-système-de-pins-exec-et-data)
4. [Système Blackboard](#4-système-blackboard)
5. [Catalogue d'Actions](#5-catalogue-dactions)
6. [Catalogue de Conditions](#6-catalogue-de-conditions)
7. [Schéma JSON Unifié v3](#7-schéma-json-unifié-v3)
8. [Pipeline d'exécution ATS → ECS](#8-pipeline-dexécution-ats-ecs)
9. [Sous-graphes et SubTasks](#9-sous-graphes-et-subtasks)
10. [Ancrage dans le code existant](#10-ancrage-dans-le-code-existant)
11. [Questions ouvertes / Décisions futures](#11-questions-ouvertes-décisions-futures)

---

## 1. Vue d'ensemble et positionnement ATS

### 1.1. Définition

**ATS (Atomic Task System)** est le système de **scripting visuel algorithmique** d'Olympe Engine. Il permet aux concepteurs et développeurs de créer des logiques de comportement, gameplay, et systèmes via un **éditeur de node graph flowchart**, sans nécessiter d'écrire directement du code C++.

**Paradigme** : **Flowchart / Algorigramme** avec **flux Exec visuel** (comme Blueprint Unreal Engine Event Graphs) + **data flow typé**.

### 1.2. Positionnement vis-à-vis des Behavior Trees classiques

**ATS Visual Scripting** n'est **PAS** un Behavior Tree. C'est un système de **graphe flux libre** comparable aux **Blueprint Event Graphs** d'Unreal Engine.

| Propriété | **BT classique** | **ATS Visual Scripting** |
|-----------|------------------|--------------------------|
| Paradigme | Arbre hiérarchique fixe (parent→enfants) | Graphe flux libre avec connexions Exec |
| Types de nodes | Composite/Décorateur/Feuille | Flow Control / Action / Data / Condition (intégrés) |
| Conditionnelles | Implicites (Selector/Sequence) | **Explicites** (Branch True/False visuel) |
| Boucles | Pas de support direct | **While/For natifs** avec pins Loop/Exit |
| Flux | Top-down hiérarchique | **Algorithmique libre** (Exec pins) |
| Variables | Blackboard uniquement | **Blackboard + Data pins typées** |
| Analogie | Behavior Tree UE4 | **Blueprint Event Graph UE5** |

### 1.3. Infrastructure technique

L'éditeur ATS s'appuie sur **NodeGraphCore**, un ensemble de systèmes bas niveau :

| Module | Rôle |
|--------|------|
| `GraphDocument` | Modèle de données du graphe (nodes, connexions, metadata) |
| `NodeGraphManager` | Chargement, sauvegarde, cycle de vie des graphes |
| `BlackboardSystem` | Gestion des variables par entité (`namespace OlympeNodeGraph`) |
| `CommandSystem` | Undo/Redo via pattern Command |
| `VisualScriptSystem` | **NOUVEAU** : Runtime exécution des graphes flowchart |

### 1.4. Types de graphes supportés

| `graphKind` | Description |
|-------------|-------------|
| `VisualScript` | **Graphe flowchart algorithmique** pour IA, level scripts, game rules, cinématiques |
| `BehaviorTree` | *Ancien système* (maintenu pour compatibilité) |
| `AnimGraph` | Logique de transition d'animations |
| `EventGraph` | Réponse événementielle (inputs, messages ECS) |

**Note** : Ce document se concentre sur **`VisualScript`** (nouveau paradigme flowchart).

---

## 2. Cartographie des types de nodes

### 2.1. Flow Control Nodes (Flux d'Exécution)

Ces nodes contrôlent le **flux d'exécution** (Exec pins) du graphe.

#### EntryPoint

**Type** : `EntryPoint`  
**Display Name** : Event Tick  
**Description** : Point d'entrée obligatoire du graphe, déclenché chaque frame (Event Tick).

**Pins** :
- **Exec Out** : 1 sortie vers premier node logique

**JSON** :
```json
{
  "id": 1,
  "type": "EntryPoint",
  "name": "Event Tick",
  "execOut": [2]
}
```

**Règles** :
- **Obligatoire** : 1 seul EntryPoint par graphe
- Toujours `id: 1` par convention

---

#### Branch (If/Else visuel)

**Type** : `Branch`  
**Display Name** : Branch  
**Description** : Embranchement conditionnel visuel avec **pins True/False**.

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out True** : Exécuté si condition vraie
- **Exec Out False** : Exécuté si condition fausse
- **Data In** : Optionnel (pour data pins connectées)

**Paramètres** :
- `conditionType` : Type de condition (`CompareValue`, `IsSet`, `InRange`, etc.)
- `parameters` : Params condition (voir section 6)

**JSON** :
```json
{
  "id": 3,
  "type": "Branch",
  "name": "Target Close?",
  "conditionType": "CompareValue",
  "parameters": [
    {"name": "left", "value": "local:TargetDistance"},
    {"name": "operator", "value": "<="},
    {"name": "right", "value": 60.0}
  ],
  "execOut": {
    "true": [4],
    "false": [10]
  }
}
```

**Couleur UI** : Orange (`0xFFFF8800`)

---

#### Switch (Multi-branches)

**Type** : `Switch`  
**Display Name** : Switch  
**Description** : Dispatcher selon valeur enum/string.

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out** : N sorties (case1, case2, ..., default)

**Paramètres** :
- `switchType` : `"Enum"` ou `"String"`
- `enumType` : Nom du type enum (si `Enum`)
- `key` : Clé Blackboard à tester
- `execOut` : Map `{valeur: [nodeIds]}`

**JSON** :
```json
{
  "id": 5,
  "type": "Switch",
  "name": "Switch on State",
  "switchType": "Enum",
  "enumType": "AIState",
  "key": "local:CurrentState",
  "execOut": {
    "Patrol": [6],
    "Combat": [10],
    "Investigate": [15],
    "default": [20]
  }
}
```

**Couleur UI** : Bleu (`0xFF4488FF`)

---

#### Sequence (Actions ordonnées)

**Type** : `Sequence`  
**Display Name** : Sequence  
**Description** : Exécute N actions dans l'ordre (pins then0, then1, ...).

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out** : N sorties (`then0`, `then1`, `then2`, ...)

**JSON** :
```json
{
  "id": 6,
  "type": "Sequence",
  "name": "Attack Sequence",
  "execOut": {
    "then0": [7],
    "then1": [8],
    "then2": [9]
  }
}
```

**Couleur UI** : Vert (`0xFF88FF44`)

---

#### While (Boucle conditionnelle)

**Type** : `While`  
**Display Name** : While  
**Description** : Répète corps tant que condition vraie, avec pins **Loop** et **Exit**.

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out Loop** : Corps de boucle (reboucle vers While)
- **Exec Out Exit** : Sortie quand condition fausse

**Paramètres** :
- `conditionType`, `parameters` : Même format que Branch

**JSON** :
```json
{
  "id": 8,
  "type": "While",
  "name": "While Not Close",
  "conditionType": "CompareValue",
  "parameters": [
    {"name": "left", "value": "local:TargetDistance"},
    {"name": "operator", "value": ">"},
    {"name": "right", "value": 10.0}
  ],
  "execOut": {
    "loop": [9],
    "exit": [10]
  }
}
```

**Couleur UI** : Cyan (`0xFFFF8844`)

**⚠️ Important** : While s'exécute **dans le même frame** jusqu'à condition fausse. Pour boucles multi-frames, utiliser flag BB + Branch.

---

#### ForEach (Itération sur liste)

**Type** : `ForEach`  
**Display Name** : For Each  
**Description** : Parcourt tableau Blackboard, pins **LoopBody** et **Completed**.

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out LoopBody** : Exécuté pour chaque élément
- **Exec Out Completed** : Exécuté après fin liste
- **Data Out currentItem** : Élément courant
- **Data Out currentIndex** : Index courant

**Paramètres** :
- `arrayKey` : Clé Blackboard (array)

**JSON** :
```json
{
  "id": 15,
  "type": "ForEach",
  "name": "For Each Waypoint",
  "arrayKey": "local:PatrolRoute",
  "execOut": {
    "loopBody": [16],
    "completed": [20]
  },
  "dataOut": [
    {"name": "currentItem", "type": "Vector"},
    {"name": "currentIndex", "type": "Int"}
  ]
}
```

---

#### DoOnce (Exécution unique)

**Type** : `DoOnce`  
**Display Name** : Do Once  
**Description** : Ne passe qu'une fois, bloqué après 1ère exécution.

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out** : 1 sortie (bloquée après 1ère exécution)
- **Reset (optionnel)** : Réouvre la porte

**Paramètres** :
- `startClosed` : bool (default false)

**JSON** :
```json
{
  "id": 20,
  "type": "DoOnce",
  "name": "First Alert Only",
  "startClosed": false,
  "execOut": [21]
}
```

---

#### Delay (Timer)

**Type** : `Delay`  
**Display Name** : Delay  
**Description** : Pause flux X secondes, puis active pin **Completed**.

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out Completed** : Activé après délai

**Paramètres** :
- `duration` : float (secondes)

**JSON** :
```json
{
  "id": 25,
  "type": "Delay",
  "name": "Wait 2s",
  "duration": 2.0,
  "execOut": [26]
}
```

**Comportement** :
- Crée `DelayTimerComponent` ECS
- `VisualScriptSystem::UpdateDelayTimers()` décrémente timers
- Active pin `completed` quand `elapsed >= duration`

---

### 2.2. Action Nodes (Tâches)

**Type** : `Action`  
**Description** : Exécute une tâche concrète (mouvement, IA, audio, etc.).

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out** : 1 sortie (après exécution)

**Paramètres** :
- `actionType` : String (voir section 5 - Catalogue Actions)
- `parameters` : Params spécifiques action

**Structure générique** :
```json
{
  "id": 30,
  "type": "Action",
  "name": "Move To Goal",
  "actionType": "MoveToGoal",
  "parameters": [
    {"name": "speed", "value": 1.0}
  ],
  "execOut": [31]
}
```

**Couleur UI** : Bleu clair (`0xFF6699FF`)

---

### 2.3. Data Nodes (Variables)

#### GetBBValue (Get Blackboard Value)

**Type** : `GetBBValue`  
**Description** : Lit valeur Blackboard → sortie Data pin.

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out** : 1 sortie
- **Data Out** : 1 sortie (type selon clé BB)

**Paramètres** :
- `key` : Clé Blackboard (format `scope:path`)

**JSON** :
```json
{
  "id": 40,
  "type": "GetBBValue",
  "name": "Get Distance",
  "key": "local:TargetDistance",
  "dataOut": [{"name": "distance", "type": "Float"}],
  "execOut": [41]
}
```

---

#### SetBBValue (Set Blackboard Value)

**Type** : `SetBBValue`  
**Description** : Écrit Data pin → Blackboard.

**Pins** :
- **Exec In** : 1 entrée
- **Exec Out** : 1 sortie
- **Data In** : 1 entrée (valeur à écrire)

**Paramètres** :
- `key` : Clé Blackboard

**JSON** :
```json
{
  "id": 45,
  "type": "SetBBValue",
  "name": "Set Alert Level",
  "key": "local:AlertLevel",
  "dataIn": [{"name": "value", "type": "Int"}],
  "execOut": [46]
}
```

---

#### Math (Opérations mathématiques)

**Type** : `Math`  
**Description** : Calculs arithmétiques (+, -, *, /, %).

**Pins** :
- **Data In A** : float
- **Data In B** : float
- **Data Out result** : float

**Paramètres** :
- `operation` : `"+"`, `"-"`, `"*"`, `"/"`, `"%"`

**JSON** :
```json
{
  "id": 50,
  "type": "Math",
  "operation": "*",
  "dataIn": [
    {"name": "A", "type": "Float"},
    {"name": "B", "type": "Float"}
  ],
  "dataOut": [{"name": "result", "type": "Float"}]
}
```

---

### 2.4. Nodes Spéciaux

| Type | Rôle |
|------|------|
| **Comment** | Bloc de commentaire visuel (pas d'exécution) |
| **Reroute** | Point de reroutage de lien (comme UE Blueprint) |
| **SubGraph** | Référence un sous-graphe externe (asset JSON) |

---

## 3. Système de Pins (Exec et Data)

### 3.1. Types de pins

#### Exec Pins (Flux d'exécution)

**Visuel** : Lignes **blanches/bleues** épaisses.

**Types de sorties Exec** :
- **Simple** : 1 sortie (`execOut`), ex : Action, Delay
- **Conditionnelle** : 2 sorties (`true`, `false`), ex : Branch
- **Multiple** : N sorties (`case1...caseN`, `default`), ex : Switch
- **Ordonnées** : N sorties (`then0...thenN`), ex : Sequence
- **Boucle** : 2 sorties (`loop`, `exit`), ex : While

**Règles** :
- Exec → Exec uniquement (pas Exec → Data)
- 1 pin Exec In par node
- Multiple sorties Exec possibles

---

#### Data Pins (Flux de données)

**Visuel** : Lignes **colorées fines** (couleur = type).

**Types Data supportés** :

| Type | Couleur | Exemples |
|------|---------|----------|
| `Bool` | Rouge | true, false |
| `Int` | Cyan | 42, -10, 0 |
| `Float` | Vert | 3.14, 0.5, -2.7 |
| `String` | Magenta | "Combat", "Guard_01" |
| `Vector` | Jaune | {x:100, y:200, z:0} |
| `EntityRef` | Violet | EntityID(42) |

**Règles** :
- Type-safe : `Int` vers `Float` OK (cast implicite)
- `Int` vers `String` KO (conversion explicite requise)
- Data pins indépendantes du flux Exec

---

### 3.2. Connexions Data explicites

**Format JSON** :
```json
{
  "connections": [
    {
      "from": "node4.dataOut.distance",
      "to": "node5.dataIn.left"
    }
  ]
}
```

**Exemple** :
```
GetBBValue("TargetDistance") ──float──→ Branch.left
                                       (compare distance <= 60)
```

---

### 3.3. Règles de compatibilité

| Règle | Description |
|-------|-------------|
| **R01** | Tout graphe doit avoir **exactement 1 EntryPoint** (`rootNodeId` défini) |
| **R02** | Pas de cycles sauf via nodes boucle explicites (`While`, `ForEach`) |
| **R03** | Exec → Exec uniquement (types compatibles) |
| **R04** | Data pins type-safe (sauf cast `Int`→`Float`) |
| **R05** | Tous nodes atteignables depuis EntryPoint (sinon warning "Orphan nodes") |
| **R06** | SubGraph référencé doit exister (validation au chargement) |
| **R07** | GetBBValue/SetBBValue : clé doit exister dans section `blackboard` |

---

## 4. Système Blackboard

### 4.1. Blackboard Local (par graphe/entité)

**Basé sur** : `BlackboardSystem.h` existant (`namespace OlympeNodeGraph`).

Chaque entité possède sa propre instance Blackboard, isolée des autres.

**Types supportés** :

| Type BB | Type C++ | Notes |
|---------|----------|-------|
| `Int` | `int32_t` | Entier signé 32 bits |
| `Float` | `float` | Flottant 32 bits |
| `Bool` | `bool` | Booléen |
| `String` | `std::string` | Chaîne UTF-8 |
| `Vector` | `Vector` | Utilise `Source/vector.h` (x, y, z) |
| `EntityRef` | `EntityID` | **Extension nécessaire** : Ajouter `EntityRef` au enum `BlackboardType` |

**Note importante** : Il n'existe **PAS** de type `Vector2` séparé. Utiliser `Vector(x, y, 0)` pour valeurs 2D.

---

### 4.2. Déclaration dans le JSON du graphe

**Les variables Blackboard sont déclarées dans l'objet `blackboard` du graphe JSON** (pas dans un node séparé).

**Format** :
```json
{
  "blackboard": {
    "TargetActor": {
      "type": "EntityRef",
      "default": null
    },
    "TargetDistance": {
      "type": "Float",
      "default": 9999.0
    },
    "AlertLevel": {
      "type": "Int",
      "default": 0
    },
    "PatrolIndex": {
      "type": "Int",
      "default": 0
    },
    "LastKnownPos": {
      "type": "Vector",
      "default": {"x": 0.0, "y": 0.0, "z": 0.0}
    },
    "CurrentState": {
      "type": "String",
      "default": "Idle"
    }
  }
}
```

**Bénéfices** :
- Déclaration centralisée et lisible
- Validation possible au chargement
- Tooling autocomplétio plus simple
- Cohérence avec `BlackboardSystem.h` existant

---

### 4.3. Blackboard Global (`globalblackboard.json`)

**Fichier séparé**, éditable depuis l'éditeur ATS, chargé une fois par niveau/session et **partagé entre tous les graphes**.

**Schéma JSON** :
```json
{
  "schemaVersion": 1,
  "description": "Global Blackboard - variables partagées entre tous les graphes du niveau",
  "variables": {
    "GamePhase": {
      "type": "String",
      "default": "Exploration"
    },
    "GlobalAlert": {
      "type": "Int",
      "default": 0
    },
    "PlayerPos": {
      "type": "Vector",
      "default": {"x": 0.0, "y": 0.0, "z": 0.0}
    },
    "MissionFailed": {
      "type": "Bool",
      "default": false
    }
  }
}
```

---

### 4.4. BlackboardKeyRef (résolution des clés)

**Format** :
```json
{
  "scope": "local",
  "path": "TargetDistance"
}

{
  "scope": "global",
  "path": "PlayerPos"
}
```

**Notation raccourcie** (forme string) :
- `"local:TargetDistance"`
- `"global:PlayerPos"`

**Les deux formes sont équivalentes.** L'éditeur ATS normalise vers la forme objet en interne.

---

### 4.5. Mapping Blackboard ↔ Composants ECS

**Le Blackboard local est mappé sur les composants ECS** (voir doc technique section 6) :

| Clé BB | Composant ECS | Exemple |
|--------|---------------|---------|
| `local:CurrentState` | `AIStatedata::currentState` | "Patrol" |
| `local:Health` | `Healthdata::currentHealth` | 80.0 |
| `local:TargetActor` | `AIBlackboarddata::targetActor` | EntityID(42) |
| `local:MoveGoal` | `MoveIntentdata::target{X,Y}` | {500, 300} |
| `global:GamePhase` | `GlobalBlackboard["GamePhase"]` | "Combat" |

**Code C++** :
```cpp
std::variant<...> ReadBlackboard(EntityID entity, const std::string& key) {
    auto [scope, path] = ParseKey(key);  // "local:Health"

    if (scope == "local" && path == "Health") {
        auto* health = World::Get().GetComponent<Healthdata>(entity);
        return health ? health->currentHealth : 0.0f;
    }
    // ... autres mappings ...
}
```

---

## 5. Catalogue d'Actions

**Les actions disponibles dans l'éditeur ATS sont décrites dans `Blueprints/Catalogues/ActionTypes.json`**, chargé par `EnumCatalogManager`.

### 5.1. Principe de l'ActionRegistry

**Deux types d'implémentation** :

| `impl` | Mécanisme | Usage typique |
|--------|-----------|---------------|
| `cpp` | Fonction C++ appelée directement dans `ExecuteAction()` | Mouvement, IA, logique |
| `event` | Génère un `Message` posté via `EventQueue::Get().Push(msg)` | Caméra, audio, niveau |

**Schéma d'une entrée** :
```json
{
  "id": "gotoposition",
  "name": "Goto Position",
  "description": "Déplace l'entité vers une position absolue",
  "category": "Movement",
  "impl": "cpp",
  "parameters": [
    {
      "name": "position",
      "type": "Vector",
      "required": true,
      "description": "Position cible (utilise Vector(x,y,0))"
    },
    {
      "name": "speed",
      "type": "Float",
      "default": 200.0,
      "description": "Vitesse de déplacement"
    },
    {
      "name": "tolerance",
      "type": "Float",
      "default": 10.0,
      "description": "Rayon d'arrivée"
    }
  ],
  "returns": "Bool",
  "tooltip": "Déplace l'entité vers une position fixe Vector"
}
```

---

### 5.2. Catalogue complet des actions (niveau intermédiaire)

#### Catégorie : Movement

| id | Paramètres | impl | Description |
|----|-----------|------|-------------|
| `gotoposition` | position:Vector, speed:Float, tolerance:Float | `cpp` | Aller à position fixe |
| `movetogoal` | speed:Float, tolerance:Float | `cpp` | Aller vers MoveGoal BB |
| `setmovegoaltotarget` | - | `cpp` | Copier position target → MoveGoal |
| `setmovegoaltopos` | position:Vector | `cpp` | Écrire position → MoveGoal |
| `patrolpicknextpoint` | patrolRoute:BBRef | `cpp` | Prochain waypoint route |
| `getnextwaypoint` | graphRef:String | `cpp` | Waypoint depuis sous-graphe |
| `chooserandomnavigablepoint` | searchRadius:Float, maxAttempts:Int | `cpp` | Point navigable aléatoire |
| `followpath` | - | `cpp` | Suivre chemin calculé |

---

#### Catégorie : AIState

| id | Paramètres | impl | Description |
|----|-----------|------|-------------|
| `changestate` | newState:String | `cpp` | Mettre à jour AIState.currentState |
| `cleartarget` | - | `cpp` | Effacer TargetActor (null) |
| `setbbvalue` | key:BBRef, value:Variant | `cpp` | Écrire BB local/global |
| `getbbvalue` | key:BBRef, output pin | `cpp` | Lire BB → pin sortie |
| `waitrandomtime` | min:Float, max:Float | `cpp` | Timer aléatoire |
| `attackifclose` | range:Float, damage:Float | `cpp` | Attaque mêlée si distance ≤ range |

---

#### Catégorie : Animation/Visuel

| id | Paramètres | impl | Description |
|----|-----------|------|-------------|
| `playanimation` | animId:String | `cpp` | Change Animation.animationID |
| `stopanimation` | - | `cpp` | Reset animation (frame 0) |
| `setsprite` | spriteRef:String | `event` | Changer sprite path |
| `playvfx` | vfxId:String, position:Vector | `event` | Déclencher VFX |

---

#### Catégorie : Audio

| id | Paramètres | impl | Description |
|----|-----------|------|-------------|
| `playsound` | soundId:String, volume:Float | `event` | AudioManager::PlaySound() |
| `stopsound` | soundId:String | `event` | AudioManager::StopSound() |

---

#### Catégorie : Caméra

| id | Paramètres | impl | Mappe vers |
|----|-----------|------|-----------|
| `camerashake` | intensity:Float, duration:Float | `event` | `CameraEventHandler::TriggerCameraShake` |
| `camerazoomto` | targetZoom:Float, speed:Float | `event` | `CameraEventHandler::ZoomCameraTo` |
| `camerareset` | - | `event` | `CameraEventHandler::ResetCamera` |
| `movecameratoentity` | targetEntity:EntityRef | `event` | Camera suit entité |

---

#### Catégorie : Level/Système

| id | Paramètres | impl | Description |
|----|-----------|------|-------------|
| `loadlevel` | levelName:String | `event` | Charger niveau |
| `unloadlevel` | - | `event` | Nettoie World entities |
| `destroyentity` | entityId:EntityRef | `cpp` | World::DestroyEntity() |
| `spawnentity` | prefabPath:String, position:Vector | `cpp` | Instantiate prefab |
| `addplayer` | playerIndex:Int | `cpp` | Crée entité joueur + bindings |
| `logmessage` | message:String, level:String | `cpp` | Console log (SYSTEM_LOG) |
| `setglobalstate` | key:String, value:Variant | `cpp` | GlobalBlackboard::SetValue() |

---

#### Catégorie : Dialogue/UI

| id | Paramètres | impl | Description |
|----|-----------|------|-------------|
| `displaydialogueline` | line:String ou BB index | `event` | UIManager::ShowDialogue() |
| `hidedialogue` | - | `event` | Ferme UI dialogue |
| `shownotification` | text:String, duration:Float | `event` | UI notification popup |
| `updateobjective` | objectiveId:String, status:String | `event` | UI objective tracker |

---

#### Catégorie : Combat/Gameplay

| id | Paramètres | impl | Description |
|----|-----------|------|-------------|
| `dealdamage` | targetEntity:EntityRef, amount:Float | `cpp` | Réduit Health.currentHealth |
| `healentity` | targetEntity:EntityRef, amount:Float | `cpp` | Augmente Health (clampé max) |
| `giveitem` | itemId:String | `cpp` | Ajoute Inventory.items |
| `removeitem` | itemId:String | `cpp` | Retire Inventory.items |
| `activatetrigger` | triggerId:String | `cpp` | TriggerZone.triggered = true |
| `spawnwave` | waveType:String, count:Int, spawnPoint:Vector | `cpp` | Boucle SpawnEntity N fois |

---

### 5.3. Macro d'enregistrement C++

**Pour enregistrer une action dans `ExecuteAction()`** :

```cpp
// Dans ExecuteAction()
switch (action.actionType) {
    case VSActionType::GotoPosition: {
        if (!action.parameters.count("position")) return;
        Vector pos = std::get<Vector>(action.parameters["position"]);
        // ... implémentation ...
        break;
    }
}
```

**Pour actions non encore dans l'enum** (lookup dynamique string→function) :

```cpp
// Lookup dynamique pour types non compilés
static std::unordered_map<std::string, ActionFn> s_actionRegistry = {
    {"emitnoise", ActionImpl_EmitNoise},
    {"playvfx", ActionImpl_PlayVFX},
};

auto it = s_actionRegistry.find(action.actionTypeString);
if (it != s_actionRegistry.end()) {
    return it->second(entity, action, bb);
}
```

---

## 6. Catalogue de Conditions

**Les conditions génériques sont décrites dans `Blueprints/Catalogues/ConditionTypes.json`.**

**Format d'une entrée catalogue** :
```json
{
  "id": "CompareValue",
  "name": "Compare BB Value",
  "description": "Compare une valeur du Blackboard à un littéral",
  "category": "Logic",
  "parameters": [
    {"name": "key", "type": "BBRef", "required": true},
    {"name": "operator", "type": "String", "required": true, "enum": ["==", "!=", "<", "<=", ">", ">="]},
    {"name": "value", "type": "Variant", "required": true}
  ]
}
```

### 6.1. Liste complète des conditions

| id | Catégorie | Paramètres | Description |
|----|-----------|-----------|-------------|
| `CompareValue` | Logic | key:BBRef, operator:String, value:Variant | Compare valeur BB à littéral |
| `IsSet` | Logic | key:BBRef | Vérifie clé BB définie (non-null) |
| `IsNotSet` | Logic | key:BBRef | Vérifie clé BB null/absente |
| `InRange` | Logic | key:BBRef, min:Float, max:Float | Vérifie float/int dans [min, max] |
| `RandomChance` | Logic | probability:Float (0..1) | Retourne SUCCESS avec probabilité P |
| `HeardNoise` | Perception | - (implmentation C++ via AISenses) | Détection sonore |
| `TargetVisible` | Perception | raycast + senseurs | Vision via line-of-sight |
| `HasLineOfSight` | Perception | raycast | Vérifie ligne de vue |
| `HealthBelow` | Combat | threshold:Float | Santé < seuil |
| `HasTarget` | Combat | - | Alias `TargetVisible` |
| `IsTargetInRange` | Combat | range:Float | Vérifie distance cible ≤ range |
| `CanAttack` | Combat | cooldown:Float | Vérifie cooldown attaque écoulé |

---

### 6.2. Oprateurs de condition par type

| Type BB | Opérateurs valides |
|---------|-------------------|
| `Bool` | `==`, `!=` |
| `String` / `Enum` | `==`, `!=` |
| `Int` / `Float` | `==`, `!=`, `<`, `<=`, `>`, `>=`, `InRange(min,max)` |
| `EntityRef` | `IsSet`, `IsNotSet` |
| `Vector` | Indirect (stocker distance calculée dans Float, tester Float) |

---

## 7. Schéma JSON Unifié v3

**Schéma complet d'un graphe ATS Visual Script** :

```json
{
  "schemaVersion": 3,
  "type": "VisualScript",
  "name": "GuardCombatFlowchart",
  "description": "Comportement garde : patrol/combat/investigate",
  "metadata": {
    "author": "Atlasbruce",
    "created": "2026-03-08T10:00:00Z",
    "tags": ["AI", "Combat", "Guard"]
  },
  "settings": {
    "loopMode": "TickLoop",
    "tickInterval": 0.016,
    "runOnce": false,
    "startEnabled": true
  },
  "blackboard": {
    "CurrentState": {"type": "String", "default": "Patrol"},
    "TargetActor": {"type": "EntityRef", "default": null},
    "TargetDistance": {"type": "Float", "default": 9999.0},
    "PatrolRoute": {"type": "String", "default": "RouteA"},
    "AlertLevel": {"type": "Int", "default": 0}
  },
  "blackboardRefs": [
    {"key": "local:CurrentState", "type": "String"},
    {"key": "local:TargetActor", "type": "EntityRef"},
    {"key": "global:GamePhase", "type": "String"}
  ],
  "editorState": {
    "zoom": 1.0,
    "scrollOffset": {"x": 0, "y": 0}
  },
  "nodes": [
    {
      "id": 1,
      "type": "EntryPoint",
      "name": "Event Tick",
      "position": {"x": 100, "y": 300},
      "execOut": [2]
    },
    {
      "id": 2,
      "type": "Branch",
      "name": "Has Target?",
      "position": {"x": 300, "y": 300},
      "conditionType": "IsSet",
      "parameters": [
        {"name": "key", "value": "local:TargetActor"}
      ],
      "execOut": {
        "true": [3],
        "false": [10]
      }
    },
    {
      "id": 3,
      "type": "GetBBValue",
      "name": "Get Distance",
      "position": {"x": 500, "y": 200},
      "key": "local:TargetDistance",
      "dataOut": [{"name": "distance", "type": "Float"}],
      "execOut": [4]
    },
    {
      "id": 4,
      "type": "Branch",
      "name": "Close Enough?",
      "position": {"x": 700, "y": 200},
      "conditionType": "CompareValue",
      "dataIn": [
        {"name": "left", "sourceNode": 3, "sourcePin": "distance"}
      ],
      "parameters": [
        {"name": "operator", "value": "<="},
        {"name": "right", "value": 60.0}
      ],
      "execOut": {
        "true": [5],
        "false": [8]
      }
    },
    {
      "id": 5,
      "type": "While",
      "name": "While Not Close",
      "position": {"x": 900, "y": 150},
      "conditionType": "CompareValue",
      "parameters": [
        {"name": "left", "value": "local:TargetDistance"},
        {"name": "operator", "value": ">"},
        {"name": "right", "value": 10.0}
      ],
      "execOut": {
        "loop": [6],
        "exit": [7]
      }
    },
    {
      "id": 6,
      "type": "Action",
      "name": "Move To Goal",
      "actionType": "MoveToGoal",
      "parameters": [
        {"name": "speed", "value": 1.0}
      ],
      "execOut": [5]
    },
    {
      "id": 7,
      "type": "Action",
      "name": "Attack!",
      "actionType": "AttackIfClose",
      "parameters": [
        {"name": "range", "value": 40.0},
        {"name": "damage", "value": 15}
      ]
    },
    {
      "id": 8,
      "type": "Action",
      "name": "Change State Combat",
      "actionType": "ChangeState",
      "parameters": [
        {"name": "newState", "value": "Combat"}
      ],
      "execOut": [9]
    },
    {
      "id": 9,
      "type": "Action",
      "name": "Set Move Goal To Target",
      "actionType": "SetMoveGoalToTarget",
      "execOut": []
    },
    {
      "id": 10,
      "type": "Switch",
      "name": "Switch on State",
      "position": {"x": 500, "y": 500},
      "switchType": "Enum",
      "enumType": "AIState",
      "key": "local:CurrentState",
      "execOut": {
        "Patrol": [11],
        "Investigate": [15],
        "default": [20]
      }
    },
    {
      "id": 11,
      "type": "Action",
      "name": "Get Next Waypoint",
      "actionType": "GetNextWaypoint",
      "parameters": [
        {"name": "route", "value": "local:PatrolRoute"}
      ],
      "execOut": [12]
    },
    {
      "id": 12,
      "type": "Action",
      "name": "Move To Goal",
      "actionType": "MoveToGoal",
      "parameters": [
        {"name": "speed", "value": 0.6}
      ]
    }
  ],
  "connections": [
    {"from": "node3.dataOut.distance", "to": "node4.dataIn.left"}
  ],
  "entryPoint": 1
}
```

**Description des sections** :

| Clé | Type | Description |
|-----|------|-------------|
| `schemaVersion` | Int | Version du schéma JSON (actuel : **3**) |
| `type` | String | Type d'asset : `"VisualScript"` |
| `name` | String | Nom humain du graphe |
| `description` | String | Description textuelle |
| `metadata` | Object | Auteur, date, tags (informations éditoriales) |
| `settings` | Object | Config runtime (`loopMode`, `tickInterval`, `runOnce`) |
| `blackboard` | Object | Variables BB locales déclarées |
| `blackboardRefs` | Array | Liste clés BB référencées (validation) |
| `editorState` | Object | État caméra/zoom éditeur (non exécuté) |
| `nodes` | Array | Liste des nodes du graphe |
| `connections` | Array | Liens Data explicites |
| `entryPoint` | Int | ID du node EntryPoint |

---

## 8. Pipeline d'exécution ATS → ECS

**Le flux complet d'exécution d'un graphe ATS Visual Script dans le moteur** :

```
1. CHARGEMENT
   DataManager::LoadVisualScript()
   ├─ Parse JSON (nlohmann::json)
   └─ GraphDocument → VSGraphAsset

2. ACTIVATION (par entité)
   VisualScriptComponent ajouté sur entité ECS
   ├─ scriptAssetPath: "Blueprints/VisualScript/guard_ai.json"
   └─ enabled: true

3. UPDATE (chaque frame)
   VisualScriptSystem::Update(deltaTime)
   ├─ UpdateDelayTimers(deltaTime)
   └─ ExecuteEntityScripts(deltaTime)
       ├─ Pour chaque entité avec VisualScriptComponent
       ├─ Récupère VSRuntimeInstance
       └─ ExecuteGraph(entity, runtime)
           ├─ Démarre depuis EntryPoint
           ├─ Parcourt flux Exec récursivement
           ├─ Branch → True/False
           ├─ While → Loop/Exit
           ├─ Actions → ExecuteAction()
           └─ Fin → Reboucle frame suivant

4. LECTURE/ÉCRITURE BLACKBOARD
   BlackboardBridge::ReadBlackboard()
   ├─ Parse "local:Health" → {"local", "Health"}
   ├─ Map vers composant ECS : Healthdata::currentHealth
   └─ Return variant<...> value

   BlackboardBridge::WriteBlackboard()
   ├─ Parse "local:MoveGoal"
   ├─ Map vers MoveIntentdata component
   └─ Write value

5. ACTIONS (impl:cpp)
   ExecuteAction()
   ├─ Switch sur actionType
   └─ Appel direct logique C++

6. ACTIONS (impl:event)
   ExecuteAction()
   ├─ Créé Message
   └─ EventQueue::Get().Push(msg)
       └─ Consommé frame suivant par :
           ├─ CameraEventHandler (shake, zoom, teleport)
           ├─ AudioSystem (sons, musique)
           └─ LevelSystem (chargement niveau)

7. DEBUG (éditeur)
   EditorContext::Debug
   ├─ NodeAnnotationsManager::SetBreakpoint()
   └─ Highlight node actif dans éditeur ATS
```

---

## 9. Sous-graphes et SubTasks

**Les sous-graphes sont référencés comme des assets JSON externes** (pas inline dans le graphe parent).

### 9.1. Node SubGraph

**Type** : `SubGraph`  
**Description** : Référence un sous-graphe externe, favorise réutilisabilité.

**Paramètres** :
- `graphRef` : Chemin vers fichier JSON du sous-graphe

**JSON** :
```json
{
  "id": 99,
  "name": "Patrol Routine",
  "type": "SubGraph",
  "position": {"x": 600.0, "y": 300.0},
  "parameters": [
    {"name": "graphRef", "value": "Blueprints/AI/guard_patrol.json"}
  ],
  "execOut": [100]
}
```

---

### 9.2. Règles d'utilisation

1. Le fichier `graphRef` doit être un **GraphDocument valide** avec `graphKind` compatible.
2. Les variables du sous-graphe ont leur propre **BB local isolé** du graphe parent.
3. Pour passer valeurs entre graphes, utiliser **paramètres d'entrée explicites** (binding).
4. Validation vérifie **existence du fichier** au chargement (règle R06).
5. Profondeur de récursion **limitée par configuration** (éviter cycles infinis).

---

### 9.3. Passage de paramètres (Phase 3)

**Exemple** :
```json
{
  "id": 99,
  "type": "SubGraph",
  "parameters": [
    {"name": "graphRef", "value": "Blueprints/AI/guard_patrol.json"}
  ],
  "inputs": [
    {
      "name": "StartIndex",
      "value": {"scope": "local", "path": "PatrolIndex"}
    },
    {
      "name": "PatrolSpeed",
      "value": 150.0
    }
  ],
  "execOut": [100]
}
```

---

## 10. Ancrage dans le code existant

**Table de correspondance entre spécification ATS Visual Scripting et code existant Olympe Engine** :

| Concept ATS | Fichier existant | Classe / Enum |
|-------------|------------------|---------------|
| **Node Types** | `Source/VisualScript/VSGraphAsset.h` | `VSNodeType`, `VSConditionType`, `VSActionType` |
| **Runtime System** | `Source/Systems/VisualScriptSystem.h` | `VisualScriptSystem`, `VSRuntimeInstance` |
| **Catalogue JSON Actions** | `Blueprints/Catalogues/ActionTypes.json` | Chargé par `EnumCatalogManager` |
| **Catalogue JSON Conditions** | `Blueprints/Catalogues/ConditionTypes.json` | Chargé par `EnumCatalogManager` |
| **EnumCatalogManager** | `Source/BlueprintEditor/EnumCatalogManager.h/cpp` | `EnumCatalogManager::Get()` |
| **GraphDocument** | `Source/NodeGraphCore/GraphDocument.h/cpp` | `GraphDocument`, `NodeData`, `LinkData` |
| **BlackboardSystem (local)** | `Source/NodeGraphCore/BlackboardSystem.h` | `BlackboardSystem`, `BlackboardType`, `BlackboardValue` |
| **GlobalBlackboard** | `Source/VisualScript/GlobalBlackboard.h` | `GlobalBlackboard::Get()` |
| **Vector** | `Source/vector.h` | `Vector(x, y, z)` (pas de Vector2 séparé) |
| **EventQueue** | `Source/system/EventQueue.h` | `EventQueue::Get().Push(msg)` |
| **EventTypes** | `Source/system/message.h` | `EventType`, `EventDomain`, `Message` |
| **CameraEventHandler** | `Source/system/CameraEventHandler.h` | `TriggerCameraShake`, `ZoomCameraTo` |
| **ECS Components** | `Source/ECSComponents.h` | `AIStatedata`, `Healthdata`, `VisualScriptComponent` |
| **World** | `Source/World.h` | `World::Get().CreateEntity()`, `AddComponent()` |
| **DataManager** | `Source/DataManager.h` | `DataManager::Get().LoadVisualScript()` |

---

### 10.1. Extension nécessaire : `BlackboardType`

**Ajouter `EntityRef` au enum `BlackboardType` dans `BlackboardSystem.h`** :

```cpp
// BlackboardSystem.h (avant modification)
enum class BlackboardType {
    Int,
    Float,
    Bool,
    String,
    Vector3  // ← À remplacer par Vector
};

// BlackboardSystem.h (après modification)
enum class BlackboardType {
    Int,
    Float,
    Bool,
    String,
    Vector,    // ← Cohérence avec Source/vector.h
    EntityRef  // ← NOUVEAU (EntityID uint32_t)
};
```

---

## 11. Questions ouvertes / Décisions futures

**Les sujets suivants sont identifiés pour les phases ultérieures** :

| Sujet | Description | Priorité |
|-------|-------------|----------|
| **Binding SubGraph params** | Format binding paramètres entrée/sortie SubGraphs | **P3** |
| **Custom Enum types** | Types personnalisés Enum déclarables dans BB | **P3** |
| **Hot-reload catalogues** | Rechargement catalogues JSON en cours édition | **P3** |
| **Static type checker** | Validation statique complète (type checker compilation graphe) | **P3** |
| **Node tags/labels** | Système tags sur nodes (filtrage/recherche éditeur) | **P3** |
| **SubGraph output pins** | Retour valeurs depuis sous-graphe vers parent | **P3** |
| **BB variable watchers** | Abonnement changements BB (`onChange` callbacks) | **P4** |
| **Data pin arrays** | Support arrays dans data pins (ex: `Array<Vector>`) | **P4** |
| **ForEach multi-types** | ForEach générique (itérer `Array<T>` avec `T` custom) | **P4** |

---

**Fin du document - Version 2.0 (Visual Scripting) - 2026-03-08**

Pour questions : voir repo Olympe Engine GitHub.
