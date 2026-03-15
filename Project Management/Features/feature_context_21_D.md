# 📄 Feature Context — Phase 21-D : Dynamic Pins (Sequence & Switch)

> **Créé le:** 2026-03-15 10:26:37 UTC  
> **Dernière mise à jour:** 2026-03-15 10:26:37 UTC  
> **Statut:** ✅ COMPLÉTÉ  
> **PR:** [#382](https://github.com/Atlasbruce/Olympe-Engine/pull/382)  
> **Références croisées:** [feature_context_21_A.md](./feature_context_21_A.md) · [feature_context_22_A.md](./feature_context_22_A.md)

---

## 1. 🎯 Objectifs Conceptuels

### 1.1 Problème Résolu

**Avant cette feature :** Les nœuds `VSSequence` et `VSSwitch` avaient un nombre fixe de pins exec out défini à la compilation. L'utilisateur ne pouvait pas ajouter de branches supplémentaires ni en supprimer sans modifier le code source. Cela limitait considérablement l'expressivité des graphes — un Sequence ne pouvait par exemple pas exécuter plus de N opérations sans refactoring.

**Après cette feature :** L'utilisateur peut ajouter et supprimer des pins exec out dynamiquement depuis le canvas via des boutons [+] et [-]. Ces actions sont entièrement intégrées à l'Undo/Redo — toute modification de pins est réversible.

### 1.2 But du Système

Les Dynamic Pins rendent les nœuds de flux (Sequence, Switch) aussi flexibles que leurs équivalents dans les outils visuels de référence (Unreal Engine Blueprint, Unity Visual Scripting). L'objectif est que l'utilisateur puisse construire n'importe quelle topologie de graphe sans contrainte de pins prédéfinis, tout en conservant la cohérence de l'Undo/Redo qui est une promesse centrale du Blueprint Editor.

### 1.3 Critères de Succès
- [x] Bouton [+] toujours positionné sous le dernier pin exec out (tooltip "Add Execution Output")
- [x] Bouton [-] affiché inline à droite de chaque pin (sauf pin de base)
- [x] Pin de base (Out/Case_0) non supprimable — pas de bouton [-]
- [x] Suppression d'un pin connecté : lien retiré automatiquement + `RebuildLinks()`
- [x] `AddExecPinCommand` : undoable (retire le pin ajouté) et redoable
- [x] `RemoveExecPinCommand` : undoable (restaure pin + lien) et redoable
- [x] 25 cas de tests définis et passants

---

## 2. 🏗️ Architecture

### 2.1 Vue d'ensemble

```
Canvas (ImGui)
    │
    ├── Rendu VSSequence / VSSwitch
    │   ├── Pin Out_0  [       ] ─── (pas de [-])
    │   ├── Pin Out_1  [       ] ─── [-]  ← RemoveExecPinCommand
    │   ├── Pin Out_2  [       ] ─── [-]  ← RemoveExecPinCommand
    │   └── [+]  ← AddExecPinCommand
    │
    └── UndoRedoStack
        ├── AddExecPinCommand
        │   └── Undo: supprime le pin Out_N + lien éventuel
        │   └── Redo: recrée le pin Out_N
        └── RemoveExecPinCommand
            └── stocke: nodeID, pinName, pinIndex, linkedTarget (si connecté)
            └── Undo: recrée le pin + restaure le lien
            └── Redo: supprime le pin + lien
```

### 2.2 Composants Principaux

| Composant | Rôle | Type |
|-----------|------|------|
| `AddExecPinCommand` | Commande undoable d'ajout de pin exec out | Nouveau |
| `RemoveExecPinCommand` | Commande undoable de suppression de pin exec out (avec restauration lien) | Nouveau |
| Bouton [+] dans renderer | Rendu ImGui du bouton d'ajout sous les pins | Nouveau |
| Bouton [-] inline | Rendu ImGui du bouton de suppression par pin | Nouveau |
| `RebuildLinks()` | Resynchronisation ImNodes après modification des pins | Modifié/Utilisé |

### 2.3 Flux de Données

1. **Ajout de pin :** Clic [+] → `AddExecPinCommand` créé et pushé → pin ajouté au nœud → `RebuildLinks()` → canvas mis à jour
2. **Suppression de pin :** Clic [-] sur Out_N → `RemoveExecPinCommand` créé (stocke l'état du lien si connecté) → pin et lien retirés → `RebuildLinks()` → canvas mis à jour
3. **Undo suppression :** `RemoveExecPinCommand::Undo()` → pin recrée à son index original → lien restauré si existait → `RebuildLinks()`
4. **Redo suppression :** `RemoveExecPinCommand::Redo()` → pin et lien retirés de nouveau

---

## 3. 💡 Décisions Techniques

### 3.1 Décision Principale — Stockage de l'Index dans RemoveExecPinCommand

**Approche choisie :** `RemoveExecPinCommand` stocke l'index exact de la position du pin dans la liste, en plus du nom et de l'ID du nœud. À l'Undo, le pin est réinséré exactement à sa position originale.

**Pourquoi cette approche :**
- Garantit que l'Undo restaure l'ordre visuel exact des pins
- Évite les désynchronisations visuelles après une séquence Add/Remove/Undo/Redo
- L'index est stable pendant la durée de vie de la commande dans la stack

**Alternatives rejetées :**

| Alternative | Raison du rejet |
|-------------|----------------|
| Restaurer le pin à la fin de la liste | L'ordre visuel est différent après Undo — perturbant pour l'utilisateur |
| Numéro de pin uniquement (sans index) | En cas de réordonnancement, le pin se retrouve au mauvais endroit |

**Trade-offs acceptés :**
- ✅ Fidélité parfaite de l'Undo (même position, même connexion)
- ⚠️ Complexité légèrement supérieure dans `RemoveExecPinCommand` pour stocker l'index

---

### 3.2 Décision Secondaire — Position du Bouton [+]

**Approche choisie :** Le bouton [+] est toujours rendu **sous** le dernier pin exec out, pas dans la toolbar ni dans un menu contextuel.

**Justification :** La position inline avec les pins est plus intuitive — l'utilisateur voit immédiatement où le nouveau pin sera créé. C'est le pattern utilisé par Unreal Engine Blueprint. Un menu contextuel aurait nécessité un clic supplémentaire.

---

### 3.3 Décision Tertiaire — Pin de Base Protégé

**Approche choisie :** Le premier pin exec out (Out pour Sequence, Case_0 pour Switch) ne peut jamais être supprimé. Le bouton [-] n'est pas rendu pour ce pin.

**Justification :** Un nœud Sequence ou Switch sans aucun pin de sortie est sémantiquement invalide — il bloquerait l'exécution du graphe. La protection au niveau rendu (pas de bouton) est plus simple et plus sûre que la validation côté commande.

---

## 4. 📦 Implications sur les Modules

### 4.1 Modules Directement Modifiés

| Module | Fichiers | Nature des Changements |
|--------|----------|----------------------|
| `BlueprintEditor` | `UndoRedoStack.h`, `UndoRedoStack.cpp` | Ajout de AddExecPinCommand et RemoveExecPinCommand |
| `BlueprintEditor` | `VisualScriptNodeRenderer.h`, `VisualScriptNodeRenderer.cpp` | Rendu boutons [+] et [-] |
| `BlueprintEditor` | `VisualScriptEditorPanel.cpp` | Logique de dispatch add/remove, appel RebuildLinks |
| `Tests/BlueprintEditor` | `Phase21DTest.cpp` | 25 tests headless |
| Build | `CMakeLists.txt` | Ajout cible OlympePhase21DTests |

### 4.2 Risques de Couplage

- **VisualScriptNodeRenderer → UndoRedoStack** : Le renderer crée les commandes — couplage accepté car le renderer est spécifiquement conçu pour le Blueprint Editor
- **RemoveExecPinCommand → VSLink** : La commande stocke des données de lien pour restauration — couplage nécessaire pour l'Undo fidèle

### 4.3 Impact sur les Phases Futures

- Phase 22-A (VSSwitch Enhancement) : réutilise le mécanisme de dynamic pins pour les cases du Switch avec labels personnalisés
- Phase 22-C (Parameter Dropdowns) : aucun impact direct

---

## 5. ⚠️ Matrice de Risques

| Risque | Probabilité | Impact | Mitigation |
|--------|-------------|--------|------------|
| Désynchronisation ImNodes après add/remove | Moyen | Élevé | Appel systématique de RebuildLinks() après chaque opération |
| Index de pin corrompu après séquence Undo/Redo complexe | Faible | Élevé | Tests explicites de scénarios multi-actions dans Phase21DTest |
| Pin de base supprimé par bug | Faible | Élevé | Protection au rendu (pas de bouton [-]) + assertion dans RemoveExecPinCommand |
| Lien non restauré après Undo de suppression | Faible | Moyen | RemoveExecPinCommand stocke systématiquement linkedTargetNodeID et linkedTargetPinName |

---

## 6. 📁 Détails d'Implémentation

### 6.1 Fichiers Créés

| Fichier | Rôle |
|---------|------|
| `Tests/BlueprintEditor/Phase21DTest.cpp` | 25 scénarios de tests headless |

### 6.2 Fichiers Modifiés

| Fichier | Modification |
|---------|-------------|
| `Source/BlueprintEditor/UndoRedoStack.h` | Déclaration AddExecPinCommand, RemoveExecPinCommand |
| `Source/BlueprintEditor/UndoRedoStack.cpp` | Implémentation des deux nouvelles commandes |
| `Source/BlueprintEditor/VisualScriptNodeRenderer.h` | Interface rendu boutons [+]/[-] |
| `Source/BlueprintEditor/VisualScriptNodeRenderer.cpp` | Rendu inline des boutons par pin |
| `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` | Dispatch des actions add/remove, appel RebuildLinks |
| `CMakeLists.txt` | Ajout cible OlympePhase21DTests |

### 6.3 Patterns Utilisés

- **Command Pattern** — AddExecPinCommand / RemoveExecPinCommand héritent de la base ICommand existante
- **Snapshot Pattern** — RemoveExecPinCommand prend un snapshot de l'état (pinIndex + lien) au moment de l'exécution

### 6.4 Structure de RemoveExecPinCommand (conceptuelle)

Données stockées par la commande :
- `nodeID` — identifiant du nœud propriétaire
- `pinName` — nom du pin supprimé (ex: "Out_2")
- `pinIndex` — position dans la liste pour restauration fidèle
- `linkedTargetNodeID` — ID du nœud cible si pin était connecté (-1 si non connecté)
- `linkedTargetPinName` — nom du pin cible si pin était connecté

---

## 7. ✅ Règles de Validation et Cas de Tests

### 7.1 Sélection des Cas de Tests Critiques

| # | Scénario | Préconditions | Résultat Attendu | Statut |
|---|----------|--------------|-----------------|--------|
| 1 | Add pin sur Sequence | Sequence avec Out_0 | Out_1 créé, nœud valide | ✅ |
| 2 | Undo add pin | Out_1 ajouté | Out_1 retiré, état initial restauré | ✅ |
| 3 | Redo add pin | Undo effectué | Out_1 recréé | ✅ |
| 4 | Remove pin sans lien | Out_1 existant non connecté | Out_1 retiré | ✅ |
| 5 | Undo remove (sans lien) | Out_1 retiré | Out_1 restauré à son index | ✅ |
| 6 | Remove pin avec lien | Out_1 connecté à NodeB.ExecIn | Out_1 retiré, lien retiré | ✅ |
| 7 | Undo remove (avec lien) | Out_1 et son lien retirés | Out_1 restauré + lien restauré | ✅ |
| 8 | Pin de base non supprimable | Out_0 visible | Aucun bouton [-] sur Out_0 | ✅ |
| 9 | Add → Remove → Undo → Undo | Double action | État initial exact après double Undo | ✅ |
| 10 | Multiple pins — ordre préservé | Out_0, Out_1, Out_2 | Après remove Out_1 + undo : ordre restauré | ✅ |
| 11–25 | [Scénarios Switch avec cases] | Switch avec Case_0 | Comportement identique à Sequence | ✅ |

---

## 8. 📜 Historique des Propositions Copilot

### Proposition 1 — 2026-03-14 12:00:00 UTC

**Contexte :** Spec UX des boutons [+] et [-] pour les dynamic pins.

**Proposition :**
- Bouton [+] sous le dernier pin exec out
  - Position fixe : toujours en bas des pins, jamais inline
  - Tooltip : "Add Execution Output"
  - Crée un pin nommé automatiquement : Out_1, Out_2... (Sequence) ou Case_1, Case_2... (Switch)
- Bouton [-] inline à droite de chaque pin (sauf pin de base)
  - Tooltip : "Remove Execution Output"
  - Supprime le pin et son lien éventuel
- Pin de base non supprimable
  - Out pour Sequence, Case_0 pour Switch
  - Protection au niveau rendu (pas de bouton [-])
- Undo/Redo complet avec deux nouvelles commandes
  - AddExecPinCommand : stocke nodeID + pinName
  - RemoveExecPinCommand : stocke nodeID + pinName + pinIndex + linkedTarget

**Décision :** ✅ Acceptée par @Atlasbruce le 2026-03-14 12:30:00 UTC  
**Raison :** Comportement intuitif, conforme au référentiel Unreal Engine Blueprint, Undo/Redo fidèle.

---

## 9. 🏛️ Notes Archéologiques

### Apprentissages Clés

- L'index de position est crucial pour `RemoveExecPinCommand` — sans lui, l'ordre visuel n'est pas restauré fidèlement après Undo
- `RebuildLinks()` doit être appelé systématiquement après toute modification de pins — une seule omission crée des désynchronisations ImNodes difficiles à déboguer
- La protection du pin de base au niveau rendu (pas de bouton) est plus robuste que la validation côté commande

---

## 10. 🔗 Références

- **PR GitHub :** [#382](https://github.com/Atlasbruce/Olympe-Engine/pull/382)
- **ROADMAP_V2.md :** Section Initiative E — Phase 21-D
- **Context Files :**
  - [CONTEXT_ARCHIVE.md](../CONTEXT_ARCHIVE.md) — Section Phase 21-D
  - [feature_context_22_A.md](./feature_context_22_A.md) — Phase 22-A (réutilise les dynamic pins pour les cases)

---

*Feature context généré le : 2026-03-15 10:26:37 UTC*  
*Dernière mise à jour : 2026-03-15 10:26:37 UTC*
