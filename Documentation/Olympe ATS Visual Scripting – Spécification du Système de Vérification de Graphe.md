# Olympe ATS Visual Scripting – Spécification du Système de Vérification de Graphe

## 1. Objectifs et périmètre

Le **Graph Verification System (GVS)** d’Olympe assure la **cohérence structurelle, typée et sémantique** des graphes ATS Visual Scripting avant leur exécution runtime ou leur sauvegarde disque.

Objectifs principaux :

- Empêcher les connexions invalides (Exec/Data, types incompatibles, pins inversés).
- Garantir un graphe exécutable : un seul point d’entrée, pas de cycles interdits, pas de SubGraph circulaire.
- Vérifier la cohérence avec le **Blackboard** (clés connues, types attendus).
- Proposer des **messages de validation** exploitables dans l’éditeur (panel Validation, navigation vers le node fautif).

Le système fonctionne à deux niveaux :

1. **Validation interactive de connexion** (au drag \& drop de lien dans le Node Graph Panel).
2. **Validation globale de graphe** (à la demande, en auto‑refresh ou pré‑exécution / pré‑sauvegarde).

***

## 2. Modèle de données du graphe (rappel)

Le GVS s’appuie sur le modèle ATS Visual Script :

### 2.1. Nœuds

Chaque node possède au minimum :

- `id` : identifiant unique dans le graphe.
- `type` : type logique (`EntryPoint`, `Branch`, `Sequence`, `While`, `ForEach`, `Action`, `GetBBValue`, `SetBBValue`, `Math`, `SubGraph`, etc.).
- `name` : nom lisible (utilisé pour les messages).
- `position` : donnée purement éditoriale (ImGui).
- `parameters` : tableau d’arguments littéraux (ex. `conditionType`, `duration`, `newState`, etc.).
- `execPins` : description des pins Exec (In/Out) déclarés par ce type.
- `dataPins` : description des pins Data (In/Out, type).

Les graphes avancés ont en plus un `settings`, `blackboardRefs`, `metadata`, etc., mais ces champs sont surtout utilisés pour l’édition et le runtime, pas pour les règles de connexion de base.

### 2.2. Connexions Exec

Deux représentations possibles selon le schéma :

- Soit **dans les nœuds** :
`execOut` = dictionnaire `nomDePin → [idNodeCible, ...]` (ex. `true : [^4], false : [^10]`).
- Soit **dans un tableau global** :
`execConnections[] = { fromNode, fromPin, toNode, toPin }`.

Le GVS travaille sur une représentation **normalisée** interne (voir §4) où les connexions Exec sont toutes dans une liste unifiée.

### 2.3. Connexions Data

Même principe :

- Dans les nœuds (ex. `dataIn` avec `sourceNode`/`sourcePin`), ou
- Dans `connections[] = { from:"node4.dataOut.distance", to:"node5.dataIn.left" }`.

La normalisation donne une liste `DataConnection` : `(fromNodeID, fromPinName, toNodeID, toPinName)` avec un type pour chaque pin.

***

## 3. Système de pins et “grammaire” de connexion

### 3.1. Catégories de pins

Le GVS distingue deux familles de pins :

- **Pins Exec**
    - Transportent le **flux d’exécution**.
    - Visuellement : liens épais blancs/bleu.
    - Cardinalité typique :
        - 1 `ExecIn` par node (sauf `EntryPoint` sans In).
        - 0..N `ExecOut` (ex. Branch true/false, Sequence then0..thenN, While loop/exit).
- **Pins Data**
    - Transportent les **valeurs typées** : `bool`, `int`, `float`, `string`, `vector2`, `vector3`, `entity_handle`.
    - Visuellement : liens fins colorés.


### 3.2. Règles génériques de connexion

#### Exec

- Autorisé : `ExecOut → ExecIn`.
- Interdit : `Exec → Data`, `Data → Exec`.
- `ExecIn` : max 1 connexion entrante (nouvelle connexion remplace l’ancienne si on le souhaite).
- `ExecOut` : 0..N connexions sortantes (fan‑out possible, configurable par type de node).


#### Data

- Autorisé : `DataOut → DataIn`.
- Interdit : `DataIn → DataIn`, `DataOut → DataOut`.
- `DataIn` : max 1 connexion entrante (sinon ambigu).
- `DataOut` : 0..N connexions sortantes (fan‑out).


#### Type checking

- Types strictement compatibles avec quelques conversions implicites autorisées :
    - `int → float` (cast implicite OK).
    - Type exact identique (bool→bool, string→string…).
