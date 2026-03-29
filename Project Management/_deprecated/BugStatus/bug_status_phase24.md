# 🐛 Bug Status — Phase 24 Condition Preset System

> **Créé le :** 2026-03-17 13:37:54 UTC  
> **Dernière mise à jour :** 2026-03-17 13:37:54 UTC  
> **Phase :** 24 — Global Condition Preset System Refactor  
> **Déclenché par :** Merge PR #432  
> **Fix :** PR #430 (DRAFT — pending merge after validation)

---

## 📋 Vue d'Ensemble

Le merge de PR #432 (Phase 24.2–24.4 : NodeConditionsPanel, DynamicDataPinManager, NodeBranchRenderer) a introduit des erreurs de build dans les deux projets Visual Studio (`Olympe Engine` et `OlympeBlueprintEditor`).

**Statut global :** ✅ RÉSOLU — voir section Résolution

---

## 🔴 Erreurs de Compilation (Post-merge PR #432)

### Tableau Récapitulatif

| # | Code Erreur | Fichier Concerné | Cause | Statut |
|---|---|---|---|---|
| 1 | MSB8027 | `Olympe Engine.vcxproj.filters` | Entrée stale `Source\BlueprintEditor\ConditionPreset.cpp` | ✅ FIXED |
| 2 | LNK4042 | `ConditionPreset.obj` | Objet compilé 2× → symbole dupliqué | ✅ FIXED |
| 3 | LNK2005 | `ConditionPresetEditDialog.obj`, `ConditionPresetLibraryPanel.obj` | Constructeur `ConditionPreset::ConditionPreset()` déjà défini dans `ConditionPreset.obj` | ✅ FIXED |
| 4 | C2061 | `ConditionPresetLibraryPanel.h` | `ConditionPresetRegistry` non reconnu — include manquant | 🔶 PR #430 |
| 5 | C2143 / C4430 / C2334 | `ConditionPresetLibraryPanel.h` | Cascade de C2061 | 🔶 PR #430 |
| 6 | C7550 / C2065 | `ConditionPresetLibraryPanel.cpp` | Cascade de C2061 | 🔶 PR #430 |
| 7 | C1083 | `.vcxproj` | Fichier référencé introuvable (conséquence MSB8027) | ✅ FIXED |

---

## 🔍 Analyse Détaillée par Erreur

### Erreur 1 — MSB8027 : Fichiers en double

```
warning MSB8027: Deux ou plusieurs fichiers avec le nom ConditionPreset.cpp
généreront des sorties vers le même emplacement.
Les fichiers impliqués sont :
  Source\BlueprintEditor\ConditionPreset.cpp
  Source\Editor\ConditionPreset\ConditionPreset.cpp
```

**Cause :** Le fichier `Olympe Engine.vcxproj.filters` conservait une entrée `<ClCompile>` stale vers l'ancien emplacement `Source\BlueprintEditor\ConditionPreset.cpp` (supprimé lors de la migration Phase 24.0). Le fichier `.vcxproj` principal était déjà correct (seul `Source\Editor\ConditionPreset\ConditionPreset.cpp` référencé), mais la désynchronisation `.vcxproj` / `.filters` suffisait à déclencher MSB8027.

**Fix :** Suppression de la ligne stale dans `Olympe Engine.vcxproj.filters` :
```xml
<!-- SUPPRIMÉ -->
<ClCompile Include="Source\BlueprintEditor\ConditionPreset.cpp" />
<!-- SUPPRIMÉ -->
<ClInclude Include="Source\BlueprintEditor\ConditionPreset.h" />
```

---

### Erreurs 2–3 — LNK4042 / LNK2005 : Symbole dupliqué

```
warning LNK4042: objet spécifié plus d'une fois ; définitions supplémentaires ignorées
error LNK2005: "public: __cdecl Olympe::ConditionPreset::ConditionPreset(void)"
  déjà défini(e) dans ConditionPreset.obj
```

**Cause :** Conséquence directe de MSB8027 — deux TU (translation units) fournissaient le même symbole `Olympe::ConditionPreset::ConditionPreset()`. L'éditeur de liens trouvait la définition 2× et signalait LNK2005 pour tous les `.obj` qui linkaient contre les deux copies.

**Fichiers affectés :** `ConditionPresetEditDialog.obj`, `ConditionPresetLibraryPanel.obj`

