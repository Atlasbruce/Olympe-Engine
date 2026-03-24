# Tutoriel ATS Visual Scripting : "The Matrimandir Breach"
## Implémentez le prototype d'infiltration du GDD

**Prérequis** : 
- [ ] Grammaire ATS VS lue [ATS-VS-GRAMMAIRE-GUIDE.md]
- [ ] Éditeur Blueprint ouvert (F2 pour toggle panels)
- [ ] Assets : GuardNPC prefab, Waypoints, Terminal trigger

**Durée estimée** : 45min  
**Objectif** : Créer 4 graphes fonctionnels du prototype infiltration.

---

## Étape 1 : Setup du niveau (5min)

### 1.1 Créer le niveau "Matrimandir"

1. **Nouveau niveau** : File → New Level → `MatrimandirBreach.json`
2. **Placer les entités** (drag depuis Assets Panel) :
   ```
   📍 Player (Gordon prefab)
   👮 3x GuardNPC (avec PatrolRoute assignée)
   📹 2x CameraStatic (rotation 90°)
   💻 TerminalSecurity (trigger zone)
   🚪 SecretTrapDoor (porte finale)
   🗺️ 8x Waypoints (carré patrouille)
   ```

### 1.2 Variables Blackboard Globales

Dans `globalblackboard.json` :
```json
{
  "GamePhase": {"type":"String", "default":"Patrol"},
  "bool_HasCode": {"type":"Bool", "default":false},
  "GlobalAlert": {"type":"Int", "default":0}
}
```

**✅ Checklist** : Toutes entités placées, Global BB configuré.

---

## Étape 2 : IA Garde - Patrouille + Détection (15min)

### 2.1 Créer `Guard_Main.vs` (State Machine)

**Objectif** : FSM Patrol → Suspicion → Alerte

1. **Nouveau VisualScript** : Right-click Assets → New VisualScript → `Guard_Main`
2. **Déclarer Blackboard local** :
```json
{
  "CurrentState": {"type":"String", "default":"Patrol"},
  "TargetActor": {"type":"EntityRef", "default":null},
  "TargetDistance": {"type":"Float", "default":9999},
  "isPlayerVisible": {"type":"Bool", "default":false},
  "PatrolRoute": {"type":"String", "default":"RouteA"}
}
```

3. **Construire le Switch principal** (G11) :
```
EntryPoint ───► Switch(CurrentState) (Règle G1)
            │
            ├── Patrol ───► SubGraph(PatrolLoop)    ← Créer après
            │
            ├── Suspicion ───► SubGraph(Investigate)
            │
            └── Alerte ───► SubGraph(ChasePlayer)
```

**JSON cible EntryPoint + Switch** :
```json
// Node 1: EntryPoint
{"id":1,"type":"EntryPoint","execOut":[2]}

// Node 2: Switch
{"id":2,"type":"Switch","key":"local:CurrentState",
 "execOut":{
   "Patrol":[10], "Suspicion":[20], "Alerte":[30], "default":[1]
 }}
```

### 2.2 SubGraph `PatrolLoop.vs`

**Schéma ASCII** :
```
EntryPoint ───► GetBBValue(PatrolRoute) ───► Action(GetNextWaypoint)
         │                                    │
         └──► Action(MoveToGoal) ───► Delay(0.5s)
                                    │
                                    └──► SetBBValue(PatrolIndex++)
```

**Étapes éditeur** :
1. **Nouvelle VisualScript** : `PatrolLoop`
2. **Drag nodes** dans l'ordre :
   - EntryPoint (auto-créé)
   - GetBBValue → `PatrolIndex` (Int)
   - Action → `GetNextWaypoint` (param `route:PatrolRoute`)
   - Action → `MoveToGoal` (speed:0.8)
   - Delay → 0.5s
   - SetBBValue → `PatrolIndex = PatrolIndex + 1`
3. **Connecter Exec** : Entry → Get → Action1 → Action2 → Delay → Set
4. **Sauvegarder** et **linker** depuis Guard_Main

**✅ Test** : Play scene → Garde suit waypoints en boucle.

### 2.3 SubGraph `Investigate.vs` (Suspicion)

```
EntryPoint ───► GetBBValue(LastKnownPos) ───► Action(MoveToGoal)
                                    │
                                    └──► Delay(3s) ───► SetBBValue(CurrentState="Patrol")
```

