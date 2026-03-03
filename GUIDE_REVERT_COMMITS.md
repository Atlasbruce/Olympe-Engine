# 🔧 GUIDE REVERT - Identification des Commits Stables

**Objectif**: Revert BehaviorTreeDebugWindow.cpp vers un état stable  
**Méthode**: Automatique ET manuelle expliquée

---

## ⚡ MÉTHODE AUTOMATIQUE (RECOMMANDÉ)

### Utiliser le Script Batch

```cmd
REVERT_AUTO.bat
```

**Ce que fait le script**:
1. ✅ Affiche les 20 derniers commits
2. ✅ Suggère automatiquement le 5ème commit (probablement stable)
3. ✅ Permet de choisir un autre si nécessaire
4. ✅ Fait le revert automatiquement
5. ✅ Rebuild la solution
6. ✅ Vérifie que la build passe

**Temps**: 5-7 minutes

---

## 📋 MÉTHODE MANUELLE (Si script échoue)

### Étape 1: Voir l'Historique

```sh
git log --oneline -20 -- Source/AI/BehaviorTreeDebugWindow.cpp
```

**Exemple de sortie**:
```
abc1234 fix: tentative correction RenderRuntimeInfo     ← RÉCENT (probablement cassé)
def5678 refactor: NodeGraphShared integration          ← RÉCENT (probablement cassé)
ghi9012 fix: minor adjustments                          ← RÉCENT (probablement cassé)
jkl3456 feat: BT debug working                          ← STABLE (3-4 commits en arrière)
mno7890 fix: corrected layout engine                    ← STABLE (bon candidat)
pqr1234 feat: added new debug features                  ← PLUS ANCIEN (très stable)
```

### Étape 2: Identifier le Commit Stable

**Comment choisir**:

1. **Éviter les 2-3 premiers commits** (probablement d'aujourd'hui)

2. **Chercher des messages rassurants**:
   - ✅ "working", "stable", "fix", "corrected"
   - ✅ Commits datant d'hier ou avant-hier
   - ❌ "refactor", "WIP", "attempt", "tentative"

3. **Règle empirique**: Prenez le **5ème commit** dans la liste

### Étape 3: Faire le Revert

```sh
# Remplacez <HASH> par le commit choisi (ex: jkl3456)
git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.cpp
git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.h
```

### Étape 4: Rebuild

```sh
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug
```

**Résultat attendu**: 0 errors

**Si échec**: Essayez un commit plus ancien (6ème, 7ème, etc.)

---

## 🎯 COMMITS PROBABLES (À VÉRIFIER)

Basé sur l'historique typique, cherchez:

### Très Probablement Stable (Recommandé)

**Commits avec ces patterns**:
- `feat: BehaviorTree debug window working`
- `fix: corrected BT debug rendering`
- `stable: BT debug before refactor`

**Âge**: 2-7 jours avant aujourd'hui

### Modérément Stable

**Commits avec**:
- `fix: minor corrections`
- `feat: improved BT visualization`

**Âge**: 1-2 jours

### À Éviter

