# CONTEXT CURRENT — Session Active

**Date**: 2026-03-14
**User**: @Atlasbruce
**Status**: Phase 21-B (en cours) — Phase 21-D (specifiee, PRIORITAIRE)

---

## Developpement en Cours

- **Fonctionnalite actuelle :** Phase 21-A mergee (PR #380) — VSGraphVerifier 14 regles sur master
- **Objectif immediat :** Phase 21-B — Integration UI du verificateur de graphe dans l'editeur
- **Prochaine priorite :** Phase 21-D — Dynamic Pins Add[+]/Remove[-] + Undo/Redo (Sequence/Switch)
- **Blocages connus :** Aucun

---

## Composants Actifs

- **Modules touches :** BlueprintEditor
- **Fichiers modifies (Phase 21-B) :**
  - `Source/BlueprintEditor/VisualScriptEditorPanel.h` — ajout `m_verificationResult`, `m_verificationDone`, `m_focusNodeID`, `RunVerification()`, `RenderVerificationPanel()`
  - `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` — toolbar Verify button, panel issues, canvas node highlight rouge, invalidation sur mutations
  - `Tests/BlueprintEditor/Phase21BTest.cpp` — NOUVEAU : 3 tests UI-less
  - `CMakeLists.txt` — ajout cible OlympePhase21BTests
- **Dependencies :** `VSGraphVerifier.h` (Phase 21-A) — stable

---

## Decisions Recentes

- **2026-03-14 (Phase 21-A)** : PR #380 mergee — `VSGraphVerifier::Verify()` disponible avec 14 regles.
- **2026-03-14 (Phase 21-B)** : Integration UI en cours.
  - Toolbar : bouton `[Verify]` + badge colore (rouge/jaune/vert)
  - Panel : liste issues groupees E->W->I, bouton `[Go]` par issue
  - Canvas : nodes en erreur surlignes rouge (ImNodes color push)
  - Invalidation : `m_verificationDone = false` sur toutes mutations graphe
- **2026-03-14 (Phase 21-D)** : Spec validee par @Atlasbruce — Dynamic Pins Sequence/Switch.
  - Bouton [+] (tooltip "Add Execution Output") TOUJOURS EN DESSOUS du dernier pin out exec
  - Bouton [-] (tooltip "Remove Execution Output") inline par pin out exec supprimable
  - Pin de base (Out / Case_0) non supprimable
  - Suppression avec lien : lien retire automatiquement + RebuildLinks()
  - Undo/Redo : AddExecPinCommand + RemoveExecPinCommand (stocke lien pour restauration)
  - Nodes concernes : VSSequence et VSSwitch

---

## Notes Techniques Importantes

- **C++14 strict** : pas de structured bindings, std::optional, std::string_view.
- **SYSTEM_LOG** : tous les logs utilisent SYSTEM_LOG.
- **Two-phase** : invalidation decorrellee du rendu ImNodes
- **VSGraphVerifier** : stateless — `Verify(template)` sans contexte editeur
- **Phase 21-D** : RemoveExecPinCommand doit stocker (nodeID src, nodeID dst, pinID src, pinID dst) du lien pour restauration au Undo.
- **Phase 21-D** : Bouton [+] rendu EN DERNIER dans la boucle de rendu des pins du node (apres tous les pins out exec).

---

## Prochaines Etapes

1. Phase 21-B : merger PR UI (en cours agent Copilot)
2. **Phase 21-D** : Dynamic Pins Add[+]/Remove[-] Undo/Redo — Sequence + Switch (PRIORITAIRE)
3. Phase 21-C : blocage Save() si erreurs E-level
4. Phase 22 : Design & Icons Font Awesome
5. Phase 23 : Diversification types de graphes
6. Phase 24 : Runtime Execution & Debugger

---
