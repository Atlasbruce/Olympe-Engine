# 🚀 Development

Documentation for contributors and developers working on Olympe Engine.

---

## 📖 Documentation in This Section

| Document | Description | Audience |
|----------|-------------|----------|
| **[Development Plan](DEVELOPMENT_PLAN.md)** | Roadmap and sprint planning | Contributors |
| **[Phase 1 Summary](PHASE1_SUMMARY.md)** | Blueprint system phase 1 recap | All developers |
| **[Contributing Guide](CONTRIBUTING.md)** | How to contribute to the project | Contributors |
| **[Changelog](CHANGELOG.md)** | Version history and changes | All users |

---

## 🤝 Contributing to Olympe Engine

We welcome contributions! Here's how to get started:

### Quick Start for Contributors

1. **Fork the repository**
   ```bash
   # On GitHub, click "Fork"
   git clone https://github.com/YOUR_USERNAME/Olympe-Engine.git
   cd Olympe-Engine
   ```

2. **Create a branch**
   ```bash
   git checkout -b feature/my-awesome-feature
   ```

3. **Make your changes**
   - Follow coding standards
   - Write tests
   - Update documentation

4. **Commit and push**
   ```bash
   git add .
   git commit -m "Add awesome feature"
   git push origin feature/my-awesome-feature
   ```

5. **Create Pull Request**
   - Go to GitHub
   - Click "New Pull Request"
   - Describe your changes

See: **[Contributing Guide](CONTRIBUTING.md)** for detailed guidelines.

---

## 📋 Development Roadmap

### Current Status (2025)

- ✅ **Phase 1: Blueprint System** - Complete
- ✅ **ECS Core** - Complete
- ✅ **Input System** - Complete
- ✅ **AI Systems** - Complete
- ✅ **Blueprint Editor** - Complete
- 🔄 **Documentation Reorganization** - In Progress

### Upcoming Features

#### Short Term (Next 3 months)
- [ ] Advanced Blueprint Editor features
- [ ] Hot-reload for blueprints
- [ ] Additional component types
- [ ] Network multiplayer foundation

#### Medium Term (6 months)
- [ ] Visual scripting system
- [ ] Advanced AI behaviors
- [ ] Particle system
- [ ] Audio system improvements

#### Long Term (12 months)
- [ ] 3D rendering support
- [ ] Physics engine integration
- [ ] Mobile platform support
- [ ] Asset pipeline tools

See: **[Development Plan](DEVELOPMENT_PLAN.md)** for complete roadmap.

---

## 🏗️ Project Structure

```
Olympe-Engine/
├── Source/                  # Engine source code
│   ├── ECS_Components.h    # Component definitions
│   ├── ECS_Systems.h       # System implementations
│   ├── World.h/cpp         # ECS World management
│   ├── InputsManager.h/cpp # Input system
│   └── AI/                 # AI systems
│       ├── BehaviorTree.h/cpp
│       ├── AIPerception.h/cpp
│       └── AIState.h/cpp
│
├── Blueprints/             # Entity blueprints
│   ├── AI/                 # Behavior trees
│   ├── Enemies/            # Enemy blueprints
│   ├── Items/              # Item blueprints
│   └── NPCs/               # NPC blueprints
│
├── Resources/              # Game assets
│   ├── Sprites/            # Texture files
│   └── Audio/              # Sound files
│
├── Documentation/          # All documentation
│   ├── 01-Getting-Started/
│   ├── 02-Architecture/
│   ├── 03-Core-Systems/
│   ├── 04-AI-Systems/
│   ├── 05-Tools/
│   ├── 06-Development/     # You are here
│   └── 07-Reference/
│
├── Examples/               # Example projects
│
├── SDL/                    # SDL3 library
│
└── Olympe Engine.sln       # Visual Studio solution
```

---

## 💻 Development Environment Setup

### Required Tools

- **Visual Studio 2022** (Windows) or **g++ 9+** (Linux)
- **Git** for version control
- **SDL3** library
- **ImGui** (included in third_party/)

### IDE Configuration

#### Visual Studio 2022
1. Open `Olympe Engine.sln`
2. Set build configuration (Debug/Release)
3. Build → Build Solution (Ctrl+Shift+B)

#### VSCode (Linux)
```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/SDL/include",
                "${workspaceFolder}/Source/third_party"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/g++",
            "cStandard": "c17",
            "cppStandard": "c++17"
        }
    ]
}
```

---

## 🧪 Testing

### Running Tests

```bash
# Build tests
make test

# Run specific test suite
./tests/input_tests
./tests/ai_tests
./tests/blueprint_tests
```

### Writing Tests

