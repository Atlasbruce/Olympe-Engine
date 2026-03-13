# Olympe Engine — Roadmap V2

**Date :** 2026-03-13
**Auteur :** @Atlasbruce
**Statut :** Planification active

---

## Vue d'ensemble

La Roadmap V2 définit les 4 initiatives majeures post-Phase 20 pour le Blueprint Editor et l'engine.

```
Ordre d'implémentation : A → B → C → D
```

| Initiative | Titre | Priorité | Statut |
|---|---|---|---|
| A | Graph Verification System (GVS) | 🔴 P0 | Phase 21 — En cours |
| B | Design & Icons Font Awesome | 🟡 P1 | En attente spec design |
| C | Diversification Types de Graphes | 🟢 P2 | Planification future |
| D | Runtime Execution & Debugger | 🔵 P3 | Planification future |

---

## 🔴 Initiative A — Graph Verification System (GVS)

### Contexte
Le `VSConnectionValidator` actuel ne valide que les connexions individuelles (self-loop, duplicate pin, cycle).
L'Initiative A implémente un **validateur/compilateur global de graphe** capable d'analyser l'intégralité
du graphe et de produire des messages d'erreur/warning exploitables dans l'éditeur.

### Spec de référence
> « Olympe ATS Visual Scripting – Spécification du Système de Vérification de Graphe »
> Fournie par @Atlasbruce le 2026-03-13.

### Phases d'implémentation

#### Phase 21-A : VSGraphVerifier — Validateur global
**Nouveaux fichiers :**
- `Source/BlueprintEditor/VSGraphVerifier.h`
- `Source/BlueprintEditor/VSGraphVerifier.cpp`
- `Tests/BlueprintEditor/Phase21ATest.cpp`

**Architecture :**
```cpp
// Résultat de validation
struct VSVerificationIssue {
    enum class Severity { Error, Warning, Info };
    Severity    severity;
    std::string message;
    int         nodeID;   // -1 si pas de node spécifique
    std::string pinName;  // "" si pas de pin spécifique
};

struct VSVerificationResult {
    std::vector<VSVerificationIssue> issues;
    bool IsValid() const; // true si aucune erreur (warnings OK)
};

// Validateur stateless
class VSGraphVerifier {
public:
    static VSVerificationResult Verify(const TaskGraphTemplate& graph);

private:
    // Règles structurelles
    static void CheckEntryPoint(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckDanglingNodes(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckExecCycles(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckSubGraphCircular(const TaskGraphTemplate& g, VSVerificationResult& r);

    // Règles de type safety
    static void CheckExecPinTypes(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckDataPinTypes(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckPinDirections(const TaskGraphTemplate& g, VSVerificationResult& r);

    // Règles Blackboard
    static void CheckBlackboardKeys(const TaskGraphTemplate& g, VSVerificationResult& r);
    static void CheckBlackboardTypes(const TaskGraphTemplate& g, VSVerificationResult& r);
};
```

**Règles implémentées :**

| Règle | Sévérité | Description |
|---|---|---|
| EntryPoint unique | ERROR | 0 ou >1 EntryPoint dans le graphe |
| Node isolé | WARNING | Node sans aucune connexion exec |
| Cycle interdit | ERROR | Cycle détecté dans le graphe exec (sauf While/ForEach explicitement) |
| SubGraph circulaire | ERROR | SubGraph qui référence son propre fichier (direct/indirect) |
| Pin exec → data | ERROR | Connexion entre pin exec et pin data |
| Pin data incompatible | ERROR | Types de données incompatibles sur connexion data |
| Pin inversé | ERROR | Connexion output→output ou input→input |
| BB key inconnue | WARNING | GetBBValue/SetBBValue référence une clé absente du Blackboard |
| BB type mismatch | ERROR | Type de la variable BB incompatible avec l'usage du node |
| Node EntryPoint sans sortie | WARNING | EntryPoint non connecté |
| Delay sans valeur | WARNING | DelaySeconds == 0 sur node Delay |
| AtomicTask sans ID | ERROR | AtomicTaskID vide sur node AtomicTask |
| SubGraph sans path | ERROR | SubGraphPath vide sur node SubGraph |
| Branch sans condition | WARNING | ConditionID vide sur node Branch |

**Tests (Phase21ATest.cpp) :**
- `Test_EmptyGraph_NoEntryPoint` → ERROR EntryPoint
- `Test_SingleEntryPoint_Valid`
- `Test_MultipleEntryPoints_Error`
- `Test_IsolatedNode_Warning`
- `Test_AtomicTask_NoID_Error`
- `Test_BlackboardKey_Missing_Warning`
- `Test_DataPin_TypeMismatch_Error`

---

#### Phase 21-B : Panel Validation UI
**Fichiers modifiés :**
- `Source/BlueprintEditor/VisualScriptEditorPanel.h` — ajout `m_verificationResult`, `m_autoVerify`
- `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` — `RenderValidationOverlay()` enrichi

