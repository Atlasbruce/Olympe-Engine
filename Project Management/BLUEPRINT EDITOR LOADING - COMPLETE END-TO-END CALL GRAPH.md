📊 BLUEPRINT EDITOR LOADING - COMPLETE END-TO-END CALL GRAPH
ÉTAPE 0: APPLICATION ENTRY POINT
Code
BlueprintEditorStandaloneMain.cpp
└─ SDL_AppInit()
   │
   ├─ Objectif: Initialiser SDL3 et créer la fenêtre principale
   ├─ SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)
   │  └─ Initialise le subsystème vidéo et événements SDL
   │
   ├─ SDL_CreateWindow("Olympe Blueprint Editor Standalone", 1920, 1080)
   │  └─ Crée une fenêtre resizable
   │
   ├─ SDL_CreateRenderer()
   │  └─ Crée un renderer pour les graphiques
   │
   ├─ ImGui::CreateContext()
   │  └─ Initialise le contexte global ImGui
   │
   ├─ ImGui_ImplSDL3_InitForSDLRenderer()
   │  └─ Connecte ImGui à SDL3 et au renderer
   │
   └─ TRANSITION TO ÉTAPE 1 ✅
ÉTAPE 1: BACKEND INITIALIZATION
Code
BlueprintEditor::Get().Initialize()
├─ Objectif: Initialiser le système backend complet
├─
├─ PHASE A: État de base
│  ├─ m_IsActive = false
│  ├─ m_HasUnsavedChanges = false
│  ├─ m_AssetRootPath = "Blueprints"
│  └─ m_GamedataRootPath = "Gamedata"
│
├─ PHASE B: Managers d'infrastructure
│  ├─ EnumCatalogManager::Get().Initialize()
│  │  └─ Charge tous les enum descripteurs des blueprints
│  │
│  ├─ NodeGraphManager::Get().Initialize()
│  │  └─ Prépare les structures de graphe de nœuds
│  │
│  ├─ EntityInspectorManager::Get().Initialize()
│  │  └─ Initialise l'inspecteur d'entités pour le runtime
│  │
│  └─ TemplateManager::Get().Initialize()
│     └─ Prépare le gestionnaire de modèles
│
├─ PHASE C: Command Stack (Undo/Redo)
│  └─ m_CommandStack = new CommandStack()
│     └─ Crée la pile de commandes pour l'édition
│
├─ PHASE D: System d'extensions
│  └─ InitializePlugins()
│     ├─ Register BehaviorTreeEditorPlugin
│     ├─ Register EntityPrefabEditorPlugin
│     └─ Register AdditionalEditorPlugins
│
├─ PHASE E: Configuration
│  └─ LoadConfig("blueprint_editor_config.json")
│     └─ Charge les préférences éditeur
│
└─ TRANSITION TO ÉTAPE 2 ✅
ÉTAPE 2: STANDALONE EDITOR MODE INITIALIZATION
Code
BlueprintEditor::Get().InitializeStandaloneEditor()
├─ Objectif: Initialiser l'éditeur en mode standalone (WYSIWYG)
├─
├─ PHASE A: EditorContext setup
│  └─ EditorContext::Get().InitializeStandalone()
│     └─ Active les fonctionnalités complètes d'édition
│
├─ PHASE B: Pré-chargement des graphes ATS
│  └─ PreloadATSGraphs()
│     │
│     ├─ Scanne `Blueprints/` récursivement
│     ├─ Scanne `Gamedata/` récursivement
│     │
│     ├─ POUR CHAQUE fichier .ats ou .json trouvé:
│     │  ├─ TaskGraphLoader::Parse(filepath)
│     │  │  └─ Charge et parse le JSON en TaskGraphTemplate
│     │  │
│     │  ├─ m_template.BuildLookupCache()
│     │  │  └─ Construit les index internes pour requêtes rapides
│     │  │
│     │  └─ VSGraphVerifier::Verify(m_template)
│     │     └─ Valide la structure et rapporte les erreurs
│     │
│     └─ Cache tous les templates pour accès rapide plus tard
│
└─ TRANSITION TO ÉTAPE 3 ✅
ÉTAPE 3: GUI INITIALIZATION
Code
BlueprintEditorGUI::Initialize()
├─ Objectif: Initialiser l'interface utilisateur (frontend)
├─
├─ PHASE A: Panel creation
│  ├─ AssetBrowser::Initialize("Blueprints/")
│  │  └─ Prépare l'interface de navigation des assets
│  │
│  ├─ AssetInfoPanel::Create()
│  │  └─ Crée le panneau d'informations d'assets
│  │
│  ├─ InspectorPanel::Create()
│  │  └─ Crée le panneau d'inspection des entités
│  │
│  ├─ NodeGraphPanel::Create()
│  │  └─ Crée le panneau de graphe de nœuds
│  │
│  └─ EntitiesPanel::Create()
│     └─ Crée le panneau des entités runtime
│
├─ PHASE B: TabManager initialization
│  └─ TabManager::Get().Initialize()
│     └─ Prépare le système d'onglets multi-graphes
│
├─ PHASE C: UI state
│  ├─ m_ShowAssetBrowser = true
│  ├─ m_ShowNodeEditor = true
│  ├─ m_ShowVSEditor = true
│  └─ ... (autres flags de visibilité)
│
└─ TRANSITION TO ÉTAPE 4 ✅
ÉTAPE 4: WORLD BRIDGE SETUP
Code
WorldBridge_RegisterTaskCallback()
├─ Objectif: Connecter TaskSystem à l'UI pour live debugging
├─
├─ TaskExecutionBridge::Install()
│  ├─ Enregistre callback: NodeGraphPanel::SetActiveDebugNode()
│  │  └─ Highlight le nœud en cours d'exécution dans l'UI
│  │
│  └─ Enregistre callback: WorldBridge_SetBlackboard()
│     └─ Met à jour InspectorPanel avec les valeurs live du blackboard
│
├─ Chaque frame durant l'exécution:
│  ├─ TaskSystem appelle ces callbacks
│  ├─ UI mise à jour en temps réel
│  └─ Débogage visuel du graphe actif
│
└─ TRANSITION TO ÉTAPE 5 ✅
ÉTAPE 5: MAIN LOOP ACTIVATION
Code
SDL_AppIterate() (chaque frame)
├─ Objectif: Boucle principale - rendu et traitement des entrées
├─
├─ PHASE A: Traitement des événements
│  ├─ SDL_PollEvent(&event)
│  │  ├─ Captures clics souris, mouvements, touches clavier
│  │  └─ Traite les fermetures de fenêtre
│  │
│  └─ ImGui_ImplSDL3_ProcessEvent(&event)
│     └─ Transmet les événements à ImGui
│
├─ PHASE B: Rendu ImGui (début)
│  ├─ ImGui_ImplSDL3_NewFrame()
│  ├─ ImGui_ImplSDLRenderer3_NewFrame()
│  └─ ImGui::NewFrame()
│
├─ PHASE C: Rendu de l'UI (voir ÉTAPE 6)
│  └─ BlueprintEditorGUI::Render()
│
├─ PHASE D: Finalization ImGui
│  ├─ ImGui::Render()
│  ├─ ImGui_ImplSDLRenderer3_RenderDrawData()
│  └─ SDL_RenderPresent(renderer)
│
└─ TRANSITION TO ÉTAPE 6 ✅
ÉTAPE 6: MAIN GUI RENDERING
Code
BlueprintEditorGUI::Render()
├─ Objectif: Rendu de tous les panels et fenêtres
├─
├─ PHASE A: Menu bar
│  └─ RenderMenuBar()
│     ├─ File menu (New, Open, Save, Save As, Exit)
│     ├─ Edit menu (Undo, Redo, Preferences)
│     ├─ View menu (Show/Hide panels)
│     └─ Help menu (About, Shortcuts, Documentation)
│
├─ PHASE B: Layout principal (fixed layout)
│  └─ RenderFixedLayout()
│     │
│     ├─ LEFT PANEL (40% width):
│     │  ├─ TOP: AssetBrowser::RenderContent()
│     │  │  └─ Affiche l'arborescence des assets (blueprints, scenes)
│     │  │
│     │  ├─ SPLITTER (draggable)
│     │  │
│     │  ├─ MIDDLE: AssetInfoPanel::Render()
│     │  │  └─ Affiche les métadonnées de l'asset sélectionné
│     │  │
│     │  ├─ SPLITTER (draggable)
│     │  │
│     │  └─ BOTTOM: NodeGraphPanel::Render()
│     │     └─ Affiche les nœuds du runtime entities
│     │
│     ├─ CENTER PANEL (50% width):
│     │  └─ TabManager::RenderTabBar() + RenderActiveCanvas()
│     │     └─ Affiche les graphes ouverts en onglets
│     │        └─ TRANSITION TO ÉTAPE 7 ✅
│     │
│     └─ RIGHT PANEL (10% width):
│        ├─ InspectorPanel::Render()
│        │  └─ Affiche les propriétés de l'entité sélectionnée
│        │
│        └─ TemplateBrowserPanel::Render()
│           └─ Affiche les modèles disponibles
│
└─ RETOUR à ÉTAPE 5 (prochaine frame)
ÉTAPE 7: ACTIVE CANVAS RENDERING (Graph Editor)
Code
TabManager::RenderActiveCanvas()
├─ Objectif: Rendu du graphe actif dans l'onglet actif
├─
├─ PHASE A: Vérification de l'onglet actif
│  └─ EditorTab* activeTab = GetActiveTab()
│     └─ Si aucun onglet, return (nada)
│
├─ PHASE B: Delegation au renderer
│  └─ activeTab->renderer->Render()
│     │
│     ├─ SI graphType == "VisualScript":
│     │  └─ VisualScriptEditorPanel::Render()
│     │     └─ TRANSITION TO ÉTAPE 8A ✅
│     │
│     └─ SI graphType == "BehaviorTree":
│        └─ BehaviorTreeEditorPanel::Render()
│           └─ TRANSITION TO ÉTAPE 8B
│
└─ FIN rendereing canvas
ÉTAPE 8A: VISUAL SCRIPT EDITOR PANEL RENDERING
Code
VisualScriptEditorPanel::Render()
├─ Objectif: Rendu du panneau d'édition Visual Script complet
├─
├─ PHASE A: Fenêtre ImGui
│  └─ ImGui::Begin("Visual Script Editor", &m_visible, ImGuiWindowFlags_NoMove)
│
├─ PHASE B: Contenu principal (delegation)
│  └─ RenderContent()
│     │
│     ├─ PHASE B.1: Toolbar (hauteur fixe ~40px)
│     │  └─ RenderToolbar()
│     │     │
│     │     ├─ Bouton "New Graph" (crée nouveau graphe)
│     │     ├─ Bouton "Open" → ShowFileDialog()
│     │     │  └─ Ouvre fichier.ats dans TabManager
│     │     │
│     │     ├─ Bouton "Save" (Ctrl+S)
│     │     │  └─ SerializeAndWrite() → JSON file
│     │     │
│     │     ├─ Bouton "Run Graph"
│     │     │  └─ RunGraphSimulation()
│     │     │     └─ Exécute le graphe étape par étape, trace l'exécution
│     │     │
│     │     ├─ Bouton "Verify"
│     │     │  └─ RunVerification()
│     │     │     └─ VSGraphVerifier::Verify(m_template)
│     │     │        └─ Valide et affiche les erreurs/avertissements
│     │     │
│     │     └─ Bouton "Pause/Resume" (si en simulation)
│     │
│     ├─ PHASE B.2: Canvas area (flex, 70% height)
│     │  │
│     │  ├─ ImGui::BeginChild("Canvas")
│     │  │
│     │  ├─ ImNodes::BeginNodeEditor()
│     │  │
│     │  ├─ RenderCanvas()
│     │  │  │ └─ TRANSITION TO ÉTAPE 8B ✅
│     │  │
│     │  ├─ ImNodes::EndNodeEditor()
│     │  │
│     │  └─ ImGui::EndChild()
│     │
│     ├─ SPLITTER (horizontal, draggable)
│     │
│     └─ PHASE B.3: Right Panel (30% height)
│        │
│        ├─ PART A: Node Properties (top 50%)
│        │  └─ RenderNodeProperties()
│        │     └─ Affiche les propriétés du nœud sélectionné
│        │        ├─ Type, Position, ID
│        │        ├─ Paramètres spécifiques (selon le type)
│        │        └─ Edit modals pour conditions, etc.
│        │
│        ├─ SPLITTER (horizontal, draggable)
│        │
│        └─ PART B: Tab-based panel (bottom 50%)
│           │ (Phase 26 UX Enhancement)
│           │
│           ├─ ImGui::BeginTabBar("RightPanelTabs")
│           │
│           ├─ TAB 0: "Preset Bank"
│           │  └─ RenderRightPanelTabContent(0)
│           │     └─ ConditionPresetLibraryPanel::Render()
│           │        └─ Liste des presets de conditions disponibles
│           │
│           ├─ TAB 1: "Local Variables"
│           │  └─ RenderRightPanelTabContent(1)
│           │     └─ RenderBlackboardPanel(LOCAL)
│           │        └─ Affiche variables locales du graphe
│           │
│           ├─ TAB 2: "Global Variables"
│           │  └─ RenderRightPanelTabContent(2)
│           │     └─ RenderGlobalBlackboardPanel()
│           │        └─ Affiche variables globales du système
│           │
│           └─ ImGui::EndTabBar()
│
├─ PHASE C: Modals et dialogs
│  ├─ RenderSaveAsDialog()
│  │  └─ Modal pour "Save As..." (Ctrl+Shift+S)
│  │
│  ├─ RenderConditionEditorModal()
│  │  └─ Modal pour éditer les conditions d'un nœud
│  │
│  └─ RenderSubGraphFilePickerModal()
│     └─ Modal pour sélectionner un subgraph
│
└─ ImGui::End()
ÉTAPE 8B: CANVAS RENDERING (ImNodes Integration)
Code
VisualScriptEditorPanel::RenderCanvas()
├─ Objectif: Rendu des nœuds et connexions avec ImNodes
├─
├─ PHASE A: Setup ImNodes context
│  ├─ ImNodes::SetCurrentContext(m_imnodesContext)
│  │  └─ Active le contexte ImNodes pour ce panel
│  │
│  ├─ ImNodes::SetNodeGridSpacePos(nodeID, gridPos)
│  │  └─ Restaure la position de chaque nœud depuis m_template
│  │
│  └─ ImNodes::BeginNodeEditor()
│
├─ PHASE B: Rendu des nœuds
│  │
│  ├─ POUR CHAQUE node DANS m_template.Nodes:
│  │  │
│  │  ├─ PHASE B.1: Nœud visuel
│  │  │  ├─ ImNodes::BeginNode(nodeUID)
│  │  │  │  └─ Commence le rendu du nœud
│  │  │  │
│  │  │  ├─ Rendu du titre
│  │  │  │  ├─ Couleur selon le type
│  │  │  │  ├─ Icône
│  │  │  │  └─ Nom du nœud
│  │  │  │
│  │  │  ├─ PHASE B.2: Input pins (LEFT side)
│  │  │  │  │
│  │  │  │  ├─ Exec input pins (flèches vertes)
│  │  │  │  │  └─ ExecInAttrUID(nodeID, 0) = ImNodes pin
│  │  │  │  │
│  │  │  │  └─ Data input pins (carrés bleus)
│  │  │  │     └─ DataInAttrUID(nodeID, i) = ImNodes pin
│  │  │  │
│  │  │  ├─ PHASE B.3: Contenu du nœud (centre)
│  │  │  │  └─ Type-specific rendering (voir RenderNodeContent)
│  │  │  │     ├─ Dropdowns pour les paramètres
│  │  │  │     ├─ Champs de texte
│  │  │  │     ├─ Sliders, checkboxes
│  │  │  │     └─ Boutons "Edit" pour modals
│  │  │  │
│  │  │  ├─ PHASE B.4: Output pins (RIGHT side)
│  │  │  │  │
│  │  │  │  ├─ Exec output pins (flèches vertes)
│  │  │  │  │  └─ ExecOutAttrUID(nodeID, i) = ImNodes pin
│  │  │  │  │
│  │  │  │  └─ Data output pins (carrés bleus)
│  │  │  │     └─ DataOutAttrUID(nodeID, i) = ImNodes pin
│  │  │  │
│  │  │  ├─ Right-click context menu
│  │  │  │  ├─ "Duplicate"
│  │  │  │  ├─ "Delete"
│  │  │  │  ├─ "Copy"
│  │  │  │  └─ "Paste"
│  │  │  │
│  │  │  └─ ImNodes::EndNode()
│  │  │
│  │  └─ PHASE B.5: Node selection handling
│  │     ├─ Left-click = select single node
│  │     ├─ Ctrl+Left-click = multi-select
│  │     ├─ Mouse drag = node movement
│  │     │  └─ Sync m_editorNodes[].posX/Y chaque frame
│  │     │
│  │     └─ Delete key = remove selected nodes
│  │
│  └─ FIN loop nœuds
│
├─ PHASE C: Rendu des connexions (links)
│  │
│  ├─ POUR CHAQUE connection DANS m_template.ExecConnections:
│  │  └─ ImNodes::Link(linkUID, srcPinUID, dstPinUID)
│  │     └─ Dessine une courbe Bezier verte (exec connection)
│  │
│  ├─ POUR CHAQUE connection DANS m_template.DataConnections:
│  │  └─ ImNodes::Link(linkUID, srcPinUID, dstPinUID)
│  │     └─ Dessine une courbe Bezier bleue (data connection)
│  │
│  └─ Link context menus
│     ├─ Right-click on link = "Delete"
│     └─ Validation des erreurs de connexion
│
├─ PHASE D: Connection creation (user interaction)
│  │
│  ├─ ImNodes détecte nouveau lien (ImNodes::IsLinkCreated)
│  │  ├─ Récupère srcPinUID et dstPinUID
│  │  ├─ Valide la connexion (VSConnectionValidator::IsValid)
│  │  │  └─ Vérifie: type compatible, pas de self-loop, etc.
│  │  │
│  │  └─ SI valide:
│  │     ├─ AddConnectionCommand->Execute()
│  │     │  └─ Ajoute à m_template.ExecConnections ou DataConnections
│  │     │
│  │     └─ m_dirty = true
│  │
│  └─ FIN link creation
│
├─ PHASE E: Minimap rendering (Phase 37)
│  └─ m_canvasEditor->RenderMinimap()
│     └─ Affiche une minimap du graphe complet
│
└─ ImNodes::EndNodeEditor()
ÉTAPE 9: WHEN USER LOADS A BLUEPRINT
Code
User: Menu → File → Open (ou double-click asset dans browser)
├─
├─ PHASE A: File selection
│  ├─ ShowFileDialog() → Affiche file picker
│  └─ User choisit un fichier .ats
│
├─ PHASE B: Tab creation
│  └─ TabManager::OpenFileInTab(filepath)
│     │
│     ├─ VÉRIFICATION: Est-ce que le fichier est déjà ouvert?
│     │  ├─ SI OUI: Activate existing tab
│     │  └─ SI NON: Continuer...
│     │
│     ├─ PHASE B.1: Détection du type de graphe
│     │  └─ DetectGraphType(filepath)
│     │     ├─ Ouvre le fichier JSON
│     │     ├─ Lit le champ "type" ou "version"
│     │     └─ Retourne "VisualScript", "BehaviorTree", etc.
│     │
│     ├─ PHASE B.2: Création du renderer
│     │  ├─ SI VisualScript:
│     │  │  └─ renderer = new VisualScriptEditorPanel()
│     │  │     └─ TRANSITION TO ÉTAPE 10 ✅
│     │  │
│     │  └─ SI BehaviorTree:
│     │     └─ renderer = new BehaviorTreeEditorPanel()
│     │
│     ├─ PHASE B.3: Création du document
│     │  ├─ document = new VisualScriptGraphDocument(renderer)
│     │  │  └─ Adapter pattern pour Load/Save/metadata
│     │  │
│     │  └─ document->Load(filepath)
│     │
│     ├─ PHASE B.4: Création de l'EditorTab
│     │  ├─ tab.tabID = NextTabID()
│     │  ├─ tab.displayName = DisplayNameFromPath(filepath)
│     │  ├─ tab.filePath = filepath
│     │  ├─ tab.graphType = DetectGraphType(filepath)
│     │  ├─ tab.renderer = renderer
│     │  ├─ tab.document = document
│     │  └─ m_tabs.push_back(tab)
│     │
│     └─ PHASE B.5: Activation
│        └─ SetActiveTab(tabID)
│           └─ m_activeTabID = tabID
│           └─ Marquer pour selection le prochain frame
│
└─ FIN tab creation
   └─ Prochaine frame affichera le nouveau graphe