**Fix :** Résolu par la suppression de l'entrée stale (Erreur 1).

---

### Erreur 4 — C2061 : Identificateur inconnu `ConditionPresetRegistry`

```
error C2061: erreur de syntaxe : identificateur 'ConditionPresetRegistry'
```

**Cause :** `ConditionPresetLibraryPanel.h` utilise le type `Olympe::ConditionPresetRegistry` (comme membre ou paramètre) mais n'inclut pas le header correspondant.

**Include manquant :**
```cpp
// À ajouter dans ConditionPresetLibraryPanel.h
#include "../../Editor/ConditionPreset/ConditionPresetRegistry.h"
```

**Fix :** PR #430 (DRAFT)

---

### Erreurs 5–6 — C2143 / C4430 / C2334 / C7550 / C2065 : Cascade de C2061

```
error C2143: erreur de syntaxe : ';' manquant avant '*'
error C7550: '__declspec(property)' : concept non reconnu
```

**Cause :** Cascade directe de C2061. Lorsque le compilateur ne reconnaît pas `ConditionPresetRegistry`, toutes les déclarations ultérieures dans `ConditionPresetLibraryPanel.h` et `ConditionPresetLibraryPanel.cpp` qui en dépendent échouent en cascade.

**Fix :** Résolu par PR #430 (même correction que C2061).

---

### Erreur 7 — C1083 : Fichier introuvable

```
fatal error C1083: Impossible d'ouvrir le fichier : ... ConditionPreset.cpp
```

**Cause :** Conséquence de MSB8027 — le `.vcxproj` essayait de compiler un fichier inexistant à l'ancien chemin.

**Fix :** Résolu par la suppression de l'entrée stale (Erreur 1).

---

## ✅ Résolution

### Corrections Appliquées

| Correction | Fichier Modifié | Méthode | Statut |
|---|---|---|---|
| Suppression entrée stale `ConditionPreset.cpp` | `Olympe Engine.vcxproj.filters` | `<ClCompile>` line removed | ✅ Appliqué |
| Suppression entrée stale `ConditionPreset.h` | `Olympe Engine.vcxproj.filters` | `<ClInclude>` line removed | ✅ Appliqué |
| Include manquant `ConditionPresetRegistry.h` | `Source/Editor/Panels/ConditionPresetLibraryPanel.h` | `#include` ajouté | 🔶 PR #430 |

### PR #430 — Include Fix (DRAFT)

- **Branche :** Phase 24.1 include fixes  
- **Statut :** DRAFT — validation en cours  
- **Débloque :** Phase 24.5 Runtime Evaluation  
- **Dépendances :** Aucune (fix isolé, pas de régression attendue)

---

## 🛡️ Prévention des Régressions

### Règles à respecter pour les prochaines phases

1. **Synchronisation `.vcxproj` / `.vcxproj.filters`** — Toute suppression de fichier source doit être reflétée dans les DEUX fichiers. Ne pas se fier à Visual Studio pour gérer automatiquement la cohérence.

2. **Validation des includes après déplacement de fichiers** — Lorsqu'un fichier est déplacé (`BlueprintEditor/ → Editor/ConditionPreset/`), tous les headers qui le référencent doivent être mis à jour avant le merge.

3. **Build local obligatoire avant PR** — Compiler en `Debug x64` les deux projets (`Olympe Engine` ET `OlympeBlueprintEditor`) avant toute PR touchant des headers partagés.

4. **Ordre de merge recommandé** — Merger les PRs d'include-fix (PR #430) avant les PRs qui dépendent des types qu'ils exposent (PR #432).

---

## 📁 Fichiers de Référence

| Fichier | Rôle |
|---|---|
| `Source/Editor/ConditionPreset/ConditionPreset.h` | Type central — à inclure partout où `ConditionPreset` est utilisé |
| `Source/Editor/ConditionPreset/ConditionPresetRegistry.h` | Singleton registry — à inclure dans tout panel/dialog qui manipule des presets |
| `Source/Editor/Panels/ConditionPresetLibraryPanel.h` | Fichier source du bug C2061 |
| `Olympe Engine.vcxproj.filters` | Source du bug MSB8027 (entrée stale supprimée) |

---

**Last Updated:** 2026-03-17 13:37:54 UTC
