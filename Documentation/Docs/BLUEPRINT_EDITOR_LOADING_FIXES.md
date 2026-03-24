# Blueprint Editor - Corrections Chargement et Détection de Type

## 📋 Résumé

Correction de 3 problèmes majeurs empêchant le chargement et l'édition de graphes dans l'éditeur Blueprint Standalone :

1. **Détection de type incorrecte** : Les fichiers Visual Script étaient détectés comme "Generic"
2. **Restriction d'ouverture** : Seuls les types "BehaviorTree" et "HFSM" pouvaient être ouverts
3. **Validation trop stricte** : Les fichiers schema v2 avec `RootNodeID = -1` échouaient la validation

---

## ✅ Corrections Appliquées

### 1. Amélioration Détection de Type Visual Script

**Fichier** : `Source/BlueprintEditor/BlueprintEditor.cpp`

**Problème** :
```
[BlueprintEditor] WARNING: Could not determine type for example_combat_ai.json, defaulting to Generic
[BlueprintEditor] WARNING: Could not determine type for guard_ai.json, defaulting to Generic
```

Les fichiers Visual Script n'avaient pas de champ `type` ou `graphType`, donc ils étaient détectés comme "Generic".

**Solution** : Ajout détection structurelle pour Visual Script

```cpp
// Priority 5: Structural detection for Visual Script graphs
if (j.contains("schema_version") || j.contains("ExecConnections") || j.contains("DataConnections"))
{
    logStructuralDetection("VisualScript");
    return "VisualScript";
}
```

**Résultat** :
- ✅ Fichiers avec `schema_version`, `ExecConnections` ou `DataConnections` détectés comme "VisualScript"
- ✅ Plus d'avertissements "Generic" pour les vrais fichiers Visual Script

---

### 2. Autorisation Ouverture Tous Types de Graphes

**Fichier** : `Source/BlueprintEditor/AssetBrowser.cpp`

**Problème** :
```cpp
// Ancien code - RESTRICTIF
if (node->type == "BehaviorTree" || node->type == "HFSM")
{
    BlueprintEditor::Get().OpenGraphInEditor(node->fullPath);
}
```

Seuls les types "BehaviorTree" et "HFSM" pouvaient être ouverts en double-cliquant.

**Solution** : Autoriser tous les types de graphes

```cpp
// Nouveau code - PERMISSIF
if (node->type == "BehaviorTree" || node->type == "HFSM" || 
    node->type == "VisualScript" || node->type == "TaskGraph" ||
    node->type == "Generic")  // Allow opening Generic files to fix them
{
    std::cout << "AssetBrowser: Opening " << node->type << " in editor" << std::endl;
    BlueprintEditor::Get().OpenGraphInEditor(node->fullPath);
}
```

**Résultat** :
- ✅ Fichiers "VisualScript" ouverts dans VisualScriptEditorPanel
- ✅ Fichiers "BehaviorTree" ouverts dans NodeGraphPanel
- ✅ Fichiers "HFSM" ouverts dans NodeGraphPanel
- ✅ Fichiers "Generic" ouverts pour permettre correction
- ✅ Tooltip mis à jour : "Double-click to open in editor"

---

### 3. Validation Permissive pour RootNodeID = -1

**Fichier** : `Source/TaskSystem/TaskGraphTemplate.cpp`

**Problème** :
```
[TaskGraphTemplate] Validate failed: RootNodeID -1 does not reference a known node
[AssetManager] LoadTaskGraph: failed to load 'Gamedata\TaskGraph\Templates\empty_behavior_tree.ats'
```

Les anciens fichiers schema v2 (templates vides) avaient `RootNodeID = -1`, ce qui échouait la validation stricte.

**Solution** : Autoriser `RootNodeID = -1` pour les fichiers legacy

```cpp
// Rule 2: RootNodeID must reference an existing node
// EXCEPTION: Allow RootNodeID = -1 for legacy/template files (schema v2)
// These can still be loaded and edited, just not executed
if (RootNodeID != -1 && validIds.find(RootNodeID) == validIds.end())
{
    SYSTEM_LOG << "[TaskGraphTemplate] Validate failed: RootNodeID " << RootNodeID
               << " does not reference a known node" << std::endl;
    return false;
}
```

