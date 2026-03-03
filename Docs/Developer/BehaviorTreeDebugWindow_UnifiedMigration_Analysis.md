# 🔍 ANALYSE DE MIGRATION - BehaviorTreeDebugWindow vers Architecture Unifiée

**Date**: 2025-02-19  
**Objectif**: Migrer BehaviorTreeDebugWindow pour utiliser l'architecture unifiée NodeGraphCore

---

## 📊 ÉTAT ACTUEL

### Architecture Actuelle (Hybride)
```
BehaviorTreeDebugWindow (F10)
├── ✅ Utilise déjà NodeGraphShared::Renderer (partiel)
├── ✅ Utilise NodeGraphShared::CommandAdapter (editor mode)
├── ✅ Utilise NodeGraphShared::BehaviorTreeAdapter (layout)
├── ❌ Code de rendu custom (RenderNode, RenderNodeConnections)
├── ❌ Gestion de pins custom
├── ❌ Pas de EditorContext (flags manuels)
├── ❌ ImNodes direct (pas via NodeGraphRenderer)
└── ❌ Modèle de données BehaviorTreeAsset (pas GraphDocument)
```

### Architecture Standalone (Cible)
```
NodeGraphPanel (Blueprint Editor)
├── ✅ Utilise NodeGraphCore::NodeGraphRenderer
├── ✅ Utilise NodeGraphCore::EditorContext
├── ✅ Utilise NodeGraphShared::BlueprintAdapter
├── ✅ Modèle GraphDocument unifié
├── ✅ PinRegistry pour typage fort
└── ✅ CommandStack avec undo/redo complet
```

---

## 🔍 ANALYSE DES DÉPENDANCES

### Rendu (Critique)

**Code Actuel:**
```cpp
// BehaviorTreeDebugWindow.cpp lignes 1400-1429
void RenderBehaviorTreeGraph()
{
    // Rendu custom des nodes
    for (const auto& node : tree->nodes) {
        NodeGraphShared::RenderBTNode(&node, layout, isCurrentNode, ...);
    }
    
    // Rendu custom des connections
    for (const auto& node : tree->nodes) {
        RenderNodeConnections(&node, layout, tree, currentNodeId);
    }
}
```

**Problème:** 
- Appelle des fonctions shared mais ne passe pas par NodeGraphRenderer
- Gestion ImNodes directe (BeginNodeEditor/EndNodeEditor)
- Pas de RenderMode::Debug configuré

### Gestion du Mode Éditeur

**Code Actuel:**
```cpp
bool m_editorMode = false;  // Flag manuel
```

**Problème:**
- Pas de EditorContext
- Capabilities non contrôlées
- Duplication de logique avec standalone

### Pins et Connections

**Code Actuel:**
```cpp
// RenderNode lignes 1488-1532
ImNodes::BeginInputAttribute(node->id * 10000);
ImNodes::BeginOutputAttribute(node->id * 10000 + 1);

// Pin custom rendering lignes 1573-1599
NodeGraphShared::ComputePinCenterScreen(...)
NodeGraphShared::DrawPinCircle(...)
```

**Problème:**
- Calcul de pins ad-hoc
- Pas de PinRegistry
- Typage non unifié

### Modèle de Données

**Actuel:** `BehaviorTreeAsset` (runtime AI)
**Cible:** `GraphDocument` (editor unifié)

**Gap:**
- Besoin d'un adapter BehaviorTree → GraphDocument
- Conversion bidirectionnelle pour le mode édition

---

## 🎯 STRATÉGIE DE MIGRATION

### Option A: Migration Complète (Recommandée)
**Complexité:** 🔴 Élevée  
**Bénéfices:** 🟢 Architecture unifiée, maintenabilité maximale

```
1. Créer BTGraphDocumentConverter (BehaviorTree ↔ GraphDocument)
2. Créer BTDebugAdapter (wrapper pour NodeGraphRenderer)
3. Remplacer le code de rendu par NodeGraphRenderer
4. Ajouter EditorContext::CreateDebugger()
5. Migrer vers CommandStack complet
```

**Avantages:**
- ✅ Zéro duplication de code
- ✅ Même pipeline que standalone
- ✅ Extensible (HFSM, State Graphs)

**Inconvénients:**
- ❌ Refactoring important (~1500 LOC)
- ❌ Risque de régression temporaire
- ❌ Nécessite tests extensifs

### Option B: Réécriture (Alternative)
**Complexité:** 🟡 Moyenne  
**Bénéfices:** 🟢 Code propre, structure moderne

```
1. Créer nouveau BTDebugVisualizer basé sur NodeGraphPanel
2. Configurer en RenderMode::Debug
3. Adapter seulement l'affichage runtime (no edit)
4. Tester en parallèle avec l'ancien
5. Basculer quand stable
```

**Avantages:**
- ✅ Code neuf et propre
- ✅ Pas de risque sur l'existant
- ✅ Tests en parallèle

**Inconvénients:**
- ❌ Duplication temporaire
- ❌ Double maintenance pendant transition
- ❌ Plus de temps total

### Option C: Migration Incrémentale (Pragmatique)
**Complexité:** 🟢 Faible  
**Bénéfices:** 🟡 Amélioration progressive