- Toute autre combinaison est **refusée** au moment du drag \& drop et signalée comme **Erreur de validation** si trouvée dans un graphe sérialisé (ex. `string → float`).


#### Self‑connexion

- Interdit : `(node.id == target.id)` sur Exec ou Data, sauf pour les patterns de boucle explicitement autorisés (While/ForEach, cf. §5.2).

***

## 4. Architecture du système de vérification

### 4.1. Composants principaux

- **`BlueprintValidator`**
    - API de validation “offline” d’un `TaskGraphTemplate` (ou équivalent) et retour d’une liste de `ValidationError`.
- **`ValidationPanel`**
    - UI ImGui qui affiche les résultats, avec navigation vers le node, coloration, etc.
- **`VisualScriptEditorPanel`**
    - Appelle `BlueprintValidator` périodiquement (auto‑validation) et lors de certaines actions (save, run, etc.).
- **Système de métadonnées de node** (proposition)
    - Table décrivant, pour chaque `NodeType`, ses pins déclarés, leur catégorie (Exec/Data), direction (In/Out), type, cardinalité, NodeCategory, propriétés requises.


### 4.2. Représentation normalisée interne

À la réception du graphe JSON ou des structures C++ :

1. **Normalisation des nœuds**
    - Chaque node est converti dans une structure interne `NodeInfo` :

```cpp
struct PinInfo {
    std::string name;
    enum class PinKind { ExecIn, ExecOut, DataIn, DataOut } kind;
    ValueType type; // pour data pins
    int maxConnectionsIn;  // côté destination
    int maxConnectionsOut; // côté source
};

struct NodeInfo {
    int32_t id;
    NodeType type;
    NodeCategory category; // Entry, Flow, Loop, Action, PureData
    std::vector<PinInfo> pins;
    json parameters; // toutes les propriétés
};
```

2. **Normalisation des connexions Exec / Data**
    - Liste `ExecConnection { fromNode, fromPin, toNode, toPin }`.
    - Liste `DataConnection { fromNode, fromPin, toNode, toPin }`.

Cette abstraction découple complètement la logique de validation du format JSON exact (v3, v4, v5, etc.).

***

## 5. Règles par catégorie de nœud

On introduit une **NodeCategory** pour capter la “grammaire” de haut niveau :

- `Entry` : `EntryPoint`.
- `Flow` : `Branch`, `Switch`, `Sequence`, `DoOnce`, `Delay`…
- `Loop` : `While`, `ForEach` (boucles explicites).
- `Action` : `GotoPosition`, `ChangeState`, `AttackIfClose`, `GetBBValue`, `SetBBValue`, `SubGraph`, etc.
- `PureData` : `Math`, conversions, nodes sans Exec.


### 5.1. Table de compatibilité Exec (From → To)

Règles simples :


| From \ To | Entry | Flow | Loop | Action | PureData |
| :-- | :-- | :-- | :-- | :-- | :-- |
| Entry | Non | Oui | Oui | Oui | Non |
| Flow | Non | Oui | Oui | Oui | Non |
| Loop | Non | Oui | Oui* | Oui | Non |
| Action | Non | Oui | Oui | Oui | Non |
| PureData | Non | Non | Non | Non | Non |

- `Loop → Loop (Oui*)` : uniquement si l’arc correspond à un **back‑edge de boucle autorisée** (While.loop → corps → retour While, ForEach.loopBody → corps → retour ForEach). Sinon, ce sera détecté comme cycle interdit (§6.3).

Cette table sert lors du drag \& drop ET dans la validation globale :

```cpp
bool CanConnectExec(NodeCategory fromCat, NodeCategory toCat, const NodeInfo& from, const NodeInfo& to);
```


***

## 6. Règles de validation globale

Le validateur parcourt le graphe en plusieurs passes et génère des `ValidationError` avec un `ValidationSeverity` (Critical, Error, Warning, Info).

### 6.1. Validation structurelle

1. **EntryPoint**
    - Compter les nodes de type `EntryPoint`.
    - Si 0 : `Critical` – “Missing EntryPoint”.
    - Si >1 : `Critical` – “Multiple EntryPoints”.
2. **Connexité Exec**
    - Lancer un DFS/BFS depuis l’EntryPoint sur le graphe Exec.
    - Tous les nodes non visités deviennent `Warning` – “Unreachable node (orphan)”.
3. **Pins Exec / Data et cardinalité**
    - Pour chaque `ExecIn`, compter les arcs entrants :
        - >1 → `Error` – “ExecIn has multiple sources (ambiguous flow)”.
    - Pour chaque `DataIn`, compter les arcs entrants :
        - >1 → `Error` – “DataIn has multiple sources”.
