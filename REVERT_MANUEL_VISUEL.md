# ⚡ REVERT MANUEL IMMÉDIAT - Instructions Visuelles

**Git n'est pas accessible depuis ce terminal**  
**Solution**: Faire le revert manuellement

---

## 🎯 MÉTHODE RECOMMANDÉE: Visual Studio Team Explorer

### Étape 1: Ouvrir Team Explorer dans Visual Studio

1. Ouvrir **Visual Studio**
2. Menu: **View** → **Team Explorer** (ou Ctrl+\, Ctrl+M)
3. Dans Team Explorer, cliquer sur **Home** (icône maison)
4. Cliquer sur **Changes**

### Étape 2: Voir l'historique du fichier

**Option A - Via Solution Explorer**:
1. Dans **Solution Explorer**, naviguer vers:
   - `Source/AI/BehaviorTreeDebugWindow.cpp`
2. **Right-click** sur le fichier
3. Sélectionner: **Source Control** → **View History**

**Option B - Via Team Explorer**:
1. Dans **Team Explorer**, cliquer sur **Branches**
2. Right-click sur **master** → **View History**
3. Dans la liste des commits, chercher ceux qui touchent `BehaviorTreeDebugWindow.cpp`

### Étape 3: Identifier le commit stable

Dans la fenêtre History:

1. **Regardez les 10 premiers commits**
2. **Comptez**: 1, 2, 3, 4, **5** ← Sélectionnez celui-ci
3. **Vérifiez le message du commit**:
   - ✅ Bon: "working", "stable", "fix"
   - ❌ Éviter: "WIP", "attempt", "refactor"
