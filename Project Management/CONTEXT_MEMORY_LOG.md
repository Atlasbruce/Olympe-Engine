---

## 2026-03-14 — SPEC Phase 21-D (Dynamic Pins Sequence/Switch)

**Demande par :** @Atlasbruce
**Action :** Specification et documentation de la Phase 21-D dans toute la base documentaire
**Priorite :** P0 — a implementer apres Phase 21-B

**Spec fonctionnelle validee :**
- Bouton [+] (tooltip "Add Execution Output") toujours positionne EN DESSOUS du dernier pin out exec
- Bouton [-] (tooltip "Remove Execution Output") inline par pin out exec supprimable
- Pin de base (Out / Case_0) non supprimable
- Suppression avec lien actif : lien retire automatiquement + RebuildLinks() appele
- AddExecPinCommand : pousse sur undo stack — Ctrl+Z retire le pin ajoute
- RemoveExecPinCommand : stocke (nodeID src, nodeID dst, pinID src, pinID dst) — Ctrl+Z restaure pin + lien
- Nodes concernes : VSSequence et VSSwitch

**Fichiers documentes dans ROADMAP_V2.md :**
- Source/BlueprintEditor/Commands/AddExecPinCommand.h/.cpp — NOUVEAU
- Source/BlueprintEditor/Commands/RemoveExecPinCommand.h/.cpp — NOUVEAU
- Tests/BlueprintEditor/Phase21DTest.cpp — NOUVEAU (8 tests)
- Source/BlueprintEditor/VisualScriptEditorPanel.cpp — RenderNode() modifie
- CMakeLists.txt — ajout cible OlympePhase21DTests

**Conformite :** C++14 strict, SYSTEM_LOG, namespace Olympe, pas d'emoji dans les logs