4. **Genre de pins**
    - Vérifier que chaque connexion respecte :
        - ExecOut ↔ ExecIn.
        - DataOut ↔ DataIn.
    - Sinon : `Error` – “Invalid connection type (Exec/Data mismatch)”.

### 6.2. Validation de types Data

Pour chaque `DataConnection` :

- Récupérer les types déclarés des pins source et cible (`ValueType`).
- Appliquer les règles de compatibilité :
    - OK :
        - même type,
        - ou `int → float`.
    - KO :
        - autres combinaisons, ex. `Float → Bool`, `String → vector2`.
- En cas de KO :
`Error` – “DataConnection type mismatch (sourceType → targetType)”, avec suggestion d’insérer un node de conversion.


### 6.3. Détection de cycles Exec

1. Construire le graphe orienté `(NodeID, arcs Exec)`.
2. Effectuer une recherche de cycles (DFS avec pile de récursion).
3. Pour chaque cycle détecté :
    - Si au moins un node du cycle a `NodeCategory == Loop` ET que la forme du cycle correspond au pattern attendu (ex. arcs sortant de `While.loop` et revenant sur `While`) → **autorisé**.
    - Sinon → `Error` ou `Warning` selon stratégie :
        - `Error` – “Circular ExecFlow (potential infinite loop)”.

Implementation hint :
Tu peux marquer les arcs comme “loopEdge” quand ils partent d’un pin `loop` ou `loopBody`. Lors de la vérification de cycle, si tous les arcs du cycle sont marqués comme loopEdge et le cycle passe bien par le node Loop d’origine, tu le considères valide.

### 6.4. SubGraphs (si activés)

Basé sur le système SubGraph Phase 8 :

- Parcourir récursivement les références `SubGraphPath`/`SubGraphUUID` de tous les nodes `SubGraph`.
- Maintenir un `SubGraphCallStack` pour suivre les fichiers déjà visités.
- Règles :
    - Si on tente de recharger un graphe déjà présent dans le stack → `Critical` – “Circular SubGraph dependency (A → B → A)”.
    - Si la profondeur dépasse `MAX_SUBGRAPH_DEPTH` (4 par défaut) → `Critical` – “Max SubGraph recursion depth exceeded (4)”.

On peut choisir de faire cette validation **offline cross-assets** (analyse de tous les .ats) ou “lazy” à la demande.

### 6.5. Blackboard et propriétés requises

1. **Blackboard keys**
    - Pour chaque node qui référence une clé BB (`GetBBValue`, `SetBBValue`, conditions `CompareValue` avec `value:"local:Health"`, etc.), vérifier l’existence de la clé dans un référentiel :
        - soit la liste `blackboardRefs` du graphe,
        - soit un registre global de clés ECS (AIState, AIBlackboard, etc.).
    - Si inconnue → `Warning` – “Unknown BB key (typo?)”.
2. **Propriétés requises**
    - Pour chaque type de node, on définit une liste d’attributes obligatoires côté `parameters` (ex. `conditionType` pour Branch/While, `duration` pour Delay, `newState` pour ChangeState…).
    - Si manquants et non couverts par des DataPins →
`Error` – “Missing required property 'X' on node Y”.

***

## 7. Validation interactive lors de la création de liens

Dans `VisualScriptEditorPanel`, au moment où ImNodes signale `IsLinkCreated`, on passe par un validateur **local** qui applique un sous‑ensemble des règles :

```cpp
bool VisualScriptEditorPanel::CanCreateLink(PinID startPin, PinID endPin, std::string& outError);
```

Algorithme :

1. Résoudre `startPin` et `endPin` en `(node, pinInfo)`.
2. Vérifier la **catégorie de pin** (Exec/Data) et la direction (Out → In).
3. Vérifier la **cardinalité** (pas plus d’une connexion sur un In, selon paramètres).
4. Vérifier la **table de compatibilité Exec** ou le **type checking Data**.
5. Optionnel : effectuer un **test de cycle instantané** (pour éviter d’autoriser une connexion Exec qui boucle le graphe de manière non Loop).

Si tout est OK, la connexion est créée (et éventuellement les anciennes connexions sur le pin In sont supprimées). Sinon, on ignore la création et on affiche un tooltip / message dans la status bar.

***

## 8. API de haut niveau (proposition)

### 8.1. Structures d’erreur

Tu as déjà quelque chose de très proche dans ta doc avancée :

