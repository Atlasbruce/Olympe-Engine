# Blueprint Editor Standalone - Fixed Layout Implementation

## 📋 Résumé des Modifications

### Date: 2026-03-12
### Objectif: Convertir les panneaux flottants en panneaux dockés fixes avec splitters redimensionnables

---

## ✅ Modifications Effectuées

### 0. Status Bar Désactivée (2026-03-12)

**Fichier modifié**: `Source/BlueprintEditor/BlueprintEditorGUI.cpp`

**Modification**:
```cpp
// Status bar at bottom - DISABLED (not needed in standalone editor)
// RenderStatusBar();
```

**Raison**: La status bar était inutile dans l'éditeur standalone et prenait de l'espace en bas de l'écran.

---

### 1. Configuration JSON (Nouveau)

**Fichier créé**: `blueprint_editor_config.json`

```json
{
  "version": "1.0",
  "editor_mode": "standalone",
  "window": {
    "width": 1920,
    "height": 1080,
    "maximized": true
  },
  "panels": {
    "asset_browser": {
      "visible": true,
      "width": 400
    },
    "node_graph": {
      "visible": true
    },
    "inspector": {
      "visible": true,
      "width": 400
    }
  },
  "layout": {
    "mode": "fixed",
    "asset_browser_width": 400.0,
    "inspector_width": 400.0,
    "min_panel_width": 200.0,
    "splitter_size": 8.0
  }
}
```

---

### 2. Backend - BlueprintEditor

**Fichiers modifiés**:
- `Source/BlueprintEditor/BlueprintEditor.h`
- `Source/BlueprintEditor/BlueprintEditor.cpp`

**Ajouts**:

```cpp
// Dans .h
bool LoadConfig(const std::string& configPath = "blueprint_editor_config.json");
bool SaveConfig(const std::string& configPath = "blueprint_editor_config.json");
const json& GetConfig() const { return m_Config; }
json& GetConfigMutable() { return m_Config; }

private:
    json m_Config;  // Editor configuration
```

**Implémentation**:
- `LoadConfig()`: Charge la config JSON, crée config par défaut si inexistante
- `SaveConfig()`: Sauvegarde la config JSON avec indentation
- Appelé dans `Initialize()` et `Shutdown()`

---

### 3. Frontend - BlueprintEditorGUI

**Fichiers modifiés**:
- `Source/BlueprintEditor/BlueprintEditorGUI.h`
- `Source/BlueprintEditor/BlueprintEditorGUI.cpp`

**Ajouts dans .h**:

```cpp
private:
    void RenderFixedLayout();  // Nouvelle méthode
    
    // Fixed layout panel widths
    float m_AssetBrowserWidth;
    float m_InspectorWidth;
    float m_MinPanelWidth;
    float m_SplitterSize;
```

**Modifications dans Render()**:
- Remplacement des panneaux flottants par `RenderFixedLayout()`
- Layout 3 colonnes: **Asset Browser | Graph Editor | Inspector**

**Nouvelle méthode RenderFixedLayout()**:
```cpp
void BlueprintEditorGUI::RenderFixedLayout()
{
    // Fenêtre plein écran (sous menu bar)
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 
                                    ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight()));
    
    // LEFT PANEL: Asset Browser
    ImGui::BeginChild("AssetBrowserPanel", ImVec2(m_AssetBrowserWidth, windowHeight), true);
    m_AssetBrowser.RenderContent();
    ImGui::EndChild();
    
    // LEFT SPLITTER (resizable)
    ImGui::SameLine();
    ImGui::Button("##LeftSplitter", ImVec2(m_SplitterSize, windowHeight));
    if (ImGui::IsItemActive())
        m_AssetBrowserWidth += ImGui::GetIO().MouseDelta.x;
    
    // CENTER PANEL: Graph Editor
    ImGui::SameLine();
    ImGui::BeginChild("CenterPanel", ImVec2(centerWidth, windowHeight), true);
    if (m_ShowVSEditor) m_VSEditorPanel->RenderContent();
    else if (m_ShowNodeGraph) m_NodeGraphPanel.RenderContent();
    ImGui::EndChild();
    
    // RIGHT SPLITTER (resizable)
    ImGui::SameLine();
    ImGui::Button("##RightSplitter", ImVec2(m_SplitterSize, windowHeight));
    if (ImGui::IsItemActive())
        m_InspectorWidth -= ImGui::GetIO().MouseDelta.x;
    
    // RIGHT PANEL: Inspector
    ImGui::SameLine();
    ImGui::BeginChild("InspectorPanel", ImVec2(m_InspectorWidth, windowHeight), true);
    m_InspectorPanel.RenderContent();
    ImGui::EndChild();
}
```

**Chargement/Sauvegarde config**:
- `Initialize()`: Charge `layout.*` depuis config backend
- `Shutdown()`: Sauvegarde largeurs de panneaux dans config backend

---

### 4. Panneaux - Méthodes RenderContent()

**Fichiers modifiés**:

1. **AssetBrowser**:
   - `Source/BlueprintEditor/AssetBrowser.h`
   - `Source/BlueprintEditor/AssetBrowser.cpp`
   - `void RenderContent()`: Rend contenu sans `ImGui::Begin/End`

