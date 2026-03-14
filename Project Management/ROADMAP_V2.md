# Olympe Engine — Roadmap V2

**Date :** 2026-03-14 07:09:00
**Auteur :** @Atlasbruce
**Statut :** Planification active — mise a jour apres session Phase 20 complete + 4 initiatives documentees + Initiative E ajoutee (Dynamic Pins Sequence/Switch)

---

## Vue d'ensemble

La Roadmap V2 definit les 5 initiatives majeures post-Phase 20 pour le Blueprint Editor et l'engine.

| Initiative | Titre | Priorite | Statut |
|---|---|---|---|
| A | Graph Verification System (GVS) | P0 | Phase 21 — EN COURS |
| E | Dynamic Pins Sequence/Switch — Add [+] / Remove [-] + Undo/Redo | P0 | Phase 21-D — PRIORITAIRE |
| B | Design & Icons Font Awesome | P1 | En attente spec design |
| C | Diversification Types de Graphes | P2 | Phase 23+ |
| D | Runtime Execution & Debugger | P3 | Phase 24+ |

---

## Initiative A — Graph Verification System (GVS)

### Contexte

Le VSConnectionValidator actuel valide uniquement les connexions individuelles (self-loop, duplicate pin, cycle DFS).
L'Initiative A implemente un validateur/compilateur global de graphe capable d'analyser la totalite
du graphe et de produire des messages d'erreur/warning exploitables dans l'editeur.

### Spec de reference

Fournie par @Atlasbruce le 2026-03-14.

### Phase 21-A — VSGraphVerifier stateless (TERMINEE — PR #380 mergee)

Nouveaux fichiers :
- Source/BlueprintEditor/VSGraphVerifier.h
- Source/BlueprintEditor/VSGraphVerifier.cpp
- Tests/BlueprintEditor/Phase21ATest.cpp
- Mise a jour CMakeLists.txt

14 regles implementees :

| ID | Severite | Regle |
|---|---|---|
| E001 | ERROR | Un seul EntryPoint requis |
| E002 | ERROR | Noeud dangling |
| E003 | ERROR | Cycle exec detecte (DFS) |
| E004 | ERROR | SubGraph circulaire |
| E005 | ERROR | Connexion Exec-Data ou Data-Exec |
| E006 | ERROR | Types de data pins incompatibles |
| E007 | ERROR | Direction de pin inversee |
| E008 | ERROR | Cle Blackboard inconnue |
| E009 | ERROR | Type Blackboard incompatible |
| W001 | WARNING | AtomicTask avec AtomicTaskID vide |
| W002 | WARNING | Delay avec DelaySeconds <= 0 |
| W003 | WARNING | SubGraph avec SubGraphPath vide |
| W004 | WARNING | MathOp avec MathOperator vide |
| I001 | INFO | Noeud non atteignable depuis EntryPoint |

### Phase 21-B — Panel Validation UI (EN COURS)

Fonctionnalites :
- Panel scrollable listant tous les VSVerificationIssue avec icone couleur
- Click sur une issue -> selection du node fautif dans le canvas
- Badge rouge sur toolbar si des erreurs sont presentes
- Toggle Auto-verify : re-declenche Verify() apres chaque PushCommand

### Phase 21-C — Integration pre-save + pre-exec

Pre-save : Dialog de confirmation si erreurs presentes.
Pre-exec : Blocage si ERROR, avertissement log si WARNING seulement.

---

## Initiative E — Dynamic Pins Sequence/Switch : Add [+] / Remove [-] + Undo/Redo (PRIORITAIRE)

### Contexte

Les nodes de type **VSSequence** et **VSSwitch** disposent actuellement d'un bouton [+] pour ajouter
des sorties exec (introduit Phase 20-C). Cette initiative etend et corrige le comportement
pour fournir une UX complete et un support Undo/Redo exhaustif.

### Spec fonctionnelle (fournie par @Atlasbruce le 2026-03-14)

**Regles UX :**

1. Le bouton **[+]** (tooltip : "Add Execution Output") doit TOUJOURS etre positionne EN DESSOUS
du dernier pin out exec sur le canvas (jamais au-dessus, jamais intercale).

2. Pour chaque pin out exec (au-dela du pin de base), afficher un bouton **[-]** inline
a droite du label du pin (tooltip : "Remove Execution Output").
   - Si le pin supprime possede un lien actif, le lien est retire automatiquement
     (suppression propre, pas de ghost link).
   - Le pin de base (Out / premier pin / Case_0) n'est PAS supprimable.

4. Toute action Add pin et Remove pin est couverte par le systeme Undo/Redo :
   - Add : AddExecPinCommand pousse sur l'undo stack — Ctrl+Z retire le pin ajoute.
   - Remove : RemoveExecPinCommand pousse sur l'undo stack — Ctrl+Z restaure le pin
     et son lien si existant.

### Nodes concernes

- **VSSequence** : pins Out, Out_1, Out_2, Out_N (N dynamique)
- **VSSwitch** : pins Case_0, Case_1, Case_N (N dynamique)

### Phase 21-D — Implementation Dynamic Pins Add/Remove (PROCHAINE apres 21-B)

