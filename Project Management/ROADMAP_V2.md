# Olympe Engine — Roadmap V2

**Date :** 2026-03-14  
**Auteur :** @Atlasbruce  
**Statut :** Planification active — Phase 21-D specifiee (PRIORITAIRE)

---

## Vue d'ensemble

La Roadmap V2 definit les initiatives majeures post-Phase 20 pour le Blueprint Editor et l'engine.

| Initiative | Titre | Priorite | Statut |
|---|---|---|---|
| A | Graph Verification System (GVS) | P0 | Phase 21 — EN COURS |
| E | Dynamic Pins Sequence/Switch (UX) | P0 PRIORITAIRE | Phase 21-D — A IMPLEMENTER |
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

### Phase 21-A — VSGraphVerifier stateless (TERMINEE — PR #380)

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

## Initiative E — Dynamic Pins Sequence/Switch (UX) — P0 PRIORITAIRE

### Contexte

Les nodes VSSequence et VSSwitch doivent permettre a l'utilisateur d'ajouter et
retirer des pins exec out dynamiquement depuis le canvas, avec un rendu UX
coherent (bouton [+] en bas, [-] par pin), et une couverture Undo/Redo complete.

### Spec validee par @Atlasbruce — 2026-03-14

#### Comportement UX

| Element | Position | Tooltip |
|---|---|---|
| Bouton [+] | Toujours EN DESSOUS du dernier pin exec out | "Add Execution Output" |
| Bouton [-] | Inline a droite de chaque pin exec out (sauf pin de base) | "Remove Execution Output" |

**Regles :**
- Le pin de base (Out pour Sequence, Case_0 pour Switch) n'est PAS supprimable (pas de bouton [-])
- L'ajout cree un nouveau pin nomme automatiquement : Out_1, Out_2... (Sequence) ou Case_1, Case_2... (Switch)
- La suppression d'un pin connecte retire automatiquement la liaison (equivalente a DeleteLinkCommand)
- Apres add/remove : `RebuildLinks()` appele pour synchroniser ImNodes

#### Couverture Undo/Redo

| Action | Command | Undo | Redo |
|---|---|---|---|
| Add pin exec | `AddExecPinCommand` | Retire le pin ajoute + retire lien si cree | Re-ajoute le pin |
| Remove pin exec | `RemoveExecPinCommand` | Re-ajoute le pin + restaure lien si existait | Retire le pin + lien |

**RemoveExecPinCommand stocke :**
- `nodeID`, `pinName`, `pinIndex` (position dans la liste)
- `linkedTargetNodeID`, `linkedTargetPinName` (si pin avait un lien, -1 sinon)

#### Fichiers impactes

| Fichier | Modification |
|---|---|
| `Source/BlueprintEditor/VisualScriptNodeRenderer.h/.cpp` | Rendu [+] apres pins out, [-] inline par pin |
| `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` | Logique add/remove + appel commands |
| `Source/BlueprintEditor/UndoRedoStack.h/.cpp` | AddExecPinCommand + RemoveExecPinCommand |
| `Tests/BlueprintEditor/Phase21DTest.cpp` | NOUVEAU — tests headless |
| `CMakeLists.txt` | Ajout cible OlympePhase21DTests |

#### Tests requis (Phase 21-D)

1. Add pin sur Sequence -> pin cree, undoable, redoable
2. Remove pin sans lien -> pin retire, undoable, redoable
3. Remove pin avec lien -> pin retire + lien retire, undo restaure les deux
4. Add puis Remove -> etat initial restaure apres double undo
5. Pin de base non supprimable (pas de bouton [-])

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

| Phase | Initiative | Estimation | Statut |
|---|---|---|---|
| 21-A | GVS — VSGraphVerifier | ~1 session | TERMINEE (PR #380) |
| 21-B | GVS — Panel Validation UI | ~1 session | EN COURS |
| 21-C | GVS — Pre-save/Pre-exec | ~0.5 session | A venir |
| 21-D | Dynamic Pins Sequence/Switch | ~1 session | PRIORITAIRE — A implementer |
| 22 | Font Awesome Icons | ~2 sessions (apres spec) | En attente |
| 23-A | AnimGraph | ~2 sessions | Phase 23+ |
| 23-B | LevelScript + Cinematic | ~2 sessions | Phase 23+ |
| 23-C | MenuGraph + GlobalRules | ~2 sessions | Phase 23+ |
| 24-A | Runtime Instance | ~2 sessions | Phase 24+ |
| 24-B | Debugger Multi-instances | ~2 sessions | Phase 24+ |
| 24-C | Thread safety | ~1 session | Phase 24+ |

---

**Last Updated**: 2026-03-14 14:26:00