ÉTAPE 10: BLUEPRINT LOADING (Deep Dive)
Code
VisualScriptGraphDocument::Load(filepath)
├─ Objectif: Charger un blueprint complet depuis le fichier
├─
├─ PHASE A: File I/O
│  ├─ std::ifstream file(filepath)
│  └─ nlohmann::json json = nlohmann::json::parse(file)
│     └─ Parse le JSON complet
│
├─ PHASE B: TaskGraphTemplate creation
│  └─ TaskGraphLoader::ParseSchemaV4(json)
│     │
│     ├─ PHASE B.1: Métadonnées
│     │  ├─ tmpl.Name = json["name"]
│     │  ├─ tmpl.Description = json["description"]
│     │  ├─ tmpl.Version = json["version"] (ex: 4)
│     │  └─ tmpl.Guid = json["guid"]
│     │
│     ├─ PHASE B.2: Structure du graphe
│     │  │
│     │  ├─ Nœuds (json["nodes"] array)
│     │  │  ├─ POUR CHAQUE node:
│     │  │  │  ├─ tmpl.Nodes[].NodeID
│     │  │  │  ├─ tmpl.Nodes[].Type (EntryPoint, Sequence, etc.)
│     │  │  │  ├─ tmpl.Nodes[].NodeName
│     │  │  │  ├─ tmpl.Nodes[].Parameters (stored in Properties)
│     │  │  │  ├─ tmpl.Nodes[].posX, posY (grid space)
│     │  │  │  ├─ tmpl.Nodes[].Width, Height
│     │  │  │  └─ tmpl.Nodes[].Color (pour le style)
│     │  │  │
│     │  │  └─ FIN nœuds
│     │  │
│     │  ├─ Connexions d'exécution (json["connections"] array)
│     │  │  ├─ POUR CHAQUE exec connection:
│     │  │  │  ├─ tmpl.ExecConnections[].SourceNodeID
│     │  │  │  ├─ tmpl.ExecConnections[].SourcePin ("Then", etc.)
│     │  │  │  ├─ tmpl.ExecConnections[].TargetNodeID
│     │  │  │  └─ tmpl.ExecConnections[].TargetPin ("In", etc.)
│     │  │  │
│     │  │  └─ FIN connexions exec
│     │  │
│     │  └─ Connexions de données (json["dataConnections"] array)
│     │     ├─ POUR CHAQUE data connection:
│     │     │  ├─ tmpl.DataConnections[].SourceNodeID
│     │     │  ├─ tmpl.DataConnections[].SourcePin ("Value", etc.)
│     │     │  ├─ tmpl.DataConnections[].TargetNodeID
│     │     │  └─ tmpl.DataConnections[].TargetPin ("Value", etc.)
│     │     │
│     │     └─ FIN connexions data
│     │
│     ├─ PHASE B.3: Blackboard (local variables)
│     │  └─ ParseBlackboardV4(json["blackboard"])
│     │     ├─ POUR CHAQUE entry:
│     │     │  ├─ tmpl.Blackboard[].Key (ex: "CurrentHealth")
│     │     │  ├─ tmpl.Blackboard[].Type (Int, Float, String, etc.)
│     │     │  ├─ tmpl.Blackboard[].Default (valeur par défaut)
│     │     │  └─ tmpl.Blackboard[].Description
│     │     │
│     │     └─ FIN blackboard
│     │
│     ├─ PHASE B.4: Condition Presets (Phase 24 — embedded)
│     │  └─ ParseConditionPresetsV4(json["presets"])
│     │     ├─ POUR CHAQUE preset:
│     │     │  ├─ tmpl.Presets[].ID
│     │     │  ├─ tmpl.Presets[].Name
│     │     │  ├─ tmpl.Presets[].Conditions[] (conditions combinées)
│     │     │  ├─ tmpl.Presets[].Operands[] (opérandes dynamiques)
│     │     │  └─ tmpl.Presets[].Logic (AND, OR, etc.)
│     │     │
│     │     └─ FIN presets
│     │
│     ├─ PHASE B.5: Global Variable Overrides
│     │  └─ tmpl.GlobalVariableValues = json["globalVarValues"]
│     │     └─ Stocke les overrides de variables globales pour ce graphe
│     │
│     ├─ PHASE B.6: Entry Points
│     │  ├─ tmpl.EntryPointID = json["entryPointID"]
│     │  └─ tmpl.RootNodeID = json["rootNodeID"]
│     │
│     └─ RETOUR TaskGraphTemplate* tmpl
│
├─ PHASE C: Pass au VisualScriptEditorPanel
│  └─ VisualScriptEditorPanel::LoadTemplate(tmpl, filepath)
│     │
│     ├─ PHASE C.1: Copy du template
│     │  ├─ m_template = *tmpl
│     │  └─ m_currentPath = filepath
│     │
│     ├─ PHASE C.2: Build internal caches
│     │  └─ m_template.BuildLookupCache()
│     │     ├─ Index des nœuds par ID pour requêtes rapides
│     │     ├─ Index des connexions par nœud source
│     │     └─ Vérifie l'intégrité du graphe
│     │
│     ├─ PHASE C.3: Load embedded presets (Phase 24)
│     │  └─ SI m_template.Presets.empty():
│     │     ├─ m_presetRegistry.Clear()
│     │     └─ Log: "Graph has no embedded presets"
│     │  └─ SINON:
│     │     ├─ m_presetRegistry.LoadFromPresetList(m_template.Presets)
│     │     └─ Log: "Loaded N presets from graph"
│     │
│     ├─ PHASE C.4: Initialize EntityBlackboard
│     │  └─ m_entityBlackboard->Initialize(m_template)
│     │     ├─ Load local variables depuis m_template.Blackboard
│     │     ├─ Load global variables depuis GlobalTemplateBlackboard
│     │     │  └─ GlobalTemplateBlackboard::Reload()
│     │     │
│     │     └─ SI m_template.GlobalVariableValues non-empty:
│     │        └─ m_entityBlackboard->ImportGlobalsFromJson()
│     │           └─ Restaure les overrides de variables globales
│     │
│     ├─ PHASE C.5: Sync canvas from template
│     │  └─ SyncCanvasFromTemplate()
│     │     │
│     │     ├─ POUR CHAQUE nœud DANS m_template.Nodes:
│     │     │  ├─ Crée VSEditorNode (structure intermédiaire)
│     │     │  ├─ Copie position, propriétés, données du nœud
│     │     │  └─ m_editorNodes.push_back(vsEdNode)
│     │     │
│     │     ├─ POUR CHAQUE connexion DANS m_template.ExecConnections:
│     │     │  ├─ Crée VSEditorLink
│     │     │  └─ m_editorLinks.push_back(vsEdLink)
│     │     │
│     │     └─ RebuildLinks() (reconstruit les références)
│     │
│     ├─ PHASE C.6: Reset UI state
│     │  ├─ m_selectedNodeID = -1
│     │  ├─ m_condPanelNodeID = -1
│     │  ├─ m_nodeDragStartPositions.clear()
│     │  ├─ m_dirty = false
│     │  └─ m_verificationDone = false
│     │
│     └─ Template prêt pour rendering!
│
└─ FIN Loading
   └─ Prochaine frame affichera le graphe complet
