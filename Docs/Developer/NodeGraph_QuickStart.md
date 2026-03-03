# 🚀 QUICK START - NodeGraphCore v2.0 Refactor
## Guide Rapide pour Développeurs
**Version**: v2.0.0  
**Date**: 2025-02-19

---

## ⚡ EN 3 ÉTAPES

### 1️⃣ COMPILER LE NOUVEAU CODE
```powershell
msbuild "Olympe Engine.sln" /t:Clean,Build /p:Configuration=Debug /v:minimal
```

**Attendu** :
- 0 errors
- 0 warnings
- Build time < 2 min

---

### 2️⃣ MIGRER BEHAVIORTREEDEBUGWINDOW
**Fichier** : `Source/AI/BehaviorTreeDebugWindow.cpp`

**Remplacer** la méthode `RenderNodeGraphPanel()` :

```cpp
void BehaviorTreeDebugWindow::RenderNodeGraphPanel()
{
    // ✅ Récupérer BehaviorTreeAsset
    auto& world = World::Get();
    if (m_selectedEntity == 0 || !world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
        return;
    
    const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
    const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);
    if (!tree) return;
    
    // ✅ Convertir en GraphDocument (cache)
    if (m_cachedTreeId != tree->id)
    {
        if (m_cachedGraphDoc) delete m_cachedGraphDoc;
        m_cachedGraphDoc = BTGraphDocumentConverter::ToGraphDocument(tree, &m_currentLayout);
        m_cachedTreeId = tree->id;
    }
    
    // ✅ Runtime highlighting
    m_graphPanel.SetCurrentRuntimeNode(btRuntime.AICurrentNodeIndex);
    
    // ✅ Render
    m_graphPanel.SetDocument(m_cachedGraphDoc);
    m_graphPanel.Render(ImGui::GetIO().DeltaTime);
}
```

**Gain** : ~2000 LOC → ~50 LOC (-97%)

---

### 3️⃣ TESTER
```powershell
# Lancer le jeu
.\OlympeEngine.exe

# Appuyer sur F10 (Debug Window)
# Sélectionner une entité avec BT
# Vérifier :
# ✅ Graphe affiché
# ✅ Nœud actif highlighted (jaune/orange pulsant)
# ✅ Minimap visible
# ✅ Zoom/Pan fonctionne
```

---

## 📁 FICHIERS CLÉS

### Nouveaux Fichiers (à ajouter au projet)
```
Source/NodeGraphCore/
├── NodeGraphRenderer.h/cpp
├── EditorContext.h/cpp
├── NodeGraphPanel.h/cpp
└── Commands/DeleteLinkCommand.h/cpp

Source/NodeGraphShared/
└── BTGraphDocumentConverter.h/cpp
```

### Documentation
```
Docs/Developer/
├── NodeGraph_Refactor_Summary.md           # ← VOUS ÊTES ICI
├── NodeGraph_Build_Instructions.md         # Build détaillé
├── BehaviorTreeDebugWindow_Migration_Guide.md # Migration complète
├── NodeGraph_Unified_Architecture.md       # Conception
└── NodeGraph_Architecture_Analysis.md      # Analyse
```

---

## 🔧 COMMANDES UTILES

### Build
```powershell
# Clean + Build
msbuild "Olympe Engine.sln" /t:Clean,Build /p:Configuration=Debug

# Verbose (debug errors)
msbuild "Olympe Engine.sln" /t:Build /p:Configuration=Debug /v:detailed > build.log
```

### Git
```bash
# Voir les changements
git status
git diff

# Commit
git add Source/NodeGraphCore/*
git add Source/NodeGraphShared/BTGraphDocumentConverter.*
git add Docs/Developer/NodeGraph_*.md
git commit -m "refactor: NodeGraphCore v2.0 - Unified rendering pipeline"
```

---

## ❓ TROUBLESHOOTING

### Build Error: "Cannot open include file"
```xml
<!-- Ajouter dans .vcxproj <AdditionalIncludeDirectories> -->
$(ProjectDir)NodeGraphCore;%(AdditionalIncludeDirectories)
```

### Runtime Crash: Access Violation
```cpp
// Vérifier : m_cachedGraphDoc initialisé à nullptr dans header
// Vérifier : delete m_cachedGraphDoc dans destructeur
```

### Nœud actif pas highlighted
```cpp
// Vérifier : EditorContext est en mode Debug
NodeGraph::EditorContext debugCtx = NodeGraph::EditorContext::CreateDebugger();

// Vérifier : config.enableRuntimeHighlight = true
config.enableRuntimeHighlight = true;
```

---

## 📊 RÉSULTATS ATTENDUS

| Métrique | Avant | Après | Gain |
|----------|-------|-------|------|
| LOC RenderNodeGraphPanel | 2000 | 50 | -97% |
| Duplication code | 60% | 0% | -100% |
| Temps build | 2m | 2m 10s | +5% (acceptable) |
| FPS (50 nodes) | 60 | 60 | Identique |

---

## ✅ CHECKLIST VALIDATION

- [ ] Build SUCCESS (0 errors, 0 warnings)
- [ ] F10 ouvre Debug Window
- [ ] Sélection entité affiche graphe BT
- [ ] Nœud actif highlighted (pulse)
- [ ] Minimap visible et interactive
- [ ] Zoom/Pan fonctionne
- [ ] Fit to View (F) fonctionne
- [ ] Center View (C) fonctionne
- [ ] Reset Zoom (0) fonctionne

---

## 🎯 SUPPORT

### Docs Complètes
- **Build** : `NodeGraph_Build_Instructions.md`
- **Migration** : `BehaviorTreeDebugWindow_Migration_Guide.md`
- **Architecture** : `NodeGraph_Unified_Architecture.md`

### Contacts
- **Architecture** : Voir `NodeGraph_Unified_Architecture.md`
- **Coding Rules** : `Source/COPILOT_CODING_RULES.md`

---

**VERSION**: v2.0.0  
**DERNIÈRE MAJ**: 2025-02-19
