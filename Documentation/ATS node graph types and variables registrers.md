## 1. Sequence vs Selector (avec cas d’usage)

### Sequence (AND ordonné)

- Logique : exécute ses enfants de gauche à droite, s’arrête dès qu’un enfant échoue, ne réussit que si tous les enfants réussissent.[^7][^2][^3][^1]
- Analogie : un `if` avec plusieurs étapes obligatoires, ou une porte logique AND.[^2][^1]

Exemple UE / ATS typique : « attaquer le joueur »
Sequence `AttackSequence` :

1. Condition `TargetInRange`.[^8][^2]
2. Action `MoveToGoal` (si pas encore à bonne distance).[^9][^8]
3. Action `AttackIfClose`.[^8]

Usage :

- Si `TargetInRange` = FAIL → la Sequence échoue immédiatement, on ne bouge pas et on n’attaque pas.[^1][^2]
- Si la condition et le déplacement réussissent → on passe à l’attaque.

Cas d’usage idéal :

- Enchaînement de **sous‑étapes obligatoires** : « vérifier », puis « préparer », puis « exécuter » (patrouille, ouvrir une porte, jouer une animation + déclencher VFX, etc.).[^2][^1]

***

### Selector (OR priorisé)

- Logique : exécute ses enfants de gauche à droite, s’arrête dès qu’un enfant réussit, échoue seulement si tous les enfants échouent.[^3][^7][^1][^2]
- Analogie : un `if / else if / else` ou une porte logique OR.[^1][^2]

Exemple UE / ATS typique : « choisir la meilleure action dispo »
Selector racine `Root Selector` dans ton `GuardCombatTree` :[^8]

1. Sequence `AttackSequence` (essayer d’attaquer si possible).[^8]
2. Sequence `ChaseSequence` (sinon se rapprocher du joueur).[^8]
3. Sequence `SearchLastKnownPosition` (sinon chercher là où on l’a vu pour la dernière fois).[^8]

Usage :

- Si `AttackSequence` réussit → Selector retourne SUCCESS, il ne teste pas les autres branches.[^1][^8]
- Si `AttackSequence` échoue mais `ChaseSequence` réussit → on poursuit le joueur.
- Si les deux échouent → on tombe sur la branche de recherche.

Cas d’usage idéal :

- **Priorisation de comportements** : « si je peux attaquer, j’attaque ; sinon je poursuis ; sinon je patrouille », « fuir si low HP, sinon combattre, sinon se cacher », etc.[^3][^2][^1]

***

## 2. Conditions : types, paramètres, opérateurs

L’idée : faire des conditions **génériques**, paramétrables, qui lisent le Blackboard local/global au lieu de coder une condition par cas.[^5][^10][^6][^11]

### 2.1. Types de conditions de base

Tu peux définir une petite famille de « condition types » ATS, inspirée du `UBTDecorator_Blackboard` d’Unreal :[^6][^5]

1. `CompareBlackboardValue`
    - Paramètres :
        - `Key` (ex : `"CurrentState"`, `"AlertLevel"`, `"TargetDistance"`).
        - `Operator` (==, !=, <, <=, >, >=).
        - `Value` (littéral ou autre clé).
    - Exemples :
        - `CurrentState == "Combat"`.
        - `AlertLevel >= 2`.
        - `TargetDistance <= 60`.
2. `BlackboardIsSet / IsNotSet`
    - Paramètres :
        - `Key`.
    - Exemples :
        - `HasTarget` ↔ `BlackboardIsSet("TargetActor")`.
    - Equivalent du décorateur « Is Set / Is Not Set » UE.[^12][^5][^6]
3. `BlackboardInRange`
    - Paramètres :
        - `Key` (float),
        - `Min`, `Max`.
    - Exemples :
        - `TargetDistance` dans `[0, 60]` (représente « à portée de mêlée »).
        - `AlertLevel` dans `[2, 5]`.
