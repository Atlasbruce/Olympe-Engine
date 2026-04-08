# Guide Behavior Trees (BT) — Olympe Engine (Designers)

Ce guide explique **comment concevoir** des comportements avec les Behavior Trees dans Olympe Engine, en restant **compréhensible**, **testable**, et **maintenable**.
Il contient :
- Les concepts essentiels (Success / Failure / Running)
- Les conventions Blackboard recommandées (porte/clé/coffre/inventaire)
- La documentation de **chaque node** (designer + technique)
- Des **recettes** prêtes à copier (civilian, explorer, clé/porte/coffre, inventaire, utiliser objet)
- Des règles de maintenance (pour éviter les BT fragiles)

---

## 0) Les 3 statuts (le cœur des BT)

Chaque node retourne un statut :

- **Running** : le node est “en cours”. Le BT reprendra au prochain tick.
- **Success** : le node a atteint son objectif.
- **Failure** : le node ne peut pas atteindre son objectif *dans l’état actuel* (ex : pas de clé, pas de cible, porte introuvable).

> Règle d’or : un Behavior Tree n’est pas “de l’IA magique”.  
> C’est un **orchestrateur** qui enchaîne des **Conditions** et des **Actions** en lisant/écrivant dans le **Blackboard**.

---

## 1) Conventions Blackboard (recommandées)

Dans Olympe, vos états gameplay sont dans le **blackboard** et vos objets (porte, clé, coffre) sont des **entités ECS**.

### 1.1 Clés de ciblage (cible / interaction)
- `TargetEntity` (EntityID) : la cible principale (clé repérée, ennemi, objet).
- `InteractEntity` (EntityID) : l’objet “avec lequel on interagit maintenant” (porte/coffre).
- `HasTarget` (bool)
- `TargetVisible` (bool)
- `DistanceToTarget` (float) ou `TargetInRange` (bool)

### 1.2 Clés de déplacement
- `MoveGoal` (Vector)
- `HasMoveGoal` (bool)
- `ArriveDistance` (float) : distance d’arrivée/interaction (ex: 60.0)

### 1.3 Clés “porte”
- `DoorLocked` (bool)
- `DoorOpen` (bool)
- `DoorKeyId` (int) **ou** `DoorKeyEntity` (EntityID)
- `HasKey` (bool) **ou** `HasKeyId` (int)

### 1.4 Clés “coffre”
- `ChestLocked` (bool)
- `ChestOpen` (bool)
- `ChestKeyId` (int) / `HasKeyId` (int)

### 1.5 Clés “inventaire”
- `InventoryOpen` (bool)
- `SelectedItemId` (int)
- `HasSelectedItem` (bool)

> Conseil : évitez les noms ambigus. Préférez `DoorLocked` à `Locked`.  
> Plus vos clés sont standardisées, plus vos BT sont réutilisables.

---

## 2) Les familles de nodes

- **Composites** : structurent “comment on exécute les enfants” (Selector, Sequence…)
- **Decorators** : modifient le comportement d’un enfant (Inverter, Repeater…)
- **Conditions** : testent une vérité (Blackboard, distance, visibilité…)
- **Actions** : font quelque chose (déplacement, attente, interaction…)

---

## 3) Catalogue des nodes (référence)

Les nodes listés ci-dessous correspondent aux nodes enregistrés dans la palette BT (éditeur).
Certains concepts existent aussi côté runtime sous forme d’enums (conditions/actions).  

### Légende
- ✅ Stable : comportement attendu clair
- ⚠️ Expérimental : existe dans l’éditeur mais comportement runtime à stabiliser

---

# A) COMPOSITES (structure)

## Node: **Selector** (`BT_Selector`) ✅
### Nature / objectif
**Composite** “Plan A sinon Plan B sinon Plan C”.  
Teste ses enfants dans l’ordre : dès qu’un enfant réussit, le Selector réussit.

### Success / Failure / Running
- **Success** : un enfant retourne Success
- **Failure** : tous les enfants retournent Failure
- **Running** : l’enfant courant est Running

### Exemple in situ — “ouvrir une porte”
```
Selector
├─ Sequence  (porte déjà ouverte)
│  ├─ CheckBlackboardValue(key="DoorOpen", operator="==", value="true")
│  └─ Wait(duration=0.0)  // pattern "no-op"
└─ Sequence  (porte fermée)
   ├─ CheckBlackboardValue(key="DoorOpen", operator="==", value="false")
   └─ OpenDoor            // action custom ou ATS
```

