# Phase 24.1 - Data Pin Stack-Based Recursive Evaluation System

## Résumé Exécutif

**Objectif**: Implémenter un système d'exécution par pile récursive pour les "nodes data pure" dans l'éditeur de Blueprint, permettant des réseaux de dépendances de données évalués automatiquement lors de l'exécution.

**Date d'implémentation**: 2026-03-20

**Composants clés**:
- `DataPinEvaluator.h / DataPinEvaluator.cpp` - Moteur d'évaluation récursive
- Intégration dans `VSGraphExecutor::ResolveDataPins()`

---

## Reformulation Détaillée de la Demande

### 1. **Nouveaux Types de Nodes "Data Pure"**

Les nodes "data pure" n'ont **pas de flux d'exécution** (pas d'exec pins). Ils possèdent uniquement des **data pins**:

#### Variable Node (GetBBValue)
```
[Variable: mHealth] ──┐
                      ├──> [Output: Value]
                      │
                  (No exec flow)
```
- **Pins entrants**: Aucun
- **Pins sortants**: `Value` (donnée)
- **Fonctionnalité**: Lit une variable blackboard et la propage en sortie

#### MathOp Node
```
[Literal/Pin: A] ──┐
                   ├──> [+, -, *, /, %] ──> [Output: Result]
[Literal/Pin: B] ──┘

              (No exec flow)
```
- **Pins entrants**: `A`, `B` (peuvent être Literal, Pin, ou Variable)
- **Pins sortants**: `Result` (donnée calculée)
- **Fonctionnalité**: Applique une opération arithmétique sur deux opérandes

#### SetBBValue Node
```
[Input: Data] ──> [SetBBValue: myVar] ──> [Output: Completed]
                          │
                    (exec flow)
```
- **Pins entrants**: `Value` (donnée)
- **Pins sortants**: Exec `Completed`
- **Fonctionnalité**: Reçoit une valeur de donnée et l'écrit dans le blackboard

### 2. **Connexions de Données**

Les pins-out data se connectent aux pins-in data via `DataPinConnection`:

```
Example graph:
SetBBValue (BBKey="mResult")
    └── In("Value") ← MathOp (op="+")
         ├── In("A") ← Variable (BBKey="mHealth")
         └── In("B") ← Variable (BBKey="mFoodPortion")
```

À l'exécution:
```
SetBBValue.Execute()
  ├─ Resolve In("Value") pin
  │   ├─ Traverse to MathOp node
  │   │   ├─ Resolve In("A") pin
  │   │   │   ├─ Traverse to Variable node
  │   │   │   └─ Read "mHealth" → 10
  │   │   ├─ Resolve In("B") pin
  │   │   │   ├─ Traverse to Variable node
  │   │   │   └─ Read "mFoodPortion" → 5
  │   │   └─ Compute 10 + 5 → 15
  │   └─ Result: 15
  └─ SetBBValue("mResult", 15)
```

### 3. **Exécution par Pile (Stack-Based Recursion)**

L'évaluation utilise une **pile d'appels récursive en profondeur d'abord (DFS)** pour:

1. **Résoudre les dépendances**: Avant de calculer une valeur, évaluer d'abord tous ses inputs
2. **Détection de cycles**: Maintenir un ensemble `visitedNodes` pour détecter les références circulaires
3. **Limite de profondeur**: `MAX_RECURSION_DEPTH = 32` pour éviter les débordements de pile
4. **Mise en cache**: Stocker les valeurs calculées dans `runner.DataPinCache` pour réutilisation

---

## Architecture Implémentée

### Class: `DataPinEvaluator`

```cpp
class DataPinEvaluator {
public:
    // Main entry point: evaluate a single output pin
    static DataPinEvaluationResult EvaluateDataPin(
        int32_t nodeID,
        const std::string& pinName,
        const TaskGraphTemplate& tmpl,
        TaskRunnerComponent& runner,
        LocalBlackboard& localBB);

    // Called by VSGraphExecutor: evaluate all input pins for a node
    static bool EvaluateNodeInputPins(
        int32_t nodeID,
        const TaskGraphTemplate& tmpl,
        TaskRunnerComponent& runner,
        LocalBlackboard& localBB);

private:
    // Recursive evaluation with cycle detection
    static DataPinEvaluationResult EvaluateDataPinRecursive(...);

    // Node type handlers
    static DataPinEvaluationResult HandleVariableNode(...);
    static DataPinEvaluationResult HandleMathOpNode(...);
    static DataPinEvaluationResult HandleGetBBValueNode(...);

    // Math operations
    static TaskValue ComputeMathOperation(...);
    static float TaskValueToFloat(const TaskValue& val);
    static TaskValue FloatToTaskValue(float result);
};
```

### Data Structures

```cpp
enum class DataPinEvalStatus {
    Success,        // Evaluation succeeded
    CycleDetected,  // Circular dependency found
    InvalidNode,    // Node ID not found
    EvaluationError // Runtime error (type mismatch, etc.)
};

struct DataPinEvaluationResult {
    DataPinEvalStatus Status = DataPinEvalStatus::Success;
    TaskValue         Value;  // Computed value
    std::string       ErrorMessage; // If Status != Success
};
```

---

## Flux d'Exécution

### Phase 1: ExecuteFrame (VSGraphExecutor)

```
For each step in frame:
    1. Get current node
    2. ResolveDataPins(nodeID)  <-- NEW: calls DataPinEvaluator
    3. Execute node based on type
    4. Determine next node
```

### Phase 2: ResolveDataPins (VSGraphExecutor)

**AVANT (simple cache lookup)**:
```cpp
// Old implementation: just copy from cache or use default
for (each DataPinConnection targeting this node):
    if (cache has source pin):
        cache[dest pin] = cache[source pin]
    else:
        cache[dest pin] = default_value
```

**APRÈS (recursive evaluation)**:
```cpp
// New implementation: delegate to DataPinEvaluator
void ResolveDataPins(...) {
    bool success = DataPinEvaluator::EvaluateNodeInputPins(
        nodeID, tmpl, runner, localBB);

    if (!success) {
        log warning about failed pins
    }
}
```

### Phase 3: DataPinEvaluator::EvaluateNodeInputPins

```
1. For each DataPinConnection targeting this node:
    a. Call EvaluateDataPin(SourceNodeID, SourcePinName)
    b. Store result in runner.DataPinCache[DestinationPin]
2. Return true if all successful
```

### Phase 4: DataPinEvaluator::EvaluateDataPinRecursive

**Algorithme**:
```
EvaluateDataPinRecursive(nodeID, pinName, ...):
    1. Check recursion depth limit
    2. Check visited set for cycles
    3. Add nodeID to visited set

    4. Check cache (optimization):
       If cached, return cached value

    5. Dispatch to node type handler:
       - Variable node → HandleVariableNode
       - MathOp node → HandleMathOpNode
       - GetBBValue node → HandleGetBBValueNode

    6. If successful, cache result
    7. Remove nodeID from visited
    8. Return result
```

### Phase 5: MathOp Node Evaluation (Exemple)

```
HandleMathOpNode(nodeID, ...):
    1. Get node and MathOpRef configuration

    2. Resolve Left Operand:
       if (mode == Literal):
           leftValue = literal_value
       else if (mode == Pin):
           Find incoming DataPinConnection for "A"
           Recursively evaluate source node
           leftValue = result
       else if (mode == Variable):
           leftValue = localBB.GetValue(variableName)

    3. Resolve Right Operand:
       (Same as left operand for pin "B")

    4. Compute result:
       result = ComputeMathOperation(operator, left, right)

    5. Return result
```

---

## Exemple Exécution Complète

### Graph Setup

```
Nodes:
- ID=1: Variable(BBKey="mHealth")
- ID=2: Variable(BBKey="mFoodPortion")
- ID=3: MathOp(op="+")
  - Left: Pin mode
  - Right: Pin mode
- ID=4: SetBBValue(BBKey="mResult")

DataPinConnections:
- (1, "Value") → (3, "A")     # Variable.mHealth → MathOp.A
- (2, "Value") → (3, "B")     # Variable.mFoodPortion → MathOp.B
- (3, "Result") → (4, "Value") # MathOp.Result → SetBBValue.Value

ExecPinConnections:
- (4, "Out") → (5, "In")  # SetBBValue → Next node

LocalBlackboard Values:
- mHealth = 10 (Int)
- mFoodPortion = 5 (Int)
```

### Execution Trace

```
ExecuteFrame():
  CurrentNodeID = 4 (SetBBValue)

  ResolveDataPins(4):
    → DataPinEvaluator::EvaluateNodeInputPins(4, ...)

    For DataPinConnection (3, "Result") → (4, "Value"):
      → EvaluateDataPin(3, "Result", ...)

        EvaluateDataPinRecursive(3, "Result", ..., depth=0):
          visited = {}
          visited.insert(3)

          Type = MathOp
          HandleMathOpNode(3, ...):

            # Resolve Left Operand (Pin mode)
            Find connection for (3, "A"):
              → Connection found: (1, "Value") → (3, "A")
              → EvaluateDataPin(1, "Value", ...)

                EvaluateDataPinRecursive(1, "Value", ..., depth=1):
                  visited.insert(1)
                  Type = GetBBValue
                  HandleVariableNode(1, ...):
                    return localBB.GetValue("mHealth")  # 10 (Int)
                  visited.erase(1)
                  cache[1:Value] = 10
                  return 10

              leftValue = 10

            # Resolve Right Operand (Pin mode)
            Find connection for (3, "B"):
              → Connection found: (2, "Value") → (3, "B")
              → EvaluateDataPin(2, "Value", ...)

                EvaluateDataPinRecursive(2, "Value", ..., depth=1):
                  visited.insert(2)
                  Type = GetBBValue
                  HandleVariableNode(2, ...):
                    return localBB.GetValue("mFoodPortion")  # 5 (Int)
                  visited.erase(2)
                  cache[2:Value] = 5
                  return 5

              rightValue = 5

            # Compute Result
            result = ComputeMathOperation("+", 10, 5)
              → TaskValueToFloat(10) = 10.0
              → TaskValueToFloat(5) = 5.0
              → 10.0 + 5.0 = 15.0
              → FloatToTaskValue(15.0) = TaskValue(15.0f)

            return 15.0f

          visited.erase(3)
          cache[3:Result] = 15.0f
          return 15.0f

      cache[4:Value] = 15.0f

    return true

  HandleSetBBValue(4):
    dataValue = cache[4:Value]  # 15.0f
    localBB.SetValue("mResult", 15.0f)
    return FindExecTarget(4, "Out", ...)  # Next node
```

---

## Gestion des Erreurs

### Cas d'Erreur Gérés

1. **InvalidNode**: Node ID n'existe pas dans le template
2. **CycleDetected**: Référence circulaire dans le réseau de data pins
3. **RecursionDepthExceeded**: Dépassement de `MAX_RECURSION_DEPTH`
4. **EvaluationError**: Erreur de type, division par zéro, etc.

### Gestion

```cpp
DataPinEvaluationResult result = EvaluateDataPin(...);

if (result.Status != DataPinEvalStatus::Success) {
    // Log error and use default value
    SYSTEM_LOG << "Error: " << result.ErrorMessage << "\n";
    // Cache remains empty, node uses default pin value
    return false;
}
```

---

## Optimisations

### 1. **Mise en Cache**
- Les valeurs calculées sont stockées dans `runner.DataPinCache`
- Réutilisation dans le même frame sans recalcul

### 2. **Cycle Detection Early Exit**
- Détection instantanée des références circulaires
- Pas d'évaluation sans fin

### 3. **Depth Limiting**
- `MAX_RECURSION_DEPTH = 32` prévient débordement de pile C++
- `MAX_EVALUATION_CHAIN_LENGTH = 256` prévient configurations pathologiques

### 4. **Type Conversion Unifiée**
- `TaskValueToFloat()`: Conversion transparente Bool/Int/Float
- Simplification des opérations arithmétiques

---

## Intégration avec Phase 24

### Phase 24.0: Condition Presets
- Presets stockés dans blueprint JSON (v4 schema)
- `TaskGraphTemplate::Presets` champ ajouté

### Phase 24.1: Data Pin Evaluation (CETTE IMPLÉMENTATION)
- Évaluation récursive des data pins
- Support pour nodes "data pure"
- Intégration transparente dans VSGraphExecutor

### Phase 24.X: Futures Améliorations
- Évaluation parallèle des dépendances indépendantes
- Optimisations de compilation pour graphes statiques
- Visual profiling des nodes coûteux

---

## Fichiers Affectés

### Nouveaux Fichiers
- `Source/TaskSystem/DataPinEvaluator.h` - Interface publique
- `Source/TaskSystem/DataPinEvaluator.cpp` - Implémentation

### Fichiers Modifiés
- `Source/TaskSystem/VSGraphExecutor.cpp` - Intégration, inclusion de DataPinEvaluator
- `Source/TaskSystem/VSGraphExecutor.h` - Documentation mise à jour

### Fichiers Non Modifiés (Dépendances)
- `TaskGraphTemplate.h/cpp` - Structures existantes utilisées
- `TaskGraphTypes.h/cpp` - Types existants réutilisés
- `LocalBlackboard.h/cpp` - Accès aux variables existant
- `TaskRunnerComponent.h/cpp` - Cache existant réutilisé

---

## Tests

### Cas de Test Critiques

1. **Simple Variable Read**
   - Setup: Variable node → SetBBValue
   - Expected: Valeur lue correctement

2. **MathOp avec Literals**
   - Setup: MathOp(5 + 3) → SetBBValue
   - Expected: Résultat = 8

3. **MathOp avec Pin Dependencies**
   - Setup: Variable + Variable → MathOp → SetBBValue
   - Expected: Calcul correct, valeur propagée

4. **Cycle Detection**
   - Setup: Node A → Node B → Node A (circular)
   - Expected: Erreur détectée, log warning

5. **Recursion Depth Limit**
   - Setup: Longue chaîne (>32 nœuds)
   - Expected: Erreur de profondeur, graceful failure

6. **Type Conversion**
   - Setup: Int operands + Float operands
   - Expected: Conversion correcte, résultat valide

7. **Cache Reuse**
   - Setup: Même pin évalué deux fois
   - Expected: Deuxième accès utilise cache

---

## Limitations et Restrictions

1. **Pas d'Effets Secondaires**: Les nodes data pure ne modifient pas l'état
2. **Types Supportés**: Bool, Int, Float (conversion unifiée)
3. **Opérateurs Supportés**: +, -, *, /, %
4. **Pas de Branchement**: Les data pins ne contrôlent pas le flux exec
5. **Evaluation Déterministe**: Même graphe → même résultat

---

## Conclusion

Cette implémentation de Phase 24.1 fournit un système d'exécution par pile récursive pour les réseaux de data pins dans les graphes Visual Script. Elle:

✅ Évalue automatiquement les dépendances de données
✅ Détecte et prévient les références circulaires
✅ Cache les résultats pour performance
✅ Supporte les types Bool, Int, Float
✅ Intègre transparemment dans VSGraphExecutor
✅ Reste C++14 compatible
✅ Permet l'évolution future vers évaluation parallèle

