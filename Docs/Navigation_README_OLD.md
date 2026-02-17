# Olympe Engine Documentation

## 📖 Documentation Site
Visit [https://atlasbruce.github.io/Olympe-Engine/](https://atlasbruce.github.io/Olympe-Engine/) for the full documentation with search, navigation, and up-to-date content.

## 📁 Documentation Structure

### 🚀 Quick Start
- **[Getting Started Guide](https://atlasbruce.github.io/Olympe-Engine/getting-started/quick-start)** - Installation and first project
- **[Project Structure](https://atlasbruce.github.io/Olympe-Engine/getting-started/project-structure)** - Understanding the codebase

### 📚 User Guides

#### Editors
- **[BT Graph Editor](https://atlasbruce.github.io/Olympe-Engine/editors/bt-graph-editor)** - Visual behavior tree editor (F8)
- **[Animation Editor](https://atlasbruce.github.io/Olympe-Engine/editors/animation-editor)** - Sprite animation authoring (F9)
- [Animation Editor User Guide](Animation_Editor_User_Guide.md) - Detailed animation editor guide (local)
- [BT Editor User Guide](../BT_EDITOR_USER_GUIDE.md) - Detailed BT editor guide (local)

#### Input System
- [Input System User Guide](../Documentation/Input/INPUT_USER_GUIDE.md) - Configure and use input
- [Input System Architecture](../Documentation/Input/INPUT_ARCHITECTURE.md) - Technical details
- [Input JSON Schemas](../Documentation/Input/INPUT_SCHEMAS.md) - Configuration reference

#### Tiled Editor Integration
- [Tiled Quick Start](../TILED_QUICK_START.md) - Creating your first map
- [Layer Properties Guide](../LAYER_PROPERTIES_GUIDE.md) - Layer configuration
- [Tiled Isometric Maps](../Documentation/TILED_ISOMETRIC.md) - Isometric map handling

### 🔧 Technical Reference

#### Architecture
- [Architecture Overview](Developer/00_Architecture_Overview.md) - System design
- [ECS Overview](https://atlasbruce.github.io/Olympe-Engine/technical-reference/architecture/ecs-overview) - Entity-Component-System
- [Adding ECS Components](Adding_ECS_Components_Guide.md) - Component creation guide

#### Animation System
- [Animation System](Animation_System.md) - Animation system overview
- [Animation Editor Architecture](Developer/Animation_Editor_Architecture.md) - Technical architecture
- [Animation Editor Architecture (Web)](https://atlasbruce.github.io/Olympe-Engine/technical-reference/animation/animation-editor-architecture) - Web docs

#### Behavior Trees (AI)
- [BT Graph Editor Architecture](Developer/BT_Graph_Editor_Architecture.md) - Technical architecture
- [BT Editor Architecture (Web)](https://atlasbruce.github.io/Olympe-Engine/technical-reference/behavior-trees/bt-editor-architecture) - Web docs
- [Behavior Trees Overview](https://atlasbruce.github.io/Olympe-Engine/technical-reference/behavior-trees/behavior-trees-overview) - AI system

#### Navigation & Pathfinding
- [Navigation System Reference](Navigation_System_Reference.md) - Navigation API
- [Pathfinding Integration](Pathfinding_Integration_Guide.md) - Pathfinding setup
- [Collision Types Reference](Collision_Types_Reference.md) - Collision system
- *See also: [Navigation_README_OLD.md](Navigation_README_OLD.md) for detailed navigation docs*

### 👨‍💻 Developer Guides
- [Testing Guide](../TESTING_GUIDE.md) - Testing practices
- [Standalone Window Pattern](../STANDALONE_WINDOW_PATTERN.md) - Editor window architecture
- [Coding Standards](https://atlasbruce.github.io/Olympe-Engine/contributing/coding-standards) - Code conventions
- [Code Documentation Guide](CODE_DOCUMENTATION_GUIDE.md) - Documentation standards

### 📖 API Reference
- **[Doxygen API Docs](https://atlasbruce.github.io/Olympe-Engine/api)** - Complete API reference

### 🗄️ Archive
- [Archived Development Notes](Archive/README.md) - Historical documentation

## 🎯 Common Tasks

### Creating Animations
1. Press **F9** to open Animation Editor
2. Create new animation bank
3. Add spritesheet with grid configuration
4. Define animation sequences with frame ranges
5. Preview and save
6. [Full Guide →](Animation_Editor_User_Guide.md)

### Creating Behavior Trees
1. Press **F8** to open BT Debugger/Editor
2. Enable "Editor Mode"
3. Create nodes from palette (Selector, Sequence, Action, Condition)
4. Connect nodes with pin-based system
5. Configure node properties
6. Validate and save
7. [Full Guide →](../BT_EDITOR_USER_GUIDE.md)

### Setting Up Input
1. Edit `Config/Inputs.json` for input profiles
2. Define actions (keyboard, mouse, gamepad)
3. Assign devices to players
4. Query input state in game code
5. [Full Guide →](../Documentation/Input/INPUT_USER_GUIDE.md)

### Creating Tiled Maps
1. Create isometric/orthogonal map in Tiled
2. Add object layers for entities
3. Set layer properties (collision, depth)
4. Export as TMJ/JSON
5. Load in engine via TiledLevelLoader
6. [Full Guide →](../TILED_QUICK_START.md)

## 📦 Project Organization

```
Olympe-Engine/
├── Docs/                    # Documentation (you are here)
│   ├── Developer/           # Technical architecture docs
│   ├── Archive/             # Historical development notes
│   └── README.md            # This file
├── Documentation/           # Legacy docs (being consolidated)
│   ├── Input/               # Input system docs
│   └── TILED_ISOMETRIC.md
├── Source/                  # Engine source code
│   ├── AI/                  # Behavior trees, pathfinding
│   ├── Animation/           # Animation system
│   ├── Editor/              # Animation Editor
│   ├── Rendering/           # Rendering systems
│   └── TiledLevelLoader/    # Tiled integration
├── website/                 # Docusaurus documentation site
│   ├── docs/                # Markdown documentation
│   └── static/              # Static assets
├── Config/                  # Configuration files
├── Gamedata/                # Game assets
│   ├── Animations/          # Animation banks (JSON)
│   ├── BehaviorTrees/       # BT files (JSON)
│   └── Sprites/             # Sprite sheets
└── Examples/                # Example projects
```

## 🔍 Finding Documentation

**By Topic**:
- **Editors**: See "Editors" section above
- **AI**: See "Behavior Trees" in Technical Reference
- **Animation**: See "Animation System" in Technical Reference
- **Input**: See "Input System" in User Guides
- **Maps**: See "Tiled Editor Integration" in User Guides

**By File Type**:
- `.md` in `Docs/` - Main documentation
- `.md` in `Documentation/` - Legacy docs
- `.md` in `website/docs/` - Docusaurus site source
- Doxygen output in `website/static/api/` - API reference

**Search**:
- Use website search: https://atlasbruce.github.io/Olympe-Engine/
- Use `grep` on repository for code references

## 📝 Contributing to Documentation

1. **User-facing docs**: Edit files in `website/docs/`
2. **Technical docs**: Edit files in `Docs/Developer/`
3. **API docs**: Add Doxygen comments to source code
4. Follow [Code Documentation Guide](CODE_DOCUMENTATION_GUIDE.md)

## 🆘 Getting Help

- **Search the docs**: https://atlasbruce.github.io/Olympe-Engine/
- **Browse examples**: `Examples/` directory
- **Check source code**: `Source/` with Doxygen comments
- **Open an issue**: https://github.com/Atlasbruce/Olympe-Engine/issues

## 📅 Documentation Updates

- **2026-02-17**: Complete documentation overhaul
  - Added BT Graph Editor and Animation Editor documentation
  - Consolidated and archived obsolete files
  - Updated Docusaurus site with new editor pages
  - Fixed broken links across documentation
  
- **2026-01**: Animation Editor V1 release
- **2025**: BT Editor implementation
- **2025**: Initial Docusaurus site setup

## 🔗 Related Links

- [GitHub Repository](https://github.com/Atlasbruce/Olympe-Engine)
- [Documentation Site](https://atlasbruce.github.io/Olympe-Engine/)
- [API Reference](https://atlasbruce.github.io/Olympe-Engine/api)
- [GitHub Issues](https://github.com/Atlasbruce/Olympe-Engine/issues)
- [GitHub Discussions](https://github.com/Atlasbruce/Olympe-Engine/discussions)