ÉTAPE 11: RENDERING THE LOADED GRAPH (Loop)
Code
Chaque frame après loading:

RenderCanvas() avec m_editorNodes et m_editorLinks
├─ Affiche chaque nœu avec ses pins
├─ Affiche chaque connexion (Bezier curve)
├─ Gère les interactions utilisateur:
│  ├─ Click sur nœud → RenderNodeProperties()
│  ├─ Drag nœud → Update m_editorNodes[].posX/Y
│  ├─ Hover sur pin → Affiche tooltip
│  ├─ Créer lien → Validation + AddConnectionCommand
│  ├─ Delete lien → DeleteLinkCommand
│  └─ Right-click → Context menus
│
└─ Chaque mutation est undoable (CommandStack)
📌 SUMMARY DIAGRAM
Code
Entry Point (main.cpp)
        ↓
    SDL3 Init
        ↓
Backend Init (Managers, Registries)
        ↓
Standalone Editor Init (PreloadGraphs)
        ↓
GUI Init (Panels, TabManager)
        ↓
WorldBridge Setup (Debug callbacks)
        ↓
Main Loop (each frame)
        ├─ Events processing
        ├─ ImGui new frame
        ├─ BlueprintEditorGUI::Render()
        │  └─ RenderFixedLayout()
        │     ├─ LEFT: AssetBrowser, AssetInfo, NodeGraph
        │     ├─ CENTER: TabManager
        │     │  └─ TabManager::RenderActiveCanvas()
        │     │     └─ activeTab->renderer->Render()
        │     │        └─ VisualScriptEditorPanel::Render()
        │     │           └─ RenderContent()
        │     │              ├─ RenderToolbar()
        │     │              ├─ RenderCanvas()  ← ImNodes rendering
        │     │              └─ RenderRightPanel() (tabs for properties/presets/blackboard)
        │     └─ RIGHT: Inspector, TemplateBrowser
        ├─ ImGui render + SDL render present
        └─ Continue to next frame

When user opens blueprint:
        ↓
File dialog → Select .ats file
        ↓
TabManager::OpenFileInTab()
        ↓
VisualScriptGraphDocument::Load()
        ↓
TaskGraphLoader::ParseSchemaV4()
        ↓
VisualScriptEditorPanel::LoadTemplate()
        ↓
SyncCanvasFromTemplate()
        ↓
Next frame renders the graph
Cette analyse montre le cheminement complet du Blueprint Editor, de l'initialisation à l'affichage d'un graphe chargé. Chaque étape a un objectif clair et les modules communiquent via des interfaces bien définies.