**Règle appliquée** : G7 (DoOnce près source), G10 (BB pour état durable).

---

## Étape 3 : Caméra Statique + Détection (10min)

### 3.1 `Camera_Rotating.vs`

**Fonction** : Rotation + FOV + Alerte si joueur >1.5s

**Schéma** :
```
EntryPoint ───► Action(RotateCamera) ───► GetBBValue(PlayerInFOV)
         │                                    │
         └──► Branch(duration > 1.5s?) ─── true ───► Action(TriggerGlobalAlert)
                                             │
                                             └── false ───► Delay(0.1s) ───► back to Entry
```

**Implémentation éditeur** :
1. **Variables locales** : `PlayerInFOV_time` (Float, default:0)
2. **Nodes** :
   - Action `RotateCameraLeftRight` (param: angle:90°)
   - GetBBValue `PlayerInFOV_time`
   - Branch `CompareValue > 1.5`
   - Action `SetGlobalAlert` (global:GlobalAlert +=1)
   - Delay 0.1s (tick fréquent)
3. **Boucle** : Delay.completed → EntryPoint (via Reroute)

**Règle** : G5 (stateful au lieu de While blocking).

**✅ Test** : Joueur dans FOV → barre détection monte → alerte après 1.5s.

---

## Étape 4 : Terminal + Trappe Finale (10min)

### 4.1 Trigger `TerminalSecurity.vs`

**Interaction 3s → bool_HasCode = true**

```
Event("OnPlayerInteract") ───► DoOnce ───► Sequence (Règle G7)
               │
               ├── then0 ───► Action(ShowProgressBar) ───► Delay(3s)
               │
               └── then1 ───► SetBBValue(global:bool_HasCode=true)
                              │
                              └──► Action(ShowCodeUI)
```

### 4.2 Trigger `SecretTrapDoor.vs`

```
Event("OnPlayerInteract") ───► GetBBValue(global:bool_HasCode)
                              │
                              └──► Branch(HasCode?)
                                    │
                                    ├── true ───► Action(OpenTrapDoor) ───► Action(LoadNextLevel)
                                    │
                                    └── false ───► Action(ShowMessage("Need Helix Code"))
```

**✅ Test** : Terminal → code obtenu → trappe s'ouvre → victoire.

---

## Étape 5 : Polish & Feedback Visuel (5min)

### 5.1 Ajouter Feedbacks

**Pour gardes** (dans chaque SubGraph) :
```
... ───► Branch(isPlayerVisible)
         │
         └── true ───► Action(ChangeAlertColor, color:"Yellow")
```

**Pour caméras** :
```
Action(RotateCamera) ───► Action(UpdateFOVVisual)
```

### 5.2 Variables de Debug

Ajouter dans tous les graphes :
```
Action(LogMessage, msg:"State: " + CurrentState)
```

---

## Étape 6 : Validation Finale (Checklist GDD)

| Feature GDD | Implémenté | Test OK |
|-------------|------------|---------|
| Garde Patrouille waypoints | ✅ `PatrolLoop` | ☐ |
| Suspicion/Investigate | ✅ `Investigate` | ☐ |
| Caméra rotation + timer 1.5s | ✅ `Camera_Rotating` | ☐ |
| Terminal 3s interaction | ✅ `TerminalSecurity` | ☐ |
| Trappe conditionnelle | ✅ `SecretTrapDoor` | ☐ |
| GlobalAlert propagation | ✅ BB global | ☐ |
| Feedback visuel FOV | ✅ AlertColor | ☐ |

---

## Bonus : Raccourcis Éditeur (QUICKSTART.md)

- **F2** : Toggle panels
- **Ctrl+Z** : Undo (CommandSystem)
- **Ctrl+D** : Dupliquer node sélectionné
- **Tab** : Auto-connecter pin le plus proche
- **Middle-click drag** : Pan canvas
- **Ctrl+MouseWheel** : Zoom

**Fichiers créés** :
```
Blueprints/VisualScripts/
├── Guard_Main.vs          ← State Machine
├── PatrolLoop.vs          ← Patrouille
├── Investigate.vs         ← Suspicion
├── Camera_Rotating.vs     ← Caméra
├── TerminalSecurity.vs    ← Hack 3s
└── SecretTrapDoor.vs      ← Victoire
```

**🎉 Prototype fonctionnel !** Play scene → teste l'infiltration complète.

**Prochain tuto** : Système bruit joueur + Mode Stealth.
