# 📄 Feature Context — Phase 24 : Condition Preset System

> **Créé le :** 2026-03-17 13:37:54 UTC  
> **Dernière mise à jour :** 2026-03-17 13:37:54 UTC  
> **Statut :** 🟠 EN COURS — Phases 24.0–24.4 ✅ MERGED — Phase 24.5 NEXT  
> **PRs :** #422 ✅ #424 ✅ #426 ✅ #428 ✅ #432 ✅ — #430 🔶 DRAFT  
> **Références croisées :** [ARCHITECTURE.md](../Architecture/ARCHITECTURE.md) | [bug_status_phase24.md](../BugStatus/bug_status_phase24.md) | [PROJECT_STATUS.md](../PROJECT_STATUS.md)

---

## 1. 🎯 Objectifs Conceptuels

### 1.1 Problème Résolu

**Avant Phase 24 :** Les conditions dans les nœuds `NodeBranch` étaient stockées directement dans le nœud (embedded). Chaque nœud dupliquait ses propres définitions de conditions. Il était impossible de réutiliser une condition entre plusieurs nœuds, et toute modification impliquait d'éditer chaque nœud individuellement. De plus, les valeurs d'opérandes pouvaient provenir d'une variable Blackboard, d'une constante inline, ou d'un pin dynamique — mais cette distinction n'était pas formalisée dans le modèle de données.

**Après Phase 24 :** Les conditions sont des entités globales (des *presets*) stockées dans un registre projet (`condition_presets.json`). Les nœuds ne stockent plus que des *références* à ces presets (`NodeConditionRef`). Un même preset peut être réutilisé par N nœuds. Les opérandes sont typés (`Variable | Const | Pin`) et les pins dynamiques sont auto-générés à partir des refs.

### 1.2 Rationale Architecture

| Décision | Justification |
|---|---|
| Presets globaux vs embedded | Réutilisabilité, cohérence, édition centralisée |
| `NodeConditionRef` (ref par ID) | Découplage données/logique, serialization légère |
| UUID pour les pins dynamiques | Garantit l'unicité globale même en cas de copie de nœud |
| `ConditionPresetRegistry` singleton | Point d'entrée unique, chargement lazy, CRUD atomique |
| C++14 compliance | Contrainte moteur — pas de `std::optional`, `std::filesystem`, structured bindings |

### 1.3 Critères de Succès