### Notes techniques (maintenance)
- Le Selector ne lit/écrit pas le blackboard : il orchestre uniquement.
- Bon node pour gérer les exceptions et fallback.

---

## Node: **Sequence** (`BT_Sequence`) ✅
### Nature / objectif
**Composite** “recette en étapes”.  
Exécute les enfants dans l’ordre : si une étape échoue, toute la recette échoue.

### Success / Failure / Running
- **Success** : tous les enfants retournent Success
- **Failure** : un enfant retourne Failure
- **Running** : un enfant est Running

### Exemple in situ — “aller au coffre puis ouvrir”
```
Sequence
├─ MoveToTarget(speed=120)    // cible = InteractEntity (le coffre)
├─ CheckBlackboardValue(key="ChestLocked", operator="==", value="false")
└─ OpenChest                  // action custom ou ATS
```

### Notes techniques
- Le Sequence est le node “par défaut” pour une interaction en plusieurs étapes.

---

## Node: **Parallel** (`BT_Parallel`) ⚠️
### Nature / objectif (designer)
Conceptuellement : exécuter plusieurs enfants “en parallèle”.

### Recommandation designers
⚠️ **À éviter** tant que son comportement runtime n’est pas strictement défini et aligné.
Utilisez plutôt :
- un BT (Selector/Sequence) + une action ATS (qui peut gérer un comportement “concurrent”),
- ou des systèmes ECS dédiés.

---

# B) DECORATORS (modificateurs)

## Node: **Inverter** (`BT_Inverter`) ✅
### Nature / objectif
Décorateur “NOT”. Il inverse Success ↔ Failure de son enfant.

### Success / Failure / Running
- **Success** si l’enfant retourne Failure
- **Failure** si l’enfant retourne Success
- **Running** si l’enfant est Running

### Exemple in situ — “si je n’ai pas la clé → chercher clé”
```
Sequence
├─ Inverter
│  └─ CheckBlackboardValue(key="HasKey", operator="==", value="true")
└─ FindKey   // action custom ou ATS
```

### Notes techniques
- 1 enfant exactement.
- Très utile pour exprimer des conditions “négatives” proprement.

---

## Node: **Repeater** (`BT_Repeater`) ⚠️ (Stable concept / impl à vérifier)
### Nature / objectif
Répète un enfant N fois (paramètre `repeatCount`).

### Success / Failure / Running (concept)
- **Running** tant que des répétitions sont en cours
- **Success** quand N répétitions sont terminées (ou selon politique)
- **Failure** selon politique (ex: l’enfant échoue de manière bloquante)

### Exemple in situ — “réessayer d’ouvrir 3 fois”
```
Repeater(repeatCount=3)
└─ OpenDoor
```

### Notes techniques
- Très utile comme “retry” simple, mais assurez-vous que l’action répétée est **idempotente** (ne casse pas l’état si répétée).
- Si l’action est “une animation”, assurez-vous qu’elle gère bien les re-entrées.

---

## Node: **Until Success** (`BT_UntilSuccess`) ⚠️
### Nature / objectif (designer)
Répéter l’enfant jusqu’à ce qu’il réussisse.

### Exemple in situ
- “continuer à chercher jusqu’à trouver la clé”
```
UntilSuccess
└─ FindKey
```

### Notes techniques (maintenance)
- À stabiliser : nécessite un état (ou une politique) claire sur ce qui se passe si l’enfant échoue.
- Souvent, “chercher une clé” est mieux géré via une action ATS qui contient une boucle (ou un système de perception).

---

## Node: **Until Failure** (`BT_UntilFailure`) ⚠️
### Nature / objectif (designer)
Répéter l’enfant tant qu’il réussit ; arrêter quand il échoue.

### Exemple in situ
- “continuer à explorer tant qu’il reste des points à visiter”
```
UntilFailure
└─ ExploreNextPoint
```

---

## Node: **Cooldown** (`BT_Cooldown`) ⚠️
### Nature / objectif (designer)
Empêche l’exécution trop fréquente. Paramètre : `cooldownDuration`.

### Exemple in situ — “civilian consulte inventaire au max 1 fois / 10s”
```
Cooldown(cooldownDuration=10)
└─ ConsultInventory
```

