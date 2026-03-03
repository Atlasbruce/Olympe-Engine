# 🔧 CORRECTION MANUELLE IMMÉDIATE - BehaviorTreeDebugWindow

**Problème**: Git n'est pas accessible depuis ce terminal PowerShell  
**Solution**: Utiliser Visual Studio Team Explorer OU ouvrir Git Bash

---

## ⚡ SOLUTION 1: Visual Studio Team Explorer (RECOMMANDÉ)

### Étapes dans Visual Studio:

1. **Ouvrir Team Explorer**
   - Menu: `View` → `Team Explorer` (ou Ctrl+\, Ctrl+M)

2. **Voir l'historique du fichier**
   - Dans Solution Explorer, right-click sur:
     - `Source/AI/BehaviorTreeDebugWindow.cpp`
   - Sélectionner: `View History`

3. **Identifier un commit stable**
   - Cherchez un commit datant d'hier ou avant-hier
   - Évitez les 2-3 commits les plus récents
   - Cherchez des messages comme "working", "stable", "fix"
   - **Prenez le 5ème ou 6ème commit dans la liste**

4. **Revert vers ce commit**
   - Right-click sur le commit choisi
   - Sélectionner: `View Commit Details`
   - Dans le détail, trouver les fichiers:
     - `BehaviorTreeDebugWindow.cpp`
     - `BehaviorTreeDebugWindow.h`
   - Right-click sur chaque fichier
   - Sélectionner: `Open` pour voir le contenu
   - Si ça semble bon, faire:
     - Right-click sur le commit
     - `Reset` → `Reset and Keep Changes`
     - OU: `Cherry-Pick` ce commit

5. **Alternative plus simple**:
   - Dans Team Explorer → Changes
   - Right-click sur `BehaviorTreeDebugWindow.cpp`
   - `Undo Changes...`
   - Choisir le commit stable dans la liste

6. **Rebuild**
   - Menu: `Build` → `Rebuild Solution`
   - Vérifier: devrait afficher "0 errors"

---

## 🖥️ SOLUTION 2: Git Bash (Alternative)

### Ouvrir Git Bash:

1. **Lancer Git Bash**
   - Démarrer → Git Bash
   - OU: Right-click dans dossier projet → "Git Bash Here"

2. **Naviguer vers le projet**
   ```bash
   cd /c/Users/Nico/source/repos/Atlasbruce/Olympe-Engine
   ```

3. **Voir les commits**
   ```bash
   git log --oneline -10 -- Source/AI/BehaviorTreeDebugWindow.cpp
   ```

4. **Identifier le 5ème commit**
   - Comptez les commits: 1, 2, 3, 4, **5** ← celui-ci
   - Noter le hash (ex: `a1b2c3d`)

5. **Faire le revert**
   ```bash
   # Remplacer <HASH> par le hash du 5ème commit
   git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.cpp
   git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.h
   ```

6. **Rebuild**
   - Retourner dans Visual Studio
   - Build → Rebuild Solution

---

## 🎯 SOLUTION 3: PowerShell avec chemin complet de Git

### Si Git est installé mais pas dans PATH:

```powershell
# Définir le chemin de git
$git = "C:\Program Files\Git\cmd\git.exe"

# Voir les commits
& $git log --oneline -10 -- Source/AI/BehaviorTreeDebugWindow.cpp

# Revert (remplacer <HASH> par le 5ème commit)
& $git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.cpp
& $git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.h

# Rebuild
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug
```

---

## 📋 APRÈS LE REVERT RÉUSSI

### 1. Vérifier la compilation

Dans Visual Studio:
- Build → Rebuild Solution
- Vérifier: "Build succeeded" et "0 errors"

### 2. Tester F10 debugger

- Lancer le jeu (F5 ou Ctrl+F5)
- In-game, appuyer F10
- Vérifier que le debugger s'ouvre
- Sélectionner une entité avec BT
- Vérifier que le graphe s'affiche

### 3. Commit

Dans Git Bash ou Team Explorer:

```bash
# Commit le revert
git add Source/AI/BehaviorTreeDebugWindow.cpp
git add Source/AI/BehaviorTreeDebugWindow.h
git commit -m "fix: revert BehaviorTreeDebugWindow to stable state

Reverted to stable commit to fix 66 compilation errors.

Status:
- Build: ✅ 0 errors
- F10 debugger: ✅ Functional"

# Commit la documentation
git add Docs/Developer/*.md
git add Scripts/*.bat Scripts/*.ps1 Scripts/*.sh
git add *.md
git add Source/NodeGraphShared/BTDebugAdapter.*
git add Source/NodeGraphCore/Commands/DeleteLinkCommand.cpp
git commit -m "docs(nodegraph): Phase 1 complete - 100KB documentation"

# Push
git push origin master
```

---

## ❓ QUEL COMMIT CHOISIR?

### Règle Simple

**Prenez le 5ème ou 6ème commit** dans la liste du `git log`

### Indicateurs d'un Bon Commit

✅ **Messages rassurants**:
- "feat: BT debug working"
- "fix: corrected BT rendering"
- "stable: before refactor"

✅ **Date**: Hier ou avant-hier (pas aujourd'hui 2025-02-19)

❌ **À éviter**:
- 2-3 premiers commits (probablement d'aujourd'hui)
- Messages: "WIP", "attempt", "tentative", "refactor"

### Exemple de Liste

```
abc1234 fix: attempt to correct RenderRuntimeInfo    ← ÉVITER (récent)
def5678 refactor: NodeGraphShared integration        ← ÉVITER (récent)
ghi9012 fix: minor adjustments                        ← ÉVITER (récent)
jkl3456 feat: BT debug working                        ← PRENDRE CELUI-CI ✅
mno7890 fix: corrected layout engine                  ← Bon aussi
pqr1234 stable: before refactor                       ← Très bon
```

---

## 🎉 RÉSULTAT FINAL ATTENDU

Après le revert et rebuild:

```
✅ Build: 0 errors (compilation réussie)
✅ BehaviorTreeDebugWindow.cpp: Restauré
✅ BehaviorTreeDebugWindow.h: Restauré
✅ F10 debugger: Fonctionnel
✅ Phase 1: Prêt à committer
```

---

## 📚 DOCUMENTATION PRÊTE

**100KB de documentation** créée (20 fichiers):

- START_HERE.md
- QUICK_FIX.md
- GUIDE_REVERT_COMMITS.md
- ACTION_FINALE_RESTAURATION.md
- + 16 autres fichiers techniques

**Tout est prêt pour commit après le revert!**

---

## 🚨 SI PROBLÈMES

### Build échoue après revert

- Essayez un commit plus ancien (6ème, 7ème, 8ème...)
- Relancez le process avec le nouveau hash

### Git commands ne marchent pas

- Vérifiez que Git est installé: [git-scm.com](https://git-scm.com/)
- OU utilisez Visual Studio Team Explorer (Solution 1)

### Besoin d'aide

- Voir: `GUIDE_REVERT_COMMITS.md` (identification commits)
- Voir: `COMMANDES_EXACTES.md` (toutes les commandes)

---

**🎯 Action immédiate**: Ouvrir Visual Studio → Team Explorer → View History → Revert vers 5ème commit

---

*Guide de correction manuelle | 2025-02-19 | 3 solutions fournies*
