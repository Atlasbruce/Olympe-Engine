# Entity Prefab Editor - Plan d'Intégration au Blueprint Editor v4

> **Statut** : 🎯 Plan de développement incrémental et sûr  
> **Date** : 2026-04-02  
> **Auteur** : Architecture Analysis  
> **Version** : 1.0

---

## 📋 Table des matières

1. [Situation actuelle](#situation-actuelle)
2. [Architecture cible](#architecture-cible)
3. [Points d'intégration](#points-dintégration)
4. [Analyse des risques](#analyse-des-risques)
5. [Plan d'implémentation phased](#plan-dimlémentation-phased)
6. [Recommandations techniques](#recommandations-techniques)
7. [Checklist d'implémentation](#checklist-dimlémentation)

---

## Situation actuelle

### Composants existants

#### ✅ Système Blueprint Editor v4

| Component | Status | Location | Notes |
|-----------|--------|----------|-------|
| **VisualScriptEditorPanel** | ✅ Active | `Source/BlueprintEditor/` | Éditeur principal v4, ProtoNode + ImNodes |
| **GraphDocument** | ✅ Stable | `Source/NodeGraphCore/` | Abstraction générique pour graphes |
| **CommandStack** | ✅ Stable | `Source/NodeGraphCore/` | Undo/Redo pattern bien établi |
| **TabManager** | ✅ Active | `Source/BlueprintEditor/` | Gestion multi-onglets |
| **ValidationPanel** | ✅ Active | `Source/BlueprintEditor/` | Validation temps réel |
| **TemplateManager** | ✅ Active | `Source/BlueprintEditor/` | Sauvegarde/chargement templates |
| **BlueprintEditorPlugin** | ✅ Interface | `Source/BlueprintEditor/` | Architecture plugin extensible |

#### ⚠️ Entity Prefab Editor existant

| Component | Status | Location | Notes |
|-----------|--------|----------|-------|
| **EntityPrefabEditorPlugin** | ⚠️ Stub | `Source/BlueprintEditor/` | Basique, pas d'implémentation |
| **PrefabLoader** | ❌ Absent | N/A | À créer |
| **ComponentLibrary** | ❌ Absent | N/A | À créer |
| **PropertyInspector** (prefabs) | ❌ Absent | N/A | À adapter/créer |
| **PrefabCanvas** | ❌ Absent | N/A | À créer |

#### 📦 Infrastructure disponible

| System | Location | Utilisable |
|--------|----------|-----------|
| **FileIO** | `Source/Core/` | ✅ Oui |
| **JSONParser** | `Source/third_party/nlohmann/` | ✅ Oui |
| **Renderer** | `Source/Rendering/` | ✅ Oui |
| **InputSystem** | `Source/Input/` | ✅ Oui |
| **ValidationSystem** | `Source/BlueprintEditor/` | ✅ Oui |

### État du code source

- **Blueprint Editor** : Mature, bien documenté, production-ready (v4)
- **GraphDocument** : Abstraction solide pour n'importe quel type de graphe
- **Entity Prefab Editor** : Skeleton existant, prêt à être complété
- **Documentation** : Excellente pour BP Editor v4, manquante pour Entity Prefab

---

## Architecture cible

### Vue globale d'intégration

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                    OLYMPE BLUEPRINT EDITOR INTEGRATED                       ║
╠══════════════════════════════════════════════════════════════════════════════╣
║                                                                              ║
║  ┌─────────────────────────────────────────────────────────────────────┐   ║
║  │ BlueprintEditor (Singleton Backend)                                │   ║
║  │ ├─ Asset Management (Blueprints/ + Gamedata/EntityPrefab/)         │   ║
║  │ ├─ Plugin Registry                                                 │   ║
║  │ │  ├─ BehaviorTreeEditorPlugin                                     │   ║
║  │ │  ├─ EntityPrefabEditorPlugin (EXTENDED)  ◄── OUR FOCUS           │   ║
║  │ │  ├─ HFSMEditorPlugin                                             │   ║
║  │ │  └─ ...                                                          │   ║
║  │ ├─ CommandStack (Undo/Redo - shared)                               │   ║
║  │ ├─ ValidationPanel (Shared validation)                             │   ║
║  │ └─ TemplateManager (Shared templates)                              │   ║
║  └─────────────────────────────────────────────────────────────────────┘   ║
║                           ▲                                                  ║
║                           │ Uses                                             ║
║                           │                                                  ║
║  ┌────────────────────────┴──────────────────────────────────────────────┐  ║
║  │ EntityPrefabEditorPlugin (EXTENDED IMPLEMENTATION)                   │  ║
║  │                                                                       │  ║
║  │ ┌─────────────────────────────────────────────────────────────────┐ │  ║
║  │ │ EntityPrefabGraphDocument                                      │ │  ║
║  │ │ extends GraphDocument                                          │ │  ║
║  │ │                                                                │ │  ║
║  │ │ ├─ CreateComponentNode()                                      │ │  ║
║  │ │ ├─ DeleteComponentNode()                                      │ │  ║
║  │ │ ├─ UpdateComponentProperty()                                  │ │  ║
║  │ │ └─ SerializeToEntityPrefab()                                  │ │  ║
║  │ └─────────────────────────────────────────────────────────────────┘ │  ║
║  │                                                                       │  ║
║  │ ┌─────────────────────────────────────────────────────────────────┐ │  ║
║  │ │ PrefabLoader                                                   │ │  ║
║  │ │ ├─ LoadFromFile(path) → EntityPrefab                           │ │  ║
║  │ │ ├─ LoadParameterSchemas()                                      │ │  ║
║  │ │ └─ MigrateSchema(oldVersion) → newVersion                      │ │  ║
║  │ └─────────────────────────────────────────────────────────────────┘ │  ║
║  │                                                                       │  ║
║  │ ┌─────────────────────────────────────────────────────────────────┐ │  ║
║  │ │ ComponentLibrary                                               │ │  ║
║  │ │ ├─ GetComponentSchema(type) → ComponentSchema                  │ │  ║
║  │ │ ├─ GetAvailableComponents() → List<ComponentEntry>             │ │  ║
║  │ │ ├─ IsComponentInEntity(prefab, type) → bool                    │ │  ║
║  │ │ └─ SearchComponents(query) → List<ComponentEntry>              │ │  ║
║  │ └─────────────────────────────────────────────────────────────────┘ │  ║
║  │                                                                       │  ║
║  │ ┌─────────────────────────────────────────────────────────────────┐ │  ║
║  │ │ PrefabCanvas (ImNodes wrapper)                                 │ │  ║
║  │ │ ├─ RenderNodes()                                               │ │  ║
║  │ │ ├─ HandleInput()                                               │ │  ║
║  │ │ ├─ AutoLayout()                                                │ │  ║
║  │ │ └─ ExportToGraph()                                             │ │  ║
║  │ └─────────────────────────────────────────────────────────────────┘ │  ║
║  │                                                                       │  ║
║  │ ┌─────────────────────────────────────────────────────────────────┐ │  ║
║  │ │ PropertyInspectorPrefab (adapter)                              │ │  ║
║  │ │ ├─ RenderComponentProperties()                                 │ │  ║
║  │ │ ├─ OnPropertyChanged()                                         │ │  ║
║  │ │ └─ ValidateProperty()                                          │ │  ║
║  │ └─────────────────────────────────────────────────────────────────┘ │  ║
║  │                                                                       │  ║
║  │ RenderEditor() – Main plugin rendering entry point                   │  ║
║  └───────────────────────────────────────────────────────────────────────┘  ║
║                                                                              ║
║  ┌──────────────────────────────────────────────────────────────────────┐  ║
║  │ BlueprintEditorGUI (Frontend)                                        │  ║
║  │                                                                      │  ║
║  │ ├─ TabManager (Canvas tabs)                                          │  ║
║  │ │  └─ Onglets : [Blueprint] [StateGraph] [Prefab-beacon*] [...]    │  ║
║  │ │                                                                    │  ║
║  │ ├─ AssetBrowser                                                      │  ║
║  │ │  ├─ Browse Blueprints/                                            │  ║
║  │ │  └─ Browse Gamedata/EntityPrefab/  ◄── Scannage automatique      │  ║
║  │ │                                                                    │  ║
║  │ ├─ RightPanel (ImGui tabs)                                           │  ║
║  │ │  ├─ Tab 1: NodeProperties (generic)                               │  ║
║  │ │  ├─ Tab 2: ComponentLibrary (prefabs)                             │  ║
║  │ │  └─ Tab 3: Blackboard (shared)                                    │  ║
║  │ │                                                                    │  ║
║  │ └─ BottomPanel                                                       │  ║
║  │    └─ Validation errors + status                                     │  ║
║  │                                                                      │  ║
║  └──────────────────────────────────────────────────────────────────────┘  ║
║                                                                              ║
║  ┌────────────────────────────────────────────────────────────────────────┐ ║
║  │ External Systems (Already Available)                                   │ ║
║  │                                                                        │ ║
║  │ ├─ FileIO                                                             │ ║
║  │ ├─ JSONParser (nlohmann/json)                                         │ ║
║  │ ├─ Renderer                                                           │ ║
║  │ ├─ InputSystem                                                        │ ║
║  │ └─ ValidationSystem                                                   │ ║
║  └────────────────────────────────────────────────────────────────────────┘ ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

### Classe EntityPrefabGraphDocument

```cpp
/**
 * Extension de GraphDocument pour Entity Prefabs
 * Gère les nœuds spécialisés pour composants d'entité
 */
class EntityPrefabGraphDocument : public GraphDocument
{
public:
    // Entity node (central)
    NodeId CreateEntityCenterNode(const std::string& entityName);

    // Component nodes (autour de l'entité)
    NodeId CreateComponentNode(
        const std::string& componentType,
        const json& properties
    );

    bool DeleteComponentNode(NodeId nodeId);

    bool UpdateComponentProperty(
        NodeId nodeId,
        const std::string& propName,
        const json& value
    );

    // Conversion vers EntityPrefab JSON
    json SerializeToEntityPrefab() const;
    bool DeserializeFromEntityPrefab(const json& prefabData);

    // Layout
    void AutoLayoutComponents();

    // Validation
    std::vector<ValidationError> ValidatePrefab() const;
};
```

---

## Points d'intégration

### 1️⃣ AssetBrowser - Scan des préfabs

**Location** : `Source/BlueprintEditor/AssetBrowser.cpp`

**Modification** :
```cpp
// Dans BlueprintEditor::RefreshAssets()
void RefreshAssets()
{
    // Existing: Scan Blueprints/
    ScanBlueprintDirectory("Blueprints/");

    // NEW: Scan Entity Prefabs
    ScanPrefabDirectory("Gamedata/EntityPrefab/");  // ◄─ À ajouter
}

// Nouveau scanning pour préfabs
void ScanPrefabDirectory(const std::string& path)
{
    // Scanne *.json + *.ats
    // Charge metadata (name, author, lastModified)
    // Ajoute au AssetNode tree avec icône prefab
}
```

**Impact** : Minimal, ajout simple de routines de scanning

### 2️⃣ TabManager - Nouvel onglet Prefab

**Location** : `Source/BlueprintEditor/TabManager.h`

**Modification** :
```cpp
enum class EditorTabType {
    BLUEPRINT_GRAPH,
    STATE_MACHINE,
    PREFAB_EDITOR,      // ◄─ NEW
    CUSTOM
};

// TabManager crée onglets Prefab au clic double sur prefab.json
void OnPrefabDoubleClicked(const std::string& filePath)
{
    auto tab = std::make_unique<EditorTab>();
    tab->type = EditorTabType::PREFAB_EDITOR;
    tab->pluginData = filePath;
    AddTab(std::move(tab));
}
```

**Impact** : Minimal, TabManager déjà prévu pour plugins

### 3️⃣ BlueprintEditorPlugin - Extended Plugin Call

**Location** : `Source/BlueprintEditor/BlueprintEditorGUI.cpp`

**Modification** :
```cpp
void BlueprintEditorGUI::RenderNodeEditor()
{
    auto activeTab = m_tabManager->GetActiveTab();

    switch (activeTab->type) {
        case EditorTabType::BLUEPRINT_GRAPH:
            m_VSEditorPanel->Render();
            break;

        case EditorTabType::PREFAB_EDITOR:
            // Delegate à plugin
            auto plugin = BlueprintEditor::Get()
                .GetPlugin("EntityPrefab");
            plugin->RenderEditor(
                activeTab->data,
                m_editorContext
            );
            break;

        // ...
    }
}
```

**Impact** : Très minimal, pattern déjà établi

### 4️⃣ CommandStack - New Commands

**Location** : `Source/NodeGraphCore/CommandStack.h`

**Nouveaux commands** :
```cpp
class AddComponentCommand : public Command
{
    // Crée ComponentNode avec defaults
};

class RemoveComponentCommand : public Command
{
    // Supprime ComponentNode
};

class UpdateComponentPropertyCommand : public Command
{
    // Modifie propriété d'un ComponentNode
};

// Utilisation standardisée : stack.ExecuteCommand(...)
```

**Impact** : Extension du pattern existant, très sûre

### 5️⃣ ValidationPanel - Validation Prefabs

**Location** : `Source/BlueprintEditor/ValidationPanel.cpp`

**Modification** :
```cpp
// Validation Panel appelle PrefabValidator
std::vector<ValidationError> PrefabValidator::Validate(
    const EntityPrefabGraphDocument& doc
)
{
    // Checks:
    // - Required components (e.g., Identity_data)
    // - Property type matching
    // - Cycles detection (si subgraphs)
    // - Schema compliance
}
```

**Impact** : Réutilise pattern ValidationPanel existant

---

## Analyse des risques

### 🟡 Risques moyens

| Risque | Probabilité | Impact | Mitigation |
|--------|------------|--------|-----------|
| **Schema mismatch** | Moyen | Haut | Validator strict + tests unitaires |
| **Performance (20+ nodes)** | Bas | Moyen | Profiler dès Phase 2 |
| **Plugin integration** | Bas | Moyen | Pattern déjà testé (BehaviorTree plugin) |
| **Undo/Redo state** | Bas | Haut | CommandStack bien établi |
| **Drag-drop conflicts** | Bas | Moyen | Test d'intégration ImGui |

### 🟢 Risques faibles

- **File I/O** : Déjà utilisé avec FileIO existant
- **JSON parsing** : nlohmann/json bien établi
- **UI rendering** : ImGui pattern maîtrisé
- **Validation** : System existant + extension simple

### 🔴 Risques critiques

**AUCUN** - Architecture suffisamment découplée et modulaire

---

## Plan d'implémentation phased

### Phase 1: Prefab Visualization (Sprint 1-2)

**Objectif** : Visualiser préfabs en read-only

#### Livrables
- ✅ PrefabLoader : Charge prefab.json
- ✅ EntityPrefabGraphDocument : Conversion vers GraphDocument
- ✅ PrefabCanvas : Affichage avec ImNodes
- ✅ Tab Prefab dans TabManager

#### Code à créer

**`Source/BlueprintEditor/PrefabLoader.h/cpp`**
```cpp
class PrefabLoader {
public:
    static EntityPrefab LoadFromFile(const std::string& path);
    static ParameterSchemaRegistry LoadSchemas();
    static bool ValidateSchema(const EntityPrefab&);
};
```

**`Source/BlueprintEditor/EntityPrefabGraphDocument.h/cpp`**
```cpp
class EntityPrefabGraphDocument : public GraphDocument {
public:
    void LoadFromPrefab(const EntityPrefab&);
    EntityPrefab SerializeToPrefab() const;
    void AutoLayoutComponents();
};
```

**`Source/BlueprintEditor/PrefabCanvas.h/cpp`**
```cpp
class PrefabCanvas {
public:
    void Render();
    void Update(float deltaTime);
    void OnMouseInput();
};
```

#### Intégration
- ✅ AssetBrowser scanne `Gamedata/EntityPrefab/`
- ✅ TabManager crée onglet au double-clic
- ✅ Plugin->RenderEditor() affiche PrefabCanvas

#### Critères d'acceptation
- [ ] Charger beacon.json sans erreur
- [ ] Afficher 5 nœuds de composants
- [ ] Metadata préfab visible
- [ ] Aucune erreur sur tous les préfabs existants

### Phase 2: Component Node Visualization (Sprint 1)

**Objectif** : Nœuds composants visuellement distincts

#### Livrables
- ✅ ComponentNodeRenderer : Rendu spécialisé
- ✅ ComponentNode class : Modèle de données
- ✅ Auto-layout circulaire

#### Code à créer

**`Source/BlueprintEditor/ComponentNode.h/cpp`**
```cpp
struct ComponentNode {
    std::string componentType;
    json properties;
    Vector2 position;

    bool operator==(const ComponentNode&) const;
};
```

**`Source/BlueprintEditor/ComponentNodeRenderer.h/cpp`**
```cpp
class ComponentNodeRenderer {
public:
    void Render(const ComponentNode&, bool selected);
    bool IsHoveredBy(Vector2 mousePos);
    Vector2 GetCenterPosition() const;
};
```

#### Critères d'acceptation
- [ ] Nœud entité central visible
- [ ] Composants en cercle autour
- [ ] 60 FPS avec 20 nœuds
- [ ] Zoom/Pan fonctionnels

### Phase 3: Component Library (Sprint 1)

**Objectif** : Sélection et affichage de composants disponibles

#### Livrables
- ✅ ParameterSchemaRegistry
- ✅ ComponentLibrary : Liste + search
- ✅ UI LibraryPanel (drag-drop ready)

#### Code à créer

**`Source/BlueprintEditor/ParameterSchemaRegistry.h/cpp`**
```cpp
class ParameterSchemaRegistry {
public:
    static void Initialize();
    static ComponentSchema GetSchema(const std::string& type);
    static std::vector<ComponentSchema> GetAllSchemas();
    static std::vector<ParameterDefinition> GetDefaults(const std::string& type);
};
```

**`Source/BlueprintEditor/ComponentLibrary.h/cpp`**
```cpp
class ComponentLibrary {
public:
    std::vector<ComponentEntry> GetAvailable(const EntityPrefab&);
    std::vector<ComponentEntry> Search(const std::string& query);
    bool IsComponentInPrefab(const EntityPrefab&, const std::string& type);
};
```

#### Critères d'acceptation
- [ ] Charger tous schémas au démarrage
- [ ] Afficher 20+ composants disponibles
- [ ] Recherche < 50ms
- [ ] Icons visuellement distinctes

### Phase 4: Property Editing (Sprint 2)

**Objectif** : Édition des propriétés composants

#### Livrables
- ✅ PropertyInspectorPrefab : Éditeurs de propriétés
- ✅ Type-aware editors (Float, Int, Bool, Vector2)
- ✅ Validation en temps réel

#### Code à créer

**`Source/BlueprintEditor/PropertyInspectorPrefab.h/cpp`**
```cpp
class PropertyInspectorPrefab {
public:
    void DisplayComponent(const ComponentNode&);
    void OnPropertyChanged(const std::string& name, const json& value);
    ValidationResult ValidateProperty(const std::string& name, const json& value);
};
```

#### Critères d'acceptation
- [ ] Éditer tous types de propriétés
- [ ] Validation instantanée (Float rejects "abc")
- [ ] Changements visibles sur nœud
- [ ] Dirty flag set automatiquement

### Phase 5: Drag-Drop Component Addition (Sprint 1)

**Objectif** : Ajouter composants via drag-drop

#### Livrables
- ✅ Drag-drop mechanism
- ✅ Drop zone detection
- ✅ Auto-layout après ajout
- ✅ Auto-switch PropertyInspector

#### Commandes à ajouter

```cpp
class AddComponentCommand : public Command {
    // Crée ComponentNode avec defaults
    // Trigger auto-layout
};
```

#### Critères d'acceptation
- [ ] Drag composant de library vers canvas
- [ ] Nœud apparaît avec defaults
- [ ] Canvas auto-layout
- [ ] PropertyInspector switch auto
- [ ] Dirty flag set

### Phase 6: Serialization & Complete Workflow (Sprint 2)

**Objectif** : Sauvegarder/charger avec undo-redo

#### Livrables
- ✅ PrefabSerializer : Sauvegarder en JSON
- ✅ Undo/Redo complet (commands)
- ✅ Multi-file editing
- ✅ Context menu (delete, duplicate, move)
- ✅ Validation avant save

#### Commandes à ajouter

```cpp
class RemoveComponentCommand : public Command {};
class UpdateComponentPropertyCommand : public Command {};
class DuplicateComponentCommand : public Command {};
class ReorderComponentCommand : public Command {};
```

#### Critères d'acceptation
- [ ] Sauvegarder en JSON valide
- [ ] Undo/Redo tous opérations
- [ ] 3+ préfabs ouverts simultanément
- [ ] Fichier modified indicator (*)
- [ ] Save bloqué si erreurs validation
- [ ] Context menu complet

---

## Recommandations techniques

### 🔒 Sécurité et robustesse

#### 1. Validation stricte
```cpp
// TOUJOURS valider avant save
PrefabValidator validator;
if (!validator.Validate(prefab).isValid) {
    showError("Cannot save: " + errors);
    return false;  // Block save
}
```

#### 2. Undo/Redo atomique
```cpp
// Toujours utiliser CommandStack
auto cmd = std::make_unique<AddComponentCommand>(...);
commandStack->ExecuteCommand(std::move(cmd));
// NOT: manually modify state + invalidate
```

#### 3. Schema versioning
```cpp
// File format toujours avec version
json prefab;
prefab["schema_version"] = 4;  // Toujours spécifier
// Migration auto si version < 4
```

### 🎯 Performance

#### Benchmarks cibles

| Operation | Target | Notes |
|-----------|--------|-------|
| Load prefab | < 100ms | Including node creation |
| Property edit | Instant | Real-time feedback |
| Canvas render | 60 FPS | 20+ components |
| Save to disk | < 50ms | JSON write |
| Drag-drop | Smooth | No frame drops |
| Search | < 50ms | 100+ items |

#### Optimisations clés

1. **Lazy loading** : ComponentLibrary lazy init
2. **Caching** : ParameterSchemas loaded once
3. **Incremental validation** : Validate on-change, not per-frame
4. **ImNodes optimization** : Use ProtoNode pattern (existing)

### 🧪 Testing Strategy

#### Unit Tests
```cpp
// PrefabLoader
TEST(PrefabLoader, LoadsValidJson) { }
TEST(PrefabLoader, MigrationV2toV4) { }
TEST(PrefabLoader, RejectsInvalidSchema) { }

// ComponentLibrary
TEST(ComponentLibrary, SearchFilters) { }
TEST(ComponentLibrary, GetDefaults) { }

// Validator
TEST(PrefabValidator, RequiredComponents) { }
TEST(PrefabValidator, TypeValidation) { }
```

#### Integration Tests
```cpp
// Full workflows
TEST(PrefabEditor, LoadEditSaveWorkflow) { }
TEST(PrefabEditor, MultiPrefabEditing) { }
TEST(PrefabEditor, UndoRedoStacking) { }
TEST(PrefabEditor, DragDropIntegration) { }
```

### 📚 Documentation

#### À créer

1. **Entity_Prefab_Editor_Integration_Guide.md**
   - Architecture overview
   - Component interfaces
   - Workflow examples

2. **Entity_Prefab_Editor_API_Reference.md**
   - Toutes les classes publiques
   - Usage patterns
   - Code examples

3. **Entity_Prefab_Editor_User_Guide.md**
   - User workflows
   - UI walkthrough
   - Troubleshooting

---

## Checklist d'implémentation

### Pre-Implementation

- [ ] Créer branche feature : `feature/entity-prefab-editor-phase1`
- [ ] Revue architecture avec team lead
- [ ] Setup Git workflow (PR, code review)
- [ ] Créer issues GitHub par phase

### Phase 1 : Visualization

- [ ] Créer PrefabLoader.h/cpp
- [ ] Créer EntityPrefabGraphDocument.h/cpp
- [ ] Créer PrefabCanvas.h/cpp
- [ ] Modifier AssetBrowser pour scan prefabs
- [ ] Modifier TabManager pour tab Prefab
- [ ] Modifier EntityPrefabEditorPlugin stub
- [ ] Tests unitaires (PrefabLoader, Canvas)
- [ ] PR + Code review
- [ ] Merge to master
- [ ] Update documentation

### Phase 2 : Component Nodes

- [ ] Créer ComponentNode.h/cpp
- [ ] Créer ComponentNodeRenderer.h/cpp
- [ ] Implémenter auto-layout
- [ ] Tests unitaires (rendering, layout)
- [ ] Performance testing (60 FPS, 20 nodes)
- [ ] PR + Merge

### Phase 3 : Component Library

- [ ] Créer ParameterSchemaRegistry.h/cpp
- [ ] Créer ComponentLibrary.h/cpp
- [ ] UI ComponentLibraryPanel
- [ ] Tests search/filter
- [ ] PR + Merge

### Phase 4 : Property Editing

- [ ] Créer PropertyInspectorPrefab.h/cpp
- [ ] Type-aware editors
- [ ] Integration avec ValidationPanel
- [ ] Tests validation
- [ ] PR + Merge

### Phase 5 : Drag-Drop

- [ ] AddComponentCommand
- [ ] Drag-drop implementation
- [ ] Auto-layout integration
- [ ] Integration tests
- [ ] PR + Merge

### Phase 6 : Serialization

- [ ] RemoveComponentCommand, UpdatePropertyCommand, etc.
- [ ] PrefabSerializer
- [ ] Undo/Redo full stack
- [ ] Multi-tab state management
- [ ] Context menu
- [ ] Full integration tests
- [ ] Performance profiling
- [ ] PR + Merge

### Post-Implementation

- [ ] Complete documentation
- [ ] User testing
- [ ] Performance audit
- [ ] Security audit
- [ ] Release notes
- [ ] Team training

---

## Prochaines étapes

### Recommandation immédiate

**✅ Commencer par Phase 1 : Prefab Visualization**

1. Créer branche : `feature/entity-prefab-editor-phase1`
2. Implémenter les 3 classes clés
3. Intégrer au TabManager
4. PR pour review

### Effort estimé

| Phase | Duration | Dev Days | Risk |
|-------|----------|----------|------|
| Phase 1 | 2 sprints | 8-10 | 🟢 Low |
| Phase 2 | 1 sprint | 4-5 | 🟢 Low |
| Phase 3 | 1 sprint | 4-5 | 🟢 Low |
| Phase 4 | 2 sprints | 6-8 | 🟡 Medium |
| Phase 5 | 1 sprint | 4-5 | 🟡 Medium |
| Phase 6 | 2 sprints | 8-10 | 🟢 Low |
| **Total** | **10 sprints** | **34-43** | 🟢 **Low** |

### Ressources requises

- 1-2 développeurs C++
- 1 QA engineer
- Architecture review (1-2 sessions)
- Documentation writer (0.5 FTE)

---

## Conclusion

L'intégration du Entity Prefab Editor au Blueprint Editor v4 est **totalement faisable** et **à faible risque** grâce à :

✅ Architecture Blueprint v4 solide et mature  
✅ Plugin system déjà éprouvé  
✅ Infrastructure complète (validation, undo/redo, tab manager)  
✅ Pattern de code consistant et bien documenté  

**Recommandation** : Commencer Phase 1 immédiatement, livrer MVPs bi-hebdomadaires.

---

**Document Version** : 1.0  
**Last Updated** : 2026-04-02  
**Author** : Copilot Architecture Analysis
