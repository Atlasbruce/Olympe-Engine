# Documentation Olympe ATS Visual Scripting (Flowchart)

**Version 1.0 - 08/03/2026**
**Nicolas Chereau**
---

## Table des matières

1. [Introduction et Paradigme](#1-introduction-et-paradigme)
2. [Grammaire du Graphe](#2-grammaire-du-graphe)
3. [Référentiel Complet des Nodes](#3-référentiel-complet-des-nodes)
4. [Système de Pins (Exec et Data)](#4-système-de-pins-exec-et-data)
5. [Blackboard et Variables](#5-blackboard-et-variables)
6. [Règles de Validation](#6-règles-de-validation)
7. [Exemples Pratiques Détaillés](#7-exemples-pratiques-détaillés)
8. [Schéma JSON Complet](#8-schéma-json-complet)
9. [Best Practices](#9-best-practices)

---

## 1. Introduction et Paradigme

### 1.1. Qu'est-ce que ATS Visual Scripting ?

**ATS (Atomic Task System) Visual Scripting** est un système de **programmation visuelle algorithmique** inspiré des **Blueprint Event Graphs** d'Unreal Engine. Il permet de créer des comportements IA, scripts de niveau, dialogues et systèmes de jeu via des **flowcharts (organigrammes)** visuels.

**Paradigme** : Algorigramme avec **flux d'exécution (Exec)** et **flux de données (Data)** explicites et visuels.

```
Event Tick (chaque frame si entité active)
    ↓
EntryPoint ─→ [Branch Target Visible?] ─→ True ─→ [Attack]
                        │
                        └─→ False ─→ [Patrol]
    ↓
Reboucle au prochain frame
```

### 1.2. Modèle d'Exécution

**Event Tick** : Chaque entité avec un graphe ATS actif exécute son `EntryPoint` à chaque frame.

```
Frame N :
  1. Event Tick déclenché
  2. Parcourt graphe depuis EntryPoint
  3. Suit connexions Exec (pins True/False/Then/Loop)
  4. Termine quand plus de connexions
  5. Reboucle au prochain frame

Interruption :
  - DoOnce : bloque flux après première exécution
  - RunOnce (settings) : exécute 1 seule fois puis désactive
  - Entité détruite : stop définitif
```

### 1.3. Différence avec Behavior Tree

| Aspect | **Behavior Tree** | **ATS Visual Scripting** |
|--------|-------------------|--------------------------|
| Structure | Hiérarchie stricte (parent→enfants) | Graphe libre avec connexions Exec |
| Conditionnelles | Implicit (Selector/Sequence) | Explicites (Branch visuel True/False) |
| Boucles | Pas de support direct | While/For natifs |
| Flux | Top-down ordonné | Algorithmique visuel |
| Usage | IA tactique pure | IA + Level scripting + systèmes |

---

## 2. Grammaire du Graphe

### 2.1. Règles de Construction Fondamentales

#### Règle 1 : EntryPoint Unique
- **Obligatoire** : 1 seul node `EntryPoint` par graphe.
- **Rôle** : Point de départ à chaque Event Tick.
- **Connexion** : Toujours 1 sortie Exec vers premier node logique.

#### Règle 2 : Connexions Exec
- **Type** : Exec → Exec (pin blanche/bleue).
- **Direction** : Toujours unidirectionnelle (pas de merge automatique).
- **Branches** : Branch/Switch créent plusieurs sorties Exec (True/False, Case1/Case2...).
- **Fin de flux** : Node sans sortie Exec = fin de chemin (reboucle frame suivant).

#### Règle 3 : Connexions Data
- **Type** : Data pin → Data pin (type compatible).
- **Types supportés** : `bool`, `int`, `float`, `string`, `vector2`, `vector3`, `entity_handle`.
- **Validation** : Type-safe (int ≠ string, sauf conversion explicite).

#### Règle 4 : Cycles et Boucles
- **Cycles interdits** sauf via nodes de boucle explicites (`While`, `ForEach`).
- **While** : pin Loop reboucle vers node interne, pin Exit sort de boucle.
- **Validation éditeur** : détecte cycles invalides.

#### Règle 5 : Graphe Connexe
- Tous les nodes doivent être **atteignables depuis EntryPoint** (warning sinon).
- Nodes orphelins = ignorés runtime.

---

### 2.2. Flux d'Exécution Détaillé

```
EntryPoint (id:1)
    ↓ execOut
GetBBValue "TargetDistance" (id:2)
    ↓ execOut + dataOut.distance
Branch "Distance <= 60?" (id:3)
    ├─→ True : Attack Sequence (id:4)
    │       ↓ then0
    │   SetMoveGoal (id:5)
    │       ↓ execOut
    │   While "dist > 10" (id:6)
    │       ├─→ Loop : MoveToGoal (id:7) ─→ reboucle id:6
    │       └─→ Exit : Attack! (id:8)
    │
    └─→ False : Patrol Sequence (id:10)
            ↓ then0
        GetNextWaypoint (id:11)
            ↓ execOut
        MoveToGoal (id:12)

Fin → Reboucle EntryPoint frame suivant
```

---

## 3. Référentiel Complet des Nodes

### 3.1. Flow Control

#### **EntryPoint**
```
Pin Exec Out : execOut (1)
Usage        : Point d'entrée obligatoire, déclenché par Event Tick
Paramètres   : Aucun
Exemple      : Toujours présent, id souvent = 1
```

**JSON** :
```json
{"id":1, "type":"EntryPoint", "name":"Event Tick", "execOut":[2]}
```

---

#### **Branch** (If/Else visuel)
```
Pin Exec In  : execIn (1)
Pin Exec Out : true (1), false (1)
Paramètres   : conditionType, parameters (left, operator, right)
Usage        : Embranchement conditionnel visuel
```

**Opérateurs supportés** :
- Numériques : `==`, `!=`, `<`, `<=`, `>`, `>=`
- Booléens : `==`, `!=`
- Strings : `==`, `!=`

**Exemple JSON** :
```json
{
  "id": 3,
  "type": "Branch",
  "name": "Target Close?",
  "conditionType": "CompareValue",
  "parameters": [
    {"name":"left", "value":"local:TargetDistance"},
    {"name":"operator", "value":"<="},
    {"name":"right", "value":60.0}
  ],
  "execOut": {
    "true": [4],
    "false": [10]
  }
}
```

**Cas d'usage** :
- Test distance ennemie : attaquer ou patrouiller.
- Vérifier état : `local:CurrentState == "Combat"`.
- HP faible : fuir si `local:Health < 30`.

---

#### **Switch** (Multi-branches)
```
Pin Exec In  : execIn (1)
Pin Exec Out : case1, case2, ..., default
Paramètres   : switchType (Enum/String), key, cases[]
Usage        : Dispatcher selon valeur enum/string
```

**Exemple JSON** :
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

**Cas d'usage** :
- Router selon état IA : Patrol/Combat/Flee/Idle.
- Gestion phase mission : Intro/Objective1/Objective2/Complete.
- Type ennemi : Guard/Boss/Drone → comportements différents.

---

#### **Sequence** (Actions ordonnées)
```
Pin Exec In  : execIn (1)
Pin Exec Out : then0, then1, then2, ...
Usage        : Exécute N actions dans l'ordre (pas de test)
```

**Exemple JSON** :
```json
{
  "id": 4,
  "type": "Sequence",
  "name": "Attack Sequence",
  "execOut": {
    "then0": [5],
    "then1": [6],
    "then2": [7]
  }
}
```

**Cas d'usage** :
- Enchaînement attaque : SetTarget → MoveToGoal → PlayAttackAnim → DealDamage.
- Script cinématique : MoveCamera → PlayDialogue → FadeOut.
- Init entité : SpawnAt → SetHealth → ActivateAI.

---

#### **While** (Boucle conditionnelle)
```
Pin Exec In  : execIn (1)
Pin Exec Out : loop (reboucle corps), exit (sort de boucle)
Paramètres   : conditionType, parameters (même que Branch)
Usage        : Répète corps tant que condition vraie
```

**Exemple JSON** :
```json
{
  "id": 6,
  "type": "While",
  "name": "While Not Close Enough",
  "conditionType": "CompareValue",
  "parameters": [
    {"name":"left", "value":"local:TargetDistance"},
    {"name":"operator", "value":">"},
    {"name":"right", "value":10.0}
  ],
  "execOut": {
    "loop": [7],
    "exit": [8]
  }
}
```

**Loop flow** :
```
While (id:6) → test condition
  ├─→ True : pin Loop → MoveToGoal (id:7)
  │              └─→ execOut reboucle vers While (id:6)
  └─→ False : pin Exit → Attack (id:8)
```

**Cas d'usage** :
- Déplacement : tant que `distance > tolerance`, avancer.
- Collecte : tant que `itemsCollected < 10`, chercher items.
- Attente : tant que `timerRunning`, attendre.

**⚠️ Attention** : While s'exécute **dans le même frame** jusqu'à condition fausse. Pour boucles multi-frames, utiliser flag BB + Branch.

---

#### **ForEach** (Itération sur liste)
```
Pin Exec In   : execIn (1)
Pin Exec Out  : loopBody (chaque élément), completed (fin liste)
Pin Data Out  : currentItem, currentIndex
Paramètres    : arrayKey (BB clé liste)
Usage         : Parcourt tableau Blackboard
```

**Exemple JSON** :
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
  "dataOut": {
    "currentItem": {"type":"vector2"},
    "currentIndex": {"type":"int"}
  }
}
```

**Cas d'usage** :
- Patrouille : for each waypoint in route → MoveTo.
- Spawn wave : for each enemyType in wave → SpawnEnemy.
- Dialogue : for each line in conversation → Display.

---

#### **DoOnce** (Exécution unique)
```
Pin Exec In  : execIn (1)
Pin Exec Out : execOut (1, bloqué après 1ère exécution)
Paramètres   : startClosed (bool), resetPin (optionnel)
Usage        : Ne passe qu'une fois
```

**Exemple JSON** :
```json
{
  "id": 20,
  "type": "DoOnce",
  "name": "First Alert Only",
  "startClosed": false,
  "execOut": [21]
}
```

**Cas d'usage** :
- Alert son première fois seulement.
- Init spawn points (une fois au début niveau).
- Tutorial popup (première visite zone).

**Reset** : via pin `Reset` (connecté depuis autre node) réouvre la porte.

---

#### **Delay** (Timer)
```
Pin Exec In  : execIn (1)
Pin Exec Out : completed (après délai)
Paramètres   : duration (float, secondes)
Usage        : Pause flux X secondes
```

**Exemple JSON** :
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
- Frame N : Delay activé → stocke timestamp.
- Frames N+1...N+X : flux bloqué.
- Frame N+X (2s écoulées) : pin `completed` activée → suite.

**Cas d'usage** :
- Cooldown attaque : Attack → Delay 1.5s → ResetAttack.
- Spawn vagues : SpawnWave1 → Delay 10s → SpawnWave2.
- Animation sync : PlayAnim → Delay (anim duration) → NextAction.

---

### 3.2. Actions IA

#### **GotoPosition**
```
Pin Exec In   : execIn (1)
Pin Exec Out  : execOut (1)
Paramètres    : targetKey (BB), speed, tolerance
Usage         : Déplace entité vers position Blackboard
Effet         : Écrit MoveIntent component
```

**Exemple JSON** :
```json
{
  "id": 30,
  "type": "Action",
  "actionType": "GotoPosition",
  "parameters": [
    {"name":"targetKey", "value":"local:MoveGoal"},
    {"name":"speed", "value":1.0},
    {"name":"tolerance", "value":10.0}
  ],
  "execOut": [31]
}
```

**Cas d'usage** :
- Aller vers waypoint : `MoveGoal = waypoint.position`.
- Fuir : `MoveGoal = PlayerPos + oppositeDir * 200`.
- Cinématique : `MoveGoal = scriptedPosition`.

---

#### **ChangeState**
```
Pin Exec In   : execIn (1)
Pin Exec Out  : execOut (1)
Paramètres    : newState (enum string)
Usage         : Change `local:CurrentState`
Effet         : AIState.currentState = newState
```

**Exemple JSON** :
```json
{
  "id": 35,
  "type": "Action",
  "actionType": "ChangeState",
  "parameters": [
    {"name":"newState", "value":"Combat"}
  ],
  "execOut": [36]
}
```

**Cas d'usage** :
- Transition : Patrol → Combat (détection joueur).
- Flee : Combat → Flee (HP < 20%).
- Reset : Any → Idle (joueur hors zone).

---

#### **SetMoveGoalToTarget**
```
Pin Exec In   : execIn (1)
Pin Exec Out  : execOut (1)
Paramètres    : targetKey (default "local:TargetActor")
Usage         : Écrit position cible dans MoveGoal
Effet         : MoveGoal = TargetActor.position
```

**Cas d'usage** :
- Poursuite : lit `TargetActor` → écrit `MoveGoal`.
- Combo avec While : tant que `distance > 50`, SetGoal + Move.

---

#### **AttackIfClose**
```
Pin Exec In   : execIn (1)
Pin Exec Out  : execOut (1)
Paramètres    : range, damage, cooldown
Usage         : Inflige dégâts si cible dans range
Effet         : TargetActor.Health -= damage (si dist <= range)
```

**Cas d'usage** :
- Attaque mêlée : après MoveToGoal, AttackIfClose(40, 15).

---

#### **GetNextWaypoint**
```
Pin Exec In   : execIn (1)
Pin Exec Out  : execOut (1)
Pin Data Out  : waypointPos (vector2)
Paramètres    : routeKey (BB patrol route)
Usage         : Lit prochain waypoint route patrouille
Effet         : BB.currentWaypointIndex++ (circulaire)
```

**Cas d'usage** :
- Patrouille : boucle GetNextWaypoint → SetMoveGoal → Move.

---

### 3.3. Data Nodes

#### **GetBBValue** (Get Blackboard Value)
```
Pin Exec In   : execIn (1)
Pin Exec Out  : execOut (1)
Pin Data Out  : value (type selon clé)
Paramètres    : key (BB key reference)
Usage         : Lit Blackboard → sortie Data
```

**Exemple JSON** :
```json
{
  "id": 40,
  "type": "GetBBValue",
  "name": "Get Distance",
  "key": "local:TargetDistance",
  "dataOut": [{"name":"distance", "type":"float"}],
  "execOut": [41]
}
```

**Connexion Data** :
```
GetBBValue.distance (Data Out)
    ↓ connexion Data
Branch.conditionIn.left (Data In)
```

---

#### **SetBBValue** (Set Blackboard Value)
```
Pin Exec In   : execIn (1)
Pin Exec Out  : execOut (1)
Pin Data In   : value (type selon clé)
Paramètres    : key (BB key reference)
Usage         : Écrit Data In → Blackboard
```

**Exemple JSON** :
```json
{
  "id": 45,
  "type": "SetBBValue",
  "name": "Set Alert Level",
  "key": "local:AlertLevel",
  "dataIn": [{"name":"value", "type":"int"}],
  "execOut": [46]
}
```

**Cas d'usage** :
- Calcul : GetHP → Math(HP * 0.5) → SetBB("HalfHP").
- Trigger : détection → SetBB("AlertLevel", 3).

---

#### **Math** (Opérations mathématiques)
```
Pin Data In   : A, B (float)
Pin Data Out  : result (float)
Paramètres    : operation (+, -, *, /, %, random)
Usage         : Calculs sans modifier BB
```

**Exemple JSON** :
```json
{
  "id": 50,
  "type": "Math",
  "operation": "*",
  "dataIn": [
    {"name":"A", "type":"float"},
    {"name":"B", "type":"float"}
  ],
  "dataOut": [{"name":"result", "type":"float"}]
}
```

**Flux Data** :
```
GetBBValue("Speed") → Math.A
Literal(1.5) → Math.B
Math.result → SetBBValue("BoostedSpeed")
```

---

### 3.4. Conditions (types pour Branch/While)

#### **CompareValue**
```
Paramètres : left (value/key), operator, right (value/key)
Usage      : Compare deux valeurs
Opérateurs : ==, !=, <, <=, >, >=
```

**Exemples** :
```json
{"conditionType":"CompareValue", "parameters":[
  {"name":"left", "value":"local:Health"},
  {"name":"operator", "value":"<"},
  {"name":"right", "value":30}
]}
```

---

#### **IsSet / IsNotSet**
```
Paramètres : key (BB reference)
Usage      : Teste si clé BB définie (non null)
```

**Exemples** :
```json
{"conditionType":"IsSet", "parameters":[
  {"name":"key", "value":"local:TargetActor"}
]}
```

**Cas d'usage** :
- `IsSet("TargetActor")` → a une cible.
- `IsNotSet("PatrolRoute")` → pas de route assignée.

---

#### **InRange**
```
Paramètres : key, min, max
Usage      : Teste si valeur dans intervalle [min, max]
```

**Exemples** :
```json
{"conditionType":"InRange", "parameters":[
  {"name":"key", "value":"local:AlertLevel"},
  {"name":"min", "value":2},
  {"name":"max", "value":5}
]}
```

---

#### **StateIs**
```
Paramètres : stateKey, targetState
Usage      : Teste état enum
```

**Exemples** :
```json
{"conditionType":"StateIs", "parameters":[
  {"name":"stateKey", "value":"local:CurrentState"},
  {"name":"targetState", "value":"Combat"}
]}
```

---

## 4. Système de Pins (Exec et Data)

### 4.1. Pins Exec (Flux d'Exécution)

**Visuel** : Lignes blanches/bleues épaisses.

**Types de sorties Exec** :
- **Simple** : 1 sortie (`execOut`), ex : Action, Delay.
- **Conditionnelle** : 2 sorties (`true`, `false`), ex : Branch.
- **Multiple** : N sorties (`case1...caseN`, `default`), ex : Switch.
- **Ordonnées** : N sorties (`then0...thenN`), ex : Sequence.
- **Boucle** : 2 sorties (`loop`, `exit`), ex : While.

**Règles** :
- Exec → Exec uniquement (pas Exec → Data).
- 1 pin Exec In par node.
- Multiple sorties Exec possibles.

---

### 4.2. Pins Data (Flux de Données)

**Visuel** : Lignes colorées fines (couleur = type).

**Types Data supportés** :

| Type | Couleur | Exemples |
|------|---------|----------|
| `bool` | Rouge | true, false |
| `int` | Cyan | 42, -10, 0 |
| `float` | Vert | 3.14, 0.5, -2.7 |
| `string` | Magenta | "Combat", "Guard_01" |
| `vector2` | Jaune | {x:100, y:200} |
| `vector3` | Orange | {x:0, y:10, z:5} |
| `entity_handle` | Blanc | ref entité |

**Connexions Data** :
```
GetBBValue "TargetDistance" (Data Out float)
    ↓ connexion Data verte
Branch "Distance <= 60" (Data In left : float)
```

**Validation** :
- Type-safe : `int` vers `float` OK (cast implicite).
- `int` vers `string` KO (conversion explicite requise).

---

### 4.3. Littéraux (Literal Values)

Nodes peuvent avoir **paramètres littéraux** (valeurs fixes) :

```json
{
  "id": 60,
  "type": "Branch",
  "parameters": [
    {"name":"left", "value":"local:Health"},
    {"name":"operator", "value":"<"},
    {"name":"right", "value":30}  // ← littéral
  ]
}
```

Ou **Data In pin** (connexion dynamique) :

```json
{
  "id": 61,
  "type": "Branch",
  "dataIn": [
    {"name":"left", "sourceNode":60, "sourcePin":"distance"}
  ],
  "parameters": [
    {"name":"operator", "value":"<"},
    {"name":"right", "value":30}
  ]
}
```

---

## 5. Blackboard et Variables

### 5.1. Système de Blackboard

**Blackboard** = dictionnaire clé-valeur pour stocker état IA/entité.

**Deux scopes** :
- **Local** : `local:Key` → composants entité (AIBlackboard, AIState, MoveIntent, etc.).
- **Global** : `global:Key` → état monde/mission (partagé toutes entités).

---

### 5.2. Clés Local Blackboard

**Mappées sur composants ECS** :

| Clé BB | Composant ECS | Type | Exemple |
|--------|---------------|------|---------|
| `local:CurrentState` | `AIState.currentState` | string | "Patrol", "Combat" |
| `local:PreviousState` | `AIState.previousState` | string | "Idle" |
| `local:TargetActor` | `AIBlackboard.targetActor` | entity_handle | ref joueur |
| `local:TargetDistance` | Calculé (senses) | float | 125.5 |
| `local:AlertLevel` | `AISenses.alertLevel` | int | 0-5 |
| `local:PatrolRoute` | `AIBlackboard.patrolRoute` | string | "RouteA" |
| `local:MoveGoal` | `MoveIntent.target{X,Y}` | vector2 | {100, 200} |
| `local:Health` | `Health.currentHealth` | float | 80 |

---

### 5.3. Clés Global Blackboard

**Partagées niveau/mission** :

| Clé BB | Type | Exemple | Usage |
|--------|------|---------|-------|
| `global:GamePhase` | string | "Stealth", "Combat", "Escape" | Change comportements |
| `global:PlayerLastKnownPos` | vector2 | {500, 300} | Investiguer |
| `global:AlertLevelGlobal` | int | 3 | Renfort spawn |
| `global:ObjectiveComplete` | bool | true | Transition mission |
| `global:WaveNumber` | int | 2 | Difficulté spawn |

---

### 5.4. Syntaxe Référence BB

**Format** : `scope:path`

```
local:CurrentState        → AIState.currentState
local:Health              → Health.currentHealth
global:GamePhase          → global_blackboard["GamePhase"]
```

**Dans JSON** :
```json
{"name":"key", "value":"local:TargetDistance"}
{"name":"key", "value":"global:AlertLevelGlobal"}
```

---

## 6. Règles de Validation

### 6.1. Validation Structure

**Éditeur doit vérifier** :

| Règle | Description | Erreur |
|-------|-------------|--------|
| **1 EntryPoint** | Exactement 1 node EntryPoint | "Missing EntryPoint" / "Multiple EntryPoints" |
| **Connexité** | Tous nodes atteignables depuis EntryPoint | Warning "Orphan nodes" |
| **Cycles** | Pas de cycles sauf While/ForEach | Error "Invalid cycle detected" |
| **Pins valides** | Exec→Exec, Data type-safe | Error "Invalid connection type" |
| **BB keys** | Clés existent dans référentiel | Warning "Unknown BB key" |

---

### 6.2. Validation Connexions

**Exec pins** :
- ✅ Action.execOut → Branch.execIn
- ✅ Branch.true → Action.execIn
- ❌ Action.execOut → GetBBValue.dataOut (types incompatibles)

**Data pins** :
- ✅ GetBBValue(float) → Branch.left(float)
- ✅ GetBBValue(int) → Branch.left(float) (cast implicite)
- ❌ GetBBValue(string) → Math.A(float) (pas de cast auto)

---

### 6.3. Warnings

**Non-bloquants mais à vérifier** :
- Node orphelin (pas connecté EntryPoint).
- BB key inconnue (typo ?).
- Data pin non connectée (utilise valeur par défaut).
- While sans Exit path (boucle infinie potentielle).

---

## 7. Exemples Pratiques Détaillés

### 7.1. Guard AI Complet (Patrol → Combat → Investigate)

**Flowchart visuel** :
```
EntryPoint
  ↓
GetBBValue("TargetActor") → IsSet?
  ├─→ True : Branch "Distance <= 60?"
  │         ├─→ True : Attack Sequence
  │         │          ├─→ SetMoveGoalToTarget
  │         │          └─→ While(dist>10) → MoveToGoal → loop
  │         │                            └─→ Exit → AttackIfClose
  │         └─→ False : ChangeState("Combat") → SetMoveGoal → Move
  │
  └─→ False : Switch(CurrentState)
              ├─→ "Patrol" : GetNextWaypoint → SetMoveGoal → Move
              ├─→ "Investigate" : MoveToLastKnownPos
              └─→ default : DoOnce → ChangeState("Patrol")
```

**JSON complet** (voir section 8.2).

---

### 7.2. Level Trigger : Porte qui s'ouvre

**Objectif** : Quand joueur entre zone trigger, ouvrir porte + son.

```
EntryPoint
  ↓
Branch "PlayerInZone?"
  ├─→ True : DoOnce
  │           ├─→ PlaySound("DoorOpen")
  │           ├─→ SetBBValue("DoorState", "Opening")
  │           └─→ Delay(1.5s)
  │                 └─→ SetBBValue("DoorState", "Open")
  │
  └─→ False : (rien)
```

**JSON** :
```json
{
  "nodes": [
    {"id":1, "type":"EntryPoint", "execOut":[2]},
    {
      "id":2, "type":"Branch",
      "conditionType":"IsSet",
      "parameters":[{"name":"key", "value":"local:PlayerInTrigger"}],
      "execOut":{"true":[3], "false":[]}
    },
    {"id":3, "type":"DoOnce", "execOut":[4]},
    {
      "id":4, "type":"Action", "actionType":"PlaySound",
      "parameters":[{"name":"soundId", "value":"DoorOpen"}],
      "execOut":[5]
    },
    {
      "id":5, "type":"SetBBValue",
      "key":"local:DoorState",
      "dataIn":[{"name":"value", "literal":"Opening"}],
      "execOut":[6]
    },
    {"id":6, "type":"Delay", "duration":1.5, "execOut":[7]},
    {
      "id":7, "type":"SetBBValue",
      "key":"local:DoorState",
      "dataIn":[{"name":"value", "literal":"Open"}]
    }
  ]
}
```

---

### 7.3. Spawn Wave System

**Objectif** : Spawner vagues ennemis avec délai.

```
EntryPoint
  ↓
ForEach(WaveList)
  ├─→ LoopBody : SpawnWave(currentItem)
  │                └─→ Delay(waveInterval)
  │                      └─→ (retour ForEach)
  │
  └─→ Completed : SetBBValue("AllWavesSpawned", true)
```

**Cas d'usage** : Tower defense, horde mode.

---

### 7.4. Dialogue Simple

```
EntryPoint
  ↓
Branch "DialogueActive?"
  ├─→ True : GetBBValue("CurrentLineIndex")
  │           ↓
  │         Branch "Index < TotalLines"
  │           ├─→ True : DisplayLine(Index)
  │           │           └─→ Delay(3s)
  │           │                 └─→ SetBBValue("Index", Index+1)
  │           │
  │           └─→ False : SetBBValue("DialogueActive", false)
  │
  └─→ False : (attendre trigger)
```

---

## 8. Schéma JSON Complet

### 8.1. Structure Asset

```json
{
  "schemaversion": 3,
  "type": "VisualScript",
  "name": "GuardAI_Flowchart",
  "description": "Comportement garde : patrol/combat/investigate",
  "metadata": {
    "author": "Atlasbruce",
    "created": "2026-03-08T09:00:00Z",
    "tags": ["AI", "Guard", "Combat"]
  },
  "settings": {
    "loopMode": "TickLoop",
    "tickInterval": 0.016,
    "runOnce": false,
    "startEnabled": true
  },
  "blackboardRefs": [
    {"key":"local:CurrentState", "type":"string"},
    {"key":"local:TargetActor", "type":"entity_handle"},
    {"key":"local:TargetDistance", "type":"float"},
    {"key":"global:GamePhase", "type":"string"}
  ],
  "nodes": [...],
  "connections": [...],
  "entryPoint": 1
}
```

---

### 8.2. Exemple Guard AI JSON Complet

```json
{
  "schemaversion": 3,
  "type": "VisualScript",
  "name": "GuardCombatFlowchart",
  "settings": {"loopMode": "TickLoop", "tickInterval": 0.016},
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
      "type": "GetBBValue",
      "name": "Get Target",
      "position": {"x": 300, "y": 300},
      "key": "local:TargetActor",
      "dataOut": [{"name":"target", "type":"entity_handle"}],
      "execOut": [3]
    },
    {
      "id": 3,
      "type": "Branch",
      "name": "Has Target?",
      "position": {"x": 500, "y": 300},
      "conditionType": "IsSet",
      "parameters": [{"name":"key", "value":"local:TargetActor"}],
      "execOut": {
        "true": [4],
        "false": [20]
      }
    },
    {
      "id": 4,
      "type": "GetBBValue",
      "name": "Get Distance",
      "position": {"x": 700, "y": 200},
      "key": "local:TargetDistance",
      "dataOut": [{"name":"distance", "type":"float"}],
      "execOut": [5]
    },
    {
      "id": 5,
      "type": "Branch",
      "name": "Close Enough?",
      "position": {"x": 900, "y": 200},
      "conditionType": "CompareValue",
      "dataIn": [
        {"name":"left", "sourceNode":4, "sourcePin":"distance"}
      ],
      "parameters": [
        {"name":"operator", "value":"<="},
        {"name":"right", "value":60.0}
      ],
      "execOut": {
        "true": [6],
        "false": [15]
      }
    },
    {
      "id": 6,
      "type": "Sequence",
      "name": "Attack Sequence",
      "position": {"x": 1100, "y": 150},
      "execOut": {
        "then0": [7],
        "then1": [8]
      }
    },
    {
      "id": 7,
      "type": "Action",
      "name": "Set Goal To Target",
      "position": {"x": 1300, "y": 100},
      "actionType": "SetMoveGoalToTarget",
      "execOut": [8]
    },
    {
      "id": 8,
      "type": "While",
      "name": "While Not Close",
      "position": {"x": 1300, "y": 200},
      "conditionType": "CompareValue",
      "parameters": [
        {"name":"left", "value":"local:TargetDistance"},
        {"name":"operator", "value":">"},
        {"name":"right", "value":10.0}
      ],
      "execOut": {
        "loop": [9],
        "exit": [10]
      }
    },
    {
      "id": 9,
      "type": "Action",
      "name": "Move To Goal",
      "position": {"x": 1500, "y": 200},
      "actionType": "MoveToGoal",
      "parameters": [
        {"name":"speed", "value":1.0}
      ],
      "execOut": [8]
    },
    {
      "id": 10,
      "type": "Action",
      "name": "Attack!",
      "position": {"x": 1300, "y": 300},
      "actionType": "AttackIfClose",
      "parameters": [
        {"name":"range", "value":40.0},
        {"name":"damage", "value":15}
      ]
    },
    {
      "id": 15,
      "type": "Sequence",
      "name": "Chase Sequence",
      "position": {"x": 1100, "y": 350},
      "execOut": {
        "then0": [16],
        "then1": [17]
      }
    },
    {
      "id": 16,
      "type": "Action",
      "name": "Change State Combat",
      "position": {"x": 1300, "y": 350},
      "actionType": "ChangeState",
      "parameters": [{"name":"newState", "value":"Combat"}],
      "execOut": [17]
    },
    {
      "id": 17,
      "type": "Action",
      "name": "Set Goal To Target",
      "position": {"x": 1300, "y": 400},
      "actionType": "SetMoveGoalToTarget",
      "execOut": [18]
    },
    {
      "id": 18,
      "type": "Action",
      "name": "Move To Goal",
      "position": {"x": 1300, "y": 450},
      "actionType": "MoveToGoal",
      "parameters": [{"name":"speed", "value":1.0}]
    },
    {
      "id": 20,
      "type": "Switch",
      "name": "Switch on State",
      "position": {"x": 700, "y": 500},
      "switchType": "Enum",
      "enumType": "AIState",
      "key": "local:CurrentState",
      "execOut": {
        "Patrol": [21],
        "Investigate": [25],
        "default": [30]
      }
    },
    {
      "id": 21,
      "type": "Action",
      "name": "Get Next Waypoint",
      "position": {"x": 900, "y": 500},
      "actionType": "GetNextWaypoint",
      "parameters": [{"name":"route", "value":"local:PatrolRoute"}],
      "execOut": [22]
    },
    {
      "id": 22,
      "type": "Action",
      "name": "Set Move Goal Waypoint",
      "position": {"x": 1100, "y": 500},
      "actionType": "SetMoveGoalToPatrolPoint",
      "execOut": [23]
    },
    {
      "id": 23,
      "type": "Action",
      "name": "Move To Goal",
      "position": {"x": 1300, "y": 500},
      "actionType": "MoveToGoal",
      "parameters": [{"name":"speed", "value":0.6}]
    },
    {
      "id": 25,
      "type": "Action",
      "name": "Move To Last Known Pos",
      "position": {"x": 900, "y": 600},
      "actionType": "MoveToLastKnownPosition"
    },
    {
      "id": 30,
      "type": "DoOnce",
      "name": "Reset To Patrol",
      "position": {"x": 900, "y": 700},
      "execOut": [31]
    },
    {
      "id": 31,
      "type": "Action",
      "name": "Change State Patrol",
      "position": {"x": 1100, "y": 700},
      "actionType": "ChangeState",
      "parameters": [{"name":"newState", "value":"Patrol"}]
    }
  ],
  "connections": [
    {"from":"node4.dataOut.distance", "to":"node5.dataIn.left"}
  ],
  "entryPoint": 1
}
```

---

## 9. Best Practices

### 9.1. Organisation Graphe

✅ **Bon** :
- EntryPoint en haut gauche.
- Flux principal gauche → droite.
- Branches True en haut, False en bas.
- Grouper nodes liés visuellement.
- Noms explicites : "Target Close?" pas "Branch3".

❌ **Éviter** :
- Croisements connexions (confus).
- Nodes éparpillés sans logique spatiale.
- Noms génériques : "Action1", "Branch2".

---

### 9.2. Performance

✅ **Optimisations** :
- Utiliser DoOnce pour init (pas chaque frame).
- Conditions simples en premier (early exit).
- While : limiter itérations par frame (ou flag BB + Branch multi-frame).
- Delay préférable à While pour attente longue.

❌ **Éviter** :
- While infinis (crash).
- Trop de GetBBValue redondants (cache si possible).
- Graphes énormes (>100 nodes) : découper en sous-graphes.

---

### 9.3. Debuggabilité

✅ **Facilite debug** :
- Noms descriptifs nodes et pins.
- Commentaires visuels (notes éditeur).
- SetBBValue("DebugState", "AttackPhase") pour traces.
- Sequence pour actions critiques ordonnées (logs clairs).

---

### 9.4. Réutilisabilité

✅ **Modularité** :
- Sous-graphes pour comportements communs (Patrol, Flee).
- BB keys standardisées (`local:CurrentState` partout).
- Actions génériques : GotoPosition vs GotoPlayer (spécifique).

---

## 10. Annexes

### 10.1. Raccourcis Clavier Éditeur (proposition)

| Action | Shortcut |
|--------|----------|
| Add Node | Right-click / Space |
| Delete Node | Del |
| Connect Pin | Drag pin → pin |
| Disconnect | Alt + Click pin |
| Comment Box | C |
| Align Nodes | Shift + A |
| Run Graph | F5 |

---

### 10.2. Codes Erreur Validation

| Code | Message | Solution |
|------|---------|----------|
| E001 | Missing EntryPoint | Ajouter node EntryPoint |
| E002 | Invalid cycle | Supprimer connexion formant cycle |
| E003 | Type mismatch | Vérifier types pins Data |
| W001 | Orphan nodes | Connecter à EntryPoint ou supprimer |
| W002 | Unknown BB key | Vérifier orthographe clé |

---

### 10.3. Migration depuis Behavior Tree

**Conversion patterns** :

| BT | ATS Visual Scripting |
|----|----------------------|
| Selector | Switch ou succession de Branch |
| Sequence | Sequence node ou connexions Exec simples |
| Condition feuille | Branch avec True→Action, False→(vide) |
| Decorator | Branch avant Action |
| Service | Pas d'équivalent (ajouter GetBBValue manuellement) |

---

**Fin de la documentation ATS Visual Scripting v1.0**

Pour questions/feedback : voir repo Olympe Engine GitHub.