4. `RandomChance`
    - Paramètres :
        - `Probability` [0–1].
    - Usage :
        - Ajouter un peu d’aléatoire dans certains choix.
5. Conditions spécialisées (via code, mais même modèle)
    - `HeardNoise`, `TargetVisible`, `HasLineOfSight`… comme tu as déjà.[^11][^13][^14][^8]
    - Elles peuvent être implémentées en dur côté C++ (accès aux senseurs, raycasts, etc.).

***

### 2.2. Référencement des variables (BB local / global)

Pour rester simple et robuste, je te conseille :

- Un type `BlackboardKeyRef` dans l’éditeur, avec :
    - `Scope` : `Global` | `Local`.
    - `Path` : nom logique ou chemin (ex: `"CurrentState"`, `"AIState.currentState"`).

Exemples :

- Local :
    - `Scope = Local`, `Path = "CurrentState"` → se mappe à `AIState.currentState` sur l’entité.[^15]
    - `Scope = Local`, `Path = "PatrolRoute"` → `AIBlackboard.patrolRoute`.[^15]
    - `Scope = Local`, `Path = "AlertLevel"` → `AISenses.alertLevel`.[^15]
- Global :
    - `Scope = Global`, `Path = "GamePhase"`.
    - `Scope = Global`, `Path = "AlertLevelGlobal"`.

Dans l’éditeur de condition, le paramètre `Key` serait donc un champ de type `BlackboardKeyRef`, avec :

- un **dropdown de Scope** (Global/Local),
- un **dropdown de clé** filtrée par type (bool/int/float/string/vector) issu d’un référentiel de clés.[^5][^11]

***

### 2.3. Opérateurs de test à prévoir

Pour couvrir 95% des cas classiques :[^10][^6][^5]

- Bool :
    - `== true`, `== false` (en pratique, UI : « Est vrai », « Est faux »).
- Enum / String :
    - `==`, `!=`.
- Numérique (int, float) :
    - `==`, `!=`, `<`, `<=`, `>`, `>=`, `InRange(min, max)`.
- Objet / Handle :
    - `IsSet`, `IsNotSet` (check null/not null).
- Vector2/3 :
    - le plus courant est indirect : on stocke distance dans un float via une action/service, puis on teste ce float.

***

## 3. Paramétrage des conditions dans l’éditeur ATS

### 3.1. Modèle générique de paramètre

Côté data tu peux avoir :

```cpp
enum class ConditionKind {
    CompareValue,
    IsSet,
    IsNotSet,
    InRange,
    RandomChance,
    Custom // pour HeardsNoise, TargetVisible, etc.
};

struct ConditionParam {
    std::string name;
    Variant value;
};

struct ConditionNode {
    int id;
    std::string name;
    ConditionKind kind;
    std::vector<ConditionParam> params;
};
```

Exemple JSON pour « TargetDistance ≤ 60 (BB local) » :

```json
{
  "id": 3,
  "name": "Target In Range?",
  "type": "Condition",
  "conditionType": "CompareValue",
  "parameters": [
    { "name": "Key", "value": "local:TargetDistance" },
    { "name": "Operator", "value": "<=" },
    { "name": "Value", "value": 60.0 }
  ]
}
```

Exemple pour « A une cible ? » :

```json
{
  "conditionType": "IsSet",
  "parameters": [
    { "name": "Key", "value": "local:TargetActor" }
  ]
}
```

Exemple pour condition custom « HeardNoise » (comme ton `InvestigateTree`) :[^13]

```json
{
  "conditionType": "HeardNoise",
  "parameters": []
}
```


***

## 4. Tableau par catégorie de node avec cas d’usage

### 4.1. Composites

| Node | Description courte | Cas d’usage typique |
| :-- | :-- | :-- |
| Sequence | AND ordonné, stop au 1er FAIL.[^7][^3][^1][^2] | Enchaîner plusieurs étapes : vérifier → se déplacer → agir (ouvrir porte, attaquer).[^1][^2] |
| Selector | OR priorisé, stop au 1er SUCCESS.[^7][^3][^1][^2] | Choisir la meilleure stratégie dispo : attaquer > poursuivre > patrouiller.[^8][^3][^1] |