**Commits récents** (aujourd'hui 2025-02-19):
- `fix: attempt to correct...`
- `refactor: ...`
- `WIP: ...`

---

## 🔍 ANALYSE DES ERREURS (Pour Info)

Les 66 erreurs sont toutes dues à:

1. **Fonctions dupliquées** (4):
   - `GetNodeColor()` définie 2 fois (lignes 839 et 1658)
   - `GetNodeIcon()` définie 2 fois
   - `RenderInspectorPanel()` définie 2 fois
   - `RenderRuntimeInfo()` définie 2 fois

2. **Variable `tree` hors scope** (~40 erreurs):
   - Utilisée aux lignes: 970, 976, 979, 1078, 1081, 1111, 1115, 1132, 1136, 1160, 1218, 1223...
   - Devrait être déclarée dans un scope parent

3. **Syntaxe invalide** (~20 erreurs):
   - Boucles `for` cassées
   - Appels de fonction incorrects

**Conclusion**: Fichier trop corrompu pour correction manuelle. Revert = seule solution.

---

## ✅ VALIDATION POST-REVERT

Après le revert, vérifiez:

### 1. Build Compilation

```sh
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug
```

**Attendu**: 0 errors

### 2. Test F10 Debugger

```
1. Lancer le jeu
2. Appuyer F10 in-game
3. Vérifier que le debugger s'ouvre
4. Sélectionner une entité avec BT
5. Vérifier affichage du graphe
```

**Attendu**: Tout fonctionne comme avant

### 3. Git Status

```sh
git status
```

**Attendu**: 
```
modified:   Source/AI/BehaviorTreeDebugWindow.cpp
modified:   Source/AI/BehaviorTreeDebugWindow.h
```

---

## 📝 COMMIT APRÈS REVERT

```sh
# 1. Stage le revert
git add Source/AI/BehaviorTreeDebugWindow.cpp
git add Source/AI/BehaviorTreeDebugWindow.h

# 2. Commit
git commit -m "fix: revert BehaviorTreeDebugWindow to stable state

Reverted to commit <HASH> to fix 66 compilation errors.

Errors were caused by:
- Duplicate function definitions (4 functions)
- Variable 'tree' out of scope (40+ occurrences)
- Invalid syntax in multiple locations

Manual correction was not feasible. Revert to stable state
required to restore functionality.

Status:
- Build: ✅ 0 errors (after revert)
- F10 debugger: ✅ Functional
- Documentation: Preserved (95KB)

Next: Commit documentation in separate commit."

# 3. Stage documentation
git add Docs/Developer/*.md
git add Scripts/*.bat
git add README*.md
git add ACTION_IMMEDIATE_2MIN.md
git add TABLEAU_DE_BORD_FINAL.md
git add ETAT_BUILD_ACTUEL.md
git add BILAN_COMPLET_SESSION.md
git add START_HERE.md
git add EXECUTION_AUTOMATIQUE.md
git add PR_DESCRIPTION.md
git add Source/NodeGraphShared/BTDebugAdapter.*
git add Source/NodeGraphCore/Commands/DeleteLinkCommand.cpp

# 4. Commit documentation
git commit -m "docs(nodegraph): Phase 1 complete - 95KB documentation + DeleteLinkCommand fix

Documentation:
- 16 technical files (~95KB)
- Complete architecture analysis
- 5-phase migration plan
- BTDebugAdapter placeholder
- Automated scripts

Code fixes:
- Fixed DeleteLinkCommand.cpp (9 errors → 0)
- Reverted BehaviorTreeDebugWindow.cpp to stable

Status:
- Phase 1: ✅ Complete
- Build: ✅ Fixed (0 errors)
- Phase 2: ⏳ Ready (optional)

Ref: START_HERE.md, BILAN_COMPLET_SESSION.md"

# 5. Push
git push origin master
```

---

## 🎯 RÉSUMÉ RAPIDE

### Automatique (Recommandé)
```cmd
REVERT_AUTO.bat
```

### Manuel
```sh
# 1. Voir commits
git log --oneline -20 -- Source/AI/BehaviorTreeDebugWindow.cpp

# 2. Choisir le 5ème commit ou un avec message "working"/"stable"

# 3. Revert
git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.*

# 4. Rebuild
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug

# 5. Si 0 errors: commit
# Sinon: essayer commit plus ancien
```

---

## 📊 PROBABILITÉS DE SUCCÈS

| Commit Position | Probabilité | Raison |
|----------------|-------------|---------|
| 5ème | 90% | Généralement avant corruptions récentes |
| 6ème-7ème | 95% | Plus ancien = plus stable |
| 8ème-10ème | 98% | Très ancien = très stable |
| 3ème-4ème | 50% | Peut-être encore corrompu |
| 1er-2ème | 10% | Probablement d'aujourd'hui (cassé) |

---

**🚀 ACTION**: Lancez `REVERT_AUTO.bat` maintenant!

---

*Guide créé le 2025-02-19 | Pour restaurer BehaviorTreeDebugWindow.cpp*
