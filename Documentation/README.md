# 📚 Olympe Engine Documentation

> **Central documentation hub for Olympe Engine V2**

---

## Table of Contents

- [Getting Started](#getting-started)
- [Core Systems](#core-systems)
- [Tools](#tools)
- [Advanced Topics](#advanced-topics)
- [Contributing](#contributing)

---

## Getting Started

### New to Olympe Engine?

Start here:

1. 📖 **[Main README](../README.md)** - Overview and features
2. 🚀 **[Input Quick Start](Input/INPUT_QUICK_START.md)** - Create your first controllable entity in 5 minutes
3. 🎮 **[Input Examples](Input/INPUT_EXAMPLES.md)** - Practical code examples

### Quick Links

- 🇫🇷 [Guide en Français](../GUIDE_FRANCAIS.md) - Documentation en français
- 🏗️ [Architecture Overview](../ARCHITECTURE.md) - System architecture
- 📋 [Development Plan](../DEVELOPMENT_PLAN.md) - Roadmap and phases

---

## Core Systems

### Input System 🎮

Complete documentation for the modern Input ECS system.

| Document | Description | Audience |
|----------|-------------|----------|
| **[System Guide](Input/INPUT_SYSTEM_GUIDE.md)** | Main documentation - architecture, concepts, components | All developers |
| **[Quick Start](Input/INPUT_QUICK_START.md)** | Get started in 5-10 minutes | Beginners |
| **[API Reference](Input/INPUT_API_REFERENCE.md)** | Complete API documentation | Developers |
| **[Examples](Input/INPUT_EXAMPLES.md)** | Practical code examples (movement, shooting, etc.) | Developers |
| **[Architecture](Input/INPUT_ARCHITECTURE.md)** | Design decisions, performance, extensions | Advanced developers |
| **[Migration](Input/INPUT_MIGRATION.md)** | Migrate from old SDL code to ECS | Existing projects |
| **[Troubleshooting](Input/INPUT_TROUBLESHOOTING.md)** | Problem solving and debugging | All developers |

**Key Features:**
- ✅ Multi-player local (keyboard + gamepads)
- ✅ Hotplug support (automatic reconnection)
- ✅ ECS integration (components + systems)
- ✅ Thread-safe managers
- ✅ Pull & Push APIs

---

### Blueprint System 📋

Entity templates and visual editor.

| Document | Description |
|----------|-------------|
| **[Blueprint README](../Blueprints/README.md)** | Blueprint file format and usage |
| **[Architecture](../ARCHITECTURE.md)** | Blueprint system architecture |
| **[Phase 1 Summary](../PHASE1_SUMMARY.md)** | Phase 1 implementation details |

**Tools:**
- 🛠️ [Blueprint Editor](../OlympeBlueprintEditor/README.md) - Visual entity editor

---

## Tools

### Olympe Blueprint Editor

Visual editor for creating entity blueprints without coding.

- **[Editor Documentation](../OlympeBlueprintEditor/README.md)** - Main documentation
- **[Features](../OlympeBlueprintEditor/FEATURES.md)** - Complete feature list
- **[Quick Start](../OlympeBlueprintEditor/QUICK_START.md)** - Getting started
- **[Usage Guide](../OlympeBlueprintEditor/EDITOR_USAGE.md)** - How to use

---

## Advanced Topics

### Architecture & Design

- 🏗️ **[Architecture Overview](../ARCHITECTURE.md)** - System design and structure
- 📊 **[ECS Components](../Source/ECS_Components.h)** - Available components
- ⚙️ **[ECS Systems](../Source/ECS_Systems.h)** - System definitions

### Performance & Optimization

- 📈 [Input System Performance](Input/INPUT_ARCHITECTURE.md#performance) - Optimization techniques
- 🔄 [ECS Best Practices](Input/INPUT_ARCHITECTURE.md#best-practices) - Writing efficient systems

---

## Documentation by Audience

### 🎓 For Beginners

Start your journey with Olympe Engine:

1. [Main README](../README.md) - What is Olympe Engine?
2. [Input Quick Start](Input/INPUT_QUICK_START.md) - First controllable entity (5 min)
3. [Input Examples](Input/INPUT_EXAMPLES.md) - Learn from examples
4. [Troubleshooting](Input/INPUT_TROUBLESHOOTING.md) - When things don't work

### 👨‍💻 For Developers

Deep dive into the engine:

1. [Input System Guide](Input/INPUT_SYSTEM_GUIDE.md) - Complete system overview
2. [API Reference](Input/INPUT_API_REFERENCE.md) - All classes and methods
3. [Architecture](Input/INPUT_ARCHITECTURE.md) - Design decisions and internals
4. [Migration Guide](Input/INPUT_MIGRATION.md) - Migrate existing projects

### 🎨 For Game Designers

Create content without coding:

1. [Blueprint Editor](../OlympeBlueprintEditor/README.md) - Visual entity creation
2. [Blueprint Format](../Blueprints/README.md) - Understanding blueprints
3. [Input System Guide](Input/INPUT_SYSTEM_GUIDE.md) - Configure controls

### 🔧 For Contributors

Help improve the engine:

1. [Development Plan](../DEVELOPMENT_PLAN.md) - Current roadmap
2. [Architecture](../ARCHITECTURE.md) - System design
3. [ClickUp Tasks](../CLICKUP_TASKS.md) - Task tracking

---

## Contributing

### Documentation Standards

When contributing documentation:

- ✅ Use clear, concise language
- ✅ Include code examples
- ✅ Add diagrams (ASCII art is fine)
- ✅ Cross-reference related documents
- ✅ Test all code examples
- ✅ Use emojis for section icons (optional but nice)

### File Organization

```
Documentation/
├── README.md (this file)
├── Input/
│   ├── INPUT_SYSTEM_GUIDE.md
│   ├── INPUT_QUICK_START.md
│   ├── INPUT_API_REFERENCE.md
│   ├── INPUT_EXAMPLES.md
│   ├── INPUT_ARCHITECTURE.md
│   ├── INPUT_MIGRATION.md
│   └── INPUT_TROUBLESHOOTING.md
└── (future subsystems...)
```

---

## Getting Help

### Questions?

1. **Check the documentation** - Most questions are answered here
2. **Troubleshooting guide** - [Input Troubleshooting](Input/INPUT_TROUBLESHOOTING.md)
3. **Examples** - [Input Examples](Input/INPUT_EXAMPLES.md)
4. **Contact** - nchereau@gmail.com

### Found a Bug?

- Check [Troubleshooting](Input/INPUT_TROUBLESHOOTING.md) first
- Review existing issues
- Provide minimal reproduction code

### Want a Feature?

- Check [Development Plan](../DEVELOPMENT_PLAN.md) - it might be planned
- Submit a detailed feature request
- Consider contributing an implementation

---

## Version History

### V2.0 (2025)

**Input System:**
- ✅ Complete ECS integration
- ✅ Multi-player support
- ✅ Hotplug handling
- ✅ Comprehensive documentation

**Blueprint System:**
- ✅ Phase 1 implementation
- ✅ Visual editor
- ✅ JSON file format

---

## License

See LICENSE file in the repository.

---

## Credits

**Olympe Engine Team - 2025**

- **Nicolas Chereau** - Engine architect & lead developer
- Contributors - Thank you!

---

**Last Updated:** 2025  
**Documentation Version:** 2.0