Exemples complémentaires :

- Sequence « Ouvrir une porte et passer » :[^1]

1. Condition `DoorIsClosed`.
2. Action `MoveTo(Door)`.
3. Action `OpenDoor`.
4. Action `MoveTo(InsideRoom)`.
- Selector « Comportement global de garde » :[^3][^1][^8]

1. Sequence `CombatSequence` (si ennemi proche et en vue).
2. Sequence `InvestigateSequence` (si bruit entendu).[^13]
3. Sequence `PatrolSequence` (fallback, comme ton `GuardPatrolTree`).[^9]

***

### 4.2. Actions

| Node | Paramètres / données | Cas d’usage |
| :-- | :-- | :-- |
| `MoveToGoal` | `Speed`, `Tolerance`, destination dans BB.[^13][^9][^8] | Aller vers un waypoint, un ennemi, une position cliquée par le designer. |
| `AttackIfClose` | `Range`, `Damage`, cooldown.[^8] | Appliquer des dégâts si la distance (BB) est ≤ Range. |
| `PatrolPickNextPoint` | `PatrolRoute` (BB local).[^9][^15] | Choisir le prochain waypoint d’une route prédéfinie. |
| `SetMoveGoalToTarget` | `TargetActor` (BB).[^8] | Lire la position de la cible depuis BB local, écrire la destination dans `MoveIntent`. |
| `ChangeState` | `NewState` (string/enum). | Mettre à jour `AIState.currentState` dans le BB local.[^15] |
| `GotoPosition` | `Position` (literal ou BB key). | Déplacement scénarisé : aller à une position précise (cinématique, script de level). |

Tous ces nodes utilisent soit le **BB local** (composants de l’entité), soit le **BB global** pour lire/écrire leurs paramètres.[^16][^11][^15]

***

### 4.3. Conditions

| Type de condition | Paramètres clés | Exemple concret ATS |
| :-- | :-- | :-- |
| `CompareValue` | `Key`, `Operator`, `Value`.[^6][^5][^10] | `TargetDistance <= 60` pour autoriser `AttackIfClose`. |
| `IsSet` / `IsNotSet` | `Key`.[^6][^5][^12] | `local:TargetActor` est défini → on a une cible. |
| `InRange` | `Key`, `Min`, `Max`.[^10][^2] | `AlertLevel` dans `[2, 5]` pour déclencher fuite / recherche. |
| `RandomChance` | `Probability`.[^10] | 30% de chances de jouer une anim alternative ou d’appeler du renfort. |
| `HeardNoise` (custom) | (implémentation C++ directe).[^13][^11][^14] | Ton `InvestigateTree` ne part en investigation que si du bruit a été détecté.[^13] |
| `TargetVisible` (custom) | (raycast + senseurs).[^8][^11][^14] | Dans ton `GuardCombatTree`, utilisé pour déclencher la poursuite si la cible est visible.[^8] |


***

### 4.4. Décorateurs (pour plus tard, mais même logique)

Même si tu utilises pour l’instant des nodes `Condition` séparés, tu peux plus tard migrer certaines conditions en **Decorators attachés** à un Sequence/Action, comme en UE :[^17][^6][^5]

- Decorator `BlackboardKey` :
    - Propriétés :
        - `BlackboardKey` (KeyRef).
        - `Operator` (==, !=, <, etc.).
    - Rôle :
        - Autoriser ou bloquer un node / une branche selon une clé BB.

L’avantage :

- Graphes plus lisibles :
    - Selector avec 3 Sequences, chacune décorée avec ses conditions (HP low, ennemi visible, etc.), comme dans beaucoup de BT UE.[^6][^5][^3]

***

Pour la suite, tu veux qu’on :