#### Nouveaux fichiers prevus

- Source/BlueprintEditor/Commands/AddExecPinCommand.h
- Source/BlueprintEditor/Commands/AddExecPinCommand.cpp
- Source/BlueprintEditor/Commands/RemoveExecPinCommand.h
- Source/BlueprintEditor/Commands/RemoveExecPinCommand.cpp
- Tests/BlueprintEditor/Phase21DTest.cpp
- Mise a jour CMakeLists.txt

#### Fichiers modifies prevus

- Source/BlueprintEditor/VisualScriptEditorPanel.cpp :
  - RenderNode() ou equivalent : rendu bouton [+] apres les pins out exec (toujours en dernier)
  - RenderNode() : rendu bouton [-] par pin out exec supprimable, inline avec le label
  - Gestion suppression de lien associe lors de RemoveExecPin + RebuildLinks()
- Source/BlueprintEditor/VisualScriptEditorPanel.h : declarations si nouvelles methodes

#### Contraintes techniques

- C++14 strict : pas de structured bindings, pas de std::optional, pas de std::string_view.
- SYSTEM_LOG pour tous les logs (pas d'emoji dans les chaines).
- Undo/Redo : AddExecPinCommand et RemoveExecPinCommand heritent de IUndoRedoCommand.
- RemoveExecPinCommand : stocke l'etat du lien (nodeID src + nodeID dst + pin IDs) pour le restaurer au Undo.
- Pas de ghost links : RebuildLinks() appele apres chaque Remove.
- Ordre visuel : bouton [+] rendu EN DERNIER dans la liste des pins du node, jamais intercale.
- Tooltips ASCII : "Add Execution Output" et "Remove Execution Output".

#### Tests prevus (Phase21DTest.cpp)

| Test | Description |
|---|---|
| AddPin_Sequence_CreatesPin | Ajouter un pin sur VSSequence -> pin present dans le template |
| RemovePin_Sequence_RemovesPin | Supprimer un pin -> pin absent du template |
| RemovePin_WithLink_RemovesLink | Supprimer un pin avec lien -> lien absent du template |
| Undo_AddPin_RestoresState | Ctrl+Z apres Add -> pin disparu |
| Undo_RemovePin_RestoresPin | Ctrl+Z apres Remove -> pin restaure |
| Undo_RemovePin_WithLink_RestoresLink | Ctrl+Z apres Remove avec lien -> lien restaure |
| RedoAddPin | Ctrl+Y apres Undo Add -> pin re-ajoute |
| PinBaseNotRemovable | Le pin de base (Out / Case_0) ne peut pas etre supprime |

---

## Initiative B — Font Awesome Icons & Design

### Statut

En attente du document de specifications design de @Atlasbruce.

### Scope prevu

- Icones par type de node dans la palette et sur le canvas
- Icones pour les pins (exec = triangle, data = cercle colore)
- Icones dans le panel Properties et la toolbar
- Integration de la police FA comme texture ImGui (font .ttf embarquee, C++14 compatible)

---

## Initiative C — Diversification des Types de Graphes

Types de graphes prevus :

| Type | Description |
|---|---|
| AIGraph | Graphe comportemental AI (actuel) |
| AnimGraph | Transitions d animations |
| LevelScript | Scripts de niveau |
| CinematicGraph | Scenes cinematiques |
| MenuGraph | Definition de menus UI |
| GlobalRules | Regles de jeu globales |

Phases prevues :
- Phase 23-A : Infrastructure GraphType + AnimGraph
- Phase 23-B : LevelScript + CinematicGraph
- Phase 23-C : MenuGraph + GlobalRules

---

## Initiative D — Runtime Execution & Debugger Multi-Instances

Fonctionnalites prevues :
- Multi-instances : Plusieurs entites AI executent le meme graphe avec des Blackboards independants
- Visualisation read-only dans le Debugger avec node actif surbrillance par instance
- Breakpoints par instance
- Step-through

Phases prevues :
- Phase 24-A : GraphRuntimeInstance + GraphRuntimeManager (single-thread)
- Phase 24-B : Integration Debugger Panel multi-instances
- Phase 24-C : Thread safety + performance

---

## Calendrier indicatif

| Phase | Initiative | Estimation |
|---|---|---|
| 21-A | GVS — VSGraphVerifier | TERMINEE |
| 21-B | GVS — Panel Validation UI | ~1 session (EN COURS) |
| 21-C | GVS — Pre-save/Pre-exec | ~0.5 session |
| 21-D | Dynamic Pins Add[+]/Remove[-] Undo/Redo (Sequence/Switch) | ~1 session (PRIORITAIRE) |
| 22 | Font Awesome Icons | ~2 sessions (apres spec) |
| 23-A | AnimGraph | ~2 sessions |
| 23-B | LevelScript + Cinematic | ~2 sessions |
| 23-C | MenuGraph + GlobalRules | ~2 sessions |
| 24-A | Runtime Instance | ~2 sessions |
| 24-B | Debugger Multi-instances | ~2 sessions |
| 24-C | Thread safety | ~1 session |

---

**Last Updated**: 2026-03-14 12:00:00