### Notes techniques (maintenance)
- Nécessite une gestion de timestamp (souvent dans le runtime state ou le blackboard).
- Tant que ce comportement n’est pas verrouillé, préférez une action ATS “ConsultInventoryWithCooldown.ats”.

---

# C) CONDITIONS (feuilles booléennes)

## Node: **Check Blackboard Value** (`BT_CheckBlackboardValue`) ✅ (fondamental)
### Nature / objectif
Condition générique : compare une valeur de blackboard (`key`) à une valeur attendue (`value`) avec un opérateur (`operator`).

### Success / Failure
- **Success** : la comparaison est vraie
- **Failure** : la comparaison est fausse

### Exemples in situ
- Porte verrouillée ?
  - `DoorLocked == true`
- A une clé ?
  - `HasKey == true`
- Coffre ouvert ?
  - `ChestOpen == true`
- A un item sélectionné ?
  - `HasSelectedItem == true`

### Notes techniques (maintenance)
- Node clé pour l’évolutivité : on peut étendre les opérateurs, supporter types, etc.
- Très bon pour gameplay (portes/clés/coffres) car tout est déjà “state-based”.

---

## Node: **Has Target** (`BT_HasTarget`) ✅ (si vous utilisez TargetEntity)
### Nature / objectif
Vérifie si l’agent a une cible pertinente.

### Exemples in situ
- Explorer : si une clé est visible, la “targeter”.

### Notes techniques
- Dans certains pipelines, “HasTarget” est très proche de “TargetVisible”.
- Pour du gameplay, vous pouvez aussi faire :
  - `CheckBlackboardValue(TargetEntity != 0)` si vos opérateurs le permettent,
  - ou `CheckBlackboardValue(HasTarget == true)`.

---

## Node: **Is Target In Range** (`BT_IsTargetInRange`) ✅
### Nature / objectif
Vérifie si la cible est à portée d’interaction (ouvrir porte, ramasser clé, ouvrir coffre).

### Exemple in situ
```
Sequence
├─ MoveToTarget(speed=120)
└─ IsTargetInRange(distance=60)
```

> Si vous avez déjà `DistanceToTarget` dans le blackboard, vous pouvez aussi utiliser :
`CheckBlackboardValue(DistanceToTarget <= 60)`.

---

## Node: **Can See Target** (`BT_CanSeeTarget`) ✅
### Nature / objectif
Vérifie la visibilité/ligne de vue (utile pour “trouver une clé visible”).

---

# D) ACTIONS (feuilles d’exécution)

## Node: **Wait** (`BT_Wait`) ✅
### Nature / objectif
Attendre N secondes.

### Success / Running
- **Running** tant que le temps n’est pas écoulé
- **Success** quand la durée est atteinte

### Exemple in situ — civilian “pause”
```
Sequence
├─ WanderStep
└─ Wait(duration=2.0)
```

---

## Node: **Wait Random Time** (`BT_WaitRandomTime`) ✅
### Nature / objectif
Attendre une durée aléatoire entre `minDuration` et `maxDuration`.

### Exemple in situ — civilian “rythme naturel”
```
Sequence
├─ LookAround
└─ WaitRandomTime(minDuration=1.0, maxDuration=4.0)
```

---

## Node: **Set Blackboard Value** (`BT_SetBlackboardValue`) ⚠️
### Nature / objectif (designer)
Écrire dans le blackboard (key/value).

### Exemple in situ
- “Je cible cette porte”
```
SetBlackboardValue(key="InteractEntity", value="12345")
```

### Notes techniques (maintenance)
⚠️ Avec un blackboard **typé** (struct), ce node doit mapper une string (`key`) vers un champ.
Sinon il risque d’être :
- non-op,
- fragile,
- ou dangereux (si le mapping est incomplet).
Recommandation : limiter son usage à des clés explicitement supportées, ou basculer vers ATS.

---

## Node: **Move To Target** (`BT_MoveToTarget`) ✅ (concept stable)
### Nature / objectif (designer)
Se déplacer vers `TargetEntity` ou `InteractEntity` (selon votre convention).
Paramètre `speed`.

### Success / Failure / Running (concept)
- **Running** : tant que pas à portée
- **Success** : arrivé (distance <= ArriveDistance / seuil interne)
- **Failure** : pas de cible, path impossible

