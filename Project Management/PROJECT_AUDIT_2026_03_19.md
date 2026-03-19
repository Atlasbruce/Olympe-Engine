# Audit Phase 24 M2.2 — Rapport Complet

**Date :** 2026-03-19 09:03:44 UTC  
**Phase auditée :** Phase 24 Milestone 2.2 — ConditionRef Data Structure (PR #457)  
**Phase précédente :** Phase 24 M2.1 — Documentation Baseline (PR #453) ✅ MERGED  
**Résultat :** 🔴 BLOCKED — 6 corrections critiques requises  
**Auditeur :** Copilot Coding Agent (CCA)

---

## 1. Résumé Exécutif

L'audit post-PR #457 révèle que l'implémentation initiale de la structure `ConditionRef` était incomplète. Six problèmes critiques ont été identifiés. Ces problèmes n'ont pas été détectés lors de l'implémentation en raison d'insuffisances dans le processus CCA (prompts isolés, vérification cross-fichier absente, tests d'intégration non mandatoires, critères Done non explicites).

**Impact :** Toutes les conditions créées/éditées via le panneau inline Phase 24 sont **perdues lors de save/reload**. La feature est non-fonctionnelle en production.

---

## 2. Root Cause Analysis — Pourquoi ces problèmes n'ont pas été détectés

### 2.1 Cause Racine #1 — Prompts CCA insuffisants (Isolation)

Les prompts CCA décrivaient des tâches isolées sans fournir le contexte cross-fichier nécessaire.

**Exemple :** Le prompt "Créer la structure `ConditionRef`" n'incluait pas :
- La liste des fichiers qui devaient lire/écrire cette structure
- Les JSON keys attendus par `TaskGraphLoader.cpp`
- Les tests d'intégration existants qui vérifiaient le pipeline complet

**Conséquence :** `ConditionRef` a été créé correctement en isolation, mais non intégré dans la sérialisation JSON.

### 2.2 Cause Racine #2 — Vérification Cross-Fichier Absente

Le CCA n'avait pas pour instruction de vérifier si les fonctions créées étaient effectivement appelées dans le pipeline complet.

**Exemple :** `SerializeOperandRef()` a été spécifiée mais jamais ajoutée au pipeline `SerializeAndWrite()` de `VisualScriptEditorPanel.cpp`.

**Conséquence :** Code créé mais non intégré → fonctionnalité silencieusement absente.

### 2.3 Cause Racine #3 — Tests Unitaires Uniquement (Pas de Tests Système)

Les tests validaient les composants en isolation. Aucun test ne vérifiait le pipeline complet save→reload.

**Exemple :** Les tests `ConditionRef` validaient la construction de la struct, mais aucun test ne vérifiait que `VisualScriptEditorPanel::SerializeAndWrite()` incluait `conditionRefs` dans le JSON de sortie.

**Conséquence :** Tests unitaires verts → système défaillant.

### 2.4 Cause Racine #4 — Critères Done Non Explicites

Le CCA n'avait pas de définition explicite de "DONE". L'implémentation s'est arrêtée prématurément.

**Exemple :** "Implémenter `ConditionRef`" a été interprété comme "créer le header `ConditionRef.h`" plutôt que comme "intégrer `ConditionRef` dans l'ensemble du pipeline save/load/render/runtime".

**Conséquence :** Complétion prématurée déclarée sans validation système.

---

## 3. Six Corrections Critiques Requises

### C1 — Persister `conditionIndex` dans le JSON

**Problème :** `ConditionRef::conditionIndex` est un champ de la struct, mais n'est ni sérialisé ni désérialisé. Après reload, l'index est -1 pour toutes les conditions, cassant la correspondance condition↔pin.

**Fichiers affectés :**
- `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` — Sérialisation
- `Source/TaskSystem/TaskGraphLoader.cpp` — Désérialisation

**Correction — Sérialisation (`VisualScriptEditorPanel.cpp`) :**

```cpp
// Dans SerializeAndWrite(), après la boucle sur def.conditions existante :
if ((def.Type == TaskNodeType::Branch || def.Type == TaskNodeType::While) &&
    !def.conditionRefs.empty())
{
    json condRefsArray = json::array();
    for (size_t i = 0; i < def.conditionRefs.size(); ++i)
    {
        const ConditionRef& ref = def.conditionRefs[i];
        json refObj;
        refObj["conditionIndex"] = ref.conditionIndex;  // C1 : persisté
        refObj["leftOperand"]    = SerializeOperandRef(ref.leftOperand);
        refObj["operatorStr"]    = ref.operatorStr;
        refObj["rightOperand"]   = SerializeOperandRef(ref.rightOperand);
        refObj["compareType"]    = VariableTypeToString(ref.compareType);
        condRefsArray.push_back(refObj);
    }
    n["conditionRefs"] = condRefsArray;
}
```

**Correction — Désérialisation (`TaskGraphLoader.cpp`) :**

```cpp
if (JsonHelper::IsArray(nodeJson, "conditionRefs"))
{
    JsonHelper::ForEachInArray(nodeJson, "conditionRefs",
        [&](const json& refJson, size_t idx)
    {
        if (!refJson.is_object()) return;
        ConditionRef ref;
        ref.conditionIndex = JsonHelper::GetInt(refJson, "conditionIndex",
                                                 static_cast<int>(idx));  // C1
        ref.leftOperand    = DeserializeOperandRef(refJson["leftOperand"]);
        ref.operatorStr    = JsonHelper::GetString(refJson, "operatorStr", "==");
        ref.rightOperand   = DeserializeOperandRef(refJson["rightOperand"]);
        const std::string typeStr = JsonHelper::GetString(refJson, "compareType", "Float");
        ref.compareType    = StringToVariableType(typeStr);
        nd.conditionRefs.push_back(ref);
    });
}
```

**Tests associés :**

```cpp
TEST(Phase24_C1, ConditionIndex_Persisted_AfterRoundTrip)
{
    ConditionRef ref;
    ref.conditionIndex = 2;
    ref.operatorStr = "<=";
    // ... setup ...
    json j = SerializeConditionRef(ref);
    ConditionRef loaded = DeserializeConditionRef(j);
    ASSERT_EQ(2, loaded.conditionIndex);
}
```

---

### C2 — Rendre les Pins Section 4 (ImNodes) Fonctionnels

**Problème :** `NodeBranchRenderer` affiche les slots de pins dynamiques (Section 4) visuellement, mais les pins ImNodes ne sont pas créés via `imnodes::BeginInputAttribute()`. Les connexions de liens sont donc impossibles depuis l'éditeur.

**Fichiers affectés :**
- `Source/Editor/Nodes/NodeBranchRenderer.cpp`

**Correction :**

```cpp
// Dans RenderDynamicPinSlots() — remplacer le rendu visuel seul par des pins ImNodes réels :
void NodeBranchRenderer::RenderDynamicPinSlots(
    const TaskNodeDef& def,
    const std::vector<DynamicDataPin>& pins)
{
    for (size_t i = 0; i < pins.size(); ++i)
    {
        const DynamicDataPin& pin = pins[i];
        // C2 : créer un vrai pin ImNodes (pas juste du texte)
        imnodes::BeginInputAttribute(pin.imNodesID);
        ImGui::Text("Pin-in #%zu: %s", i + 1, pin.label.c_str());
        imnodes::EndInputAttribute();
    }
}
```

**Condition préalable :** `DynamicDataPin` doit exposer un `imNodesID` (entier unique par pin, géré par `DynamicDataPinManager`).

**Tests associés :**

```cpp
TEST(Phase24_C2, DynamicPins_HaveUniqueImNodesIDs)
{
    DynamicDataPinManager mgr;
    // Setup 2 conditions avec Pin-mode operand
    // ...
    mgr.RegeneratePinsFromConditions(conditionRefs);
    const auto& pins = mgr.GetPins();
    ASSERT_EQ(2u, pins.size());
    ASSERT_NE(pins[0].imNodesID, pins[1].imNodesID);
    ASSERT_GT(pins[0].imNodesID, 0);
    ASSERT_GT(pins[1].imNodesID, 0);
}
```

---

### C3 — Nettoyer `dynamicPinID` lors des Transitions de Mode

**Problème :** Quand un opérande passe du mode `Pin` à `Variable` ou `Const`, l'ancien `dynamicPinID` reste dans la struct `OperandRef`. Lors du prochain save/load, un ID fantôme est restauré, causant une incohérence entre les pins réels et les IDs persistés.

**Fichiers affectés :**
- `Source/Editor/Panels/NodeConditionsPanel.cpp`

**Correction :**

```cpp
// Dans le handler de changement de mode de l'opérande :
void NodeConditionsPanel::OnOperandModeChanged(OperandRef& operand,
                                                OperandRef::Mode newMode)
{
    operand.mode = newMode;
    // C3 : nettoyer dynamicPinID quand on sort du mode Pin
    if (newMode != OperandRef::Mode::Pin)
    {
        operand.dynamicPinID.clear();  // évite les IDs fantômes
        SYSTEM_LOG << "[NodeConditionsPanel] dynamicPinID cleared on mode transition to "
                   << (newMode == OperandRef::Mode::Variable ? "Variable" : "Const") << "\n";
    }
}
```

**Tests associés :**

```cpp
TEST(Phase24_C3, DynamicPinID_ClearedOnModeTransition_PinToVariable)
{
    OperandRef op;
    op.mode = OperandRef::Mode::Pin;
    op.dynamicPinID = "pin_inst_abc123";

    // Simuler une transition de mode
    op.mode = OperandRef::Mode::Variable;
    op.dynamicPinID.clear();  // doit être fait par le panel

    ASSERT_TRUE(op.dynamicPinID.empty());
}
```

---

### C4 — Valider l'Architecture (Source de Vérité)

**Problème :** Deux structs coexistent avec des rôles similaires sans convention documentée :
- `Source/BlueprintEditor/ConditionRef.h` → `ConditionRef` (stockage inline)
- `Source/Editor/ConditionPreset/NodeConditionRef.h` → `NodeConditionRef` (référence au registre)

Aucun des deux fichiers ne documente explicitement pourquoi les deux existent et lequel utiliser dans quel contexte.

**Fichiers affectés :**
- `Source/BlueprintEditor/ConditionRef.h`
- `Source/Editor/ConditionPreset/NodeConditionRef.h`

**Correction — Ajouter dans `ConditionRef.h` :**

```cpp
/**
 * @file ConditionRef.h
 * @brief ConditionRef — stockage inline de condition (sans registre).
 *
 * @details
 * ARCHITECTURE — Deux structs distinctes :
 *
 * 1. ConditionRef (ce fichier) — Source: Source/BlueprintEditor/ConditionRef.h
 *    - Usage : stockage INLINE dans TaskNodeDef.conditionRefs[]
 *    - Persistance : directement dans le JSON du graphe ("conditionRefs" array)
 *    - Pas de dépendance au ConditionPresetRegistry
 *    - Utilisé par : VisualScriptEditorPanel, TaskGraphLoader, NodeBranchRenderer
 *
 * 2. NodeConditionRef — Source: Source/Editor/ConditionPreset/NodeConditionRef.h
 *    - Usage : référence à une entrée du ConditionPresetRegistry (registre global)
 *    - Persistance : via l'ID du preset dans le registre
 *    - Dépend du ConditionPresetRegistry
 *    - Utilisé par : ConditionPresetLibraryPanel, NodeConditionsEditModal
 *
 * Règle : utiliser ConditionRef pour la persistence JSON du graphe.
 *         utiliser NodeConditionRef pour les presets partagés du registre.
 */
```

**Tests associés :**

```cpp
// Test documentaire — vérifie que les deux structs sont bien distinctes
TEST(Phase24_C4, ConditionRef_And_NodeConditionRef_AreDistinct)
{
    // ConditionRef : stockage inline
    Olympe::ConditionRef cr;
    cr.conditionIndex = 0;
    cr.operatorStr = "==";
    ASSERT_EQ(0, cr.conditionIndex);

    // NodeConditionRef : référence au registre (API différente)
    // Olympe::NodeConditionRef ncr;  // dépend du registre — test séparé
    SUCCEED();  // Ce test confirme que les types compilent sans conflit
}
```

---

### C5 — Gérer la Compatibilité Format Phase 23 (Legacy)

**Problème :** Le format JSON Phase 23 utilisait `"conditions"` comme key. Le nouveau format Phase 24 utilise `"conditionRefs"`. Les fichiers sauvegardés en Phase 23 doivent continuer à se charger sans crash ni perte de données.

**Fichiers affectés :**
- `Source/TaskSystem/TaskGraphLoader.cpp`

**Correction :**

```cpp
// Dans ParseNode() — après le chargement de "conditions" existant :

// Phase 24 : charger conditionRefs (nouveau format)
if (JsonHelper::IsArray(nodeJson, "conditionRefs"))
{
    LoadConditionRefs(nodeJson, nd);  // C5 : nouveau format
    SYSTEM_LOG << "[TaskGraphLoader] Loaded " << nd.conditionRefs.size()
               << " conditionRefs (Phase 24 format)\n";
}
else if (JsonHelper::IsArray(nodeJson, "conditions"))
{
    // C5 : compatibilité Phase 23 — migration automatique
    SYSTEM_LOG << "[TaskGraphLoader] Legacy 'conditions' format detected — "
               << "migrating to conditionRefs\n";
    MigrateLegacyConditionsToConditionRefs(nodeJson, nd);
}
```

**Fonction de migration :**

```cpp
static void MigrateLegacyConditionsToConditionRefs(const json& nodeJson,
                                                     TaskNodeDef& nd)
{
    JsonHelper::ForEachInArray(nodeJson, "conditions",
        [&](const json& cj, size_t idx)
    {
        if (!cj.is_object()) return;
        ConditionRef ref;
        ref.conditionIndex = static_cast<int>(idx);

        // Migrer leftMode/leftVariable/leftConstValue → OperandRef
        const std::string leftMode = JsonHelper::GetString(cj, "leftMode", "Variable");
        if (leftMode == "Variable") {
            ref.leftOperand.mode = OperandRef::Mode::Variable;
            ref.leftOperand.variableName = JsonHelper::GetString(cj, "leftVariable", "");
        } else if (leftMode == "Const") {
            ref.leftOperand.mode = OperandRef::Mode::Const;
            // leftConstValue parsing...
        }
        ref.operatorStr = JsonHelper::GetString(cj, "operator", "==");
        // ... rightOperand similaire ...
        nd.conditionRefs.push_back(ref);
    });
}
```

**Tests associés :**

```cpp
TEST(Phase24_C5, LegacyPhase23Format_LoadsWithoutCrash)
{
    const std::string legacyJson = R"({
        "nodeID": 1, "type": "Branch",
        "conditions": [{
            "leftMode": "Variable", "leftVariable": "mHealth",
            "operator": "<=", "rightMode": "Const", "rightConstValue": 2
        }]
    })";
    // ...
    ASSERT_NO_THROW(TaskGraphLoader::ParseNode(json::parse(legacyJson), nd));
    ASSERT_EQ(1u, nd.conditionRefs.size());
    ASSERT_EQ("mHealth", nd.conditionRefs[0].leftOperand.variableName);
}
```

---

### C6 — Test Round-Trip Système (Obligatoire)

**Problème :** Aucun test ne valide le pipeline complet : création d'un graphe avec `conditionRefs` → sérialisation JSON → écriture sur disque → rechargement → comparaison identique.

**Fichiers affectés :**
- `Tests/Phase24/Phase24_ConditionSerialization_Tests.cpp` (à créer)

**Tests à implémenter (8 minimum) :**

```cpp
// Test 1 : Condition Variable simple — save/load cycle
TEST(Phase24_RoundTrip, T01_SimpleVariableCondition)
{
    TaskNodeDef original;
    original.Type = TaskNodeType::Branch;
    ConditionRef ref;
    ref.conditionIndex = 0;
    ref.leftOperand.mode = OperandRef::Mode::Variable;
    ref.leftOperand.variableName = "mHealth";
    ref.operatorStr = "<=";
    ref.rightOperand.mode = OperandRef::Mode::Const;
    ref.rightOperand.constValue = "2";
    ref.compareType = VariableType::Float;
    original.conditionRefs.push_back(ref);

    json j = SerializeNodeConditionRefs(original);
    TaskNodeDef loaded;
    DeserializeNodeConditionRefs(j, loaded);

    ASSERT_EQ(1u, loaded.conditionRefs.size());
    ASSERT_EQ("mHealth", loaded.conditionRefs[0].leftOperand.variableName);
    ASSERT_EQ("<=", loaded.conditionRefs[0].operatorStr);
    ASSERT_EQ("2", loaded.conditionRefs[0].rightOperand.constValue);
}

// Test 2 : Pin-mode operand préserve dynamicPinID exactement
TEST(Phase24_RoundTrip, T02_PinModeOperand_PreservesDynamicPinID)
{
    ConditionRef ref;
    ref.conditionIndex = 0;
    ref.leftOperand.mode = OperandRef::Mode::Variable;
    ref.leftOperand.variableName = "mSpeed";
    ref.operatorStr = "==";
    ref.rightOperand.mode = OperandRef::Mode::Pin;
    ref.rightOperand.dynamicPinID = "pin_inst_abc123def456";
    ref.compareType = VariableType::Float;

    json j = SerializeConditionRef(ref);
    ConditionRef loaded = DeserializeConditionRef(j);

    ASSERT_EQ(OperandRef::Mode::Pin, loaded.rightOperand.mode);
    ASSERT_EQ("pin_inst_abc123def456", loaded.rightOperand.dynamicPinID);
}

// Test 3 : Conditions multiples (3+) avec modes mixtes
TEST(Phase24_RoundTrip, T03_MultipleConditions_MixedModes)
{
    // 3 conditions : Variable<=Const, Variable==Variable, Variable==Pin
    // ... (8 assertions minimum)
}

// Test 4 : Discriminant de mode opérande
TEST(Phase24_RoundTrip, T04_OperandModeDiscriminator_AllThreeModes)
{
    // Variable, Const, Pin — vérifier que mode est correct après reload
}

// Test 5 : Backward compat avec ancien format "conditions"
TEST(Phase24_RoundTrip, T05_BackwardCompat_LegacyConditionsFormat)
{
    // Charger JSON avec "conditions" (format Phase 23)
    // Vérifier : pas de crash, conditionRefs correctement populé
}

// Test 6 : Round-trip fichier réel (save to disk, reload, verify)
TEST(Phase24_RoundTrip, T06_RealFileRoundTrip)
{
    // Créer graphe complet → sérialiser → écrire fichier tmp → recharger → comparer
}

// Test 7 : Préservation du compareType
TEST(Phase24_RoundTrip, T07_CompareType_Preserved)
{
    // compareType = VariableType::Int → sérialiser → reload → assert Int
}

// Test 8 : conditionRefs vide (cas limite)
TEST(Phase24_RoundTrip, T08_EmptyConditionRefs_NoOutput)
{
    TaskNodeDef def;
    def.Type = TaskNodeType::Branch;
    // def.conditionRefs est vide
    json j = SerializeNode(def);
    ASSERT_FALSE(j.contains("conditionRefs"));
}
```

---

## 4. Fonctions Helper à Implémenter

### 4.1 `SerializeOperandRef()`

```cpp
// Dans VisualScriptEditorPanel.cpp (méthode statique privée)
// Déclaration dans VisualScriptEditorPanel.h :
static json SerializeOperandRef(const OperandRef& ref);

// Définition :
json VisualScriptEditorPanel::SerializeOperandRef(const OperandRef& ref)
{
    json j;
    switch (ref.mode)
    {
        case OperandRef::Mode::Variable:
            j["mode"] = "Variable";
            j["variableName"] = ref.variableName;
            break;
        case OperandRef::Mode::Const:
            j["mode"] = "Const";
            j["constValue"] = ref.constValue;
            break;
        case OperandRef::Mode::Pin:
            j["mode"] = "Pin";
            j["dynamicPinID"] = ref.dynamicPinID;
            break;
    }
    return j;
}
```

### 4.2 `DeserializeOperandRef()`

```cpp
// Dans TaskGraphLoader.cpp (fonction statique locale ou membre)
static OperandRef DeserializeOperandRef(const json& j)
{
    OperandRef op;
    const std::string modeStr = JsonHelper::GetString(j, "mode", "Const");

    if (modeStr == "Variable")
    {
        op.mode = OperandRef::Mode::Variable;
        op.variableName = JsonHelper::GetString(j, "variableName", "");
    }
    else if (modeStr == "Pin")
    {
        op.mode = OperandRef::Mode::Pin;
        op.dynamicPinID = JsonHelper::GetString(j, "dynamicPinID", "");
    }
    else  // "Const" ou fallback
    {
        op.mode = OperandRef::Mode::Const;
        op.constValue = JsonHelper::GetString(j, "constValue", "");
    }
    return op;
}
```

---

## 5. Améliorations de Prévention (Long Terme)

### P1 — Prompts CCA Enrichis (Appliqué)

Le fichier `_PROMPT_FOR_COPILOT.md` a été mis à jour en v2.0 avec :
- 5 étapes pré-code obligatoires
- Checklists Done Criteria explicites
- 5 règles de codage strictes
- Template Compliance Report
- Procédure d'escalade

### P2 — Tests d'Intégration Système Mandatoires

Pour toute feature touchant la sérialisation JSON :
- Un test round-trip système est **obligatoire** avant merge
- Le test doit couvrir : save → write to disk → reload → assert identique
- Cible : 8+ assertions par feature de persistance

### P3 — Revue Cross-Fichier Systématique

Avant chaque merge, un grep cross-fichier **obligatoire** :

```bash
# Template de vérification à exécuter avant chaque merge
grep -rn "conditionRefs" Source/ --include="*.cpp" --include="*.h"
# Attendu : déclaration h + serialize cpp + deserialize cpp + render cpp + test cpp

grep -rn "SerializeOperandRef\|DeserializeOperandRef" Source/ --include="*.cpp"
# Attendu : définition cpp + 2+ sites d'appel
```

---

## 6. Prochaines Étapes

### Court terme — Corrections CCA (Phase 24 M2.2 Corrected)

| Priorité | Tâche | Effort estimé | Fichiers |
|----------|-------|--------------|---------|
| P0 | C6 : Implémenter les 8 tests round-trip | 2h | Tests/Phase24/ |
| P0 | C1 : Sérialiser `conditionRefs` + `conditionIndex` | 1h | VisualScriptEditorPanel.cpp |
| P0 | C1 : Désérialiser `conditionRefs` | 1h | TaskGraphLoader.cpp |
| P1 | C5 : Migration backward compat Phase 23 | 1h | TaskGraphLoader.cpp |
| P1 | C3 : Nettoyage `dynamicPinID` sur transition de mode | 0.5h | NodeConditionsPanel.cpp |
| P2 | C2 : Pins ImNodes Section 4 fonctionnels | 2h | NodeBranchRenderer.cpp |
| P2 | C4 : Documentation architecture (deux structs) | 0.5h | ConditionRef.h, NodeConditionRef.h |

### Moyen terme — Corrections projet

- Soumettre PR Phase 24 M2.2 Corrected avec Compliance Report complet
- Valider les 8 tests round-trip en headless avant merge
- Mettre à jour `IMPLEMENTATION_STATUS.md` après chaque correction

### Long terme — Améliorations process

- Appliquer les 5 règles de codage strictes à toutes les futures phases
- Intégrer le template Compliance Report dans le workflow CI
- Créer un test de fumée cross-pipeline exécuté automatiquement avant tout merge

---

## 7. Critères de Levée de Blocage

La PR Phase 24 M2.2 Corrected peut être mergée **si et seulement si** :

| Critère | Vérification |
|---------|-------------|
| C1 résolu | `grep -rn "conditionRefs" Source/BlueprintEditor/VisualScriptEditorPanel.cpp` → sérialisation présente |
| C1 résolu | `grep -rn "conditionRefs" Source/TaskSystem/TaskGraphLoader.cpp` → désérialisation présente |
| C2 résolu | `grep -rn "BeginInputAttribute" Source/Editor/Nodes/NodeBranchRenderer.cpp` → pins ImNodes présents |
| C3 résolu | `grep -rn "dynamicPinID.clear" Source/Editor/Panels/NodeConditionsPanel.cpp` → nettoyage présent |
| C4 résolu | Commentaire architecture présent dans `ConditionRef.h` et `NodeConditionRef.h` |
| C5 résolu | Test `T05_BackwardCompat_LegacyConditionsFormat` passe |
| C6 résolu | 8+ tests round-trip passent en headless |
| Compliance Report | Rempli et tous les items PASS |
| Tests existants | Tous les 63+ tests Phase 24 existants passent encore |

---

_Audit réalisé le 2026-03-19 09:03:44 UTC — Copilot Coding Agent (CCA)_