```
1. Garder BehaviorTreeAsset comme source de vérité
2. Migrer uniquement le rendu vers NodeGraphRenderer
3. Wrapper léger sans GraphDocument
4. Édition reste sur système actuel
```

**Avantages:**
- ✅ Risque minimal
- ✅ Résultats rapides
- ✅ Pas de casse existant

**Inconvénients:**
- ❌ Architecture toujours hybride
- ❌ Duplication partielle reste
- ❌ Pas de bénéfices long-terme

---

## 🏗️ DÉCISION ARCHITECTURALE

### Recommandation: **Option A - Migration Complète**

**Justification:**
1. **Dette technique:** L'hybride actuel accumule de la dette
2. **Cohérence:** Un seul pipeline pour tous les éditeurs
3. **Maintenabilité:** < 3000 LOC total vs ~5000 LOC actuel
4. **Extensibilité:** HFSM, State Graphs pourront réutiliser

**Plan de dérisquage:**
1. Migration en branche dédiée `feature/btdebug-unified`
2. Tests manuels extensifs avec checklist
3. Validation runtime sur scènes existantes
4. Rollback plan documenté

---

## 📋 COMPOSANTS À CRÉER

### 1. BTGraphDocumentConverter
**Fichier:** `Source/NodeGraphShared/BTGraphDocumentConverter.h|.cpp`

```cpp
namespace Olympe::NodeGraphShared {

class BTGraphDocumentConverter
{
public:
    // Convert BehaviorTree to editable document
    static GraphDocument* FromBehaviorTree(const BehaviorTreeAsset* tree);
    
    // Convert document back to BehaviorTree
    static BehaviorTreeAsset* ToBehaviorTree(const GraphDocument* doc);
    
    // Sync runtime changes to document (for live edit)
    static void SyncRuntimeToDocument(
        const BehaviorTreeAsset* tree, 
        GraphDocument* doc
    );
};

}
```

**Responsabilité:**
- Conversion bidirectionnelle BT ↔ GraphDocument
- Préservation de toutes les métadonnées runtime
- Validation des contraintes BT (single root, etc.)

### 2. BTDebugAdapter
**Fichier:** `Source/NodeGraphShared/BTDebugAdapter.h`

```cpp
namespace Olympe::NodeGraphShared {

class BTDebugAdapter
{
public:
    BTDebugAdapter(
        const BehaviorTreeAsset* tree,
        uint32_t activeNodeId,
        NodeGraph::NodeGraphRenderer* renderer
    );
    
    // Configure renderer for debug mode
    void ConfigureDebugMode();
    
    // Update active node highlighting
    void SetActiveNode(uint32_t nodeId);
    
    // Render using unified pipeline
    void Render(float deltaTime);
    
private:
    const BehaviorTreeAsset* m_tree;
    uint32_t m_activeNodeId;
    NodeGraph::NodeGraphRenderer* m_renderer;
    GraphDocument* m_tempDocument; // converted on-the-fly
};

}
```

**Responsabilité:**
- Bridge entre BehaviorTree runtime et NodeGraphRenderer
- Configuration RenderMode::Debug
- Highlighting des nodes actifs

### 3. EditorContext Debug Mode
**Fichier:** `Source/NodeGraphCore/EditorContext.cpp`

```cpp
// Ajouter dans EditorContext.cpp

EditorContext EditorContext::CreateDebugger()
{
    EditorContext ctx(EditorMode::Debug);
    ctx.m_capabilities.canCreateNodes = false;
    ctx.m_capabilities.canDeleteNodes = false;
    ctx.m_capabilities.canMoveNodes = false;
    ctx.m_capabilities.canConnectPins = false;
    ctx.m_capabilities.canEditProperties = false;
    ctx.m_capabilities.canUndo = false;
    ctx.m_capabilities.canRedo = false;
    ctx.m_capabilities.canSave = false;
    ctx.m_capabilities.showRuntimeState = true; // 🔑 Debug mode
    return ctx;
}
```

---

## 🚨 POINTS D'ATTENTION

### Performances
- Conversion BT → GraphDocument à chaque frame ?
  - ❌ Non ! Conversion une seule fois au load
  - ✅ Update incrémental uniquement du node actif

### Compatibilité Runtime
- Le BehaviorTreeAsset ne doit PAS être modifié
- GraphDocument est une vue éditable temporaire
- Sauvegarde recrée le BehaviorTreeAsset

### Mode Édition
- En mode édition, le document devient la source de vérité
- Le BehaviorTreeAsset est recréé au Save
- Undo/Redo opèrent sur le GraphDocument

---

## 📝 NEXT STEPS

1. ✅ Validation de l'approche avec l'équipe
2. ⏳ Créer BTGraphDocumentConverter
3. ⏳ Créer BTDebugAdapter
4. ⏳ Migrer BehaviorTreeDebugWindow
5. ⏳ Tests & validation
6. ⏳ Documentation & cleanup

---

## 📚 RÉFÉRENCES

- `NodeGraph_Unified_Architecture.md` - Architecture cible
- `BehaviorTreeDebugWindow_Migration_Guide.md` - Historique migration
- `NodeGraphShared_Migration.md` - Migration shared utilities