### Exemple in situ — ouvrir porte
```
Sequence
├─ MoveToTarget(speed=120)  // vers InteractEntity = door
└─ OpenDoor
```

---

## Node: **Move To Position** (`BT_MoveToPosition`) ✅
### Nature / objectif
Se déplacer vers une position (x,y).

### Exemple in situ — explorer un waypoint
```
Sequence
├─ MoveToPosition(x=1200, y=450)
└─ WaitRandomTime(1, 2)
```

---

## Node: **Attack Target** (`BT_AttackTarget`) ✅ (hors scope civilian)
Attaquer la cible. Utile si vous avez des NPC hostiles.

---

## Node: **Play Animation** (`BT_PlayAnimation`) ✅
### Nature / objectif
Jouer une animation.

### Exemples in situ
- Porte : “play_open_door”
- Coffre : “open_chest”
- Clé : “pickup_key”

---

## Node: **Emit Sound** (`BT_EmitSound`) ✅
### Nature / objectif
Jouer un son.

### Exemples in situ
- “door_unlock”
- “chest_open”
- “key_pickup”

---

# 4) Recettes gameplay (prêtes à copier)

## Recette 1 — Explorer → trouver une clé → ouvrir une porte verrouillée

### Blackboard minimal
- `InteractEntity` = la porte (EntityID)
- `DoorLocked` (bool)
- `DoorOpen` (bool)
- `HasKey` (bool)
- `TargetEntity` = la clé (EntityID) quand elle est repérée

### BT (structure)
```
Sequence
├─ MoveToTarget(speed=120)  // vers la porte (InteractEntity)
├─ Selector
│  ├─ Sequence
│  │  ├─ CheckBlackboardValue(DoorOpen == true)
│  │  └─ Wait(0)
│  └─ Sequence
│     ├─ CheckBlackboardValue(DoorOpen == false)
│     ├─ Selector
│     │  ├─ Sequence  (j’ai déjà la clé)
│     │  │  ├─ CheckBlackboardValue(HasKey == true)
│     │  │  └─ UnlockDoor        // custom/ATS
│     │  └─ Sequence  (chercher la clé)
│     │     ├─ FindKey           // custom/ATS (détection + set TargetEntity)
│     │     ├─ MoveToTarget      // vers TargetEntity (la clé)
│     │     ├─ PickupKey         // custom/ATS (HasKey=true)
│     │     └─ UnlockDoor
│     └─ OpenDoor                // custom/ATS
```

---

## Recette 2 — Ouvrir un coffre verrouillé avec une clé
Identique à la porte avec `ChestLocked / ChestOpen`.

---

## Recette 3 — Civilian : routine + consultation inventaire
```
Sequence
├─ WanderStep                // custom/ATS
├─ WaitRandomTime(2, 7)
└─ Cooldown(10)
   └─ ConsultInventory       // custom/ATS
```

> Si Cooldown est expérimental, remplacez par `ConsultInventoryWithCooldown.ats`.

---

## Recette 4 — Utiliser un objet (si présent dans inventaire)
```
Selector
├─ Sequence
│  ├─ CheckBlackboardValue(HasSelectedItem == true)
│  └─ UseSelectedItem        // custom/ATS
└─ Sequence
   ├─ ConsultInventory       // custom/ATS
   └─ UseSelectedItem
```

---

# 5) Règles pour des BT maintenables (important)

1) **Standardisez les clés blackboard** (sinon les BT deviennent non réutilisables)
2) **Gardez les BT “structure”** et mettez la logique complexe dans des Actions (idéalement ATS)
3) **Évitez Parallel** tant qu’il n’est pas supporté proprement
4) Les nodes “writer” (SetBlackboardValue) doivent être utilisés avec prudence tant que le mapping typé n’est pas garanti
5) Pour chaque action gameplay, définissez clairement :
   - Quel champ du blackboard elle lit ?
   - Quel champ elle écrit ?
   - Qu’est-ce qui déclenche Success / Failure / Running ?

---

# 6) Annexe technique (pour programmeurs / maintenance)

- Les nodes de la palette BT sont enregistrés via un registre (typeName, displayName, paramètres).
- Le runtime BT “classique” supporte des enums de conditions/actions (ex : TargetVisible, TargetInRange, WaitRandomTime, RequestPathfinding…).
- Certains nodes UI avancés (Cooldown/Until/Parallel) nécessitent une stabilisation runtime.

Fin.