# VERSION STAMP — Olympe Engine

**Créé le:** 2026-03-17 10:07:51 UTC  
**Dernière mise à jour:** 2026-03-18 20:31:22 UTC  
**Projet:** Olympe Engine  
**GitHub Repo:** https://github.com/Atlasbruce/Olympe-Engine  

---

## Version Actuelle

| Champ | Valeur |
|-------|--------|
| **Version** | 0.24.0 |
| **Build Timestamp** | 2026-03-18 20:31:22 UTC |
| **Git Commit SHA** | `fecadedf207d35b70037277f646a03f43f5b6d1c` |
| **Git Branch** | `copilot/fix-compilation-failure-syntax` |
| **PR Reference** | #450 |
| **Phase** | 24-Rendering-HOTFIX |
| **Build Config** | Debug |
| **FULL_VERSION_STRING** | `OLYMPE_VERSION:0.24.0\|BUILD:2026-03-18 20:31:22 UTC\|SHA:fecadedf207d35b70037277f646a03f43f5b6d1c\|BRANCH:copilot/fix-compilation-failure-syntax\|PR:#450\|PHASE:24-Rendering-HOTFIX\|CONFIG:Debug` |

---

## Procédure de Mise à Jour

Après chaque PR mergée sur `master`, mettre à jour :

1. **`Source/System/version.h`** — mettre à jour tous les champs `constexpr const char*`
2. **Ce fichier `VERSION_STAMP.md`** — ajouter une ligne dans l'historique ci-dessous
3. **Vérifier la compilation** : `strings <exe> | grep OLYMPE_VERSION` doit retourner la nouvelle valeur

### Champs à Mettre à Jour

| Champ | Source | Format |
|-------|--------|--------|
| `MAJOR` / `MINOR` / `PATCH` | Décision équipe | Semver |
| `BUILD_TIMESTAMP` | Date/heure UTC du merge | `YYYY-MM-DD HH:MM:SS UTC` |
| `GIT_COMMIT_SHA` | `git rev-parse HEAD` | 40 caractères hexadécimaux |
| `GIT_BRANCH` | `git branch --show-current` | string (ex: `master`) |
| `PR_NUMBER` | Numéro GitHub de la PR mergée | `#NNN` |
| `PHASE` | Identifiant de la phase | `XX-Y-NomFeature` |
| `BUILD_CONFIG` | Configuration de build | `Debug` ou `Release` |
| `FULL_VERSION_STRING` | Concaténation de tous les champs | Une seule ligne continue |

---

## Historique des Versions

| Version | Date (UTC) | Git SHA (7 chars) | PR | Phase | Config |
|---------|------------|-------------------|----|-------|--------|
| 0.24.0 | 2026-03-17 10:07:51 UTC | `0000000` | #000 | 24-0-VersionStamp | Debug |
| 0.24.0 | 2026-03-18 14:50:40 UTC | `0000000` | #442 | 24-RENDERING-Complete-V1 | Debug |
| 0.24.0 | 2026-03-18 19:57:31 UTC | `ee76388` | #449 | 24-Rendering-FINAL-CORRECTION | Debug |
| 0.24.0 | 2026-03-18 20:31:22 UTC | `fecaded` | #450 | 24-Rendering-HOTFIX | Debug |

---

## Référence Build Metadata

### Format FULL_VERSION_STRING

La chaîne `FULL_VERSION_STRING` doit rester **une seule ligne continue** pour être détectable par `strings` :

```
OLYMPE_VERSION:X.Y.Z|BUILD:YYYY-MM-DD HH:MM:SS UTC|SHA:XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|BRANCH:branch|PR:#NNN|PHASE:XX-Y-Feature|CONFIG:Debug
```

### Vérification Binaire

```bash
# Vérifier que la version est embarquée dans l'exécutable :
strings OlympeEngine.exe | grep OLYMPE_VERSION

# Résultat attendu :
# OLYMPE_VERSION:0.24.0|BUILD:2026-03-17 10:07:51 UTC|SHA:0000000000000000000000000000000000000000|BRANCH:master|PR:#000|PHASE:24-0-VersionStamp|CONFIG:Debug
```

### Récupération à l'Exécution

```cpp
// Accès au FULL_VERSION_STRING depuis le code :
const char* version = Olympe::VersionStamp::FULL_VERSION_STRING;

// Afficher toutes les infos au démarrage (appel OBLIGATOIRE en premier dans main) :
Olympe::VersionStamp::PrintVersionInfo();
```

---

## Intégration avec le Système PM

| Fichier PM | Relation |
|------------|----------|
| `CONTEXT_STATUS.md` | Référence la phase active et sa version |
| `CONTEXT_CURRENT.md` | Mentionne le numéro de version courant |
| `BugTracking/BUG_REGISTRY.md` | Associe les bugs à une version précise |
| `Features/feature_context_XX_Y.md` | Contient le contexte de la phase associée à cette version |
| `ROADMAP_V2.md` | Aligne le numéro de version avec la roadmap |

---

## Checklist Compilation version.h

Avant chaque PR incluant des modifications de `version.h` :

- [ ] Aucune fonctionnalité C++17/20 utilisée
- [ ] Tous les champs sont `constexpr const char*`
- [ ] `FULL_VERSION_STRING` est une seule ligne continue (pas de retour chariot)
- [ ] `GIT_COMMIT_SHA` est exactement 40 caractères hexadécimaux
- [ ] `BUILD_TIMESTAMP` respecte le format `YYYY-MM-DD HH:MM:SS UTC`
- [ ] `PrintVersionInfo()` utilise uniquement `SYSTEM_LOG` (pas de `std::cout`)
- [ ] Aucun emoji ou caractère ASCII étendu dans les chaînes de log
- [ ] Code dans `namespace Olympe { }` avec commentaire de fermeture
- [ ] Include relatif correct : `"../system/system_utils.h"`
- [ ] Compilation 0 erreurs, 0 warnings (MSVC + GCC/Clang)
- [ ] `strings <exe> | grep OLYMPE_VERSION` retourne la nouvelle valeur
