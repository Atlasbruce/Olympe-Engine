# 📄 Feature Context — Phase 21-B : VSGraphVerifier UI Integration

> **Créé le:** 2026-03-15 10:26:37 UTC  
> **Dernière mise à jour:** 2026-03-15 10:26:37 UTC  
> **Statut:** ✅ COMPLÉTÉ  
> **PR:** [#381](https://github.com/Atlasbruce/Olympe-Engine/pull/381)  
> **Références croisées:** [feature_context_21_A.md](./feature_context_21_A.md) · [feature_context_21_C.md — à créer]

---

## 1. 🎯 Objectifs Conceptuels

### 1.1 Problème Résolu

**Avant cette feature :** Le `VSGraphVerifier` (Phase 21-A) produisait une liste d'issues structurées mais aucune interface n'existait pour les présenter à l'utilisateur. Les erreurs et avertissements étaient ignorés ou nécessitaient une consultation du log console, inaccessible lors de l'édition visuelle.

**Après cette feature :** Un panneau dédié dans le Blueprint Editor affiche toutes les issues de validation avec leurs codes, sévérités et descriptions. L'utilisateur peut cliquer sur une issue pour naviguer directement vers le nœud fautif dans le canvas. Un badge rouge sur la toolbar signale la présence d'erreurs.

### 1.2 But du Système

L'intégration UI du Graph Verifier transforme la validation technique (Phase 21-A) en outil de productivité utilisateur. Le principe est celui des panneaux "Problems" des IDEs modernes (VS Code, Visual Studio) : une liste centralisée et cliquable des erreurs, avec navigation directe vers la source du problème dans l'éditeur.

### 1.3 Critères de Succès
- [x] Panel scrollable listant toutes les `VSVerificationIssue` avec icône colorée par sévérité
- [x] Click sur une issue → sélection du nœud fautif dans le canvas ImNodes
- [x] Badge rouge visible sur la toolbar si des erreurs (ERROR) sont présentes
- [x] Toggle Auto-verify : redéclenche `Verify()` après chaque `PushCommand`
- [x] Distinction visuelle claire entre ERROR (rouge), WARNING (orange), INFO (bleu)

---

## 2. 🏗️ Architecture

### 2.1 Vue d'ensemble

```
Toolbar  ──────────── [Verify Button]  [🔴 Badge si ERRORs]
                              │
                              ▼ appelle Verify()
                     VSGraphVerifier
                              │ retourne issues
                              ▼
                     Issues Panel (scrollable)
                     ┌────────────────────────┐
                     │ 🔴 E001 Node#12 — ...  │ ←── click → sélection canvas
                     │ 🟠 W003 Node#5  — ...  │
                     │ 🔵 I001 Node#8  — ...  │
                     └────────────────────────┘
                              │ sélection nœud
                              ▼
                     Canvas ImNodes ─── node surbrillance
```

### 2.2 Composants Principaux

| Composant | Rôle | Type |
|-----------|------|------|
| Issues Panel | Panel ImGui scrollable affichant les issues | Nouveau |
| Toolbar Badge | Indicateur visuel rouge/vert selon présence d'ERRORs | Nouveau |
| Auto-verify Toggle | Bouton toggle pour vérification automatique après chaque action | Nouveau |
| Node Focus Logic | Logique de sélection et centrage sur un nœud depuis le panel | Nouveau |

### 2.3 Flux de Données

1. L'utilisateur clique sur "Verify" dans la toolbar OU une action modifie le graphe (auto-verify)
2. `VSGraphVerifier::Verify()` est appelé avec l'état courant du graphe
3. Les issues retournées sont stockées dans `VisualScriptEditorPanel` pour l'affichage
4. Le badge de la toolbar est mis à jour (rouge si ERROR, vert si aucune erreur)
5. Le panel d'issues est rendu avec les nouvelles données
6. Au click sur une issue, le canvas centre la vue sur le nœud correspondant

---

## 3. 💡 Décisions Techniques

### 3.1 Décision Principale — Panel Dédié vs Overlay Canvas

**Approche choisie :** Panel séparé (dockable) affichant les issues sous forme de liste, inspiré des IDEs modernes.

**Pourquoi cette approche :**
- L'overlay sur le canvas surchargerait visuellement l'espace d'édition
- Un panel séparé permet de scroller à travers de nombreuses issues sans gêner l'édition
- Le modèle IDE (panel Problems) est familier aux développeurs et donc intuitif
- Compatible avec le système de docking ImGui existant

**Alternatives rejetées :**

| Alternative | Raison du rejet |
|-------------|----------------|
| Tooltip au survol du nœud fautif | Trop discret — l'utilisateur doit passer sa souris sur chaque nœud pour voir les erreurs |
| Overlay rouge sur le nœud seulement | Manque de détail — pas de message lisible, pas de vue globale des erreurs |
| Modal dialog bloquant | Interruption trop agressive du workflow d'édition |

**Trade-offs acceptés :**
- ✅ Vue globale de toutes les erreurs simultanément
- ✅ Navigation directe vers la source de chaque erreur
- ⚠️ Un panel supplémentaire prend de la place écran (compensé par le docking)

---

### 3.2 Décision Secondaire — Auto-verify après PushCommand

**Approche choisie :** Toggle permettant à l'utilisateur d'activer/désactiver la vérification automatique après chaque commande undoable.

**Justification :** La vérification continue est utile pendant l'édition mais peut ralentir sur des graphes complexes. Le toggle donne le contrôle à l'utilisateur — actif par défaut pour les petits graphes, désactivable pour les graphes larges.

---

## 4. 📦 Implications sur les Modules

### 4.1 Modules Directement Modifiés

| Module | Fichiers | Nature des Changements |
|--------|----------|----------------------|
| `BlueprintEditor` | `VisualScriptEditorPanel.h`, `VisualScriptEditorPanel.cpp` | Ajout du panel issues, toolbar badge, auto-verify logic |

### 4.2 Risques de Couplage

- **VisualScriptEditorPanel → VSGraphVerifier** : Couplage direct — le panel possède une instance de verifier et stocke les issues
- **Issues Panel → ImNodes** : Le click sur une issue utilise l'API ImNodes pour centrer/sélectionner le nœud

### 4.3 Impact sur les Phases Futures

- Phase 21-C (Pre-save/Pre-exec) : réutilise la logique de `Verify()` déjà intégrée dans l'EditorPanel
- Phase 22-C : les nouvelles règles (E020–E025) apparaissent automatiquement dans le panel sans modification

---

## 5. ⚠️ Matrice de Risques

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Performance auto-verify sur grand graphe | Moyen | Moyen | Toggle utilisateur pour désactiver ; throttling si nécessaire |
| Désynchronisation nœud sélectionné / canvas view | Faible | Faible | Utiliser l'API ImNodes::SetNodeSelected + FocusNode |
| Panel vide confus (pas d'issues = tout OK ?) | Faible | Faible | Message "✅ No issues found" quand la liste est vide |

---

## 6. 📁 Détails d'Implémentation

### 6.1 Fichiers Modifiés

| Fichier | Modification |
|---------|-------------|
| `Source/BlueprintEditor/VisualScriptEditorPanel.h` | Ajout membres : issues list, auto-verify flag, render methods |
| `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` | Implémentation : issues panel render, toolbar badge, auto-verify trigger |

### 6.2 Patterns Utilisés

- **Observer-like** — le panel "observe" l'état de vérification et met à jour l'affichage
- **Lazy Evaluation** — `Verify()` n'est appelé que si auto-verify est activé ou sur demande explicite

---

## 7. ✅ Règles de Validation et Cas de Tests

### 7.1 Cas de Tests (Phase 21-B)

| # | Scénario | Préconditions | Résultat Attendu | Statut |
|---|----------|--------------|-----------------|--------|
| 1 | Graphe sans erreur | Graphe valide | Panel affiche "No issues", badge vert | ✅ |
| 2 | Graphe avec ERROR | E001 déclenché | Badge rouge, issue listée en rouge | ✅ |
| 3 | Graphe avec WARNING | W001 déclenché | Badge vert/orange, issue listée en orange | ✅ |
| 4 | Click sur issue ERROR | Node#12 fautif | Node#12 sélectionné et centré dans canvas | ✅ |
| 5 | Auto-verify ON, ajout nœud | Graphe modifié | Verify() re-déclenché automatiquement | ✅ |
| 6 | Auto-verify OFF | Toggle désactivé | Verify() non déclenché automatiquement | ✅ |
| 7 | Multiple issues | 3 ERRORs + 2 WARNINGs | 5 issues listées, ordonnées par sévérité | ✅ |
| 8 | Correction d'erreur + re-verify | E001 corrigé | Issue disparaît du panel | ✅ |

---

## 8. 📜 Historique des Propositions Copilot

### Proposition 1 — 2026-03-14 10:00:00 UTC

**Contexte :** Design du panel UI pour les résultats de validation.

**Proposition :**
- Panel scrollable dockable à la fenêtre Blueprint Editor
  - Icône colorée par sévérité (🔴 ERROR, 🟠 WARNING, 🔵 INFO)
  - Affichage : code règle + ID nœud + message descriptif
  - Click sur une ligne → focus nœud dans canvas
- Badge sur toolbar (bouton "Verify")
  - Rouge avec compteur si ERRORs présents
  - Vert si aucune issue ou seulement WARNINGs/INFOs
- Toggle "Auto-verify" dans toolbar
  - Par défaut : activé
  - Désactivable pour graphes complexes

**Décision :** ✅ Acceptée  
**Raison :** Design clair, familier (modèle IDE), non bloquant pour le workflow d'édition.

---

## 9. 🏛️ Notes Archéologiques

### Apprentissages Clés

- L'API ImNodes pour la sélection et le focus de nœuds fonctionne bien pour la navigation depuis un panel externe
- Le toggle auto-verify est crucial pour la performance — sans lui, les utilisateurs avec de grands graphes auraient des ralentissements perceptibles
- L'affichage "No issues found" est important UX — un panel vide sans message est ambigu

---

## 10. 🔗 Références

- **PR GitHub :** [#381](https://github.com/Atlasbruce/Olympe-Engine/pull/381)
- **ROADMAP_V2.md :** Section Initiative A — Phase 21-B
- **Context Files :**
  - [CONTEXT_ARCHIVE.md](../CONTEXT_ARCHIVE.md) — Section Phase 21-B
  - [feature_context_21_A.md](./feature_context_21_A.md) — Phase 21-A (VSGraphVerifier — fournit les issues)

---

*Feature context généré le : 2026-03-15 10:26:37 UTC*  
*Dernière mise à jour : 2026-03-15 10:26:37 UTC*
