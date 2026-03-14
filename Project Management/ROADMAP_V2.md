# Olympe Engine — Roadmap V2

**Date :** 2026-03-14 07:09:00
**Auteur :** @Atlasbruce
**Statut :** Planification active — mise a jour apres session Phase 20 complete + 4 initiatives documentees

---

## Vue d'ensemble

La Roadmap V2 definit les 4 initiatives majeures post-Phase 20 pour le Blueprint Editor et l'engine.

| Initiative | Titre | Priorite | Statut |
|---|---|---|---|
| A | Graph Verification System (GVS) | P0 | Phase 21 — EN COURS |
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

### Phase 21-A — VSGraphVerifier stateless (PROCHAINE)

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

### Phase 21-B — Panel Validation UI

Fonctionnalites :
- Panel scrollable listant tous les VSVerificationIssue avec icone couleur
- Click sur une issue -> selection du node fautif dans le canvas
- Badge rouge sur toolbar si des erreurs sont presentes
- Toggle Auto-verify : re-declenche Verify() apres chaque PushCommand

### Phase 21-C — Integration pre-save + pre-exec

Pre-save : Dialog de confirmation si erreurs presentes.
Pre-exec : Blocage si ERROR, avertissement log si WARNING seulement.

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
| 21-A | GVS — VSGraphVerifier | ~1 session |
| 21-B | GVS — Panel Validation UI | ~1 session |
| 21-C | GVS — Pre-save/Pre-exec | ~0.5 session |
| 22 | Font Awesome Icons | ~2 sessions (apres spec) |
| 23-A | AnimGraph | ~2 sessions |
| 23-B | LevelScript + Cinematic | ~2 sessions |
| 23-C | MenuGraph + GlobalRules | ~2 sessions |
| 24-A | Runtime Instance | ~2 sessions |
| 24-B | Debugger Multi-instances | ~2 sessions |
| 24-C | Thread safety | ~1 session |

---

**Last Updated**: 2026-03-14 07:09:00