- [x] `ConditionPreset` défini et sérialisable JSON (PR #422)
- [x] `ConditionPresetRegistry` avec CRUD + Load/Save (PR #422)
- [x] `NodeConditionRef` et `DynamicDataPin` définis (PR #422)
- [x] Panel global `ConditionPresetLibraryPanel` opérationnel (PR #424)
- [x] Dialog `ConditionPresetEditDialog` (Create + Edit modes) (PR #424)
- [x] `NodeConditionsPanel` — assignation de presets à un nœud (PR #432)
- [x] `DynamicDataPinManager` — génération automatique de pins (PR #432)
- [x] `NodeBranchRenderer` mis à jour pour le rendu Phase 24 (PR #432)
- [x] 31 tests Phase 24.0 passants
- [x] 18 tests Phase 24.1 passants
- [ ] Runtime Evaluation opérationnelle (Phase 24.5)
- [ ] Undo/Redo complet pour CRUD presets (Phase 24.6)

---

## 2. 🏗️ Architecture

### 2.1 Vue d'Ensemble

```
CONDITION PRESET SYSTEM (Phase 24)
│
├─ GLOBAL REGISTRY (ConditionPresetRegistry)
│  └─ condition_presets.json → map<id, ConditionPreset>
│     ├─ preset_001 : [bb_health] < [25.0]
│     ├─ preset_002 : [bb_speed] >= [Pin]
│     └─ ...
│
├─ GLOBAL UI (ConditionPresetLibraryPanel)
│  ├─ Liste tous les presets avec preview
│  ├─ [+] Add → ConditionPresetEditDialog (Create)
│  ├─ [✎] Edit → ConditionPresetEditDialog (Edit)
│  └─ [Dup] [Del] par preset
│
├─ NODE LAYER (NodeBranch)
│  ├─ conditions: NodeConditionRef[]
│  │  └─ { presetID, logicalOp, leftPinID, rightPinID }
│  └─ dynamicPins: DynamicDataPin[]
│     └─ { pinID (UUID), label, dataType, runtimeValue }
│
├─ NODE UI (NodeConditionsPanel)
│  └─ Sélectionne/assigne des presets au nœud sélectionné
│
└─ PIN MANAGER (DynamicDataPinManager)
   └─ Génère/supprime les DynamicDataPins en fonction des NodeConditionRef
```

### 2.2 Modèle de Données Complet

#### `Operand` (`Source/Editor/ConditionPreset/Operand.h`)

```cpp
enum class OperandMode { Variable, Const, Pin };

struct Operand {
    OperandMode mode;
    std::string value;  // key BB, valeur float, ou "" (Pin)
};
```

#### `ConditionPreset` (`Source/Editor/ConditionPreset/ConditionPreset.h`)

```cpp
enum class ComparisonOp { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };

struct ConditionPreset {
    std::string  id;    // UUID : "preset_001"
    std::string  name;  // "Health Below Threshold"
    Operand      left;
    ComparisonOp op;
    Operand      right;

    std::string GetPreview() const;  // "[bb_health] < [25.0]"
    bool NeedsLeftPin() const;
    bool NeedsRightPin() const;
    nlohmann::json ToJson() const;
    static ConditionPreset FromJson(const nlohmann::json&);
};
```

#### `NodeConditionRef` (`Source/Editor/ConditionPreset/NodeConditionRef.h`)

```cpp
struct NodeConditionRef {
    std::string presetID;    // référence ConditionPresetRegistry
    std::string logicalOp;   // "AND" | "OR"
    std::string leftPinID;   // UUID ou "" si opérande non-Pin
    std::string rightPinID;  // UUID ou "" si opérande non-Pin
};
```

#### `DynamicDataPin` (`Source/Editor/ConditionPreset/DynamicDataPin.h`)

```cpp
struct DynamicDataPin {
    std::string pinID;      // UUID global : "pin_inst_abc123"
    std::string label;      // "In #1L: [bb_health] < [25.0]"
    std::string dataType;   // "Float"
    float runtimeValue;     // résolu à l'évaluation (Phase 24.5)
};
```

#### `ConditionPresetRegistry` (`Source/Editor/ConditionPreset/ConditionPresetRegistry.h`)

```cpp
class ConditionPresetRegistry {
public:
    static ConditionPresetRegistry& Instance();
    bool Load(const std::string& path);
    bool Save(const std::string& path) const;
    void AddPreset(const ConditionPreset& p);
    bool UpdatePreset(const std::string& id, const ConditionPreset& p);
    bool DeletePreset(const std::string& id);
    const ConditionPreset* GetPreset(const std::string& id) const;
    std::vector<ConditionPreset> GetAllPresets() const;
};
```

---

## 3. 📦 PRs Mergées — Détail

### PR #422 — Phase 24.0 : Core Data Models

**Fichiers créés :**

| Fichier | Description |
|---|---|
| `Source/Editor/ConditionPreset/Operand.h/.cpp` | Struct `Operand` + `OperandMode` |
| `Source/Editor/ConditionPreset/ConditionPreset.h/.cpp` | Struct `ConditionPreset` + `ComparisonOp` + helpers JSON |
| `Source/Editor/ConditionPreset/ConditionPresetRegistry.h/.cpp` | Singleton registry + CRUD + Load/Save |
| `Source/Editor/ConditionPreset/NodeConditionRef.h/.cpp` | Struct `NodeConditionRef` |
| `Source/Editor/ConditionPreset/DynamicDataPin.h/.cpp` | Struct `DynamicDataPin` |

**Note JSON :** La clé de sérialisation de l'opérateur est `"op"` (écriture), avec acceptation de `"operator"` en lecture pour rétro-compatibilité.

**Tests :** 31 tests headless — `OlympePhase24_0_OperandTests`, `OlympePhase24_0_ConditionPresetTests`, `OlympePhase24_0_RegistryTests`, `OlympePhase24_0_DynamicDataPinTests`

---

### PR #424 — Phase 24.1 : Global Condition Preset Library UI

**Fichiers créés :**

| Fichier | Description |
|---|---|
| `Source/Editor/Panels/ConditionPresetLibraryPanel.h/.cpp` | Panel global de gestion des presets |
| `Source/Editor/Dialogs/ConditionPresetEditDialog.h/.cpp` | Dialog modale création/édition |

**Fonctionnalités `ConditionPresetLibraryPanel` :**
- Liste scrollable de tous les presets avec preview `[left] op [right]`
- Champ de recherche par nom
- Boutons `[+]` (Add), `[Dup]` (Duplicate), `[Del]` (Delete) par entrée
- Ouverture `ConditionPresetEditDialog` en mode Create ou Edit

**Fonctionnalités `ConditionPresetEditDialog` :**
- Mode Create : formulaire vierge, confirme → `AddPreset()`
- Mode Edit : formulaire pré-rempli, confirme → `UpdatePreset()`
- Sélection Left/Right operand : mode (Variable/Const/Pin) + valeur
- Sélection opérateur ComparisonOp
- Validation : nom non vide, opérandes cohérents

**Tests :** 18 tests headless — `OlympePhase24_1_LibraryPanelTests`, `OlympePhase24_1_EditDialogTests`

**Known issue :** Include path `ConditionPresetRegistry.h` manquant dans `ConditionPresetLibraryPanel.h` → PR #430 (fix en cours)

---

### PR #426 — Phase 24.2 : Documentation

- Mise à jour `Project Management/Architecture/ARCHITECTURE.md`
- Mise à jour `Project Management/PROJECT_STATUS.md`

---

### PR #428 — Phase 24.3 : Version Stamp System

- Système de version stamping et métadonnées de build
- Initialisation de la version à chaque démarrage de l'éditeur

---

### PR #432 — Phase 24.4 : NodeConditionsPanel, DynamicDataPinManager, NodeBranchRenderer

**Fichiers créés :**

| Fichier | Description |
|---|---|
| `Source/Editor/Panels/NodeConditionsPanel.h/.cpp` | Panel d'assignation de presets à un nœud |
| `Source/Editor/ConditionPreset/DynamicDataPinManager.h/.cpp` | Gestionnaire de pins dynamiques |
| `Source/Editor/Nodes/NodeBranchRenderer.h/.cpp` | Rendu canvas du nœud Branch (Phase 24) |

**Fonctionnalités `NodeConditionsPanel` :**
- Affiche les `NodeConditionRef[]` du nœud sélectionné
- Dropdown pour sélectionner un preset depuis le registry
- Sélection logicalOp (`AND` / `OR`)
- Affichage des pinIDs générés

**Fonctionnalités `DynamicDataPinManager` :**
- `RebuildPins(node, registry)` : régénère les `DynamicDataPin[]` depuis les `NodeConditionRef[]`
- Naming convention : `"In #<condIdx>L: <preview>"` / `"In #<condIdx>R: <preview>"`
- UUID stable : ne recrée un pin que si le `presetID` change (évite les reconnexions intempestives)

**Fonctionnalités `NodeBranchRenderer` :**
- Rendu ImNodes du nœud Branch avec les pins dynamiques
- Affiche la liste des conditions avec preview (nom du preset + opérateur)
- Intègre `DynamicDataPinManager::RebuildPins()` avant le rendu

**Tests :** `OlympePhase24_2_NodeConditionsPanelTests`, `OlympePhase24_3_DynamicDataPinManagerTests`, `OlympePhase24_4_NodeBranchRendererTests`

> ⚠️ **Post-merge build errors** — voir [bug_status_phase24.md](../BugStatus/bug_status_phase24.md)

---

### PR #430 — Phase 24.1 Include Fixes (DRAFT 🔶)

**Problème :** `ConditionPresetLibraryPanel.h` ne déclare pas l'include vers `ConditionPresetRegistry.h`, causant C2061 en cascade.

**Fix :**
```cpp
// ConditionPresetLibraryPanel.h — à ajouter
#include "../../Editor/ConditionPreset/ConditionPresetRegistry.h"
```

**Statut :** DRAFT — validation avant merge  
**Débloque :** Phase 24.5

---

## 4. 📐 Serialization

### Format `condition_presets.json`

```json
{
  "version": 1,
  "presets": [
    {
      "id": "preset_001",
      "name": "Health Below Threshold",
      "left":  { "mode": "Variable", "value": "bb_health" },
      "op":    "<",
      "right": { "mode": "Const",    "value": "25.0" }
    },
    {
      "id": "preset_002",
      "name": "Speed Check",
      "left":  { "mode": "Variable", "value": "bb_speed" },
      "op":    ">=",
      "right": { "mode": "Pin",      "value": "" }
    }
  ]
}
```

**Note rétro-compatibilité :** `FromJson()` accepte à la fois `"op"` et `"operator"` pour la clé de l'opérateur.

### Format NodeBranch JSON (Phase 24+)

```json
{
  "nodeType": "Branch",
  "nodeID": 42,
  "name": "My Branch",
  "breakpoint": false,
  "conditions": [
    {
      "presetID":   "preset_001",
      "logicalOp":  "AND",
      "leftPinID":  "",
      "rightPinID": ""
    },
    {
      "presetID":   "preset_002",
      "logicalOp":  "OR",
      "leftPinID":  "",
      "rightPinID": "pin_inst_abc123"
    }
  ],
  "dynamicPins": [
    {
      "pinID":    "pin_inst_abc123",
      "label":    "In #2R: [bb_speed] >= [Pin]",
      "dataType": "Float"
    }
  ]
}
```

---

## 5. 🔄 Migration Phase 23 → Phase 24

### Différences de Format

| Aspect | Phase 23 | Phase 24 |
|---|---|---|
| Stockage conditions | Embedded dans le nœud (`conditions[].type`, `.value`) | Registry global + refs (`conditions[].presetID`) |
| Pins | Statiques (définis à la création du nœud) | Dynamiques (auto-générés depuis les refs) |
| Édition conditions | Inline dans le panel Properties | Panel global `ConditionPresetLibraryPanel` |
| Réutilisabilité | Aucune | Preset partagé entre N nœuds |

### Stratégie de Migration

> ⚠️ Les blueprints Phase 23 ne sont **pas compatibles** avec le parser Phase 24 sans migration manuelle.

La migration n'est pas encore automatisée (Phase 24.8 final doc + futur outil de migration). Pour migrer manuellement :

1. Pour chaque nœud Branch avec des `conditions[]` de type Phase 23 :
   - Créer un `ConditionPreset` correspondant dans le registry
   - Remplacer `conditions[]` embedded par une `NodeConditionRef` pointant vers le preset
2. Supprimer les anciens champs `conditions[].type` / `conditions[].value`
3. Ajouter le tableau `dynamicPins[]` si des opérandes Pin sont utilisés

---

## 6. 🧪 Couverture Tests

| Cible CMake | Tests | Contenu |
|---|---|---|
| `OlympePhase24_0_OperandTests` | ~8 | Sérialisation `Operand`, modes, round-trip JSON |
| `OlympePhase24_0_ConditionPresetTests` | ~8 | `ConditionPreset` ToJson/FromJson, `GetPreview()`, `NeedsPin()` |
| `OlympePhase24_0_RegistryTests` | ~10 | CRUD registry, Load/Save, doublons, clé inconnue |
| `OlympePhase24_0_DynamicDataPinTests` | ~5 | Création pin, sérialisation, label format |
| `OlympePhase24_1_LibraryPanelTests` | ~9 | Panel headless : Add/Edit/Delete/Search presets |
| `OlympePhase24_1_EditDialogTests` | ~9 | Dialog headless : Create/Edit modes, validation |
| `OlympePhase24_2_NodeConditionsPanelTests` | TBD | Assignation preset→nœud, logicalOp, pinIDs |
| `OlympePhase24_3_DynamicDataPinManagerTests` | TBD | RebuildPins, UUID stable, naming convention |
| `OlympePhase24_4_NodeBranchRendererTests` | TBD | Rendu headless, pins générés, preview affiché |

**Commande de build tests Phase 24 :**
```bash
cmake -S . -B /tmp/build24 -DBUILD_TESTS=ON
make OlympePhase24_0_OperandTests \
     OlympePhase24_0_ConditionPresetTests \
     OlympePhase24_0_RegistryTests \
     OlympePhase24_0_DynamicDataPinTests \
     OlympePhase24_2_NodeConditionsPanelTests \
     OlympePhase24_3_DynamicDataPinManagerTests \
     OlympePhase24_4_NodeBranchRendererTests -j4
```

---

## 7. ⏭️ Phase 24.5 — Runtime Evaluation (NEXT)

**Objectif :** Évaluer les conditions depuis le `ConditionPresetRegistry` à l'exécution.

**Dépendances :**
- PR #430 mergé (unblock include path)
- `DynamicDataPinManager` opérationnel (PR #432 ✅)
- `ConditionPresetRegistry` chargé au démarrage (PR #422 ✅)

**Scope prévu :**

```cpp
// Nouveau fichier : Source/TaskSystem/ConditionPresetEvaluator.h/.cpp
class ConditionPresetEvaluator {
public:
    bool Evaluate(const NodeConditionRef& ref,
                  const ConditionPresetRegistry& registry,
                  const std::unordered_map<std::string, float>& pinValues,
                  const Blackboard& blackboard) const;
};
```

**Logique d'évaluation :**
1. Récupérer le `ConditionPreset` depuis `registry.GetPreset(ref.presetID)`
2. Résoudre `left` : Variable → lire Blackboard, Const → parser float, Pin → `pinValues[ref.leftPinID]`
3. Résoudre `right` : idem
4. Appliquer `ComparisonOp`
5. Combiner avec `ref.logicalOp` (AND/OR) sur le résultat précédent

**Module impacté :** `TaskGraphLoader`, `VSGraphVerifier` (règles E030–E032, W020–W021)

---

**Last Updated:** 2026-03-17 13:37:54 UTC
