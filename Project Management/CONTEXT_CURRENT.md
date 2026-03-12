# 🎯 CONTEXT CURRENT — Session Active

**Date**: 2026-03-12  
**User**: @Atlasbruce  
**Status**: 🔴 **CRITICAL BUG — Undo/Redo Non Fonctionnel (PR #361)**

---

## 🚨 **Urgence P0 : Fix Undo/Redo**

### **Situation Actuelle**
- ✅ PR #361 mergée (Delete Nodes/Links + Context Menus)
- ❌ **Undo/Redo (Ctrl+Z/Y) ne fonctionne PAS**
- ❌ **Positions de nodes ne sont PAS sauvegardées**

### **Root Cause (Diagnostiqué 2026-03-12 15:45)**
1. ❌ `MoveNodeCommand` n'est **jamais appelé** quand user déplace un node
2. ❌ `SyncNodePositionsFromImNodes()` lit `m_editorNodes.posX/Y` au lieu de `m_template.Nodes["__posX"]`
3. ❌ `DeleteNodeCommand::Undo()` ne restaure pas les links dans `m_editorLinks`

### **Impact**
- 🔴 **BLOQUANT** : User ne peut pas undo ses modifications de graph
- 🔴 **DATA LOSS** : Positions de nodes perdues après Save/Reload
- 🟡 **CRASH** : Undo après Delete peut crash si links non recréés

---

## 📋 **Prochaines Actions**

### **PR #362 : Fix Undo/Redo + Position Persistence** (URGENT)
**Scope** :
1. ✅ Track node moves via `MoveNodeCommand`  
   - Dans `RenderCanvas()`, détecter `ImNodes::GetNodeEditorSpacePos()` change  
   - Push `MoveNodeCommand(oldX, oldY, newX, newY)` avant update  

2. ✅ Fix Save/Load positions  
   - `SerializeAndWrite()` : écrire `__posX/__posY` dans `node.Parameters`  
   - `LoadTemplate()` : lire `__posX/__posY` depuis `node.Parameters`  

3. ✅ Fix `DeleteNodeCommand::Undo()`  
   - Restaurer les links dans `m_editorLinks` après restore node  

**Fichiers à Modifier** :
- `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` (RenderCanvas, SerializeAndWrite, LoadTemplate)
- `Source/BlueprintEditor/UndoRedoStack.cpp` (DeleteNodeCommand::Undo)

**Tests** :
- `Tests/BlueprintEditor/UndoRedoTest.cpp` (ajouter Test_MoveNode_Undo_Redo)
- `Tests/BlueprintEditor/Phase12Test.cpp` (ajouter Test_DeleteUndo_LinksRestored)

**Deadline** : 2026-03-12 EOD (priorité maximale)

---

## 🔄 **État des PRs**

| **PR** | **Status** | **Titre** | **Scope** |
|--------|-----------|----------|----------|
| #358 | ✅ Merged | Position Persistence (Partial) | Fix positions save (partiel, ré-ouvert en #362) |
| #359 | ✅ Merged | - | - |
| #360 | ✅ Merged | Memory System | `./Project Management/` files |
| #361 | ✅ Merged 🔴 **BROKEN** | Delete Nodes/Links + Context Menus | Delete key, F9 breakpoints |
| #362 | 🚧 **TODO NOW** | Fix Undo/Redo + Position Persistence | MoveNodeCommand tracking |
| #363 | 📅 Planned | Context Menus Polish | Font Awesome icons, Smart Editors |
| #364 | 📅 Planned | Font Awesome Full Integration | All menus + panels |

---

## 📊 **Métriques Sprint**

### **Sprint 1 (Phase 12 — Delete/Context Menus)**
- **Objectif** : Fix bugs P0 delete nodes/links + context menus
- **PRs** : #360 (Memory), #361 (Delete), #362 (Undo Fix)
- **Progress** : 66% (2/3 PRs merged, #362 en cours)
- **Blockers** : 🔴 Undo/Redo broken (fix en cours PR #362)

### **Sprint 2 (Phase 13 — Context Menus Polish)**
- **Objectif** : Smart editors (blackboard, enums) + Font Awesome icons
- **PRs** : #363 (Context Menus), #364 (Font Awesome)
- **Progress** : 0% (non démarré, bloqué par #362)
- **ETA** : 2026-03-13 (après fix #362)

---

## 🧠 **Contexte Technique**

### **Architecture Undo/Redo**
```cpp
// CORRECT (implémenté pour Add/Delete)
m_undoStack.PushCommand(
    std::unique_ptr<ICommand>(new AddNodeCommand(def)),
    m_template);

// BROKEN (non implémenté pour Move!)
// Dans RenderCanvas(), après drag :
ImVec2 pos = ImNodes::GetNodeEditorSpacePos(nodeID);
m_editorNodes[i].posX = pos.x;  // Direct update → pas d'undo!
```

### **Workflow Position Persistence**
1. **Drag** : User déplace node → ImNodes update position interne
2. **Sync** : `SyncNodePositionsFromImNodes()` copie ImNodes → `m_editorNodes.posX/Y`
3. **Save** : `SerializeAndWrite()` écrit `m_template.Nodes` → JSON
4. **PROBLEME** : `m_editorNodes.posX/Y` ≠ `m_template.Nodes["__posX"]` !

### **Fix Strategy**
- **Avant** : ImNodes → m_editorNodes → (perdu au save)
- **Après** : ImNodes → MoveNodeCommand → m_template.Parameters["__posX"] → JSON

---

## 📚 **Références**

- **Documentation UX** : `Documentation/OLYMPE_EDITOR_UX_GUIDELINES.md` (section 9.3)
- **Tests Existants** : `Tests/BlueprintEditor/UndoRedoTest.cpp` (10 tests, tous passent mais Move pas testé!)
- **Code Baseline** : PR #361 (commit `5b8327d9d2c87a04da740e81e9b96f35c1397fed`)

---

## ✅ **Checklist Avant Merge PR #362**

- [ ] `MoveNodeCommand` push dans `RenderCanvas()` quand position change
- [ ] `__posX/__posY` écrit dans `SerializeAndWrite()`
- [ ] `__posX/__posY` lu dans `LoadTemplate()`
- [ ] `DeleteNodeCommand::Undo()` restaure links
- [ ] Test `Test_MoveNode_Undo_Redo()` ajouté et passe
- [ ] Test `Test_DeleteUndo_LinksRestored()` ajouté et passe
- [ ] User test : Drag node → Ctrl+Z → position restaurée ✅
- [ ] User test : Drag node → Save → Reload → position persiste ✅

---

**Last Updated**: 2026-03-12 15:50 UTC  
**Next Review**: After PR #362 merge