```cpp
#include "catch.hpp"
#include "World.h"
#include "ECS_Components.h"

TEST_CASE("Entity creation", "[ecs]") {
    World& world = World::Get();
    
    SECTION("Can create entity") {
        EntityID entity = world.CreateEntity();
        REQUIRE(entity != 0);
    }
    
    SECTION("Can add component") {
        EntityID entity = world.CreateEntity();
        Position_data pos{100.0f, 200.0f, 0.0f};
        world.AddComponent<Position_data>(entity, pos);
        
        auto* retrieved = world.GetComponent<Position_data>(entity);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->position.x == 100.0f);
    }
}
```

---

## 📝 Coding Standards

### C++ Style Guide

#### Naming Conventions
```cpp
// Classes: PascalCase
class EntityBlueprint { };

// Functions: PascalCase
void CreateEntity();

// Variables: camelCase
int entityCount;

// Components: PascalCase with _data suffix
struct Position_data { };

// Constants: UPPER_SNAKE_CASE
const int MAX_ENTITIES = 10000;
```

#### File Organization
```cpp
// Header file (MyClass.h)
#pragma once

class MyClass {
public:
    MyClass();
    void PublicMethod();
    
private:
    void PrivateMethod();
    int privateVariable;
};

// Source file (MyClass.cpp)
#include "MyClass.h"

MyClass::MyClass() {
    // Constructor
}

void MyClass::PublicMethod() {
    // Implementation
}
```

#### Comments
```cpp
// Single line for brief explanations
int count = 0; // Entity count

/**
 * Multi-line for detailed explanations
 * @param entity The entity to process
 * @return True if successful
 */
bool ProcessEntity(EntityID entity);
```

See: **[Contributing Guide](CONTRIBUTING.md)** for complete style guidelines.

---

## 🔍 Code Review Process

### Pull Request Guidelines

1. **Clear Description**
   - What does this PR do?
   - Why is this change needed?
   - Any breaking changes?

2. **Tests**
   - Add tests for new features
   - Ensure existing tests pass
   - Document test coverage

3. **Documentation**
   - Update relevant docs
   - Add code comments
   - Update CHANGELOG.md

4. **Code Quality**
   - Follow coding standards
   - No compiler warnings
   - Clean git history

### Review Checklist

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] New code has tests
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] No merge conflicts
- [ ] Follows coding standards

---

## 🐛 Bug Reports

### Creating a Bug Report

Include:

1. **Description**: Clear summary of the bug
2. **Steps to Reproduce**: Exact steps to trigger
3. **Expected Behavior**: What should happen
4. **Actual Behavior**: What actually happens
5. **Environment**: OS, compiler, SDL version
6. **Code Sample**: Minimal reproduction code

### Bug Report Template

```markdown
## Bug Description
Brief description of the bug

## Steps to Reproduce
1. Step one
2. Step two
3. Bug occurs

## Expected Behavior
What should happen

## Actual Behavior
What actually happens

## Environment
- OS: Windows 11 / Ubuntu 22.04
- Compiler: Visual Studio 2022 / g++ 11
- SDL Version: 3.0
- Engine Version: v2.0

## Code Sample
```cpp
// Minimal code to reproduce
EntityID entity = World::Get().CreateEntity();
// Bug happens here
```
```

---

## 📊 Performance Profiling

### Using Visual Studio Profiler

1. **Debug → Performance Profiler**
2. Select **CPU Usage**
3. Start profiling
4. Run your scenario
5. Stop and analyze results

### Manual Timing

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();

// Code to profile
MySystem.Update(deltaTime);

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

std::cout << "System took: " << duration.count() << "µs" << std::endl;
```

---

## 🔗 Related Documentation

- 📚 [Main Documentation Hub](../README.md)
- 🏗️ [Architecture Overview](../02-Architecture/README.md)
- 🧩 [ECS System](../03-Core-Systems/ECS/README.md)
- 🤖 [AI Systems](../04-AI-Systems/README.md)

---

## 📞 Developer Support

### Getting Help

- 📖 **Documentation**: Read the docs first
- 💬 **Discussions**: GitHub Discussions for questions
- 🐛 **Issues**: GitHub Issues for bugs
- 📧 **Email**: nchereau@gmail.com for direct contact

### Useful Links

- **Repository**: https://github.com/Atlasbruce/Olympe-Engine
- **Issues**: https://github.com/Atlasbruce/Olympe-Engine/issues
- **Pull Requests**: https://github.com/Atlasbruce/Olympe-Engine/pulls

---

[← Back to Documentation Hub](../README.md) | [Next: Reference →](../07-Reference/README.md)
