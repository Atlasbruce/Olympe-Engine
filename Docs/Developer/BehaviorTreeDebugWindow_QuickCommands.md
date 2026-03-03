# ⚡ COMMANDES RAPIDES - Reprendre Migration
## Guide Express pour Continuer le Travail
**Date**: 2025-02-19

---

## 🚀 REPRENDRE LE TRAVAIL (3 Commandes)

```powershell
# 1. Vérifier état actuel
cd "C:\Users\Nico\source\repos\Atlasbruce\Olympe-Engine"
(Get-Content "Source\AI\BehaviorTreeDebugWindow.cpp").Count
# Devrait afficher ~2900 (avec code orphelin)

# 2. Ouvrir fichiers clés
code "Source\AI\BehaviorTreeDebugWindow.cpp"
code "Docs\Developer\BehaviorTreeDebugWindow_Cleanup_Plan.md"

# 3. Continuer avec Copilot
# Dire : "Continue le nettoyage de BehaviorTreeDebugWindow.cpp en suivant le Cleanup Plan"
```

---

## 📊 VÉRIFICATIONS RAPIDES

### État Migration
```powershell
# Vérifier nombre de lignes (doit être ~2900 actuellement, cible 1700)
(Get-Content "Source\AI\BehaviorTreeDebugWindow.cpp").Count

# Chercher méthodes obsolètes (ne doivent PAS exister)
Select-String -Path "Source\AI\BehaviorTreeDebugWindow.cpp" -Pattern "void.*RenderNode\(|void.*ApplyZoomToStyle\("

# Chercher duplications (ne doit apparaître qu'UNE fois)
Select-String -Path "Source\AI\BehaviorTreeDebugWindow.cpp" -Pattern "uint32_t.*GetNodeColor"
```

### Compilation
```powershell
# Build rapide
msbuild "Olympe Engine.sln" /t:Build /p:Configuration=Debug /v:quiet

# Build avec log
msbuild "Olympe Engine.sln" /t:Clean,Build /p:Configuration=Debug /v:minimal > build.log 2>&1
type build.log | Select-String "error|warning"
```

---

## 🔄 ROLLBACK (Si Problème)

```powershell
# Restaurer backup
Copy-Item "Source\AI\BehaviorTreeDebugWindow.cpp.backup" "Source\AI\BehaviorTreeDebugWindow.cpp" -Force

# Vérifier restauration
(Get-Content "Source\AI\BehaviorTreeDebugWindow.cpp").Count
# Devrait afficher ~3606 (version originale)

# Recompiler
msbuild "Olympe Engine.sln" /t:Clean,Build /p:Configuration=Debug
```

---

## 📁 FICHIERS CLÉS

### À Modifier
```
Source\AI\BehaviorTreeDebugWindow.cpp  # Nettoyer code orphelin (~700 LOC)
Source\AI\BehaviorTreeDebugWindow.h    # Supprimer déclarations obsolètes
```

### Documentation
```
Docs\Developer\BehaviorTreeDebugWindow_Cleanup_Plan.md    # Plan détaillé
Docs\Developer\BehaviorTreeDebugWindow_MigrationStatus.md # État actuel
Docs\Developer\BehaviorTreeDebugWindow_Summary.md         # Résumé complet
```

### Backup
```
Source\AI\BehaviorTreeDebugWindow.cpp.backup  # Version avant cleanup
```

---

## 🎯 PROCHAIN OBJECTIF

### Nettoyer Code Orphelin (~30 min)
**But** : Réduire .cpp de ~2900 LOC à ~1700 LOC

**Méthode** :
1. Identifier méthodes obsolètes (liste dans Cleanup_Plan.md)
2. Supprimer méthodes une par une
3. Supprimer code orphelin entre RenderNodeGraphPanel et GetNodeColor
4. Compiler après chaque suppression
5. Valider 0 errors

**Méthodes à supprimer** :
```
RenderNode()
RenderNodeConnections()
RenderBezierConnection()
RenderActiveLinkGlow()
RenderNodePins()
ApplyZoomToStyle()
FitGraphToView()
CenterViewOnGraph()
ResetZoom()
RenderMinimap()
GetGraphBounds()
CalculatePanOffset()
GetSafeZoom()
```

---

## ✅ CRITÈRES DE SUCCÈS

- [ ] LOC .cpp : ~1700 (actuellement ~2900)
- [ ] Méthodes obsolètes : 0 (actuellement 13)
- [ ] Code orphelin : 0 LOC
- [ ] Compilation : 0 errors, 0 warnings
- [ ] Tests : F10 fonctionne

---

## 📞 AIDE RAPIDE

### Problème Compilation
```powershell
# Voir erreurs détaillées
msbuild "Olympe Engine.sln" /t:Build /p:Configuration=Debug /v:detailed > build_verbose.log 2>&1
type build_verbose.log | Select-String -Context 5 "error C"
```

### Problème Git
```powershell
# Voir changements
git status
git diff Source/AI/BehaviorTreeDebugWindow.cpp

# Annuler changements
git checkout Source/AI/BehaviorTreeDebugWindow.cpp
```

---

**TEMPS ESTIMÉ RESTANT** : ~1h  
**DIFFICULTÉ** : Moyenne  
**STATUT** : 75% Complete
