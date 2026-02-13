---
id: api-index
title: API Reference
sidebar_label: Overview
sidebar_position: 1
---

# API Reference

Welcome to the Olympe Engine API Reference documentation. This section contains automatically generated documentation from the C++ source code.

## 📚 Documentation Sections

### Core Systems

- **[ECS Components](../../api/html/ECS__Components_8h.html)** - All component definitions
- **[ECS Systems](../../api/html/ECS__Systems_8h.html)** - System implementations
- **[Game Engine](../../api/html/classGameEngine.html)** - Core engine class
- **[World](../../api/html/classWorld.html)** - World management

### AI System

- **[Behavior Trees](../../api/html/namespaceBehaviorTree.html)** - AI behavior tree implementation
- **[AI Components](../../api/html/ECS__Components__AI_8h.html)** - AI-specific components
- **[AI Systems](../../api/html/ECS__Systems__AI_8h.html)** - AI processing systems

### Prefab & Blueprint System

- **[Prefab Factory](../../api/html/classPrefabFactory.html)** - Entity creation from prefabs
- **[Prefab Scanner](../../api/html/classPrefabScanner.html)** - Blueprint file parser
- **[Component Definition](../../api/html/structComponentDefinition.html)** - Component schema

### Input System

- **[Input Manager](../../api/html/classInputsManager.html)** - Input handling
- **[Input Device](../../api/html/structInputDeviceSlot.html)** - Device management

### Rendering & Camera

- **[Camera System](../../api/html/classCameraSystem.html)** - Camera management
- **[Rendering](../../api/html/namespaceiRender.html)** - Rendering pipeline

### Utilities

- **[Vector](../../api/html/classVector.html)** - Math vector class
- **[System Utils](../../api/html/system__utils_8h.html)** - Utility functions

## 🔍 Browse Full API

For complete API documentation with all classes, functions, and file listings:

**[→ Browse Full Doxygen Documentation](../../api/html/index.html)**

## 📖 Quick Links

- [Class Index](../../api/html/classes.html)
- [Class Hierarchy](../../api/html/hierarchy.html)
- [File List](../../api/html/files.html)
- [Namespace List](../../api/html/namespaces.html)

## 🛠️ For Developers

This API documentation is automatically generated from source code comments using Doxygen. To update:

1. Add Doxygen comments to your code:
   ```cpp
   /**
    * @brief Brief description of the class
    * 
    * Detailed description of what this class does
    * and how it should be used.
    */
   class MyComponent {
       /**
        * @brief Description of member
        */
       float myValue;
       
       /**
        * @brief Description of method
        * @param param1 Description of parameter
        * @return Description of return value
        */
       bool MyMethod(int param1);
   };
   ```

2. Commit changes - documentation updates automatically via GitHub Actions

## 📝 Documentation Standards

Follow these conventions when documenting code:

- Use `@brief` for short descriptions
- Use `@param` for parameters
- Use `@return` for return values
- Use `@see` for cross-references
- Use `@warning` for important notes
- Use `@example` for code examples

For more information, see the [Contributing Guide](../contributing/code-style).