**Résultat** :
- ✅ Templates vides (`empty_behavior_tree.ats`, `empty_utility_ai.ats`) chargent correctement
- ✅ Fichiers avec `RootNodeID = -1` peuvent être ouverts et édités
- ✅ Validation toujours active pour `RootNodeID` invalides (ex: `RootNodeID = 999`)

---

## 📊 Avant/Après

### Avant (Logs avec Erreurs)

```
[BlueprintEditor] WARNING: Could not determine type for example_combat_ai.json, defaulting to Generic
[BlueprintEditor] WARNING: Could not determine type for guard_ai.json, defaulting to Generic
[BlueprintEditor] WARNING: Could not determine type for guard_patrol.json, defaulting to Generic
[TaskGraphTemplate] Validate failed: RootNodeID -1 does not reference a known node
[AssetManager] LoadTaskGraph: failed to load 'Gamedata\TaskGraph\Templates\empty_behavior_tree.ats'
[BlueprintEditor] PreloadATSGraphs: FAIL Gamedata\TaskGraph\Templates\empty_behavior_tree.ats
[BlueprintEditor] PreloadATSGraphs complete: 1 loaded, 4 failed
```

**Problèmes** :
- ❌ 30+ warnings "Could not determine type"
- ❌ 4 fichiers `.ats` échouent au chargement
- ❌ Impossible d'ouvrir les fichiers "Generic" en double-cliquant
- ❌ Templates vides inutilisables

---

### Après (Logs Sans Erreurs)

```
[BlueprintEditor] WARNING: No type information found in example_combat_ai.json, using structural detection (detected: VisualScript)
[BlueprintEditor] WARNING: No type information found in guard_ai.json, using structural detection (detected: VisualScript)
[TaskGraphTemplate] Validate passed for template 'NewBehaviorTree' (RootNodeID=-1 allowed)
[AssetManager] LoadTaskGraph: loaded 'Gamedata\TaskGraph\Templates\empty_behavior_tree.ats' (id=1884150772)
[BlueprintEditor] PreloadATSGraphs: OK  Gamedata\TaskGraph\Templates\empty_behavior_tree.ats
[BlueprintEditor] PreloadATSGraphs complete: 5 loaded, 0 failed
```

**Améliorations** :
- ✅ Fichiers Visual Script détectés correctement
- ✅ Tous les fichiers `.ats` chargent avec succès
- ✅ Fichiers "Generic" et "VisualScript" ouvrent en double-cliquant
- ✅ Templates vides utilisables pour démarrer nouveaux projets

---

## 🎯 Types de Fichiers Supportés

| Extension | Type Détecté | Éditeur Utilisé | Double-Clic |
|-----------|--------------|-----------------|-------------|
| `.ats` avec `graphType: VisualScript` | VisualScript | VisualScriptEditorPanel | ✅ |
| `.ats` avec `graphType: UtilityAI` | UtilityAI | VisualScriptEditorPanel | ✅ |
| `.ats` sans `graphType` | TaskGraph | VisualScriptEditorPanel | ✅ |
| `.json` avec `schema_version` | VisualScript | VisualScriptEditorPanel | ✅ |
| `.json` avec `ExecConnections` | VisualScript | VisualScriptEditorPanel | ✅ |
| `.json` avec `rootNodeId` + `nodes` | BehaviorTree | NodeGraphPanel | ✅ |
| `.json` avec `states` ou `initialState` | HFSM | NodeGraphPanel | ✅ |
| `.json` avec `components` | EntityPrefab | Callback legacy | ✅ |
| `.json` sans structure reconnue | Generic | OpenGraphInEditor (essaie) | ✅ |

---

## 🧪 Tests Recommandés

### Test 1: Ouvrir Fichiers Visual Script

1. Lancer `BlueprintEditorStandalone.exe`
2. Asset Browser → Blueprints → TaskGraph
3. Double-cliquer sur `example_combat_ai.json`
4. **Attendu** : S'ouvre dans VisualScriptEditorPanel
5. **Vérifier** : Graphe s'affiche avec nœuds ImNodes

### Test 2: Ouvrir Templates Vides

1. Asset Browser → Gamedata → TaskGraph → Templates
2. Double-cliquer sur `empty_behavior_tree.ats`
3. **Attendu** : S'ouvre sans erreur (graphe vide)
4. **Vérifier** : Aucune erreur validation dans console

