# Blueprint Editor - Fonctionnalités (DEPRECATED)

⚠️ **CE DOCUMENT EST OBSOLÈTE** ⚠️

**Raison** : Ce document décrit l'ancien système Behavior Tree v2 (NodeGraphPanel) qui est **déprécié depuis Phase 7**.

**👉 Consultez la nouvelle documentation** : [`Blueprint_Editor_User_Guide_v4.md`](Blueprint_Editor_User_Guide_v4.md)

---

## Redirection rapide

- **Créer un graphe** : [`Blueprint_Editor_User_Guide_v4.md#créer-un-graphe`](Blueprint_Editor_User_Guide_v4.md#créer-un-graphe)
- **Éditer un graphe** : [`Blueprint_Editor_User_Guide_v4.md#éditer-un-graphe`](Blueprint_Editor_User_Guide_v4.md#éditer-un-graphe)
- **Opérations CRUD** : [`Blueprint_Editor_User_Guide_v4.md#opérations-crud`](Blueprint_Editor_User_Guide_v4.md#opérations-crud)
- **Raccourcis clavier** : [`Blueprint_Editor_User_Guide_v4.md#raccourcis-clavier`](Blueprint_Editor_User_Guide_v4.md#raccourcis-clavier)

---

## Changements majeurs (v2 → v4)

| Aspect | BT v2 (Ancien) | VS v4 (Actuel) |
|--------|----------------|----------------|
| **Éditeur** | NodeGraphPanel | ✅ **VisualScriptEditorPanel** |
| **Format** | Schema v2 (nested data.data.nodes) | ✅ **Schema v4 (flat nodes array)** |
| **Extension** | `.json` | ✅ `.ats` (ou `.json` avec type explicite) |
| **Nœuds composites** | Selector, Sequence (implicite) | ✅ Sequence, Branch (explicite avec pins) |
| **Connexions** | children array (implicite) | ✅ **execConnections + dataConnections** (explicite) |
| **Variables** | Pas de Blackboard | ✅ **Blackboard intégré** (local + global) |
| **Flux de données** | Pas supporté | ✅ **Data pins typées** |
| **SubGraphs** | Pas supporté | ✅ **SubGraph nodes** (Phase 3) |
| **Débogage** | Visualisation basique | ✅ **Breakpoints F9, Step F10, Call stack** |

---

## Contenu historique (BT v2 - référence uniquement)

⚠️ **NodeGraphPanel est déprécié pour l'édition** mais conservé pour **BehaviorTreeDebugWindow** (visualisation runtime).

### Architecture Legacy

**Classes** :
- `NodeGraphPanel` : UI legacy (ImNodes)
- `BTNodeGraphManager` : Gestionnaire BT v2
- `NodeGraph` : Modèle de données BT

**Types de nœuds BT v2** :
- **Selector** : Composite (premier succès)
- **Sequence** : Composite (tous doivent réussir)
- **Action** : Feuille (tâche atomique)
- **Condition** : Feuille (test booléen)
- **Decorator** : Modificateur d'enfant
- **Comment** : Annotation

**Format JSON v2** :
```json
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "data": {
    "data": {
      "rootNodeId": 1,
      "nodes": [
        {
          "id": 1,
          "type": "Selector",
          "children": [2, 3]
        }
      ]
    }
  }
}
```

### Migration recommandée

Pour tous nouveaux projets : **Utiliser VisualScriptEditorPanel (v4)**

Pour projets existants :
1. Migrer BT v2 avec script Python : `Tools/migrate_bt_to_vs.py`
2. Ou utiliser `BTtoVSMigrator::Convert()` en C++
3. Fichiers v4 créés avec extension `.ats`

---

## Anciennes fonctionnalités (référence historique)

### Créer un nœud - BT v2 (OBSOLETE)

### Créer un nœud - BT v2 (OBSOLETE)

**Via menu contextuel** (NodeGraphPanel) :
1. Clic droit → Selector, Sequence, Action, Condition, Decorator
2. Nœud créé via `CreateNewNode()` :
   ```cpp
   int nodeId = graph->CreateNode(NodeType::Action, x, y, "Action");
   ```
3. ⚠️ **Ne plus utiliser** : Migrer vers VisualScriptEditorPanel

### Connexions BT v2 (OBSOLETE)

- Structure **hiérarchique** : parent → children array
- Connexions **implicites** (pas de pins visibles)
- Format JSON :
  ```json
  {
    "id": 1,
    "type": "Selector",
    "children": [2, 3]  // ← Connexions implicites
  }
  ```

---

**FIN DU DOCUMENT LEGACY**

**👉 Pour la documentation actuelle, consultez** : [`Blueprint_Editor_User_Guide_v4.md`](Blueprint_Editor_User_Guide_v4.md)


**Via Properties Panel (panneau droit)**
1. **Sélectionner** un nœud (simple clic)
2. Le panneau **Properties** affiche les champs éditables :
   - **Name** : Nom du nœud (InputText)
   - **Paramètres spécifiques** :
     - `AtomicTask` : TaskType (ex: "MoveToLocation", "Attack")
     - `Delay` : DelaySeconds (float)
     - `Branch/While` : ConditionID (clé Blackboard bool)
     - `GetBBValue/SetBBValue` : BBKey (nom de variable)
     - `SubGraph` : SubGraphPath (chemin du sous-graphe)
     - `MathOp` : Operator (+, -, *, /)
3. Modifications appliquées **en temps réel**
4. Flag `m_dirty` activé → nécessite sauvegarde

**Déplacement de nœuds** :
- **Glisser-déposer** sur le canvas
- Positions synchronisées avec `ImNodes::GetNodeEditorSpacePos()`
- Sauvegardées dans le JSON `position: {x, y}`
- **Snap-to-Grid** : Activer avec `Ctrl+G` (grille 16px par défaut)

**Breakpoints (Débogage)** :
- Cocher **"Breakpoint (F9)"** dans Properties
- Ou appuyer **F9** avec nœud sélectionné
- Pause l'exécution runtime au nœud

### Supprimer un nœud (Delete)

1. **Sélectionner** le nœud (clic simple)
2. Appuyer sur **Delete** ou **Del**
3. Le nœud est supprimé via `RemoveNode(nodeID)` :
   - Suppression du `VSEditorNode`
   - Suppression du `TaskNodeDefinition` dans le template
   - Suppression de toutes les connexions liées (exec + data)
   - Flag `m_dirty = true`

⚠️ **Attention** : Supprime aussi toutes les connexions attachées au nœud

---

## Opérations sur les connexions

### Types de connexions

L'ATS Visual Script v4 utilise **deux types** de connexions :

#### 1. **Exec Connections** (Flux d'exécution)
- **Visuel** : Lignes blanches/bleues épaisses
- **Pins** : Triangles (◀ input, ▶ output)
- **Rôle** : Définit l'ordre d'exécution des nœuds
- **Exemple** : EntryPoint → Branch → (True → Attack | False → Patrol)
- **Format JSON** :
```json
"execConnections": [
  {
    "fromNode": 1,
    "fromPin": "Out",
    "toNode": 2,
    "toPin": "In"
  }
]
```

#### 2. **Data Connections** (Flux de données)
- **Visuel** : Lignes bleues fines (IM_COL32(160, 160, 255, 200))
- **Pins** : Cercles colorés selon le type
- **Rôle** : Transmet des valeurs typées entre nœuds
- **Types supportés** : Bool, Int, Float, String, Vector2/3, EntityID
- **Exemple** : GetBBValue("speed") → MathOp(*, 2.0) → SetBBValue("finalSpeed")
- **Format JSON** :
```json
"dataConnections": [
  {
    "fromNode": 3,
    "fromPin": "Value",
    "toNode": 4,
    "toPin": "InputA"
  }
]
```

### Créer une connexion

**Étapes** :
1. **Cliquer** sur un **pin de sortie** (droite du nœud source)
2. **Glisser** vers un **pin d'entrée** (gauche du nœud destination)
3. **Relâcher** pour créer
4. Validation automatique :
   - Exec → Exec : Toujours valide
   - Data → Data : Vérifie compatibilité des types
   - Détecte les cycles interdits

**Code** :
```cpp
// NodeGraphPanel.cpp - Legacy BT v2
void NodeGraphPanel::CreateNewNode(const char* nodeType, float screenX, float screenY)
{
    NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
    ImVec2 canvasPos = ScreenSpaceToGridSpace(ImVec2(screenX, screenY));

    // Via BlueprintAdapter + CommandStack
    Blueprint::CommandStack* stack = BlueprintEditor::Get().GetCommandStack();
    int graphId = NodeGraphManager::Get().GetActiveGraphId();
    NodeGraphShared::BlueprintAdapter adapter(stack, graphId);
    int createdId = adapter.CreateNode(nodeType, canvasPos.x, canvasPos.y, nodeType);
}
```

**Nœuds BT v2** : Selector, Sequence, Action, Condition, Decorator, Comment

### Connexions BT v2 (OBSOLETE)

Structure **hiérarchique** (children array), pas de pins visuelles distinctes.

---

**👉 POUR LA DOCUMENTATION ACTUELLE, CONSULTEZ** : [`Blueprint_Editor_User_Guide_v4.md`](Blueprint_Editor_User_Guide_v4.md)

**Dernière mise à jour** : Mars 2026  
**Status** : DEPRECATED (remplacé par v4)  
**Auteur** : Olympe Engine Team