4. **Vérifiez la date**: Hier ou avant-hier (pas aujourd'hui)

### Étape 4: Restaurer les fichiers

**Option A - Get This Version**:
1. Right-click sur le commit stable (5ème)
2. **View Commit Details**
3. Dans la liste des fichiers, trouver:
   - `BehaviorTreeDebugWindow.cpp`
   - `BehaviorTreeDebugWindow.h`
4. Right-click sur chaque fichier
5. Sélectionner: **Get This Version**
6. Répéter pour les 2 fichiers

**Option B - Reset (Plus agressif)**:
1. Right-click sur le commit stable
2. **Reset** → **Reset and Keep Changes (Soft)**
3. Ceci va "annuler" tous les commits jusqu'à ce commit
4. Vos changements seront préservés comme "Uncommitted"

### Étape 5: Rebuild

1. Menu: **Build** → **Rebuild Solution** (ou Ctrl+Alt+F7)
2. Attendre la fin (2-3 minutes)
3. **Vérifier dans Output**:
   - Devrait afficher: **"Build succeeded"**
   - **"0 errors"**

### Étape 6: Tester

1. Lancer le jeu: **Debug** → **Start Without Debugging** (Ctrl+F5)
2. In-game, appuyer **F10**
3. Vérifier que le debugger BehaviorTree s'ouvre
4. Sélectionner une entité avec BT
5. Vérifier que le graphe s'affiche correctement

### Étape 7: Commit (Si tout OK)

Dans **Team Explorer** → **Changes**:

1. **Stage** les fichiers modifiés:
   - `BehaviorTreeDebugWindow.cpp`
   - `BehaviorTreeDebugWindow.h`

2. **Message de commit**:
   ```
   fix: revert BehaviorTreeDebugWindow to stable state
   
   Reverted to stable commit to fix 66 compilation errors.
   
   Status:
   - Build: ✅ 0 errors
   - F10 debugger: ✅ Functional
   ```

3. Cliquer: **Commit Staged**

4. Ensuite, **stage** toute la documentation:
   - Dans Changes, faire **Stage All**
   - Ou sélectionner manuellement les fichiers *.md

5. **Message de commit**:
   ```
   docs(nodegraph): Phase 1 complete - 105KB documentation
   
   Documentation:
   - 22 technical files (~105KB)
   - Complete architecture analysis
   - BTDebugAdapter placeholder
   - Automated scripts
   
   Code fixes:
   - Fixed DeleteLinkCommand.cpp
   - Reverted BehaviorTreeDebugWindow.cpp
   
   Status:
   - Phase 1: ✅ Complete
   ```

6. Cliquer: **Commit All**

7. **Push**: Cliquer sur **Sync** → **Push**

---

## 📸 CAPTURES D'ÉCRAN (Points Clés)

### Team Explorer - Home
```
[Home]
├── Changes           ← Pour voir fichiers modifiés
├── Branches          ← Pour voir branches
├── Sync              ← Pour push
└── Settings
```

### Solution Explorer - Right-click Menu
```
BehaviorTreeDebugWindow.cpp
├── Cut
├── Copy
├── ...
└── Source Control
    ├── Check Out for Edit
    ├── Get Latest Version
    ├── Compare...
    └── View History  ← CLIQUER ICI
```

### History Window
```
Commit History
┌─────────────────────────────────────────┐
│ 📅 2025-02-19  abc1234  fix: attempt... │ ← Commit 1 (récent, éviter)
│ 📅 2025-02-19  def5678  refactor: ...   │ ← Commit 2 (récent, éviter)
│ 📅 2025-02-18  ghi9012  fix: minor...   │ ← Commit 3
│ 📅 2025-02-18  jkl3456  feat: working  │ ← Commit 4
│ 📅 2025-02-17  mno7890  fix: layout... │ ← Commit 5 ✅ CHOISIR
│ 📅 2025-02-16  pqr1234  stable: ...    │ ← Commit 6 (bon aussi)
└─────────────────────────────────────────┘
     Right-click → View Commit Details
```

---

## 🆘 ALTERNATIVE: Git Bash

Si Visual Studio ne fonctionne pas, utilisez **Git Bash**:

### Ouvrir Git Bash

1. **Démarrer** → Taper "Git Bash"
2. **OU**: Dans l'explorateur Windows, naviguer vers le dossier projet
3. Right-click dans le dossier → **"Git Bash Here"**

### Commandes

```bash
# Voir les commits
git log --oneline -10 -- Source/AI/BehaviorTreeDebugWindow.cpp

# Noter le hash du 5ème commit (ex: mno7890)

# Revert
git checkout mno7890 -- Source/AI/BehaviorTreeDebugWindow.cpp
git checkout mno7890 -- Source/AI/BehaviorTreeDebugWindow.h

# Vérifier
git status

# Devrait afficher:
# modified:   Source/AI/BehaviorTreeDebugWindow.cpp
# modified:   Source/AI/BehaviorTreeDebugWindow.h
```

### Rebuild

Retourner dans **Visual Studio** → **Build** → **Rebuild Solution**

---

## ✅ VÉRIFICATION FINALE

Après le revert et rebuild:

```
✅ Output Window: "Build succeeded, 0 errors"
✅ F10 debugger: S'ouvre in-game
✅ BehaviorTree graph: S'affiche correctement
✅ Pas de crash
✅ Pas de régression
```

---

## 🎉 PHASE 1 COMPLÈTE!

Une fois le revert fait et testé:

```
✅ Build: 0 errors
✅ DeleteLinkCommand: Corrigé
✅ BehaviorTreeDebugWindow: Restauré
✅ Documentation: 105KB créée
✅ Scripts: 3 scripts automatiques
✅ Phase 1: 100% COMPLÈTE
```

---

## 📞 BESOIN D'AIDE?

**Team Explorer ne trouve pas History?**
- Vérifiez que le projet est bien un repo Git
- Team Explorer → Settings → Repository Settings

**Build échoue après revert?**
- Essayez un commit plus ancien (6ème, 7ème...)
- Relancez le process

**F10 ne fonctionne pas?**
- Vérifiez que le jeu est en mode Debug
- Vérifiez les keybindings (peut-être modifié)

---

**🎯 START**: Visual Studio → Team Explorer → View History → Get Version du 5ème commit → Rebuild → Test → Commit → Push → DONE!

---

*Guide visuel créé le 2025-02-19 | Visual Studio Team Explorer recommandé*
