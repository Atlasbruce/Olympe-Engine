# ⚡ COMMANDES EXACTES - Copier/Coller

**2 options**: Script automatique OU manuel

---

## Option 1: SCRIPT AUTOMATIQUE (Recommandé) ⚡

```cmd
REVERT_AUTO.bat
```

**C'est tout!** Le script fait le reste:
- Affiche les commits
- Suggère le 5ème (probablement stable)
- Fait le revert
- Rebuild
- Donne les commandes pour commit

**Temps**: 5-7 minutes

---

## Option 2: MANUEL (Si script échoue)

### Étape 1: Voir Commits

```sh
git log --oneline -20 -- Source/AI/BehaviorTreeDebugWindow.cpp
```

### Étape 2: Identifier Commit Stable

**Règle**: Prenez le **5ème commit** ou cherchez:
- Messages: "working", "stable", "fix"
- Date: Hier ou avant-hier
- **Évitez les 2-3 premiers** (probablement d'aujourd'hui)

### Étape 3: Revert

```sh
# Remplacez <HASH> par le hash du 5ème commit
git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.cpp
git checkout <HASH> -- Source/AI/BehaviorTreeDebugWindow.h
```

### Étape 4: Rebuild

```sh
msbuild "Olympe Engine.sln" /t:Rebuild /p:Configuration=Debug
```

**Attendu**: 0 errors

**Si échec**: Essayez le 6ème ou 7ème commit

### Étape 5: Commit

```sh
# Commit le revert
git add Source/AI/BehaviorTreeDebugWindow.*
git commit -m "fix: revert BehaviorTreeDebugWindow to stable state"

# Commit la documentation
git add Docs/Developer/*.md Scripts/*.bat README*.md *.md
git add Source/NodeGraphShared/BTDebugAdapter.*
git add Source/NodeGraphCore/Commands/DeleteLinkCommand.cpp
git commit -m "docs(nodegraph): Phase 1 complete - 95KB documentation"

# Push
git push origin master
```

---

## 🎯 RÉSUMÉ

**Script**: `REVERT_AUTO.bat` (fait tout)  
**Manuel**: 5 étapes (5-10 minutes)

**Résultat**: Build compile (0 errors)

---

## ❓ Si Problèmes

**Script ne trouve pas git**:
```cmd
set PATH=%PATH%;C:\Program Files\Git\bin
REVERT_AUTO.bat
```

**Build échoue après revert**:
- Essayez un commit plus ancien (6ème, 7ème, etc.)
- Relancez `REVERT_AUTO.bat` et choisissez un autre commit

**Git checkout échoue**:
- Vérifiez que le hash est correct
- Essayez: `git checkout HEAD~5 -- Source/AI/BehaviorTreeDebugWindow.cpp`

---

**🚀 ACTION**: Lancez `REVERT_AUTO.bat` maintenant!

---

*Commandes prêtes pour copier/coller | 2025-02-19*
