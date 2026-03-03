# 🔧 PLAN DE RESTAURATION - BehaviorTreeDebugWindow.cpp

**Problème**: 66 erreurs, fichier corrompu (3237 lignes)  
**Solutions**: 3 approches par ordre de préférence

---

## ⚡ SOLUTION 1: REVERT GIT (RECOMMANDÉ)

### Pourquoi c'est la meilleure option
- ✅ Restaure un état stable connu
- ✅ Rapide (5 minutes)
- ✅ Garanti de fonctionner
- ✅ Preserve l'historique git

### Comment faire

**Étape 1**: Voir les commits récents
```sh
git log --oneline -20 -- Source/AI/BehaviorTreeDebugWindow.cpp
```

**Étape 2**: Identifier un commit stable (5ème ou 6ème dans la liste)

**Étape 3**: Faire le revert
```sh
# Remplacer <HASH> par le hash du commit choisi
git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.cpp
git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.h
```

**Étape 4**: Rebuild
```sh
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug
```

**Résultat attendu**: 0 errors

---

## 🛠️ SOLUTION 2: UTILISER LES SCRIPTS FOURNIS

### Script Batch (Windows)
```cmd
REVERT_AUTO.bat
```

Le script fait tout automatiquement:
- Affiche les commits
- Suggère le 5ème
- Fait le revert
- Rebuild
- Vérifie 0 errors

### Script PowerShell
```powershell
.\Execute_Phase1_Completion.ps1
```

---

## 📝 SOLUTION 3: RESTAURATION MANUELLE (Si git inaccessible)

### Analyse des Erreurs

Les 66 erreurs sont causées par:

1. **Fonctions dupliquées** (4 fonctions × 2 = 8 définitions):
   ```cpp
   // Lignes 839-865: PREMIÈRE définition (GARDER)
   uint32_t BehaviorTreeDebugWindow::GetNodeColor(BTNodeType type) const { ... }
   const char* BehaviorTreeDebugWindow::GetNodeIcon(BTNodeType type) const { ... }
   
   // Lignes 1658+: DEUXIÈME définition (SUPPRIMER)
   uint32_t BehaviorTreeDebugWindow::GetNodeColor(BTNodeType type) const { ... }
   const char* BehaviorTreeDebugWindow::GetNodeIcon(BTNodeType type) const { ... }
   void BehaviorTreeDebugWindow::RenderInspectorPanel() { ... }
   void BehaviorTreeDebugWindow::RenderRuntimeInfo() { ... }
   ```

2. **Variable `tree` hors scope** (~40 erreurs):
   - Lignes: 970, 976, 979, 1078, 1081, 1111, 1115, 1132, 1136, 1160, 1218, 1223...
   - `tree` devrait être déclarée dans une fonction parente
   - Problème: code déplacé hors de son scope original

3. **Syntaxe invalide** (~20 erreurs):
   - Boucles `for` cassées
   - Appels de fonction incorrects

### Actions Manuelles (NON RECOMMANDÉ - 4-8h de travail)

Si vous devez absolument corriger manuellement:

#### 1. Supprimer les fonctions dupliquées (lignes 1658+)

Cherchez et supprimez les DEUXIÈMES définitions de:
- `GetNodeColor()` (vers ligne 1658)
- `GetNodeIcon()` (vers ligne 1679)
- `RenderInspectorPanel()` (vers ligne 1693)
- `RenderRuntimeInfo()` (vers ligne 1750)

#### 2. Corriger les références à `tree`

Pour chaque erreur `'tree' : identificateur non déclaré`:
- Identifier la fonction contenant l'erreur
- Ajouter la déclaration de `tree` au début de la fonction:
  ```cpp
  const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
  if (!tree) { return; }
  ```

#### 3. Corriger les boucles `for` invalides

Ligne 1223 et 1248:
```cpp
// AVANT (invalide)
for (const auto& node : tree->nodes)  // tree non déclaré

// APRÈS (valide)
const BehaviorTreeAsset* tree = /* ... */;
for (const auto& node : tree->nodes)
```

---

## 🎯 RECOMMANDATION FINALE

### Pour 99% des cas: SOLUTION 1 (Revert Git)

**Pourquoi**:
- 5 minutes vs 4-8 heures
- Garanti de fonctionner
- Pas de risque d'erreur
- Preserve l'historique

**Comment**:
```sh
# 1. Voir commits
git log --oneline -10 -- Source/AI/BehaviorTreeDebugWindow.cpp

# 2. Prendre le 5ème ou un avec message "working"/"stable"

# 3. Revert
git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.*

# 4. Rebuild
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug
```

### Si git ne fonctionne pas: SOLUTION 2 (Scripts)

```cmd
REVERT_AUTO.bat
```

### En dernier recours: SOLUTION 3 (Manuel)

**Temps estimé**: 4-8 heures  
**Risque**: Très élevé  
**Succès**: Incertain

---

## 📊 COMPARAISON

| Approche | Temps | Risque | Succès | Effort |
|----------|-------|--------|--------|--------|
| **Revert Git** | 5 min | Aucun | ✅ 100% | Minimal |
| **Scripts** | 7 min | Faible | ✅ 95% | Minimal |
| **Manuel** | 4-8h | Élevé | ⚠️ 50% | Très élevé |

---

## ✅ ÉTAPES APRÈS RESTAURATION

Quelle que soit la méthode:

1. **Vérifier la compilation**:
   ```sh
   msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug
   ```
   Attendu: 0 errors

2. **Tester F10 debugger**:
   - Lancer le jeu
   - Appuyer F10
   - Vérifier que le debugger s'ouvre
   - Sélectionner entité
   - Vérifier affichage graphe

3. **Commit**:
   ```sh
   git add Source/AI/BehaviorTreeDebugWindow.*
   git commit -m "fix: restore BehaviorTreeDebugWindow to stable state"
   ```

4. **Commit documentation**:
   ```sh
   git add Docs/Developer/*.md Scripts/*.bat *.md
   git commit -m "docs(nodegraph): Phase 1 complete"
   git push origin master
   ```

---

## 🚨 SI VOUS ÊTES BLOQUÉ

### Git ne fonctionne pas dans votre terminal

**Option A**: Ouvrir un nouveau terminal
- Git Bash (si installé)
- PowerShell (avec git dans PATH)
- CMD (avec git dans PATH)

**Option B**: Utiliser Visual Studio
- Team Explorer → Changes
- Click droit sur fichier → Reset → Commit...

**Option C**: GitHub Desktop
- Voir l'historique du fichier
- Right-click → Revert changes to commit...

---

## 📞 SUPPORT

**Besoin d'aide pour identifier le bon commit?**  
→ Voir `GUIDE_REVERT_COMMITS.md`

**Commandes exactes?**  
→ Voir `COMMANDES_EXACTES.md`

**Scripts ne fonctionnent pas?**  
→ Ouvrir un terminal avec git et faire le revert manuel (Solution 1)

---

**🎯 Action immédiate**: Ouvrir Git Bash/PowerShell → `git log --oneline -10 -- Source/AI/BehaviorTreeDebugWindow.cpp`

---

*Plan de restauration créé le 2025-02-19 | Revert Git = meilleure solution*
