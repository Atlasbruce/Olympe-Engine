# Grammaire ATS Visual Scripting (VS)
## Guide de Syntaxe Visuelle et Patterns de Composition

**Version** : 1.0 - 09/03/2026  
**Auteur** : Mentor ATS VS  
**Objectif** : Formaliser la "syntaxe" des graphes ATS VS pour des comportements lisibles, maintainables et Blueprint-like.

---

## 1. Alphabet : Blocs de base

### 1.1 Nœuds Exec (Flux de contrôle)

| Type | Icône visuelle | Rôle | Pins Exec Out |
|------|---------------|------|---------------|
| **EntryPoint** | 🔵 | Point d'entrée (Event Tick) | 1 sortie |
| **Branch** | 🔀 | If/Else conditionnel | `true`, `false` |
| **Switch** | 🔀N | Dispatcher multi-cas | `case1`, `case2`, `default` |
| **Sequence** | ➡️➡️➡️ | Actions ordonnées | `then0`, `then1`, `then2` |
| **While** | 🔄 | Boucle conditionnelle | `loop`, `exit` |
| **ForEach** | 📋 | Itération liste | `loopBody`, `completed` |
| **DoOnce** | 🔒 | Exécution unique | 1 sortie |
| **Delay** | ⏱️ | Pause X secondes | `completed` |
| **Action** | ⚙️ | Tâche concrète | 1 sortie |
| **SubGraph** | 📁 | Appel sous-graphe | 1 sortie |

### 1.2 Nœuds Data (Préparation valeurs)

| Type | Icône | Rôle |
|------|-------|------|
| **GetBBValue** | 📖 | Lit Blackboard → Data pin |
| **SetBBValue** | 📝 | Data pin → Blackboard |
| **Math** | ➕➖✖️ | Calculs arithmétiques |
| **Const** | 🔢 | Littéraux (float, int, etc.) |

### 1.3 Couleurs des liens Data

| Type | Couleur | Exemples |
|------|---------|----------|
| Bool | 🔴 Rouge | true/false |
| Int | 🔵 Cyan | 42, -10 |
| Float | 🟢 Vert | 3.14, 0.5 |
| String | 🟣 Magenta | "Combat" |
| Vector | 🟡 Jaune | (100, 200, 0) |
| EntityRef | 🟣 Violet | EntityID(42) |

---

## 2. Règles de syntaxe G1-G15

Chaque règle s'accompagne d'un **exemple concret** avec schéma ASCII.

### G1 : Un seul EntryPoint par graphe

**✅ Correct**
```
EntryPoint ───► Action A ───► Action B
```

**❌ Incorrect**
```
EntryPoint1 ───► ...
EntryPoint2 ───► ...  ← Erreur !
```

**Exemple JSON** :
```json
{"id":1, "type":"EntryPoint", "execOut":[2]}
```

---

### G2 : Data ne bloque pas le flux Exec

**✅ Correct**
```
EntryPoint ───► Branch
         │
         └──► GetBBValue ───► Branch.dataIn
```

**❌ Incorrect**
```
EntryPoint ───► GetBBValue ───► Branch  ← Data ne doit pas être dans Exec !
```

---

### G3 : Exec → Exec, Data → Data

**✅ Correct**
```
EntryPoint ───► Branch ───► Action (Exec blanc)
         │
         └──► GetBBValue ───► Branch.left (Data vert)
```

---

### G4 : Condition dans Data + Branch

**✅ Correct** (préparation data + Branch simple)
```
EntryPoint ───► GetBBValue ───► Math ───► Branch
                              │         │
                              └─────────┘ (operator "<=")
```

**❌ Incorrect** (logique enfouie dans Action)
```
EntryPoint ───► Action("CheckDistanceAndMove")  ← Trop opaque !
```

**Exemple JSON Branch** :
```json
{
  "id":4, "type":"Branch",
  "conditionType":"CompareValue",
  "dataIn":[{"name":"left", "sourceNode":3}],
  "parameters":[{"operator":"<=", "right":60.0}]
}
```

---

### G5 : While → Stateful (multi-frame)

**✅ Correct** (pattern recommandé)
```
EntryPoint ───► Branch("IsLoopActive=true")
         │
         └──► Action("MoveOneStep") ───► Delay(0.1s)
                                    │
                                    └──► SetBBValue(IsLoopActive=true)
```

**❌ Dangereux** (boucle synchrone infinie)
```
EntryPoint ───► While("TargetDistance > 10") ───► Move  ← Freeze !
```

---

### G6 : ForEach corps court

**✅ Correct**
```
ForEach(PatrolRoute) ───► loopBody ───► MoveTo(currentItem)
                                    │
                                    └──► completed ───► SetBBValue("PatrolDone")
```