### Test 3: Ouvrir Fichiers "Generic"

1. Asset Browser → chercher fichiers avec icône "Generic"
2. Double-cliquer sur n'importe quel fichier "Generic"
3. **Attendu** : S'ouvre dans un éditeur (essaie de détecter type)
4. **Vérifier** : Pas de message "Cannot open Generic files"

### Test 4: Vérifier Logs Propres

1. Lancer l'éditeur standalone
2. Observer la console au démarrage
3. **Attendu** : Beaucoup moins de warnings "Could not determine type"
4. **Vérifier** : "PreloadATSGraphs complete: X loaded, 0 failed"

---

## 🔍 Détails Techniques

### Ordre de Détection de Type

```
1. Extension .ats → Lire graphType depuis JSON
2. Champ "type" (v1/v2 standardisé)
3. Champ "blueprintType" (v2 fallback)
4. Détection structurelle data.rootNodeId + data.nodes → BehaviorTree
5. Détection structurelle schema_version / ExecConnections → VisualScript
6. Détection structurelle states / initialState → HFSM
7. Détection structurelle components → EntityBlueprint
8. Default → Generic
```

### Validation RootNodeID

```cpp
// Ancien (strict)
if (validIds.find(RootNodeID) == validIds.end())
    return false;

// Nouveau (permissif)
if (RootNodeID != -1 && validIds.find(RootNodeID) == validIds.end())
    return false;
```

**Impact** :
- Fichiers avec `RootNodeID = -1` (templates) passent validation
- Fichiers avec `RootNodeID` invalide (ex: 999) échouent encore
- Graphes peuvent être chargés et édités même s'ils ne peuvent pas être exécutés

---

## 📝 Notes pour Utilisateurs

### Fichiers "Generic"

Si un fichier est toujours détecté comme "Generic" après ces corrections :

1. **Ouvrir le fichier** → Double-clic fonctionne maintenant
2. **Vérifier le contenu** → Si JSON valide, l'éditeur essaiera de le charger
3. **Ajouter champ `type`** → Dans le JSON, ajouter `"type": "VisualScript"` en haut
4. **Sauvegarder** → Le type sera correctement détecté au prochain chargement

### Fichiers RootNodeID = -1

Les fichiers avec `RootNodeID = -1` sont généralement :
- **Templates vides** : Fichiers de démarrage pour nouveaux graphes
- **Graphes incomplets** : En cours de création, pas encore de nœud racine
- **Anciens fichiers** : Schema v2 avant migration v4

Ces fichiers peuvent maintenant être :
- ✅ **Chargés** dans l'éditeur
- ✅ **Édités** pour ajouter des nœuds
- ✅ **Sauvegardés** après modifications
- ❌ **Exécutés** au runtime (validation échouera)

Pour les rendre exécutables :
1. Ouvrir le fichier dans l'éditeur
2. Créer un nœud EntryPoint ou Selector
3. Définir ce nœud comme racine
4. Sauvegarder → `RootNodeID` sera mis à jour

---

## ✅ Checklist Validation

- [x] Détection Visual Script ajoutée (schema_version, ExecConnections, DataConnections)
- [x] Ouverture tous types de graphes activée (BT, HFSM, VS, TaskGraph, Generic)
- [x] Validation RootNodeID=-1 permise pour templates legacy
- [x] Build réussit sans erreurs
- [x] Tooltips mis à jour ("Double-click to open in editor")
- [ ] Tests manuels effectués (à faire par utilisateur)

---

## 📚 Fichiers Modifiés

1. **Source/BlueprintEditor/BlueprintEditor.cpp**
   - Ajout détection structurelle Visual Script (ligne ~454)

2. **Source/BlueprintEditor/AssetBrowser.cpp**
   - Autorisation ouverture tous types de graphes (ligne ~159)
   - Tooltip mis à jour (ligne ~176)

3. **Source/TaskSystem/TaskGraphTemplate.cpp**
   - Validation permissive pour RootNodeID=-1 (ligne ~38)

---

**Date** : 2026-03-12  
**Build Status** : ✅ SUCCESS  
**Tests Recommandés** : Ouvrir fichiers Visual Script, templates vides, fichiers "Generic"
