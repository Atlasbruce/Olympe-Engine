# 🤝 Contributing to Olympe Engine

Thank you for your interest in contributing to Olympe Engine! This guide will help you get started.

---

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Pull Request Process](#pull-request-process)
- [Issue Guidelines](#issue-guidelines)
- [Documentation](#documentation)

---

## 🤝 Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inclusive environment for everyone.

### Expected Behavior

- ✅ Be respectful and considerate
- ✅ Welcome newcomers and help them learn
- ✅ Focus on constructive criticism
- ✅ Accept feedback gracefully
- ✅ Show empathy towards others

### Unacceptable Behavior

- ❌ Harassment or discriminatory language
- ❌ Personal attacks
- ❌ Trolling or inflammatory comments
- ❌ Publishing others' private information
- ❌ Other unprofessional conduct

---

## 🚀 Getting Started

### 1. Fork and Clone

```bash
# Fork on GitHub first, then:
git clone https://github.com/YOUR_USERNAME/Olympe-Engine.git
cd Olympe-Engine
```

### 2. Set Up Development Environment

#### Windows
```bash
# Open Visual Studio solution
start "Olympe Engine.sln"

# Build solution (Ctrl+Shift+B)
```

#### Linux
```bash
# Install dependencies
sudo apt-get install build-essential cmake libsdl3-dev

# Build
make
```

### 3. Create a Branch

```bash
git checkout -b feature/my-feature
# or
git checkout -b bugfix/fix-something
```

Branch naming conventions:
- `feature/` - New features
- `bugfix/` - Bug fixes
- `docs/` - Documentation updates
- `refactor/` - Code refactoring
- `test/` - Test improvements

---

## 🔄 Development Workflow

### 1. Make Your Changes

- Write clean, readable code
- Follow the coding standards
- Add comments where necessary
- Keep commits focused and atomic

### 2. Test Your Changes

```bash
# Build the project
make

# Run tests
make test

# Manual testing
./OlympeEngine
```

### 3. Commit Your Changes

```bash
git add .
git commit -m "Add feature: Brief description"
```

#### Commit Message Format

```
Type: Brief summary (50 chars or less)

More detailed explanation if needed (wrap at 72 chars).
Explain what and why, not how.

- Bullet points for multiple changes
- Reference issues: Fixes #123
```

**Types:**
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation only
- `style:` Code style (formatting, no logic change)
- `refactor:` Code refactoring
- `test:` Adding tests
- `chore:` Maintenance tasks

**Examples:**
```
feat: Add blueprint hot-reload functionality

Implements automatic reloading of blueprints when files change.
Uses file watcher to detect modifications and reloads affected entities.

Fixes #456
```

```
fix: Correct input handling for multiple gamepads

Previously, only the first gamepad was recognized.
Now properly handles up to 8 gamepads.

Fixes #789
```

### 4. Push and Create Pull Request

```bash
git push origin feature/my-feature
```

Go to GitHub and create a Pull Request.

---

## 💻 Coding Standards

### C++ Style

#### Naming

```cpp
// Classes: PascalCase
class EntityBlueprint { };
class WorldManager { };

// Functions: PascalCase
void CreateEntity();
void UpdateSystems(float deltaTime);

// Variables: camelCase
int entityCount;
std::string blueprintPath;

// Components: PascalCase_data
struct Position_data { };
struct Health_data { };

// Constants: UPPER_SNAKE_CASE
const int MAX_ENTITIES = 10000;
const float DEFAULT_SPEED = 100.0f;

// Enums: PascalCase with PascalCase values
enum class AIMode {
    Idle,
    Patrol,
    Combat
};
```

#### File Structure

```cpp
// Header (.h)
#pragma once

#include <vector>
#include <string>

/**
 * Brief class description
 */
class MyClass {
public:
    // Public interface first
    MyClass();
    ~MyClass();
    
    void PublicMethod();
    int GetValue() const;
    
private:
    // Private implementation
    void PrivateMethod();
    
    // Member variables last
    int m_value;
    std::vector<int> m_data;
};
```

```cpp
// Source (.cpp)
#include "MyClass.h"

#include <iostream>
#include <algorithm>

MyClass::MyClass()
    : m_value(0)
    , m_data()
{
    // Constructor
}

void MyClass::PublicMethod() {
    // Implementation
}
```

#### Formatting

```cpp
// Braces: Same line for functions, new line for control flow
void Function() {
    // code
}

if (condition)
{
    // code
}
else
{
    // code
}

// Indentation: 4 spaces (not tabs)
void Function() {
    if (condition) {
        DoSomething();
    }
}

// Line length: Prefer < 100 characters
void FunctionWithManyParameters(
    int parameter1,
    int parameter2,
    int parameter3)
{
    // Implementation
}
```

#### Comments

```cpp
// Single-line comments for brief explanations
int count = 0; // Number of entities

/**
 * Multi-line comments for detailed documentation
 * 
 * @param entity The entity to process
 * @param deltaTime Time since last frame in seconds
 * @return True if processing was successful
 */
bool ProcessEntity(EntityID entity, float deltaTime);

// TODO: Implement feature X
// FIXME: Bug in edge case Y
// NOTE: Important implementation detail
```

### ECS Patterns

#### Component Design

```cpp
// ✅ Good: Pure data, no logic
struct Position_data {
    Vector position;
};

struct Health_data {
    int currentHealth;
    int maxHealth;
};

// ❌ Bad: Logic in component
struct BadComponent_data {
    void Update() { }  // Don't do this
};
```

#### System Design

```cpp
// ✅ Good: System with clear single responsibility
class MovementSystem {
public:
    void Update(float deltaTime) {
        auto entities = World::Get().GetEntitiesWithComponents<Position_data, Velocity_data>();
        
        for (EntityID entity : entities) {
            auto* pos = World::Get().GetComponent<Position_data>(entity);
            auto* vel = World::Get().GetComponent<Velocity_data>(entity);
            
            pos->position.x += vel->velocity.x * deltaTime;
            pos->position.y += vel->velocity.y * deltaTime;
        }
    }
};
```

---

## 📝 Pull Request Process

### Before Submitting

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] New features have tests
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] Code follows style guide
- [ ] No merge conflicts

### PR Description Template

```markdown
## Description
Brief summary of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation update
- [ ] Refactoring
- [ ] Performance improvement

## Changes Made
- List specific changes
- Be clear and concise
- Reference relevant code

## Testing
Describe how you tested:
- [ ] Unit tests added
- [ ] Manual testing performed
- [ ] Tested on: Windows/Linux

## Screenshots (if applicable)
Add screenshots for UI changes

## Related Issues
Fixes #123
Closes #456

## Checklist
- [ ] Code compiles
- [ ] Tests pass
- [ ] Documentation updated
- [ ] CHANGELOG updated
```

### Review Process

1. **Automated Checks** run (build, tests)
2. **Code Review** by maintainers
3. **Feedback** addressed
4. **Approval** and merge

### Addressing Feedback

```bash
# Make requested changes
git add .
git commit -m "Address review feedback"
git push origin feature/my-feature
```

---

## 🐛 Issue Guidelines

### Bug Reports

Use this template:

```markdown
## Bug Description
Clear summary of the bug

## Steps to Reproduce
1. First step
2. Second step
3. Bug occurs

## Expected Behavior
What should happen

## Actual Behavior
What actually happens

## Environment
- OS: Windows 11 / Ubuntu 22.04
- Compiler: VS2022 / g++11
- Engine Version: v2.0

## Additional Context
Screenshots, logs, code samples
```

### Feature Requests

Use this template:

```markdown
## Feature Description
Clear description of proposed feature

## Use Case
Why is this feature needed?
What problem does it solve?

## Proposed Solution
How could this be implemented?

## Alternatives Considered
What other approaches did you consider?

## Additional Context
Mockups, examples, references
```

---

## 📖 Documentation

### When to Update Documentation

- ✅ New features added
- ✅ API changes
- ✅ Behavior changes
- ✅ Bug fixes that affect usage
- ✅ New examples or tutorials

### Documentation Standards

```markdown
# Title (H1)

Brief introduction paragraph.

## Section (H2)

Content with examples:

```cpp
// Code example
EntityID entity = World::Get().CreateEntity();
```

### Key Points
- Use bullet points
- Be clear and concise
- Include examples

**Important notes in bold**
```

### Where to Add Documentation

| Content Type | Location |
|--------------|----------|
| Getting started | `Documentation/01-Getting-Started/` |
| Architecture | `Documentation/02-Architecture/` |
| Core systems | `Documentation/03-Core-Systems/` |
| AI systems | `Documentation/04-AI-Systems/` |
| Tools | `Documentation/05-Tools/` |
| Development | `Documentation/06-Development/` |
| Reference | `Documentation/07-Reference/` |

---

## 🏆 Recognition

Contributors will be:
- ✅ Listed in CREDITS.md
- ✅ Mentioned in CHANGELOG.md
- ✅ Thanked in release notes

---

## 📞 Getting Help

### Questions?

- 📖 Check [Documentation](../README.md)
- 💬 Open a [Discussion](https://github.com/Atlasbruce/Olympe-Engine/discussions)
- 📧 Email: nchereau@gmail.com

### Unsure About Something?

It's okay! Just ask:
- Open an issue with your question
- Tag it with `question` label
- We'll help you get started

---

## 📜 License

By contributing, you agree that your contributions will be licensed under the same license as the project.

---

**Thank you for contributing to Olympe Engine! 🎮**

---

[← Back to Development](README.md) | [Next: Changelog →](CHANGELOG.md)