**Exemple JSON** :
```json
{
  "id":15, "type":"ForEach",
  "arrayKey":"local:PatrolRoute",
  "execOut":{"loopBody":[16], "completed":[20]}
}
```

---

### G7 : DoOnce près de la source événement

**✅ Correct**
```
Event("PlayerEnter") ───► DoOnce ───► PlaySound("Alert")
```

**❌ Incorrect**
```
EntryPoint ───► Action1 ───► Action2 ───► DoOnce  ← Trop tard !
```

---

### G8 : Pas de croisement Exec, utiliser Reroute

**✅ Correct**
```
EntryPoint ───► Branch ─────── Reroute ───► Action
         │                    │
         └──► Sequence ───────┘
```

---

### G9 : Branch simple (≤3 params)

**✅ Correct**
```
GetBBValue ───► Branch(operator: "≤", right:60)
```

**❌ Trop complexe**
```
Branch(Compare(GetBBValue("dist") * Math("dt") > 50 && HasLineOfSight))  ← SubGraph !
```

---

### G10 : BB vs Data pins

| Usage | Stockage | Exemple |
|-------|----------|---------|
| **État durable** | Blackboard | `CurrentState="Combat"` |
| **Frame local** | Data pins | `distance = BB.TargetPos - Entity.Pos` |
| **Global** | Global BB | `GamePhase="BossFight"` |

---

## 3. Patterns de composition avancés

### 3.1 Pattern IA “Patrol → Chase → Attack”

**Graphe principal `Guard_Main.vs`** :
```
EntryPoint ───► Switch(CurrentState)
            │
            ├──► Patrol ───► SubGraph(PatrolRoutine)
            │
            ├──► Chase ───► SubGraph(ChaseTarget)
            │
            └──► Attack ───► SubGraph(AttackRoutine)
```

**SubGraph `ChaseTarget.vs`** :
```
EntryPoint ───► GetBBValue(TargetDistance) ───► Branch(≤60?)
         │                                    │
         └──► MoveToGoal                      │
                                             │
                                             └──► true ───► SetBBValue(CurrentState="Attack")
```

---

### 3.2 Pattern Level Trigger “Porte intelligente”

```
Event("PlayerEnter") ───► DoOnce ───► Action(CloseDoor)
                                    │
                                    └──► Delay(2s) ───► Action(PlaySound("Lock"))
```

---

### 3.3 Pattern Game Rule “Wave Spawner”

```
Event("WaveTimer") ───► ForEach(WaveEnemies)
            │
            └──► loopBody ───► SpawnEntity(currentItem)
                             │
                             └──► completed ───► Delay(30s) ───► SetBBValue(NextWaveReady=true)
```

---

### 3.4 Pattern Cinématique “Dialog + Anim”

```
Event("TriggerCinema") ───► Sequence
               │
               ├──► then0 ───► Action(DisplayDialogue("Ligne1"))
               │
               ├──► then1 ───► Action(PlayAnim("Speak"))
               │
               └──► then2 ───► Delay(3s) ───► Action(HideDialogue)
```

---

## 4. Style Guide visuel

### 4.1 Layout conventionnel

```
GAUCHE ← ÉVÉNEMENTS / INPUTS          OUTPUTS → DROITE
         │
    ┌────┴────┐
    │ EntryPt │ ───► Logique principale ───► Actions finales
    └─────────┘
         │
    Data Prep ↑↓ (GetBBValue, Math)
```

### 4.2 Nommage recommandé

| Contexte | Bon nom | Mauvais nom |
|----------|---------|-------------|
| Action | `MoveToGoal` | `Action42` |
| Branch | `TargetClose?` | `IfThing` |
| BB Var | `isPlayerVisible` | `bool1` |
| SubGraph | `Guard_PatrolLoop` | `sub1` |

### 4.3 Signaux d’alarme (Refactoring)

| Symptôme | Action |
|----------|--------|
| >15 nœuds visibles | → 2-3 SubGraphs |
| 4+ Branch successifs | → Switch sur state BB |
| Duplication logique | → SubGraph commun |
| Branch avec >3 params | → Data nodes + simple Branch |
| While sans borne claire | → Pattern stateful G5 |

---

## 5. Checklist avant validation graphe

- [ ] **1 seul EntryPoint**
- [ ] **Tous nœuds atteignables** depuis EntryPoint
- [ ] **Pas de cycle Exec** (sauf While/ForEach explicites)
- [ ] **Types Data compatibles**
- [ ] **Clés BB déclarées** dans `blackboard`
- [ ] **Chaque SubGraph existe** (validation fichier)
- [ ] **Nœuds < 20** par graphe (sinon découper)
- [ ] **Lisibilité** : lecture gauche-droite, haut-bas

---

**Fin du guide - Utilise ces patterns comme un “langage visuel” pour que tes graphes restent clairs même avec 50+ nœuds !**
