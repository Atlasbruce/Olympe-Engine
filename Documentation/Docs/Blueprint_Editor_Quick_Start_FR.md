# Olympe Blueprint Editor — Guide de Démarrage Rapide (Français)

> **Version**: 2.0 (Phase 5-8)  
> **Dernière mise à jour**: 2026-03-12  
> **Public cible**: Créateurs de contenu francophones, développeurs gameplay

---

## 📚 Documentation Complète

Ce document est un **guide de démarrage rapide en français**. Pour la documentation technique complète (en anglais), consultez :

- **[User Guide v4](Blueprint_Editor_User_Guide_v4.md)** — Guide complet avec workflows étape par étape
- **[Advanced Systems](Blueprint_Editor_Advanced_Systems.md)** — SubGraphs, Profiler, Debugger
- **[Visual Diagrams](Blueprint_Editor_Visual_Diagrams.md)** — Diagrammes interactifs Mermaid
- **[Documentation Index](README_Documentation_Index.md)** — Index principal avec chemins d'apprentissage

---

## 🎯 Vue d'Ensemble

L'**Olympe Blueprint Editor** est un éditeur visuel pour créer des comportements d'IA et de gameplay via des **graphes de nœuds** (flowcharts). Il remplace l'ancien système BehaviorTree v2 par le nouveau système **ATS Visual Script v4**.

### Système Actuel (v4)

