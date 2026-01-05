# 📝 Changelog

All notable changes to Olympe Engine will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Added
- Complete documentation reorganization with thematic structure
- New Getting Started guides with installation instructions
- Comprehensive Architecture documentation
- ECS system documentation with examples
- Blueprint system guides and examples
- AI systems documentation (Behavior Trees, NPC AI)
- Tools documentation (Blueprint Editor, Asset Browser)
- Development guides for contributors
- Reference section with credits and SDL setup

### Changed
- Documentation structure reorganized into 7 themed sections
- All documentation files moved to appropriate categories
- Internal links updated to reflect new structure
- French guide integrated into Getting Started section

---

## [2.0.0] - 2025-01-05

### Major Features

#### ECS Architecture
- Complete Entity Component System implementation
- Data-oriented design for performance
- 20+ component types
- 15+ systems with proper execution order
- World management and entity lifecycle

#### Input System
- Multi-player local support (up to 8 players)
- Hotplug device detection and handling
- Keyboard + gamepad support
- ECS integration with input components
- Pull & Push API for input handling
- Thread-safe input manager

#### Blueprint System
- JSON-based entity definitions
- Visual Blueprint Editor (F2 to toggle)
- Component library with 12+ types
- Save/Load functionality
- Blueprint validation
- Example blueprints provided

#### AI Systems
- Behavior Tree engine with 6 node types
- NPC AI with perception system
- State machine (HFSM) with 6 modes
- Timesliced AI for performance
- 8 action types and 6 condition types
- EventQueue integration
- 4 example behavior trees

#### Tools
- Integrated Blueprint Editor (ImGui-based)
- Asset Browser with search and filtering
- Multi-folder asset scanning
- Type-specific asset metadata
- Backend/Frontend architecture
- Runtime hot-reload support (planned)

### Added
- `Documentation/` - Complete documentation structure
- `Documentation/Input/` - Input system documentation (7 files)
- `Documentation/AI/` - AI systems documentation
- `Blueprints/AI/` - Example behavior trees
- SDL3 integration
- ImGui integration for tools
- ImNodes for node-based editing

### Changed
- Migration from SDL2 to SDL3
- Refactored input system to ECS architecture
- Improved AI system performance with timeslicing
- Updated all systems to new ECS patterns

### Fixed
- Input device hotplug stability issues
- Behavior tree execution edge cases
- Component serialization bugs
- Memory leaks in ECS World

---

## [1.5.0] - 2024-12-15

### Added
- Initial Blueprint Editor implementation
- Console-based blueprint editing
- Component property editor
- Blueprint validation system

### Changed
- Improved JSON serialization performance
- Enhanced error handling in blueprint loading

### Fixed
- Blueprint file corruption on save errors
- Component property type mismatches

---

## [1.0.0] - 2024-11-01

### Added
- Initial ECS implementation
- Basic rendering system
- SDL2 integration
- Simple input handling
- Entity and component management

---

## Version History Summary

| Version | Date | Key Features |
|---------|------|--------------|
| **2.0.0** | 2025-01-05 | ECS, Input System, Blueprints, AI, Tools |
| 1.5.0 | 2024-12-15 | Blueprint Editor basics |
| 1.0.0 | 2024-11-01 | Initial ECS and rendering |

---

## Upgrade Guides

### Migrating to 2.0.0

#### Input System Changes

**Before (1.x):**
```cpp
// Direct SDL polling
SDL_Event event;
while (SDL_PollEvent(&event)) {
    if (event.type == SDL_KEYDOWN) {
        // Handle input
    }
}
```

**After (2.0):**
```cpp
// ECS components
World::Get().AddComponent<Controller_data>(player);
World::Get().AddComponent<PlayerController_data>(player);

// Input automatically processed by InputSystem
```

See: [Input Migration Guide](../03-Core-Systems/Input/INPUT_MIGRATION.md)

#### Blueprint Format Changes

**Before (1.x):**
```json
{
    "name": "Entity",
    "components": {
        "Position": {"x": 100, "y": 200}
    }
}
```

**After (2.0):**
```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "Entity",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {"x": 100, "y": 200, "z": 0}
            }
        }
    ]
}
```

---

## Deprecation Notices

### Deprecated in 2.0.0

- ❌ **Old Input API** - Use ECS input components instead
- ❌ **Direct SDL polling** - Use InputsManager and components
- ❌ **Legacy blueprint format** - Use schema_version 1 format

### Removed in 2.0.0

- ❌ SDL2 support (migrated to SDL3)
- ❌ Old component manager (replaced with ECS World)
- ❌ Direct entity creation (use blueprints or ECS API)

---

## Contributor Recognition

### Version 2.0.0 Contributors

- **Nicolas Chereau** - Engine architect, lead developer, documentation
- Thank you to all contributors!

---

## Links

- **Repository**: https://github.com/Atlasbruce/Olympe-Engine
- **Documentation**: [Main README](../README.md)
- **Issues**: https://github.com/Atlasbruce/Olympe-Engine/issues
- **Pull Requests**: https://github.com/Atlasbruce/Olympe-Engine/pulls

---

[← Back to Development](README.md)
