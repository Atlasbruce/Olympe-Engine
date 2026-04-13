# Contributing to Olympe Engine

## Getting Started

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes (see coding standards below)
4. Run the build and tests
5. Submit a Pull Request

## Coding Standards

### C++ Style
- **Standard**: C++14 (no C++17 features)
- **Naming**: PascalCase for classes, camelCase for methods and variables
- **Headers**: Include guards with `#pragma once`
- **Namespaces**: All engine code in `namespace Olympe { ... }`

### Documentation
- Add `@brief`, `@param`, `@return` Doxygen comments to public API
- Add `@see` cross-references where relevant

### File Organization
- One class per header file
- Implementation in corresponding `.cpp`
- Large panels split by concern: `Panel_Canvas.cpp`, `Panel_FileOps.cpp`

## Adding a New Component

1. Define the struct in a `.h` file under `Source/`
2. Register in `ECS_Components_Registration.cpp`:
   ```cpp
   world.RegisterComponent<MyComponent>();
   ```
3. Add to `Gamedata/PrefabEntities/ComponentsParameters.json`:
   ```json
   { "name": "MyComponent", "category": "Gameplay", "parameters": [...] }
   ```
4. Update documentation in `Documentation/01-Core-Concepts/Component-System.md`

## Adding a New BT Node

1. Create `Source/AI/Nodes/MyActionNode.h` + `.cpp`
2. Inherit from `IBTNode`
3. Implement `Execute(context)` returning SUCCESS/FAILURE/RUNNING
4. Register in the node factory
5. Document in `Documentation/05-Behavior-Trees/Node-Reference.md`

## PR Checklist

- [ ] Code compiles without errors or warnings
- [ ] Existing tests pass
- [ ] New functionality has tests (where applicable)
- [ ] Public API has Doxygen comments
- [ ] Documentation updated if user-facing
- [ ] No secrets or hardcoded paths