```cpp
enum class ValidationSeverity {
    Critical,  // Bloque exécution (EntryPoint manquant, SubGraph circulaire…)
    Error,     // Comportement runtime brisé (type mismatch, pins invalides…)
    Warning,   // Problèmes potentiels (nodes orphelins, While sans Exit…)
    Info       // Suggestions (optimisation, style de graphe…)
};

struct ValidationError {
    ValidationSeverity severity;
    int32_t nodeID;          // -1 si erreur de graphe globale
    std::string code;        // ex: "E001", "W002"
    std::string message;     // message lisible
    std::string suggestion;  // optionnel
};
```


### 8.2. Interface `BlueprintValidator`

```cpp
class BlueprintValidator {
public:
    std::vector<ValidationError> Validate(const TaskGraphTemplate& tmpl);
};
```

Passes internes (pseudo‑code) :

```cpp
std::vector<ValidationError> BlueprintValidator::Validate(const TaskGraphTemplate& tmpl)
{
    auto graph = Normalize(tmpl); // NodeInfo + Exec/DataConnection

    std::vector<ValidationError> errors;

    ValidateEntryPoint(graph, errors);
    ValidatePinConnections(graph, errors);
    ValidateTypes(graph, errors);
    ValidateExecCycles(graph, errors);
    ValidateSubgraphs(graph, errors);
    ValidateBlackboard(graph, errors);
    ValidateRequiredProperties(graph, errors);

    return errors;
}
```


***

## 9. Intégration éditeur

### 9.1. ValidationPanel

D’après ta doc, le `ValidationPanel` affiche en temps réel les erreurs et warnings, avec click‑to‑focus sur le node.

- Rafraîchissement auto toutes les X ms si `m_HasUnsavedChanges == true`.
- Bouton “Validate Now” pour forcer une passe complète.
- Click sur une erreur :
    - Fait appel à `BlueprintEditor::FocusNode(nodeID)` puis `VisualScriptEditorPanel::SetNodeHighlight(nodeID, color)`.


### 9.2. Points d’intégration recommandés

- **Après chaque action d’édition majeure** (ajout/suppression de node, création/suppression de lien) :
    - `MarkAsModified()` + flag interne pour validation différée.
- **Avant sauvegarde** (`Ctrl+S`) :
    - Forcer une validation; si `Critical` ou `Error` graves, proposer une confirmation ou bloquer la sauvegarde.
- **Avant exécution en runtime debug** (quand tu auras un “Play Graph”) :
    - Exiger 0 erreurs critiques.

***

## 10. Idées d’implémentation et extensions

### 10.1. Table de métadonnées de node

Pour éviter de coder en dur les règles par type, introduire un descripteur par `NodeType`:

```cpp
enum class NodeCategory { Entry, Flow, Loop, Action, PureData };

struct NodeTypeDescriptor {
    NodeType type;
    NodeCategory category;
    std::vector<PinInfo> pins;
    std::vector<std::string> requiredProperties;
    // Options spécifiques : autoriseCycleBackEdge, etc.
};
```

- Cette table peut être déclarée dans un fichier unique (`NodeTypesRegistry.cpp`) ou générée depuis un JSON de définition de nodes.
- Le validateur ne fait **que** consommer cette table, ce qui rend l’ajout de nouveaux nodes (nouvelle Action, nouveau FlowControl) trivial.


### 10.2. Automate de validation “multi‑passes”

Tu peux modéliser le GVS comme un petit automate d’états interne :

- `RawGraph` (après chargement JSON).
- `NormalizedGraph` (après migration de schéma, normalisation pins/connexions).
- `ValidatedGraph` (liste d’erreurs connue, classification par sévérité).

Chaque transition est une étape claire :

1. `Raw → Normalized` : géré par `TaskGraphLoader` / migrateurs de schéma.
2. `Normalized → Validated` : géré par `BlueprintValidator`.

### 10.3. Suggestions d’amélioration futures

- **Règles de style / best practices** (niveau Info) :
    - Plus de N nodes → conseiller de factoriser en SubGraph.
    - Trop de cross‑overs de liens sur le canvas → recommandation de ré‑organiser.
- **Validation de performance** (couplage avec Profiler) :
    - Marquer des nodes comme “hotspot potentiels” si leur profilage > X ms en moyenne.
- **Règles contextuelles** par type de jeu / IA :
    - Par exemple, interdire certaines Actions dans des contextes UI, etc.

***

Ce document te donne une base assez complète pour :

- Définir un **automate de validation** clair.
- L’implémenter proprement dans `BlueprintValidator` + `VisualScriptEditorPanel`.
- Étendre facilement tes règles au fur et à mesure que de nouveaux types de nodes ATS Visual Scripting apparaissent.