- définisse précisément le **catalogue de conditions génériques** (liste finale des types, schéma JSON + UI éditeur), ou
- rentre dans le détail du **système de BlackboardKeyRef** (déclarations de clés, scopes, types, outil d’auto‑complétion dans l’éditeur) ?
<span style="display:none">[^18][^19][^20][^21][^22][^23][^24][^25][^26][^27][^28][^29][^30][^31][^32][^33][^34][^35][^36]</span>

<div align="center">⁂</div>

[^1]: https://www.gamedeveloper.com/programming/behavior-trees-for-ai-how-they-work

[^2]: https://blog.csdn.net/qq_23091255/article/details/148518768

[^3]: https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-tree-in-unreal-engine---quick-start-guide

[^4]: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-behavior-tree-node-reference-composites

[^5]: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AIModule/BehaviorTree/Decorators/UBTDecorator_Blackboard

[^6]: https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-behavior-tree-node-reference-decorators

[^7]: https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-tree-node-reference-composites?application_version=4.27

[^8]: guard_combat.json

[^9]: guard_patrol.json

[^10]: https://robohub.org/introduction-to-behavior-trees/

[^11]: https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-trees-in-unreal-engine/?application_version=5.1

[^12]: https://www.reddit.com/r/unrealengine/comments/188jk63/can_somebody_explain_how_the_decorator_uses_the/

[^13]: investigate.json

[^14]: https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-tree-in-unreal-engine---overview/?application_version=5.3

[^15]: guard_npc.json

[^16]: https://dev.epicgames.com/documentation/en-us/unreal-engine/behavior-trees?application_version=4.27

[^17]: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AIModule/BehaviorTree/Decorators

[^18]: https://dev.epicgames.com/documentation/ja-jp/unreal-engine/unreal-engine-behavior-tree-node-reference-composites

[^19]: https://dev.epicgames.com/documentation/ko-kr/unreal-engine/unreal-engine-behavior-tree-node-reference-composites

[^20]: https://dev.epicgames.com/documentation/ru-ru/unreal-engine/unreal-engine-behavior-tree-node-reference-composites?application_version=5.5

[^21]: https://github.com/Atlasbruce/Olympe-Engine/issues/256

[^22]: https://dev.epicgames.com/documentation/ko-kr/unreal-engine/unreal-engine-behavior-tree-node-reference-decorators

[^23]: https://github.com/Postive-ToolKit/SimpleBehaviorTree

[^24]: https://dev.epicgames.com/documentation/ja-jp/unreal-engine/unreal-engine-behavior-tree-node-reference-decorators

[^25]: https://dev.epicgames.com/documentation/ko-kr/unreal-engine/behavior-tree-node-reference-composites?application_version=4.27

[^26]: https://github.com/Idered/behavior-tree/issues/1

[^27]: https://dev.epicgames.com/documentation/ja-jp/unreal-engine/behavior-tree-node-reference-decorators?application_version=4.27

[^28]: https://github.com/FlaxEngine/FlaxDocs/blob/ed3848c101d6ec3910a8f5e61d2fd1f82cbe16c3/manual/scripting/ai/behavior-trees/nodes.md/

[^29]: https://www.youtube.com/watch?v=eoXE_zdDZO0

[^30]: https://forums.unrealengine.com/t/bt-selector-switching-between-them-without-leaving-the-sequence/264875

[^31]: https://dev.epicgames.com/community/learning/tutorials/0RWy/introduction-to-the-behavior-tree-sequences-selectors

[^32]: https://www.youtube.com/watch?v=nfjctGQiQ2g

[^33]: https://3dmpengines.tistory.com/1630

[^34]: https://forums.unrealengine.com/t/behavior-trees-sequencer/315648

[^35]: https://forums.unrealengine.com/t/sequence-priority-in-selector-of-behavior-tree/1782721

[^36]: https://dev.epicgames.com/community/learning/tutorials/0RWy/introduction-to-the-behavior-tree-sequences-selectors?locale=fr-fr

