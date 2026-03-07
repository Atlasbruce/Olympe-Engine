# Conception ATS — Node Types, Variables et Blackboard

**Date :** 2026-03-07  
**Auteur :** Atlasbruce / GitHub Copilot  
**Version :** 1.0

---

## Table des matières

1. [Vue d'ensemble et positionnement ATS](#1--vue-densemble-et-positionnement-ats)
2. [Cartographie des types de nœuds](#2--cartographie-des-types-de-nœuds)
3. [Types de liaisons (Links / Pins)](#3--types-de-liaisons-links--pins)
4. [Système Blackboard](#4--système-blackboard)
5. [Catalogue d'Actions (ActionRegistry)](#5--catalogue-dactions-actionregistry)
6. [Catalogue de Conditions (ConditionTypes.json)](#6--catalogue-de-conditions-conditiontypesjson)
7. [Catalogue de Décorateurs (DecoratorTypes.json)](#7--catalogue-de-décorateurs-decoratortypesjson)
8. [Schéma JSON Unifié v2 (GraphDocument)](#8--schéma-json-unifié-v2-graphdocument)
9. [Pipeline d'exécution ATS → ECS](#9--pipeline-dexécution-ats--ecs)
10. [Services (P2 — à venir)](#10--services-p2--à-venir)
11. [Sous-graphes (SubGraph nodes)](#11--sous-graphes-subgraph-nodes)
12. [Ancrage dans le code existant](#12--ancrage-dans-le-code-existant)
13. [Questions ouvertes / Décisions futures (P3+)](#13--questions-ouvertes--décisions-futures-p3)

---

## 1 — Vue d'ensemble et positionnement ATS

### Définition

**ATS (Atomic Task System)** est le système de scripting visuel d'Olympe Engine. Il permet aux concepteurs et développeurs de créer des logiques de comportement, d'animation et de gameplay via un éditeur de **node graph**, sans nécessiter d'écrire directement du code C++.

### Positionnement vis-à-vis des Behavior Trees classiques

ATS **n'est pas** un simple Behavior Tree. C'est un **node graph générique** — analogue à Blueprint dans Unreal Engine — dont l'un des modes d'utilisation est la description d'arbres de comportement. Cette distinction est fondamentale :

| Propriété | BT classique | ATS Node Graph |
|---|---|---|
| Paradigme | Arbre hiérarchique fixe | Graphe générique orienté |
| Types de nœuds | Composite / Décorateur / Feuille | Tous types + BB + Flux + Spéciaux |
| Données | Variables d'agent (blackboard) | Blackboard local + global partagé |
| Extensibilité | Limitée aux types BT | Extensible via catalogues JSON |
| Analogie | Behavior Tree UE4 | Blueprint UE5 |

### Infrastructure technique

L'éditeur ATS s'appuie sur **`NodeGraphCore`**, un ensemble de systèmes bas niveau :

| Module | Rôle |
|---|---|
| `GraphDocument` | Modèle de données du graphe (nœuds, liens, métadonnées) |
| `NodeGraphManager` | Chargement, sauvegarde, cycle de vie des graphes |
| `CommandSystem` | Undo / Redo via pattern Command |
| `BlackboardSystem` | Gestion des variables par entité (namespace `Olympe::NodeGraph`) |

### Types de graphes supportés

| `graphKind` | Description |
|---|---|
| `BehaviorTree` | Arbre de comportement pour les entités IA |
| `AnimGraph` | Logique de transition d'animations |
| `Blueprint` | Logique de gameplay générique (analogie Blueprint UE) |
| `EventGraph` | Réponse événementielle (inputs, messages ECS) |

---

## 2 — Cartographie des types de nœuds

### 2.1 Nœuds Composites (flow control)

Les nœuds Composites contrôlent le flux d'exécution entre leurs enfants. Ils ne réalisent pas d'action directe mais décident de l'ordre et des conditions d'exécution.

| Type (`typeName`) | `displayName` | Logique | Enfants | Couleur |
|---|---|---|---|---|
| `BT_Selector` | Selector | OR priorisé — s'arrête au 1er SUCCESS | 1..N | Orange `0xFF4488FF` |
| `BT_Sequence` | Sequence | AND ordonné — s'arrête au 1er FAIL | 1..N | Vert `0xFF88FF44` |
| `BT_Parallel` | Parallel | Exécute tous en parallèle | 2..N | Cyan `0xFFFF8844` |

**Règles de retour de statut :**

- **`BT_Selector`** : Parcourt ses enfants dans l'ordre. Retourne `SUCCESS` dès qu'un enfant retourne `SUCCESS`. Retourne `FAILURE` si tous les enfants retournent `FAILURE`. Retourne `RUNNING` si l'enfant courant est en cours.
- **`BT_Sequence`** : Parcourt ses enfants dans l'ordre. Retourne `FAILURE` dès qu'un enfant retourne `FAILURE`. Retourne `SUCCESS` si tous les enfants retournent `SUCCESS`. Retourne `RUNNING` si l'enfant courant est en cours.
- **`BT_Parallel`** : Lance tous ses enfants simultanément. Le statut global dépend de la politique configurée (ex. : `SucceedOnAll`, `SucceedOnOne`). Retourne `RUNNING` tant qu'au moins un enfant est en cours et que la politique n'est pas satisfaite.

### 2.2 Nœuds Décorateurs (modifieurs)

Les Décorateurs **modifient le résultat** d'un nœud enfant unique. Ils sont attachés **inline** au nœud décoré (pas comme nœud séparé dans le graphe) via la clé `"decorator"`.

| Type | `displayName` | Logique | Nb enfants |
|---|---|---|---|
| `BT_Inverter` | Inverter | Inverse SUCCESS↔FAILURE | exactement 1 |
| `BT_Repeater` | Repeater | Répète N fois | exactement 1 |
| `BT_UntilSuccess` | Until Success | Répète jusqu'au SUCCESS | exactement 1 |
| `BT_UntilFailure` | Until Failure | Répète jusqu'au FAIL | exactement 1 |
| `BT_Cooldown` | Cooldown | Bloque ré-exécution pendant N secondes | exactement 1 |
| `BT_TimeLimit` | Time Limit | Abandonne si durée dépassée | exactement 1 |
| `BT_ForceSuccess` | Force Success | Retourne toujours SUCCESS | exactement 1 |
| `BT_ForceFailure` | Force Failure | Retourne toujours FAILURE | exactement 1 |

**Format JSON inline du décorateur** (attaché au nœud décoré) :

```json
{
  "id": 5,
  "name": "Move To Goal",
  "type": "Action",
  "actionType": "MoveToGoal",
  "decorator": {
    "type": "BT_Cooldown",
    "params": { "seconds": 2.0 }
  }
}
```

### 2.3 Nœuds Condition (feuilles)

Les nœuds Condition évaluent une expression (souvent en lisant le Blackboard) et retournent `SUCCESS` ou `FAILURE`. Ils n'ont pas d'enfants.

Types de conditions paramétrables (lisant le Blackboard) :

| `conditionType` | Paramètres | Description |
|---|---|---|
| `CompareValue` | `key` (BB ref), `operator` (`==`,`!=`,`<`,`<=`,`>`,`>=`), `value` | Compare une valeur BB à un littéral |
| `IsSet` | `key` | Vérifie qu'une clé BB est définie et non-null |
| `IsNotSet` | `key` | Vérifie qu'une clé BB est null/absente |
| `InRange` | `key`, `min`, `max` | Vérifie qu'un float/int est dans [min, max] |
| `RandomChance` | `probability` [0..1] | Retourne SUCCESS avec probabilité P |
| `HeardNoise` | *(implémentation C++ directe)* | Détection sonore via `AISenses` |
| `TargetVisible` | *(raycast + senseurs)* | Vision via line-of-sight |
| `HasLineOfSight` | *(raycast)* | Vérifie la ligne de vue |
| `HealthBelow` | `threshold` | Santé < seuil |
| `HasTarget` | — | Alias `TargetVisible` |
| `CheckBBValue` | `key`, `operator`, `value` | Alias générique de `CompareValue` |

**Exemple JSON condition :**

```json
{
  "id": 3,
  "name": "Target In Range?",
  "type": "Condition",
  "conditionType": "CompareValue",
  "parameters": {
    "key": "local:TargetDistance",
    "operator": "<=",
    "value": 60.0
  }
}
```

### 2.4 Nœuds Action (feuilles)

Les nœuds Action **exécutent une tâche concrète** et retournent `SUCCESS`, `FAILURE` ou `RUNNING`. Ils n'ont pas d'enfants.

Structure générique d'un nœud Action :

```json
{
  "id": 7,
  "name": "Move To Goal",
  "type": "Action",
  "actionType": "MoveToGoal",
  "parameters": {
    "speed": 200.0,
    "tolerance": 10.0
  }
}
```

- `actionType` : correspond à une entrée dans le `BTActionType` enum (C++) et dans `ActionTypes.json`.
- `parameters` : dictionnaire clé/valeur dont les types sont définis par le catalogue d'actions.
- L'exécution est gérée dans `ExecuteBTAction()` via un switch-case sur `BTActionType`, ou via un lookup string→function pour les types non encore compilés.

### 2.5 Nœuds Spéciaux (BB + flux)

| Type ATS | Rôle |
|---|---|
| `BB_Set` | Écrire une valeur dans le Blackboard (local ou global) |
| `BB_Get` | Lire une valeur BB → sortie de pin (pour nœuds Blueprint) |
| `SubGraph` | Référence un sous-graphe externe (asset JSON) |
| `Comment` | Bloc de commentaire visuel (pas d'exécution) |
| `Reroute` | Point de reroutage de lien (comme UE Blueprint) |

> **Note (D1) :** La déclaration des variables Blackboard se fait dans l'objet `blackboard` du JSON du graphe, pas dans un nœud `BB_Declare` visible dans le graphe. Voir Section 4.

---

## 3 — Types de liaisons (Links / Pins)

### 3.1 Types de pins

| `PinType` | Direction | Description | Couleur UI |
|---|---|---|---|
| `Execution` | Out → In | Flux de contrôle (nœud parent → enfant) | Blanc |
| `Bool` | Out/In | Valeur booléenne | Rouge |
| `Int` | Out/In | Entier 32 bits | Cyan |
| `Float` | Out/In | Flottant 32 bits | Vert |
| `String` | Out/In | Chaîne de caractères | Rose |
| `Vector` | Out/In | `Vector(x,y,z)` — utilise `Source/vector.h` | Jaune |
| `EntityRef` | Out/In | Référence à une EntityID ECS | Violet |
| `BBRef` | Out/In | Référence à une clé Blackboard | Gris |

> **Important (P1.1) :** Le type `Vector` utilise **toujours** la classe `Vector` d'Olympe définie dans `Source/vector.h`. Le constructeur surchargé `Vector(float x, float y, float z=0)` permet de l'utiliser comme un Vector2 en ignorant z. **Il n'existe pas de type `Vector2` séparé** dans l'ATS ni dans le Blackboard. Pour des valeurs 2D, utiliser `Vector(x, y, 0)`.

### 3.2 Règles de compatibilité de liens

- Un pin ne peut être connecté qu'à un pin de **même type** (pas de cast implicite, sauf `Int → Float` autorisé explicitement).
- Un pin `Execution Out` peut connecter exactement **1 enfant** (les Composites gèrent l'ordre en interne via `childIds`).
- Un pin de **valeur** (Bool, Float, etc.) peut avoir **plusieurs connexions en entrée** (la dernière écriture gagne).
- Les liens `BBRef` portent un `scope` (`local` ou `global`) et un `path` (nom de la variable).

### 3.3 Règles de graphe valide

Les règles de validation s'inspirent du modèle Blueprint UE :

| Règle | Description |
|---|---|
| **R01** | Tout graphe doit avoir exactement **1 nœud racine** (`rootNodeId` défini). |
| **R02** | Pas de **cycles** sauf dans les nœuds Repeater/UntilSuccess (gérés en interne). |
| **R03** | Tout nœud **Composite** doit avoir au moins 1 enfant. |
| **R04** | Tout nœud **Décorateur** doit avoir exactement 1 enfant. |
| **R05** | Tout nœud **Condition/Action** ne peut pas avoir d'enfants. |
| **R06** | Un lien `Execution` ne peut pas connecter deux nœuds de même catégorie Leaf (Condition→Condition est invalide sans Composite parent). |
| **R07** | Le `SubGraph` référencé doit exister (validation au chargement). |
| **R08** | Tout `BB_Get`/`BB_Set`/`CompareValue` référençant une clé locale doit correspondre à une clé déclarée dans la section `blackboard` du graphe. |

> **Note (D1 — Validation)** : L'éditeur ATS doit avertir si un `BB_Get`/`BB_Set` référence une clé absente de la section `blackboard`. Cette règle (R08) est critique pour éviter les erreurs silencieuses au runtime.

---

## 4 — Système Blackboard

### 4.1 Blackboard Local (par graphe / entité)

Le Blackboard local est basé sur `BlackboardSystem.h` existant (namespace `Olympe::NodeGraph`). Chaque entité possède sa propre instance, isolée des autres entités.

**Types supportés dans `BlackboardType` :**

| Type BB | Type C++ | Notes |
|---|---|---|
| `Int` | `int32_t` | Entier signé 32 bits |
| `Float` | `float` | Flottant 32 bits |
| `Bool` | `bool` | Booléen |
| `String` | `std::string` | Chaîne UTF-8 |
| `Vector` | `Vector(x,y,z)` | Utilise `Source/vector.h` — **remplace `Vector3`** |
| `EntityRef` | `EntityID` (`uint32_t`) | **À ajouter à `BlackboardType`** |

> **Extension nécessaire :** Ajouter `EntityRef` au enum `BlackboardType` dans `BlackboardSystem.h`. Remplacer également toute référence à `Vector3` par `Vector` dans la documentation et le code associé.

**Déclaration dans le JSON du graphe (décision D1) :**

Les variables Blackboard sont déclarées dans l'objet `blackboard` du graphe JSON lui-même — pas dans un nœud `BB_Declare` séparé dans le graphe.

```json
{
  "schemaVersion": 2,
  "graphKind": "BehaviorTree",
  "blackboard": {
    "TargetActor":    { "type": "EntityRef", "default": null },
    "TargetDistance": { "type": "Float",     "default": 9999.0 },
    "AlertLevel":     { "type": "Int",       "default": 0 },
    "PatrolIndex":    { "type": "Int",       "default": 0 },
    "LastKnownPos":   { "type": "Vector",    "default": [0.0, 0.0, 0.0] },
    "CurrentState":   { "type": "String",    "default": "Idle" }
  }
}
```

**Bénéfices de cette approche (D1) :**

- ✅ Déclaration centralisée et lisible en tête de fichier
- ✅ Validation possible à la lecture du fichier avant exécution
- ✅ Tooling d'autocomplétion plus simple (liste connue dès le chargement)
- ✅ Cohérence avec le modèle existant `BlackboardSystem.h`

**Risques à surveiller (D1) :**

- ⚠️ Si on oublie de déclarer une variable utilisée dans un nœud, erreur silencieuse au runtime
- ⚠️ Une variable peut être déclarée mais jamais utilisée (dead variable)
- ⚠️ Moins visible visuellement dans le graphe qu'un nœud explicite

### 4.2 Blackboard Global (`global_blackboard.json`)

Fichier séparé, éditable depuis l'éditeur ATS via un panneau dédié (décision D4), chargé **une fois par niveau/session** et partagé entre **tous les graphes**.

**Schéma JSON :**

```json
{
  "schemaVersion": 1,
  "description": "Global Blackboard — variables partagées entre tous les graphes du niveau",
  "variables": {
    "GamePhase":      { "type": "String", "default": "Exploration" },
    "GlobalAlert":    { "type": "Int",    "default": 0 },
    "PlayerPos":      { "type": "Vector", "default": [0.0, 0.0, 0.0] },
    "MissionFailed":  { "type": "Bool",   "default": false }
  }
}
```

Accès depuis les nœuds : préfixe **`global:`** dans le `BlackboardKeyRef`.

### 4.3 BlackboardKeyRef (résolution des clés)

Structure utilisée dans les paramètres de nœuds pour référencer une clé BB :

```json
{ "scope": "local",  "path": "TargetDistance" }
{ "scope": "global", "path": "PlayerPos" }
```

**Notation raccourcie** dans les paramètres (forme string) :

```
"local:TargetDistance"
"global:PlayerPos"
```

Les deux formes sont équivalentes. L'éditeur ATS normalise vers la forme objet en interne.

### 4.4 Opérateurs de condition par type

| Type BB | Opérateurs valides |
|---|---|
| `Bool` | `== true`, `== false` |
| `String` / Enum | `==`, `!=` |
| `Int` / `Float` | `==`, `!=`, `<`, `<=`, `>`, `>=`, `InRange(min,max)` |
| `EntityRef` | `IsSet`, `IsNotSet` |
| `Vector` | Indirect — stocker la distance calculée dans un `Float`, puis tester ce `Float` |

---

## 5 — Catalogue d'Actions (ActionRegistry)

### 5.1 Principe de l'ActionRegistry

Les actions disponibles dans l'éditeur ATS sont décrites dans **`Blueprints/Catalogues/ActionTypes.json`**, chargé par `EnumCatalogManager`.

**Deux types d'implémentation :**

| `impl` | Mécanisme | Usage typique |
|---|---|---|
| `"cpp"` | Fonction C++ appelée directement dans `ExecuteBTAction()` via le `BTActionType` enum | Mouvement, IA, logique |
| `"event"` | Génère un `Message` posté via `EventQueue::Get().Push(msg)` | Caméra, audio, niveau |

**Schéma d'une entrée dans `ActionTypes.json` :**

```json
{
  "id": "goto_position",
  "name": "Goto Position",
  "description": "Déplace l'entité vers une position absolue",
  "category": "Movement",
  "impl": "cpp",
  "parameters": [
    { "name": "position",  "type": "Vector",  "required": true,  "description": "Position cible (utilise Vector(x,y,0))" },
    { "name": "speed",     "type": "Float",   "default": 200.0,  "description": "Vitesse de déplacement" },
    { "name": "tolerance", "type": "Float",   "default": 10.0,   "description": "Rayon d'arrivée" }
  ],
  "returns": "Bool",
  "tooltip": "Déplace l'entité vers une position fixe (Vector)"
}
```

### 5.2 Catalogue complet des actions (niveau intermédiaire)

#### Catégorie Movement

| `id` | Paramètres | `impl` | Description |
|---|---|---|---|
| `goto_position` | `position: Vector`, `speed: Float`, `tolerance: Float` | cpp | Aller à une position fixe |
| `move_to_goal` | `speed: Float`, `tolerance: Float` | cpp | Aller vers la destination BB (`MoveIntent`) |
| `set_move_goal_to_target` | — | cpp | Copier position target BB → `MoveGoal` |
| `set_move_goal_to_pos` | `position: Vector` | cpp | Écrire position directement dans `MoveGoal` |
| `patrol_pick_next_point` | `patrolRoute: BBRef` | cpp | Choisir prochain waypoint de la route |
| `get_next_waypoint` | `graphRef: String` | cpp | Récupérer waypoint depuis un sous-graphe |
| `choose_random_navigable_point` | `searchRadius: Float`, `maxAttempts: Int` | cpp | Point navigable aléatoire |
| `request_pathfinding` | — | cpp | Déclencher le pathfinding vers `MoveGoal` |
| `follow_path` | — | cpp | Suivre le chemin calculé |

#### Catégorie AI/State

| `id` | Paramètres | `impl` | Description |
|---|---|---|---|
| `change_state` | `newState: String` | cpp | Mettre à jour `AIState.currentState` en BB |
| `clear_target` | — | cpp | Effacer la cible (`TargetActor` → null) |
| `set_bb_value` | `key: BBRef`, `value: Variant` | cpp | Écrire une valeur dans le BB local/global |
| `get_bb_value` | `key: BBRef` → output pin | cpp | Lire une valeur du BB (pin de sortie) |
| `wait_random_time` | `min: Float`, `max: Float` | cpp | Timer aléatoire |
| `emit_noise` | `intensity: Float`, `radius: Float` | cpp | Émettre un son via `AIEvents::EmitNoise` |
| `attack_if_close` | `range: Float`, `damage: Float` | cpp | Attaque mêlée si distance ≤ range |

#### Catégorie Animation/Visuel

| `id` | Paramètres | `impl` | Description |
|---|---|---|---|
| `set_sprite` | `spriteRef: String` | event | Changer le sprite (path ou référence) |
| `set_animation` | `animRef: String`, `loop: Bool` | event | Lancer une animation |
| `play_vfx` | `vfxId: String`, `position: Vector` | event | Déclencher un VFX |

#### Catégorie Caméra (mappage vers `CameraEventHandler`)

| `id` | Paramètres | `impl` | Mappe vers |
|---|---|---|---|
| `camera_shake` | `intensity: Float`, `duration: Float` | event | `CameraEventHandler::TriggerCameraShake()` |
| `camera_zoom_to` | `targetZoom: Float`, `speed: Float` | event | `CameraEventHandler::ZoomCameraTo()` |
| `camera_teleport` | `x: Float`, `y: Float` | event | `CameraEventHandler::TeleportCamera()` |
| `camera_reset` | — | event | `CameraEventHandler::ResetCamera()` |

#### Catégorie Level/Système

| `id` | Paramètres | `impl` | Description |
|---|---|---|---|
| `load_level` | `levelName: String` | event | Charger un niveau |
| `get_dt` | — → `Float` output | cpp | Retourner le delta time courant |
| `get_world_instance` | `instanceName: String` → `EntityRef` | cpp | Récupérer une instance nommée du monde |
| `set_position` | `instance: EntityRef`, `position: Vector` | cpp | Positionner une entité |
| `log_message` | `message: String`, `level: String` | cpp | Log de debug via `SYSTEM_LOG` |
| `sub_graph` | `graphRef: String` | cpp | Exécuter un sous-graphe externe |

### 5.3 Macro d'enregistrement C++

Pour enregistrer une action dans `ExecuteBTAction()` :

```cpp
// Dans ExecuteBTAction() — switch case
case BTActionType::GotoPosition:
{
    if (!node.parameters.count("position")) return BTStatus::Failure;
    // Lire position depuis paramètre
    // ... implémentation ...
    return BTStatus::Success;
}
```

Pour les nouvelles actions non encore dans l'enum `BTActionType`, utiliser `conditionTypeString` / `actionTypeString` + lookup dans une map string→function :

```cpp
// Lookup dynamique pour types non compilés
static std::unordered_map<std::string, ActionFn> s_actionRegistry = {
    { "emit_noise",  &ActionImpl_EmitNoise  },
    { "play_vfx",    &ActionImpl_PlayVFX    },
};

auto it = s_actionRegistry.find(node.actionTypeString);
if (it != s_actionRegistry.end()) {
    return it->second(entity, node, bb);
}
return BTStatus::Failure;
```

---

## 6 — Catalogue de Conditions (`ConditionTypes.json`)

Les conditions génériques sont décrites dans **`Blueprints/Catalogues/ConditionTypes.json`**.

**Format d'une entrée catalogue :**

```json
{
  "id": "CompareValue",
  "name": "Compare BB Value",
  "description": "Compare une valeur du Blackboard à un littéral",
  "category": "Logic",
  "parameters": [
    { "name": "key",      "type": "BBRef",   "required": true },
    { "name": "operator", "type": "String",  "required": true, "enum": ["==","!=","<","<=",">",">="] },
    { "name": "value",    "type": "Variant", "required": true }
  ]
}
```

**Liste complète des conditions supportées :**

| `id` | Catégorie | Paramètres | Description |
|---|---|---|---|
| `CompareValue` | Logic | `key: BBRef`, `operator: String`, `value: Variant` | Compare une valeur BB à un littéral |
| `IsSet` | Logic | `key: BBRef` | Vérifie qu'une clé BB est définie et non-null |
| `IsNotSet` | Logic | `key: BBRef` | Vérifie qu'une clé BB est null/absente |
| `InRange` | Logic | `key: BBRef`, `min: Float`, `max: Float` | Vérifie qu'un float/int est dans [min, max] |
| `RandomChance` | Logic | `probability: Float [0..1]` | Retourne SUCCESS avec probabilité P |
| `HeardNoise` | Perception | *(implémentation C++ via `AISenses`)* | Détection sonore |
| `TargetVisible` | Perception | *(raycast + senseurs)* | Vision via line-of-sight |
| `HasLineOfSight` | Perception | *(raycast)* | Vérifie la ligne de vue |
| `HealthBelow` | Combat | `threshold: Float` | Santé < seuil |
| `HasTarget` | Combat | — | Alias `TargetVisible` |
| `IsTargetInRange` | Combat | `range: Float` | Vérifie distance cible ≤ range |
| `CanAttack` | Combat | `cooldown: Float` | Vérifie que le cooldown d'attaque est écoulé |

---

## 7 — Catalogue de Décorateurs (`DecoratorTypes.json`)

Les décorateurs sont décrits dans **`Blueprints/Catalogues/DecoratorTypes.json`**.

**Format d'une entrée catalogue :**

```json
{
  "id": "BT_Cooldown",
  "name": "Cooldown",
  "category": "Timing",
  "parameters": [
    { "name": "seconds", "type": "Float", "required": true }
  ],
  "tooltip": "Empêche la ré-exécution pendant N secondes"
}
```

**Liste complète des décorateurs supportés :**

| `id` | `name` | Catégorie | Paramètres | Description |
|---|---|---|---|---|
| `BT_Inverter` | Inverter | Logic | — | Inverse SUCCESS↔FAILURE |
| `BT_Repeater` | Repeater | Loop | `count: Int` (0 = infini) | Répète N fois |
| `BT_UntilSuccess` | Until Success | Loop | — | Répète jusqu'au SUCCESS |
| `BT_UntilFailure` | Until Failure | Loop | — | Répète jusqu'au FAILURE |
| `BT_Cooldown` | Cooldown | Timing | `seconds: Float` | Bloque ré-exécution pendant N secondes |
| `BT_TimeLimit` | Time Limit | Timing | `seconds: Float` | Abandonne si durée dépassée |
| `BT_ForceSuccess` | Force Success | Logic | — | Retourne toujours SUCCESS |
| `BT_ForceFailure` | Force Failure | Logic | — | Retourne toujours FAILURE |
| `BT_Succeeder` | Succeeder | Logic | — | Alias de `BT_ForceSuccess` |

---

## 8 — Schéma JSON Unifié v2 (GraphDocument)

Schéma complet d'un graphe ATS `BehaviorTree` avec toutes les sections :

```json
{
  "schemaVersion": 2,
  "type": "AIGraph",
  "graphKind": "BehaviorTree",
  "name": "GuardCombatTree",
  "metadata": {
    "author": "Atlasbruce",
    "created": "2026-03-07T00:00:00",
    "tags": ["AI", "Combat"]
  },
  "blackboard": {
    "TargetActor":    { "type": "EntityRef", "default": null },
    "TargetDistance": { "type": "Float",     "default": 9999.0 },
    "AlertLevel":     { "type": "Int",       "default": 0 }
  },
  "editorState": {
    "zoom": 1.0,
    "scrollOffset": { "x": 0.0, "y": 0.0 }
  },
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "name": "Root Selector",
        "type": "BT_Selector",
        "position": { "x": 400.0, "y": 100.0 },
        "childIds": [2, 5, 8]
      },
      {
        "id": 2,
        "name": "Attack Sequence",
        "type": "BT_Sequence",
        "position": { "x": 200.0, "y": 250.0 },
        "childIds": [3, 4]
      },
      {
        "id": 3,
        "name": "Target In Range?",
        "type": "Condition",
        "conditionType": "CompareValue",
        "position": { "x": 100.0, "y": 400.0 },
        "parameters": {
          "key": "local:TargetDistance",
          "operator": "<=",
          "value": 60.0
        }
      },
      {
        "id": 4,
        "name": "Attack",
        "type": "Action",
        "actionType": "AttackIfClose",
        "position": { "x": 300.0, "y": 400.0 },
        "parameters": { "range": 60.0, "damage": 10.0 },
        "decorator": {
          "type": "BT_Cooldown",
          "params": { "seconds": 1.5 }
        }
      }
    ],
    "links": []
  }
}
```

**Description des sections principales :**

| Clé | Type | Description |
|---|---|---|
| `schemaVersion` | `Int` | Version du schéma JSON (actuel : 2) |
| `type` | `String` | Type d'asset (`"AIGraph"`) |
| `graphKind` | `String` | Sous-type (`"BehaviorTree"`, `"Blueprint"`, etc.) |
| `name` | `String` | Nom humain du graphe |
| `metadata` | `Object` | Auteur, date, tags — informations éditoriales |
| `blackboard` | `Object` | Variables BB locales déclarées (décision D1) |
| `editorState` | `Object` | État de la caméra/zoom de l'éditeur (non exécuté) |
| `data.rootNodeId` | `Int` | ID du nœud racine |
| `data.nodes` | `Array` | Liste des nœuds du graphe |
| `data.links` | `Array` | Liens explicites (optionnel pour BT, obligatoire pour Blueprint) |

---

## 9 — Pipeline d'exécution ATS → ECS

Le flux complet d'exécution d'un graphe ATS dans le moteur :

```
1. CHARGEMENT
   NodeGraphManager::LoadGraph()
   └─► GraphMigrator::LoadWithMigration()   [migration v1→v2 si nécessaire]
       └─► GraphDocument                     [modèle mémoire du graphe]

2. ACTIVATION (par entité)
   BehaviorTree::LoadTreeForEntity(entity, path)
   └─► attach BehaviorTreeRuntime_data component sur l'entité ECS

3. UPDATE (chaque frame)
   BehaviorTree::UpdateEntity(entity, dt)
   └─► parcours récursif des nœuds depuis rootNodeId
       ├─► Composites : sélectionnent/séquencent les enfants
       ├─► Conditions  : évaluent → SUCCESS/FAILURE
       └─► Actions     : exécutent → SUCCESS/FAILURE/RUNNING

4. LECTURE / ÉCRITURE BLACKBOARD
   ├─► BB local  : AIBlackboard_data component de l'entité
   └─► BB global : GlobalBlackboard (singleton niveau)

5. ACTIONS impl=event
   EventQueue::Get().Push(msg)
   └─► consommé le frame suivant par les ECS systems correspondants
       ├─► CameraEventHandler  (shake, zoom, teleport)
       ├─► AudioSystem         (sons, musique)
       └─► LevelSystem         (chargement de niveau)

6. ACTIONS impl=cpp
   ExecuteBTAction() switch-case sur BTActionType
   └─► appel direct de la logique C++

7. DEBUG (éditeur)
   EditorContext::Debug
   └─► NodeAnnotationsManager::SetBreakpoint()
       └─► highlight du nœud actif dans l'éditeur ATS
```

---

## 10 — Services (P2 — à venir)

Les **Services** s'exécutent périodiquement (tick rate configurable) tant que leur nœud Composite ou Task parent est dans la branche active de l'arbre.

Cette fonctionnalité est planifiée pour la **Phase 2** du développement ATS.

### Principe (décision D2)

Les Services sont attachés à des **nœuds Composite** (Selector, Sequence) ou à des **nœuds Task/Action**. Ils s'exécutent tant que leur nœud parent est actif dans la branche courante — analogie directe avec les Services Unreal Engine BT.

### Cas d'usage typiques

| Service | Description | Tick rate recommandé |
|---|---|---|
| `UpdateTargetDistance` | Recalcule `TargetDistance` dans le BB | 0.1s (10 Hz) |
| `ScanForTargets` | Met à jour `TargetActor` si cible dans le rayon | 0.2s (5 Hz) |
| `UpdateAlertLevel` | Décrémente `AlertLevel` si aucun stimulus | 1.0s (1 Hz) |

### Schéma JSON d'un service attaché à un Composite

```json
{
  "id": 1,
  "type": "BT_Selector",
  "services": [
    {
      "type": "UpdateTargetDistance",
      "tickRate": 0.1,
      "parameters": { "maxRange": 800.0 }
    },
    {
      "type": "ScanForTargets",
      "tickRate": 0.2,
      "parameters": { "detectionRadius": 400.0 }
    }
  ]
}
```

---

## 11 — Sous-graphes (SubGraph nodes)

### Principe (décision D3)

Les sous-graphes sont référencés comme des **assets JSON externes** — pas inline dans le graphe parent. Cette approche favorise la réutilisabilité et la maintenabilité.

### Nœud SubGraph

```json
{
  "id": 99,
  "name": "Patrol Routine",
  "type": "SubGraph",
  "position": { "x": 600.0, "y": 300.0 },
  "parameters": {
    "graphRef": "Blueprints/AI/guard_patrol.json"
  }
}
```

### Règles d'utilisation

1. Le fichier référencé par `graphRef` doit être un `GraphDocument` valide avec un `graphKind` compatible.
2. Les variables du sous-graphe ont leur **propre BB local** (isolé du graphe parent).
3. Pour passer des valeurs entre le graphe parent et le sous-graphe, utiliser des **paramètres d'entrée explicites** (binding `graphRef` → paramètres).
4. La validation vérifie l'**existence du fichier** au chargement du graphe parent (règle R07).
5. Les sous-graphes peuvent eux-mêmes référencer d'autres sous-graphes (profondeur de récursion limitée par configuration).

### Exemple de passage de paramètres

```json
{
  "id": 99,
  "name": "Patrol Routine",
  "type": "SubGraph",
  "parameters": {
    "graphRef": "Blueprints/AI/guard_patrol.json",
    "inputs": {
      "StartIndex":  { "scope": "local", "path": "PatrolIndex" },
      "PatrolSpeed": 150.0
    }
  }
}
```

---

## 12 — Ancrage dans le code existant

Table de correspondance entre la spécification ATS et le code existant dans `Atlasbruce/Olympe-Engine` :

| Concept ATS | Fichier existant | Classe / Enum |
|---|---|---|
| Types de nœuds BT | `Source/AI/BehaviorTree.h` | `BTNodeType`, `BTConditionType`, `BTActionType` |
| Registry des types | `Source/AI/AIGraphPlugin_BT/BTNodeRegistry.h` | `BTNodeRegistry`, `BTNodeTypeInfo` |
| Catalogue JSON Actions | `Blueprints/Catalogues/ActionTypes.json` | Chargé par `EnumCatalogManager` |
| Catalogue JSON Conditions | `Blueprints/Catalogues/ConditionTypes.json` | Chargé par `EnumCatalogManager` |
| Catalogue JSON Décorateurs | `Blueprints/Catalogues/DecoratorTypes.json` | Chargé par `EnumCatalogManager` |
| EnumCatalogManager | `Source/BlueprintEditor/EnumCatalogManager.h/cpp` | `EnumCatalogManager::Get()` |
| GraphDocument | `Source/NodeGraphCore/GraphDocument.h/cpp` | `GraphDocument`, `NodeData`, `LinkData` |
| BlackboardSystem (local) | `Source/NodeGraphCore/BlackboardSystem.h` | `BlackboardSystem`, `BlackboardType`, `BlackboardValue` |
| Vector | `Source/vector.h` | `Vector(x,y,z)` — **pas de Vector2** |
| EventQueue | `Source/system/EventQueue.h` | `EventQueue::Get().Push(msg)` |
| EventTypes | `Source/system/message.h` | `EventType`, `EventDomain`, `Message` |
| CameraEventHandler | `Source/system/CameraEventHandler.h` | `TriggerCameraShake()`, `ZoomCameraTo()`… |
| EditorContext | `Source/NodeGraphCore/EditorContext.h` | `EditorMode::Editor/Visualizer/Debug` |
| NodeAnnotations | `Source/NodeGraphCore/NodeAnnotations.h` | Debug highlighting |
| CommandSystem | `Source/NodeGraphCore/CommandSystem.h` | Undo/Redo |
| GraphMigrator | `Source/NodeGraphCore/GraphMigrator.cpp` | Migration v1→v2 |
| BTGraphValidator | `Source/AI/AIGraphPlugin_BT/BTGraphValidator.h/cpp` | `BTGraphValidator`, `BTValidationMessage` |

### Extension nécessaire à `BlackboardType`

Ajouter `EntityRef` au enum `BlackboardType` dans `BlackboardSystem.h` :

```cpp
// BlackboardSystem.h — enum BlackboardType (avant modification)
enum class BlackboardType { Int, Float, Bool, String, Vector3 };

// BlackboardSystem.h — enum BlackboardType (après modification)
// - Remplacer Vector3 par Vector (cohérence avec Source/vector.h)
// - Ajouter EntityRef (EntityID = uint32_t)
enum class BlackboardType { Int, Float, Bool, String, Vector, EntityRef };
```

---

## 13 — Questions ouvertes / Décisions futures (P3+)

Les sujets suivants sont identifiés pour les phases ultérieures et n'ont pas encore été tranchés :

| Sujet | Description | Priorité |
|---|---|---|
| **Binding SubGraph params** | Format de binding des paramètres d'entrée/sortie des SubGraphs entre graphe parent et sous-graphe | P3 |
| **Custom Enum types** | Système de types personnalisés (Enum custom déclarable dans le BB) | P3 |
| **Hot-reload catalogues** | Rechargement à chaud des catalogues JSON (`ActionTypes.json`, etc.) en cours d'édition sans redémarrer l'éditeur | P3 |
| **Static type checker** | Validation statique complète : type checker à la "compilation" du graphe (avant exécution) | P3 |
| **Node tags/labels** | Système de tags sur les nœuds pour filtrage et recherche dans l'éditeur ATS | P3 |
| **Services API** | API C++ pour l'enregistrement et l'exécution des Services (actuellement non implémentés) | P2 |
| **SubGraph output pins** | Mécanisme de retour de valeurs depuis un sous-graphe vers le graphe parent | P3 |
| **BB variable watchers** | Abonnement à des changements de valeur BB (onChanged callbacks) pour déclencher des comportements réactifs | P4 |

---

*Fin du document — Version 1.0 — 2026-03-07*