**UX :**
- Panel Validation (bas de l'éditeur) : liste scrollable des erreurs/warnings avec icônes
- Click sur une issue → sélectionne le node fautif + centre la vue sur lui
- Badge rouge sur la toolbar si des erreurs existent (nb d'erreurs)
- Auto-vérification optionnelle (toggle) à chaque modification du graphe
- Vérification manuelle via bouton "Verify" dans la toolbar

**Format affichage issue :**
```
[ERROR] Node #3 (AtomicTask) : AtomicTaskID is empty
[WARN]  Node #7 (GetBBValue) : Blackboard key 'enemy_hp' not found
[ERROR] Cycle detected: Node #2 → Node #5 → Node #2
```

---

#### Phase 21-C : Intégration pré-save + pré-exécution
**Comportement :**
- **Pré-sauvegarde** : Si des erreurs existent, afficher une dialog de confirmation
  "Le graphe contient N erreur(s). Sauvegarder quand même ?" (warnings n'bloquent pas)
- **Pré-exécution** : Bloquer l'exécution si des erreurs ERROR sont présentes
- **Guard interactif amélioré** : Remplacer `VSConnectionValidator::IsExecConnectionValid()`
  par un appel à `VSGraphVerifier` pour les règles plus complexes (type safety)

---

## 🟡 Initiative B — Design & Icons Font Awesome

### Contexte
Amélioration visuelle des nodes ImNodes avec des icônes Font Awesome pour améliorer la lisibilité
et la reconnaissance rapide des types de nodes et d'opérations.

### En attente
> Document de specs design technique en cours de rédaction par @Atlasbruce.

### Scope préliminaire
- Intégration de la police Font Awesome (fichier `.ttf` + merge ImGui font atlas)
- Icônes par type de node (EntryPoint, Branch, AtomicTask, etc.)
- Icônes sur les pins exec/data (triangle stylisé, cercle coloré enrichi)
- Amélioration du titre des nodes (icône + label)
- Couleurs et styles à définir dans le document de specs

### Fichiers impactés (estimé)
- `Source/BlueprintEditor/VisualScriptNodeRenderer.h/.cpp` — icônes dans `RenderNode()`
- `Source/third_party/` — ajout Font Awesome `.ttf`
- Nouveau fichier : `Source/BlueprintEditor/VSNodeIcons.h` — constantes de codepoints

---

## 🟢 Initiative C — Diversification Types de Graphes

### Contexte
Extension du système de graphes au-delà des graphes AI/BT pour couvrir d'autres domaines du jeu.

### Types de graphes envisagés

| Type | Usage | Nodes spécifiques |
|---|---|---|
| **VisualScript (AI)** | Comportements NPC (existant) | EntryPoint, Branch, AtomicTask... |
| **AnimGraph** | Transitions d'animations | AnimState, Condition, BlendTree... |
| **LevelScript** | Scripts de niveau (triggers, events) | OnTrigger, Spawn, Destroy, Timer... |
| **CinematicGraph** | Séquences cinématiques | PlayAnim, MoveCam, Subtitle, Wait... |
| **MenuGraph** | Navigation de menus UI | Screen, Button, Transition, Sound... |
| **GlobalRules** | Règles globales de jeu | OnEvent, SetGlobal, Condition, Timer... |

### Architecture envisagée
```cpp
enum class GraphType {
    VisualScript,   // Existant
    AnimGraph,      // Nouveau
    LevelScript,    // Nouveau
    CinematicGraph, // Nouveau
    MenuGraph,      // Nouveau
    GlobalRules     // Nouveau
};
```
- Factory pattern : `VSGraphFactory::Create(GraphType)` → `TaskGraphTemplate` avec nodes par défaut
- Règles GVS spécifiques par type (`VSGraphVerifier::Verify(graph, graphType)`)
- Palette de nodes filtrée par type de graphe
- Sérialisation JSON : ajout champ "graphType" (étendu)

### Dépendances
- **Dépend de Initiative A** (GVS doit supporter les règles par type de graphe)
- **Dépend de Initiative B** (icônes spécifiques par domaine)

---

## 🔵 Initiative D — Runtime Execution & Debugger

### Contexte
Recâblage de l'exécution runtime des graphes AI avec support multi-instances et visualisation
read-only du graphe en cours d'exécution dans le debugger.

### Scope

#### D1 — Multi-instance Execution
- Une instance d'exécution = un `GraphExecutionContext` (état courant : node actif, variables locales)
- Plusieurs NPC peuvent exécuter le même graphe en parallèle avec des contextes indépendants
- `GraphExecutor::Tick(context, deltaTime)` — avance l'exécution d'un tick

#### D2 — Visualisation Read-Only dans le Debugger
- Panel Blueprint Editor en mode "debug" : read-only, pas d'édition possible
- Node actuellement exécuté : surligné en vert (glow existant `RenderActiveNodeGlow`)
- Historique des nodes visités (trail visuel, fade-out)
- Variables locales du contexte affichées dans le panel Properties (read-only)
- Support multi-instances : sélecteur d'instance dans la toolbar debug

#### D3 — Breakpoints Runtime
- `DebugController` existant étendu : breakpoints liés à l'instance d'exécution
- Pause/Step/Resume par instance
- Panel Variables : inspection des valeurs BB au moment du breakpoint

### Dépendances
- **Dépend de Initiative A** (exécution bloquée si graphe invalide)
- Architecture `TaskGraphTemplate` déjà stable — bon point de départ

---

## Calendrier estimatif

```
2026-03  Phase 21-A  VSGraphVerifier — validateur global          [NEXT]
2026-03  Phase 21-B  Panel Validation UI
2026-03  Phase 21-C  Intégration pré-save + pré-exec
         ---
         Attente spec design Initiative B
         ---
2026-04  Phase 22    Design & Icons Font Awesome (Initiative B)
         ---
2026-04  Phase 23    Diversification Types de Graphes (Initiative C)
         ---
2026-05  Phase 24    Runtime Execution & Debugger (Initiative D)
```

---

**Last Updated**: 2026-03-13