| Caractéristique | Valeur |
|----------------|--------|
| **Éditeur** | VisualScriptEditorPanel (Phase 5) |
| **Format de fichier** | `.ats` ou `.json` avec `schema_version: 4` |
| **Type de graphe** | `"VisualScript"` (ATS Visual Script) |
| **Nœuds disponibles** | EntryPoint, Branch, Sequence, While, AtomicTask, GetBBValue, SetBBValue, MathOp, SubGraph, DoOnce, Delay |
| **Connexions** | ExecConnections (flux d'exécution) + DataConnections (données typées) |

### ⚠️ Système Déprécié (v2)

**NodeGraphPanel** (ancien éditeur BehaviorTree v2) est **obsolète** et sera supprimé dans la v3.0. **N'utilisez PAS ce système pour de nouveaux projets.**

---

## 🚀 Tutoriel : Créer Votre Premier Graphe (5 minutes)

### Étape 1 : Ouvrir l'Éditeur

1. Lancez Olympe Engine en mode **Standalone Editor**
2. Menu : **View → VS Graph Editor → New Graph**
3. Une fenêtre vide avec un canvas apparaît

### Étape 2 : Ajouter le Point d'Entrée

Le **EntryPoint** est le nœud de démarrage obligatoire de tout graphe.

1. Clic droit sur le canvas
2. **Flow Control → EntryPoint**
3. Un nœud bleu apparaît avec un pin de sortie `[Out]`

### Étape 3 : Ajouter une Branche Conditionnelle

Le nœud **Branch** permet de créer des conditions (if/else).

1. Clic droit sur le canvas
2. **Flow Control → Branch**
3. Un nœud vert apparaît avec :
   - Pin d'entrée : `[In]`
   - Pin de sortie : `[True]` et `[False]`
   - Pin de données : `[Condition]`

### Étape 4 : Connecter les Nœuds

1. Faites glisser depuis le pin `[Out]` du **EntryPoint**
2. Déposez sur le pin `[In]` du **Branch**
3. Une ligne bleue apparaît (connexion exec)

### Étape 5 : Configurer la Condition

1. Sélectionnez le nœud **Branch**
2. Dans le **Properties Panel** (droite), trouvez `ConditionKey`
3. Entrez : `local:HasEnemy`

### Étape 6 : Créer une Variable Blackboard

Le **Blackboard** stocke les variables de gameplay (santé, munitions, état, etc.).

1. Menu : **View → Blackboard**
2. Cliquez sur **[+ Add Variable]**
3. Remplissez :
   - **Nom** : `HasEnemy`
   - **Type** : `Bool`
   - **Valeur par défaut** : `false`
4. Cliquez **OK**

### Étape 7 : Ajouter une Tâche

Le nœud **AtomicTask** exécute du code C++ (ex : pathfinding, attaque, etc.).

1. Clic droit sur le canvas
2. **Tasks → AtomicTask**
3. Connectez le pin `[True]` du **Branch** au pin `[In]` de l'**AtomicTask**

### Étape 8 : Sauvegarder

1. **Ctrl+S** ou **File → Save**
2. Choisissez un emplacement : `Blueprints/AI/mon_premier_graphe.ats`
3. Cliquez **Save**

### Résultat

Vous avez créé un graphe simple :

```
[EntryPoint] ──► [Branch: HasEnemy?]
                      │
                      ├─ True  ──► [AtomicTask: Attack]
                      └─ False ──► (rien pour l'instant)
```

**Logique** : Si `HasEnemy` est `true`, l'IA exécute la tâche "Attack".

---

## 📖 Concepts Clés

### 1. Types de Pins

| Type | Couleur | Usage |
|------|---------|-------|
| **Exec** (Exécution) | Bleu | Contrôle le flux d'exécution (ordre des nœuds) |
| **Data** (Données) | Jaune | Transmet des valeurs typées (Int, Float, Bool, etc.) |

### 2. Types de Nœuds

#### Flow Control (Contrôle de Flux)

| Nœud | Description | Exemple |
|------|-------------|---------|
| **EntryPoint** | Point de démarrage du graphe | Obligatoire, un seul par graphe |
| **Branch** | Condition if/else | `if (HasEnemy) { Attack } else { Patrol }` |
| **Sequence** | Exécute plusieurs nœuds en séquence | Nœud 1 → Nœud 2 → Nœud 3 |
| **While** | Boucle tant que condition vraie | `while (Health > 0) { Fight }` |
| **DoOnce** | Exécute une seule fois | Initialisation au début du graphe |

#### Data (Données)

| Nœud | Description | Exemple |
|------|-------------|---------|
| **GetBBValue** | Lit une variable du Blackboard | Lire `local:EnemyHealth` → 75.0f |
| **SetBBValue** | Écrit dans le Blackboard | Écrire `local:IsAggro` ← true |
| **MathOp** | Opérations mathématiques | `A + B`, `A * B`, `A < B` |

#### Tasks (Tâches)

| Nœud | Description | Exemple |
|------|-------------|---------|
| **AtomicTask** | Exécute du code C++ | Pathfinding, attaque, dialogue |

#### Advanced (Avancé)

| Nœud | Description | Exemple |
|------|-------------|---------|
| **SubGraph** | Appelle un autre graphe | Réutilisation de logique (patrol, combat) |
| **Delay** | Pause l'exécution | Attendre 2 secondes avant action |

### 3. Blackboard (Tableau Noir)

Le **Blackboard** est un système de variables partagées entre les nœuds.

#### Scopes (Portées)

| Scope | Syntaxe | Stockage | Exemple |
|-------|---------|----------|---------|
| **local:** | `local:Health` | Par entité (NPC individuel) | Santé d'un garde spécifique |
| **global:** | `global:GameTime` | Monde entier (tous les NPCs) | Temps de jeu, météo, quêtes |

#### Exemple d'Usage

**Graphe** :
```
[GetBBValue: "local:EnemyHealth"] ──► [Branch: Health > 50?]
                                            │
                                            ├─ True  ──► [AtomicTask: Aggressive]
                                            └─ False ──► [AtomicTask: Retreat]
```

**Variables Blackboard** :
```
local:EnemyHealth = 75.0f  (santé de l'ennemi)
local:IsAggro = false      (état d'agression)
global:GameTime = 123.45f  (temps de jeu)
```

---

## 🛠️ Fonctionnalités Avancées

### 1. SubGraphs (Graphes Imbriqués)

Les **SubGraphs** permettent de réutiliser des morceaux de logique.

#### Exemple

**Graphe Principal** (`guard_ai.ats`) :
```
[EntryPoint] ──► [Branch: HasEnemy?]
                      │
                      ├─ True  ──► [SubGraph: combat_logic.ats]
                      └─ False ──► [SubGraph: patrol_logic.ats]
```

**SubGraph** (`patrol_logic.ats`) :
```
[EntryPoint] ──► [Sequence]
                      ├─ [AtomicTask: MoveToWaypoint]
                      ├─ [Delay: 2s]
                      └─ [SetBBValue: PatrolIndex++]
```

**Avantages** :
- ✅ Réutilisation de code
- ✅ Organisation modulaire
- ✅ Facilite la maintenance

**Limites** :
- ⚠️ Profondeur maximale : 4 niveaux
- ⚠️ Détection de cycles (A → B → A = erreur)

### 2. Débogage (F9 Breakpoints)

Le système de débogage permet de **mettre en pause** l'exécution et d'inspecter l'état.

#### Workflow

1. **F9** sur un nœud → Définir un point d'arrêt (breakpoint)
2. Lancez le jeu → L'exécution s'arrête au breakpoint
3. **DebugPanel** affiche :
   - **Call Stack** : Chemin d'exécution (nœud actuel + parents)
   - **Watch Variables** : Valeurs du Blackboard en temps réel
4. Contrôles :
   - **F5** : Continuer jusqu'au prochain breakpoint
   - **F10** : Exécuter le nœud actuel, pause au suivant
   - **F11** : Entrer dans un SubGraph

#### Exemple

**Graphe** :
```
[EntryPoint] ──► [Branch: HasEnemy?] 🔴 ← Breakpoint
                      │
                      ├─ True  ──► [AtomicTask: Attack]
                      └─ False ──► [AtomicTask: Patrol]
```

**DebugPanel lors de la pause** :
```
Call Stack:
  #0  Branch_CheckEnemy (guard_ai.ats:25)
  #1  EntryPoint (guard_ai.ats:5)

Watch Variables:
  local:HasEnemy = true
  local:EnemyHealth = 75.0f
  global:GameTime = 123.45f
```

### 3. Profiler de Performance

Le **Profiler** mesure le temps d'exécution de chaque nœud.

#### Activation

1. **View → Profiler → Begin Profiling**
2. Jouez le jeu pendant 30 secondes
3. **View → Profiler Panel**

#### Interprétation

**Hotspot Table** :
```
┌─────────────────┬───────┬──────────┬──────────┬───────────┐
│ Nom du Nœud     │ Count │ Moy (ms) │ Max (ms) │ Total (ms)│
├─────────────────┼───────┼──────────┼──────────┼───────────┤
│ PathfindToTarget│  120  │  0.45    │  1.23    │   54.0    │ ← Coûteux !
│ UpdateVision    │  240  │  0.12    │  0.34    │   28.8    │
│ Branch_CheckHP  │  480  │  0.02    │  0.05    │    9.6    │ ← Rapide
└─────────────────┴───────┴──────────┴──────────┴───────────┘
```

**Actions** :
- 🔥 **PathfindToTarget** prend 0.45ms en moyenne → Optimiser le pathfinding (mettre en cache)
- ✅ **Branch_CheckHP** prend 0.02ms → Performances acceptables

### 4. Système de Templates

Les **Templates** permettent de sauvegarder des graphes comme modèles réutilisables.

#### Workflow

1. Créez un graphe (ex : logique de patrouille complète)
2. **File → Save as Template**
3. Remplissez :
   - **Nom** : "Guard Patrol Base"
   - **Description** : "Logique de patrouille standard pour gardes"
   - **Catégorie** : "AI"
4. Cliquez **Save**

#### Réutilisation

1. **View → Template Library**
2. Catégorie **AI** → Sélectionnez "Guard Patrol Base"
3. Cliquez **Apply to Current Graph**
4. Le template est copié dans votre graphe actuel

### 5. Validation en Temps Réel

Le **ValidationPanel** détecte les erreurs automatiquement.

#### Types d'Erreurs

| Sévérité | Description | Exemple |
|----------|-------------|---------|
| **🔴 CRITICAL** | Bloque l'exécution | Pas de EntryPoint, cycle de SubGraphs |
| **⚠️ ERROR** | Causera des crashs | Type de pin incompatible (Float → Bool) |
| **⚠️ WARNING** | Problèmes potentiels | Nœud inaccessible, boucle infinie |
| **ℹ️ INFO** | Suggestions | Variable Blackboard inutilisée |

#### Exemple

**ValidationPanel** :
```
🔴 CRITICAL (1)
  • Missing EntryPoint node
    Fix: Add an EntryPoint node from Flow Control palette

⚠️ ERROR (2)
  • Node 5 (Branch): Missing required property 'ConditionKey'
    Fix: Set ConditionKey in Properties panel
  • Node 12: DataConnection type mismatch (Float → Bool)
    Fix: Insert type conversion node or change pin type

⚠️ WARNING (1)
  • Node 8 (AtomicTask): Unreachable (no incoming exec connections)
    Fix: Connect an exec pin to this node or delete it
```

**Action** : Cliquez sur une erreur → L'éditeur focus le nœud concerné en rouge.

---

## ⌨️ Raccourcis Clavier

| Touche | Action | Contexte |
|--------|--------|----------|
| **F5** | Continuer (reprendre l'exécution) | Débogage |
| **F9** | Basculer breakpoint | Nœud sélectionné |
| **F10** | Pas à pas (exécuter nœud actuel) | Débogage |
| **F11** | Entrer dans SubGraph | Débogage |
| **Ctrl+S** | Sauvegarder le graphe | Éditeur |
| **Ctrl+Z** | Annuler | Éditeur |
| **Ctrl+Y** | Refaire | Éditeur |
| **Ctrl+C** | Copier nœuds | Éditeur |
| **Ctrl+V** | Coller nœuds | Éditeur |
| **Delete** | Supprimer nœuds sélectionnés | Éditeur |

---

## 📝 Formats de Fichiers

### Schema v4 (Actuel)

```json
{
  "schema_version": 4,
  "graphType": "VisualScript",
  "name": "Guard AI",
  "description": "Logique de patrouille et combat pour gardes",
  "nodes": [
    {
      "NodeID": 1,
      "NodeType": "EntryPoint",
      "NodeName": "Entry",
      "Position": {"x": 100, "y": 200}
    },
    {
      "NodeID": 2,
      "NodeType": "Branch",
      "NodeName": "Check Enemy",
      "Properties": {"ConditionKey": "local:HasEnemy"},
      "Position": {"x": 300, "y": 200}
    }
  ],
  "ExecConnections": [
    {
      "FromNodeID": 1,
      "FromPinName": "Out",
      "ToNodeID": 2,
      "ToPinName": "In"
    }
  ],
  "DataConnections": [],
  "Blackboard": {
    "Variables": [
      {"Key": "HasEnemy", "Type": "Bool", "DefaultValue": false},
      {"Key": "EnemyHealth", "Type": "Float", "DefaultValue": 100.0}
    ]
  }
}
```

### Schema v5 (SubGraphs)

```json
{
  "schema_version": 5,
  "graphType": "VisualScript",
  "name": "Guard AI",
  "data": {
    "rootGraph": {
      "nodes": [...],
      "ExecConnections": [...]
    },
    "subgraphs": {
      "<uuid-patrol>": {
        "uuid": "<uuid-patrol>",
        "name": "Patrol Behavior",
        "nodes": [...],
        "inputPins": ["start", "patrolRadius"],
        "outputPins": ["success", "failure"]
      }
    }
  }
}
```

---

## 🎓 Chemins d'Apprentissage

### Niveau 1 : Débutant Complet (1 heure)

**Objectif** : Créer un graphe simple avec conditions et variables Blackboard.

1. Suivez le **Tutoriel 5 Minutes** ci-dessus
2. Ajoutez un deuxième **Branch** après l'**AtomicTask**
3. Créez une variable `local:Health` (Float, default: 100.0)
4. Utilisez **GetBBValue** pour lire `Health` et la connecter au **Branch**
5. Testez le graphe en mode jeu

**Ressources** :
- [User Guide v4](Blueprint_Editor_User_Guide_v4.md) — Sections 1-4

### Niveau 2 : Utilisateur Intermédiaire (2 heures)

**Objectif** : Maîtriser le Blackboard, le débogage et la validation.

1. Créez un graphe d'IA de combat avec :
   - **Branch** : Vérifier santé < 30 → Fuite
   - **While** : Boucle de combat tant que ennemi vivant
   - **SetBBValue** : Mettre à jour état d'aggression
2. Définissez 5 variables Blackboard (santé, munitions, état, etc.)
3. Utilisez **F9** pour placer 3 breakpoints
4. Lancez le jeu, inspectez les variables dans **DebugPanel**
5. Utilisez **ValidationPanel** pour corriger les erreurs

**Ressources** :
- [User Guide v4](Blueprint_Editor_User_Guide_v4.md) — Sections 5-7
- [Advanced Systems](Blueprint_Editor_Advanced_Systems.md) — "Blackboard System", "Debug System"

### Niveau 3 : Développeur Avancé (3 heures)

**Objectif** : Utiliser SubGraphs, Templates et Profiler pour systèmes complexes.

1. Créez 3 graphes :
   - `guard_ai.ats` (graphe principal)
   - `patrol_logic.ats` (SubGraph pour patrouille)
   - `combat_logic.ats` (SubGraph pour combat)
2. Utilisez des nœuds **SubGraph** dans `guard_ai.ats` pour appeler les 2 autres
3. Sauvegardez `guard_ai.ats` comme **Template** "Guard AI Base"
4. Activez le **Profiler**, jouez 30 secondes, analysez les hotspots
5. Optimisez les nœuds les plus lents (ex : réduire fréquence de pathfinding)

**Ressources** :
- [Advanced Systems](Blueprint_Editor_Advanced_Systems.md) — Toutes les sections
- [Visual Diagrams](Blueprint_Editor_Visual_Diagrams.md) — "SubGraph Call Stack", "Profiler Data Flow"

---

## 🆘 Dépannage

### Erreur : "Missing EntryPoint node"

**Cause** : Votre graphe n'a pas de nœud **EntryPoint**.

**Solution** :
1. Clic droit sur le canvas
2. **Flow Control → EntryPoint**
3. Connectez-le au reste du graphe

### Erreur : "DataConnection type mismatch (Float → Bool)"

**Cause** : Vous essayez de connecter un pin de sortie **Float** à un pin d'entrée **Bool**.

**Solution** :
- Utilisez un nœud **MathOp** pour convertir : `Float > 0.0 → Bool`
- Ou changez le type de la variable Blackboard

### Erreur : "Circular SubGraph dependency detected"

**Cause** : Vos SubGraphs s'appellent mutuellement en boucle (A → B → A).

**Solution** :
- Restructurez la logique pour éviter les cycles
- Utilisez des variables Blackboard pour communiquer entre graphes au lieu de SubGraphs imbriqués

### Erreur : "Max SubGraph recursion depth exceeded (4)"

**Cause** : Vous avez imbriqué plus de 4 niveaux de SubGraphs.

**Solution** :
- Aplatissez la hiérarchie (fusionnez certains SubGraphs)
- Réévaluez votre architecture (peut-être trop de niveaux d'abstraction)

---

## 📞 Support

### Documentation Complète

- **[User Guide v4](Blueprint_Editor_User_Guide_v4.md)** — Guide complet (EN)
- **[Advanced Systems](Blueprint_Editor_Advanced_Systems.md)** — Fonctionnalités avancées (EN)
- **[Visual Diagrams](Blueprint_Editor_Visual_Diagrams.md)** — Diagrammes interactifs (EN)
- **[Documentation Index](README_Documentation_Index.md)** — Index principal (EN)

### Signaler un Bug

1. Vérifiez la documentation ci-dessus
2. Créez une issue GitHub avec :
   - Version de l'éditeur
   - Fichier `.ats` problématique (si applicable)
   - Capture d'écran
   - Comportement attendu vs réel

### Contribuer

1. Forkez le dépôt
2. Créez une branche : `feature/mon-amelioration`
3. Suivez les standards de code C++14
4. Soumettez une Pull Request avec description détaillée

---

## 🎉 Conclusion

Vous avez maintenant les bases pour créer des **graphes d'IA complexes** avec l'Olympe Blueprint Editor v4 !

**Prochaines étapes** :
1. ✅ Créez votre premier graphe (5 min)
2. ✅ Ajoutez des variables Blackboard (10 min)
3. ✅ Testez le débogage avec F9 (15 min)
4. ✅ Créez un SubGraph réutilisable (30 min)
5. ✅ Profilez et optimisez (45 min)

**Bon développement ! 🚀**

---

**Licence** : MIT License  
**Copyright** © 2026 Olympe Engine Team
