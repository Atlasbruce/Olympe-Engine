# 📄 Feature Context — Phase 22-A : VSSwitch Node Enhancement

> **Créé le:** 2026-03-15 10:26:37 UTC  
> **Dernière mise à jour:** 2026-03-15 10:26:37 UTC  
> **Statut:** ✅ COMPLÉTÉ  
> **PR:** [#384](https://github.com/Atlasbruce/Olympe-Engine/pull/384)  
> **Références croisées:** [feature_context_21_D.md](./feature_context_21_D.md) · [feature_context_22_C.md](./feature_context_22_C.md)

---

## 1. 🎯 Objectifs Conceptuels

### 1.1 Problème Résolu

**Avant cette feature :** Le nœud `VSSwitch` affichait ses cases avec des labels génériques ("Case_0", "Case_1"...) qui ne correspondaient pas aux valeurs réelles de la Blackboard variable surveillée. L'utilisateur ne pouvait pas associer une sémantique métier à chaque branche (ex: "Combat", "Patrol", "Idle"). De plus, le texte des labels était centré, créant des problèmes d'alignement visuels sur les connexions de pins.

**Après cette feature :** L'utilisateur peut nommer chaque case avec un label personnalisé significatif (ex: "Combat", "Patrol", "Idle"). Les labels sont alignés à droite pour un rendu visuel cohérent. Le nœud affiche en temps réel la valeur actuelle de la variable Blackboard qu'il surveille.

### 1.2 But du Système

L'amélioration du VSSwitch rend le graphe comportemental auto-documenté. Au lieu d'afficher des codes techniques ("Case_0", "Case_3"), l'utilisateur voit directement les sémantiques métier sur les connexions. Cela améliore considérablement la lisibilité des graphes AI complexes.

La fonctionnalité s'inscrit dans le paradigme "What you see is what the AI does" — le graphe doit être compréhensible sans référence externe.

### 1.3 Critères de Succès
- [x] Labels personnalisés modifiables directement dans le panneau Properties
- [x] Alignement à droite des labels de cases sur le canvas
- [x] Affichage runtime de la variable Blackboard surveillée (ex: "var: bb_ai_mode = 5")
- [x] Modification de label undoable (intégré à l'UndoRedo existant)
- [x] 10 tests passants

---

## 2. 🏗️ Architecture

### 2.1 Vue d'ensemble

```
VSSwitch Node (canvas)
┌─────────────────────────┐
│ Switch                  │
│ var: bb_ai_mode = 5 ←── runtime display
│                         │
│ ExecIn ──[  ]           │
│          [  ] ── Combat │ ← label personnalisé, aligné droite
│          [  ] ── Patrol │
│          [  ] ── Idle   │
│          [  ] ── [+]    │ ← dynamic pin (Phase 21-D)
└─────────────────────────┘

Properties Panel
┌─────────────────────────────┐
│ Blackboard Variable: [bb_ai_mode] ▼ │
│ Case 0 Label: [Combat     ]  │
│ Case 1 Label: [Patrol     ]  │
│ Case 2 Label: [Idle       ]  │
└─────────────────────────────┘
```

### 2.2 Composants Principaux

| Composant | Rôle | Type |
|-----------|------|------|
| Custom case labels | Structure de données pour stocker les labels par case index | Nouveau |
| Right-align rendering | Logique de rendu ImNodes avec alignement à droite | Nouveau |
| Runtime variable display | Affichage de la valeur Blackboard courante dans le titre du nœud | Nouveau |
| Label edit in Properties | Interface d'édition des labels dans le panneau Properties | Nouveau |

### 2.3 Flux de Données

1. L'utilisateur édite un label dans le panneau Properties
2. La modification est packagée dans une commande undoable (`SetSwitchCaseLabelCommand`)
3. Le nœud est mis à jour avec le nouveau label
4. Le canvas rerender le nœud avec le label aligné à droite
5. À runtime, la valeur Blackboard est lue depuis le `LocalBlackboard` actif et affichée dans le titre

---

## 3. 💡 Décisions Techniques

### 3.1 Décision Principale — Labels stockés dans le nœud (pas dans le graphe)

**Approche choisie :** Les labels personnalisés sont stockés directement dans la structure de données du `VSSwitch` node, sérialisés avec les autres propriétés du nœud dans le JSON.

**Pourquoi cette approche :**
- Les labels font partie de la définition du nœud, pas de la structure du graphe
- La sérialisation/désérialisation est cohérente avec les autres propriétés
- Pas de mapping externe nécessaire (pas de structure séparée label-map)

**Alternatives rejetées :**

| Alternative | Raison du rejet |
|-------------|----------------|
| Labels dans un registry séparé | Complexité inutile — les labels sont spécifiques à chaque instance de nœud |
| Labels stockés dans le graphe (metadata) | Couplage excessif entre graphe et données de présentation |

**Trade-offs acceptés :**
- ✅ Simplicité — les labels suivent naturellement le nœud (copie, déplacement, save/load)
- ⚠️ Rupture de compatibilité avec les fichiers JSON v1 (Switch v1 → v2 sans migration)

---

### 3.2 Décision Secondaire — Alignement à droite sans modification de l'API ImNodes

**Approche choisie :** Utiliser les offsets de positionnement ImGui (SameLine + SetCursorPosX calculé) pour simuler l'alignement à droite dans le contexte ImNodes.

**Justification :** ImNodes ne supporte pas nativement l'alignement à droite des pin labels. L'utilisation des primitives ImGui standard est la seule approche compatible C++14 sans fork de la bibliothèque.

---

### 3.3 Décision Tertiaire — Pas de migration v1 → v2

**Approche choisie :** Les fichiers JSON créés avant Phase 22-A avec des nœuds VSSwitch sont incompatibles (les labels personnalisés n'existent pas → fallback sur "Case_N").

**Justification :** La phase est en développement actif, aucun graphe de production existant n'utilise VSSwitch de manière critique. La migration rétroactive aurait complexifié le format JSON sans bénéfice utilisateur mesurable à ce stade.

---

## 4. 📦 Implications sur les Modules

### 4.1 Modules Directement Modifiés

| Module | Fichiers | Nature des Changements |
|--------|----------|----------------------|
| `BlueprintEditor` | `VisualScriptNodes.h`, `VisualScriptNodes.cpp` | Ajout champ labels[], sérialisation JSON |
| `BlueprintEditor` | `VisualScriptNodeRenderer.cpp` | Rendu aligné à droite, runtime display |
| `BlueprintEditor` | `VisualScriptEditorPanel.cpp` | UI Properties pour édition labels |
| `BlueprintEditor` | `UndoRedoStack.h`, `UndoRedoStack.cpp` | SetSwitchCaseLabelCommand |
| `Tests/BlueprintEditor` | `Phase22ATest.cpp` | 10 tests |

### 4.2 Risques de Couplage

- **VSSwitch → LocalBlackboard** : Lecture de la valeur pour runtime display — couplage en lecture seule, acceptable

---

## 5. ⚠️ Matrice de Risques

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Incompatibilité JSON avec fichiers existants | Faible (pas de prod) | Faible | Fallback sur "Case_N" si le champ labels est absent |
| Alignement à droite mal calculé sur labels longs | Moyen | Faible | Tronquer les labels trop longs avec "..." |
| Runtime display Blackboard non disponible | Moyen | Faible | Afficher "var: [non disponible]" si Blackboard null |

---

## 6. 📁 Détails d'Implémentation

### 6.1 Fichiers Créés

| Fichier | Rôle |
|---------|------|
| `Tests/BlueprintEditor/Phase22ATest.cpp` | 10 scénarios de tests |

### 6.2 Fichiers Modifiés

| Fichier | Modification |
|---------|-------------|
| `Source/BlueprintEditor/VisualScriptNodes.h` | Ajout du champ `caseLabels` dans VSSwitch |
| `Source/BlueprintEditor/VisualScriptNodes.cpp` | Sérialisation/désérialisation des labels dans JSON |
| `Source/BlueprintEditor/VisualScriptNodeRenderer.cpp` | Rendu aligné à droite + runtime variable display |
| `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` | Champs d'édition labels dans Properties panel |
| `Source/BlueprintEditor/UndoRedoStack.h` | Déclaration SetSwitchCaseLabelCommand |
| `Source/BlueprintEditor/UndoRedoStack.cpp` | Implémentation SetSwitchCaseLabelCommand |
| `CMakeLists.txt` | Ajout cible OlympePhase22ATests |

### 6.3 Patterns Utilisés

- **Command Pattern** — SetSwitchCaseLabelCommand pour l'édition undoable des labels
- **Commit-on-release** — La modification du label est commitée seulement quand l'utilisateur relâche le champ d'édition (cohérent avec le pattern Properties existant)

---

## 7. ✅ Règles de Validation et Cas de Tests

### 7.1 Cas de Tests (Phase 22-A)

| # | Scénario | Préconditions | Résultat Attendu | Statut |
|---|----------|--------------|-----------------|--------|
| 1 | Label par défaut | Nouveau VSSwitch | Labels = "Case_0", "Case_1", etc. | ✅ |
| 2 | Modifier label Case_0 | VSSwitch existant | Label "Combat" affiché sur canvas | ✅ |
| 3 | Undo modification label | Label modifié | Label précédent restauré | ✅ |
| 4 | Redo modification label | Undo effectué | Nouveau label réappliqué | ✅ |
| 5 | Save/Load avec labels personnalisés | Graphe avec labels | Labels restaurés après rechargement | ✅ |
| 6 | Alignement à droite | Labels de longueurs variées | Alignement cohérent sans débordement | ✅ |
| 7 | Runtime display — Blackboard disponible | bb_ai_mode = 5 | "var: bb_ai_mode = 5" dans titre | ✅ |
| 8 | Runtime display — Blackboard absent | Aucun Blackboard actif | Titre sans runtime display | ✅ |
| 9 | Add case puis modifier son label | Dynamic pin ajouté | Nouveau case modifiable dans Properties | ✅ |
| 10 | Labels avec JSON v1 (fallback) | Ancien fichier sans labels | Fallback sur "Case_N" sans erreur | ✅ |

---

## 8. 📜 Historique des Propositions Copilot

### Proposition 1 — 2026-03-14 16:00:00 UTC

**Contexte :** Amélioration UX du nœud VSSwitch après la complétion de Phase 21-D.

**Proposition :**
- Labels personnalisés par case
  - Champ d'édition dans le panneau Properties par case
  - Stockage dans la structure VSSwitch, sérialisé en JSON
  - Modification undoable via SetSwitchCaseLabelCommand
- Alignement à droite des labels sur le canvas
  - Utilisation des offsets ImGui (SetCursorPosX) pour simulation
  - Sans modification de l'API ImNodes
- Runtime variable display dans le titre du nœud
  - Lecture de la valeur Blackboard courante
  - Format : "var: [nom_variable] = [valeur]"
- Pas de migration v1 → v2 (fallback sur "Case_N")

**Décision :** ✅ Acceptée par @Atlasbruce  
**Raison :** Amélioration UX significative, compatible avec les dynamic pins existants (Phase 21-D), implémentation non breaking pour le développement en cours.

---

## 9. 🏛️ Notes Archéologiques

### Apprentissages Clés

- Le commit-on-release pour les champs de texte Properties est essentiel — sans lui, chaque frappe crée une entrée Undo distincte, rendant l'UndoRedo inutilisable pour l'édition de texte
- L'alignement à droite avec les primitives ImGui est faisable mais requiert un calcul précis de la largeur disponible — à documenter comme pattern réutilisable

---

## 10. 🐛 Problèmes Connus & Régressions

> **Statut de la Phase :** ✅ COMPLÉTÉE — PR [#384](https://github.com/Atlasbruce/Olympe-Engine/pull/384) mergée le 2026-03-15 UTC

### 10.1 Bugs Actifs

*Aucun bug actif sur cette phase.*

| ID | Titre | Sévérité | Statut | Workaround |
|----|-------|----------|--------|-----------|
| — | Aucun | — | — | — |

### 10.2 Problèmes Connus Non Classifiés

- L'alignement à droite des labels de case repose sur des offsets ImGui calculés manuellement — si la largeur du nœud change (nouveau type, police différente), les labels peuvent être mal alignés. Non bloquant mais à surveiller.
- Le mécanisme "commit-on-release" pour les champs de texte Properties est crucial pour l'UX Undo/Redo — tout champ de texte futur dans le Properties Panel doit implémenter ce pattern (risque de régression si oublié).

### 10.3 Bugs Corrigés Durant Cette Phase

*Aucun bug corrigé documenté (phase initiale VSSwitch).*

| ID | Titre | Sévérité | PR Fix | Résolu le |
|----|-------|----------|--------|-----------|
| — | — | — | — | — |

### 10.4 Tests de Régression

- **Couverture :** Labels de case, Undo/Redo `SetSwitchCaseLabelCommand`, fallback "Case_N" (migration v1→v2)
- **Dépendances :** Réutilise les dynamic pins de Phase 21-D — toute régression sur 21-D est une régression potentielle ici
- **Dépendance aval :** Phase 22-C utilise `BBVariableRegistry` pour le runtime display de VSSwitch — à tester si Phase 22-C est modifiée

### 10.5 Références Bug Registry

- [BUG_REGISTRY.md](../BugTracking/BUG_REGISTRY.md) — Registre centralisé
- [BUG_PROTOCOL.md](../BugTracking/BUG_PROTOCOL.md) — Protocole de gestion

---

## 11. 🔗 Références

- **PR GitHub :** [#384](https://github.com/Atlasbruce/Olympe-Engine/pull/384)
- **ROADMAP_V2.md :** Section Phase 22-A
- **Context Files :**
  - [CONTEXT_ARCHIVE.md](../CONTEXT_ARCHIVE.md) — Section Phase 22-A
  - [feature_context_21_D.md](./feature_context_21_D.md) — Phase 21-D (dynamic pins, fondation pour cette phase)
  - [feature_context_22_C.md](./feature_context_22_C.md) — Phase 22-C (étend les registries pour les variables Blackboard)

---

*Feature context généré le : 2026-03-15 10:26:37 UTC*  
*Dernière mise à jour : 2026-03-15 12:44:21 UTC*