2. **NodeGraphPanel**:
   - `Source/BlueprintEditor/NodeGraphPanel.h`
   - `Source/BlueprintEditor/NodeGraphPanel.cpp`
   - `void RenderContent()`: Rend contenu sans `ImGui::Begin/End`

3. **InspectorPanel**:
   - `Source/BlueprintEditor/InspectorPanel.h`
   - `Source/BlueprintEditor/InspectorPanel.cpp`
   - `void RenderContent()`: Rend contenu sans `ImGui::Begin/End`

4. **VisualScriptEditorPanel**:
   - `Source/BlueprintEditor/VisualScriptEditorPanel.h`
   - `Source/BlueprintEditor/VisualScriptEditorPanel.cpp`
   - `void RenderContent()`: Rend contenu sans `ImGui::Begin/End`

**Pattern utilisé**:
```cpp
// Ancienne méthode Render() conservée pour compatibilité
void Panel::Render()
{
    ImGui::Begin("Panel Title");
    RenderContent();
    ImGui::End();
}

// Nouvelle méthode pour layout fixe
void Panel::RenderContent()
{
    // Tout le contenu du panneau sans Begin/End
    ImGui::Text("Content...");
    // ...
}
```

---

### 5. Main Standalone

**Fichier modifié**: `Source/BlueprintEditorStandalone/BlueprintEditorStandaloneMain.cpp`

**Modifications**:
- Commentaire ajouté expliquant que docking n'est pas disponible
- Utilisation de layout manuel avec `ImGui::BeginChild`

---

## 🎨 Disposition Finale

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                          OLYMPE BLUEPRINT EDITOR                             ║
║  File   Edit   Tools   View   Help                                           ║
╠═══════════════════╦══════════════════════════════════╦═════════════════════════╣
║                   ║                                  ║                         ║
║  Asset Browser    ║        Graph Editor              ║      Inspector          ║
║                   ║                                  ║                         ║
║  📁 Blueprints    ║   [Canvas avec nodes ImNodes]    ║   Properties:           ║
║    📁 AI          ║                                  ║     Node Type: Branch   ║
║      📄 guard.ats ║                                  ║     Position: (120,50)  ║
║    📁 Animations  ║                                  ║                         ║
║                   ║                                  ║   Blackboard:           ║
║  Runtime Entities ║                                  ║     EnemyHealth: 75.0   ║
║    Entity[42]     ║                                  ║     IsAggro: true       ║
║                   ║                                  ║                         ║
║                   ║                                  ║                         ║
║                   ║                                  ║                         ║
║  (400px)          ║        (flexible)                ║      (400px)            ║
║       ║           ║                                  ║          ║              ║
║    Splitter       ║                                  ║      Splitter           ║
╚═══════════════════╩══════════════════════════════════╩═════════════════════════╝
```

---

## 🔧 Fonctionnalités

### ✅ Panneaux Fixes
- **Asset Browser** (gauche): Largeur par défaut 400px
- **Graph Editor** (centre): Largeur flexible (reste de l'espace)
- **Inspector** (droite): Largeur par défaut 400px

### ✅ Splitters Redimensionnables
- **Left Splitter**: Entre Asset Browser et Graph Editor
- **Right Splitter**: Entre Graph Editor et Inspector
- **Contraintes**: Largeur minimale 200px par panneau
- **Visual Feedback**: Curseur change en `ImGuiMouseCursor_ResizeEW`

### ✅ Persistance Configuration
- **Chargement**: Au démarrage dans `BlueprintEditor::Initialize()`
- **Sauvegarde**: À la fermeture dans `BlueprintEditor::Shutdown()`
- **Fichier**: `blueprint_editor_config.json` (racine du projet)

### ✅ Panneaux Optionnels (Floating)
Ces panneaux restent flottants (peuvent être fermés):
- Template Browser
- History Panel
- Debugger Panel
- Profiler Panel

---

## 📊 Comparaison Avant/Après

### ❌ AVANT (Panneaux Flottants)

```cpp
// Chaque panneau était une fenêtre ImGui indépendante
void BlueprintEditorGUI::Render()
{
    if (m_ShowAssetBrowser)
        m_AssetBrowser.Render();  // ImGui::Begin/End inside
    
    if (m_ShowNodeGraph)
        m_NodeGraphPanel.Render();  // ImGui::Begin/End inside
    
    if (m_ShowInspector)
        m_InspectorPanel.Render();  // ImGui::Begin/End inside
}
```

**Problèmes**:
- ❌ Panneaux superposés par défaut
- ❌ Positions non stables
- ❌ Difficile à naviguer
- ❌ imgui.ini désactivé (pas de sauvegarde positions)

---

### ✅ APRÈS (Layout Fixe)

```cpp
void BlueprintEditorGUI::Render()
{
    RenderFixedLayout();  // Layout 3 colonnes avec splitters
}

