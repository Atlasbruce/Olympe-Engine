---
id: code-style
title: Code Style Guide
sidebar_label: Code Style
sidebar_position: 1
---

# Code Style Guide

## Overview

This guide covers coding conventions and documentation practices for Olympe Engine development.

## C++ Style Guidelines

### Naming Conventions

- **Classes**: `PascalCase` (e.g., `GameEngine`, `PrefabFactory`)
- **Structs**: `PascalCase` with `_data` suffix for components (e.g., `Health_data`, `Transform_data`)
- **Functions**: `PascalCase` (e.g., `Initialize()`, `Process()`)
- **Variables**: `camelCase` (e.g., `currentHealth`, `maxHealth`)
- **Member Variables**: `m_` prefix (e.g., `m_entityList`)
- **Static Variables**: `s_` prefix (e.g., `s_instance`)

### Code Organization

- Keep header files focused on declarations
- Implement complex logic in .cpp files
- Use forward declarations when possible
- Group related functionality together

## Documentation Guidelines

All public APIs should be documented using Doxygen comments. See the [Code Documentation Guide](https://github.com/Atlasbruce/Olympe-Engine/blob/master/Docs/CODE_DOCUMENTATION_GUIDE.md) for detailed examples.

### Quick Reference

```cpp
/**
 * @brief Brief description
 * @param paramName Parameter description
 * @return Return value description
 */
ReturnType FunctionName(ParamType paramName);
```

## Best Practices

1. **Write clear, self-documenting code**
2. **Document all public APIs**
3. **Use const correctness**
4. **Prefer composition over inheritance**
5. **Follow RAII principles**
6. **Write unit tests for new features**

## Resources

- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Code Documentation Guide](https://github.com/Atlasbruce/Olympe-Engine/blob/master/Docs/CODE_DOCUMENTATION_GUIDE.md)