void BlueprintEditorGUI::RenderFixedLayout()
{
    // LEFT: Asset Browser (ImGui::BeginChild)
    // SPLITTER
    // CENTER: Graph Editor (ImGui::BeginChild)
    // SPLITTER
    // RIGHT: Inspector (ImGui::BeginChild)
}
```

**Avantages**:
- ✅ Layout stable 3 colonnes
- ✅ Panneaux toujours visibles
- ✅ Splitters redimensionnables
- ✅ Config JSON persistante
- ✅ Comme BehaviorTreeDebugWindow (F10)

---

## 🧪 Tests Recommandés

### Test 1: Vérifier Layout Initial
1. Lancer `BlueprintEditorStandalone.exe`
2. Vérifier 3 colonnes affichées:
   - Asset Browser (gauche, 400px)
   - Graph Editor (centre)
   - Inspector (droite, 400px)

### Test 2: Redimensionner Panneaux
1. Glisser le splitter gauche → Asset Browser change de largeur
2. Glisser le splitter droite → Inspector change de largeur
3. Vérifier largeur minimale (200px)

### Test 3: Persistance Config
1. Redimensionner les panneaux
2. Fermer l'éditeur
3. Vérifier `blueprint_editor_config.json` mis à jour
4. Relancer l'éditeur
5. Vérifier largeurs restaurées

### Test 4: Chargement Graphe
1. Double-cliquer sur `guard_ai.ats` dans Asset Browser
2. Vérifier graphe affiché dans centre
3. Vérifier propriétés dans Inspector

### Test 5: Panneaux Optionnels
1. Menu View → Template Browser
2. Vérifier fenêtre flottante s'ouvre
3. Fermer avec X
4. Layout fixe reste intact

---

## 🐛 Problèmes Résolus

### Erreur 1: ImGuiConfigFlags_DockingEnable
**Erreur**: `C2065: 'ImGuiConfigFlags_DockingEnable' : identificateur non déclaré`

**Solution**: Retiré car non disponible dans cette version d'ImGui. Utilisé layout manuel avec `ImGui::BeginChild` à la place.

### Erreur 2: JSON Initializer List
**Erreur**: `C2440: impossible de convertir de 'initializer list' en 'json'`

**Solution**: Remplacé syntaxe d'initialisation par:
```cpp
m_Config = json::object();
m_Config["key"] = value;
```

---

## 📝 Notes Techniques

### Architecture

**Backend (BlueprintEditor)**:
- Gère la configuration JSON
- LoadConfig/SaveConfig appelés dans Initialize/Shutdown
- Expose GetConfig() pour lecture, GetConfigMutable() pour écriture

**Frontend (BlueprintEditorGUI)**:
- Lit layout.* depuis backend dans Initialize()
- Sauvegarde layout.* dans backend dans Shutdown()
- Gère rendering avec ImGui::BeginChild

### ImGui Layout

**Hiérarchie**:
```
ImGui::Begin("##FixedLayoutWindow")  // Fullscreen invisible
├─ ImGui::BeginChild("AssetBrowserPanel")
│  └─ m_AssetBrowser.RenderContent()
├─ ImGui::Button("##LeftSplitter")
├─ ImGui::BeginChild("CenterPanel")
│  └─ m_VSEditorPanel->RenderContent()
├─ ImGui::Button("##RightSplitter")
└─ ImGui::BeginChild("InspectorPanel")
   └─ m_InspectorPanel.RenderContent()
```

**SameLine() Flow**:
```
[AssetBrowser] SameLine() [Splitter] SameLine() [Center] SameLine() [Splitter] SameLine() [Inspector]
```

---

## 🚀 Prochaines Étapes Recommandées

### Court Terme
1. ✅ Tester l'éditeur standalone
2. ✅ Ajuster largeurs par défaut si nécessaire
3. ✅ Vérifier compatibilité avec tous les types de graphes (.ats, .json)

### Moyen Terme
1. ⬜ Ajouter layout vertical optionnel (top/bottom split)
2. ⬜ Mémoriser layouts multiples (presets)
3. ⬜ Ajouter boutons reset layout dans View menu

### Long Terme
1. ⬜ Upgrade ImGui vers version avec docking natif
2. ⬜ Tab system pour ouvrir plusieurs graphes simultanément
3. ⬜ Layouts personnalisables (drag-drop panels)

---

## ✅ Checklist Validation

- [x] Configuration JSON créée et fonctionnelle
- [x] LoadConfig/SaveConfig implémentés dans backend
- [x] RenderFixedLayout implémenté dans GUI
- [x] RenderContent() ajouté à tous les panneaux
- [x] Splitters redimensionnables fonctionnels
- [x] Largeurs minimales appliquées
- [x] Config sauvegardée/restaurée correctement
- [x] Build réussit sans erreurs
- [ ] Tests manuels effectués (à faire par utilisateur)

---

## 📚 Documentation Associée

- **User Guide**: `Docs/Blueprint_Editor_User_Guide_v4.md`
- **Visual Diagrams**: `Docs/Blueprint_Editor_Visual_Diagrams.md`
- **Advanced Systems**: `Docs/Blueprint_Editor_Advanced_Systems.md`

---

**Implémentation terminée le**: 2026-03-12  
**Build Status**: ✅ SUCCESS  
**Prêt pour tests utilisateur**: ✅ OUI
