# 📚 Olympe Engine Documentation

> **Central documentation hub for Olympe Engine V2**  
> *Complete, organized, and easy to navigate*

---

## 🗺️ Documentation Structure

Olympe Engine documentation is organized into **7 thematic sections** for easy navigation:

| Section | Description | Audience |
|---------|-------------|----------|
| **[01 - Getting Started](01-Getting-Started/)** | Quick start, installation, basics | Everyone |
| **[02 - Architecture](02-Architecture/)** | System design and structure | Developers |
| **[03 - Core Systems](03-Core-Systems/)** | Input, Blueprint, ECS systems | Developers |
| **[04 - AI Systems](04-AI-Systems/)** | Behavior Trees, NPC AI | Developers |
| **[05 - Tools](05-Tools/)** | Blueprint Editor, Asset Browser | Designers, Developers |
| **[06 - Development](06-Development/)** | Contributing, roadmap | Contributors |
| **[07 - Reference](07-Reference/)** | API reference, credits | Everyone |

---

## 🚀 Quick Start Paths

### I'm New to Olympe Engine
1. 📖 **[Main README](../README.md)** - What is Olympe Engine?
2. 🚀 **[Getting Started Guide](01-Getting-Started/README.md)** - Your first steps
3. 🎮 **[Quick Start Tutorial](01-Getting-Started/README.md#quick-start)** - Build something in 5 minutes
4. 💡 **[Examples](03-Core-Systems/Input/INPUT_EXAMPLES.md)** - Learn from code

### I Want to Build a Game
1. 📋 **[Blueprint System](03-Core-Systems/Blueprint/README.md)** - Create entities visually
2. 🎨 **[Blueprint Editor](05-Tools/BLUEPRINT_EDITOR.md)** - Use the visual editor (F2)
3. 🎮 **[Input System](03-Core-Systems/Input/INPUT_SYSTEM_GUIDE.md)** - Handle player input
4. 🤖 **[AI Systems](04-AI-Systems/README.md)** - Add intelligent NPCs

### I'm a Developer
1. 🏗️ **[Architecture Overview](02-Architecture/README.md)** - Understand the design
2. 🧩 **[ECS System](03-Core-Systems/ECS/README.md)** - Master the core architecture
3. 📖 **[API Reference](03-Core-Systems/Input/INPUT_API_REFERENCE.md)** - Detailed API docs
4. 🔧 **[Contributing Guide](06-Development/CONTRIBUTING.md)** - Help improve the engine

### I Want to Contribute
1. 🤝 **[Contributing Guide](06-Development/CONTRIBUTING.md)** - How to contribute
2. 📋 **[Development Plan](06-Development/DEVELOPMENT_PLAN.md)** - Current roadmap
3. 🏗️ **[Architecture Docs](02-Architecture/README.md)** - Understand the codebase
4. 📝 **[Changelog](06-Development/CHANGELOG.md)** - Recent changes

---

## 📦 Section Summaries

### 01 - Getting Started 🚀

**Everything you need to start using Olympe Engine**

- [Getting Started Guide](01-Getting-Started/README.md) - Your launchpad
- [Installation Guide](01-Getting-Started/INSTALLATION.md) - Setup on Windows/Linux
- [Guide Français](01-Getting-Started/GUIDE_FRANCAIS.md) - Documentation en français

**Perfect for:** New users, game designers, anyone wanting a quick start

---

### 02 - Architecture 🏗️

**Deep dive into engine design and structure**

- [Architecture Overview](02-Architecture/README.md) - High-level design
- [General Architecture](02-Architecture/ARCHITECTURE_GENERALE.md) - Complete system details
- [Blueprint Architecture](02-Architecture/BLUEPRINT_ARCHITECTURE.md) - Blueprint system design

**Perfect for:** Developers wanting to understand the engine internals

---

### 03 - Core Systems 🧩

**Documentation for the engine's core systems**

#### Input System 🎮
- [Input System Guide](03-Core-Systems/Input/INPUT_SYSTEM_GUIDE.md) - Complete guide
- [Quick Start](03-Core-Systems/Input/INPUT_QUICK_START.md) - 5-minute tutorial
- [API Reference](03-Core-Systems/Input/INPUT_API_REFERENCE.md) - Detailed API
- [Examples](03-Core-Systems/Input/INPUT_EXAMPLES.md) - Practical code
- [Architecture](03-Core-Systems/Input/INPUT_ARCHITECTURE.md) - Design details
- [Migration Guide](03-Core-Systems/Input/INPUT_MIGRATION.md) - Upgrade from old code
- [Troubleshooting](03-Core-Systems/Input/INPUT_TROUBLESHOOTING.md) - Problem solving

Features: Multi-player, hotplug, ECS integration, thread-safe

#### Blueprint System 📋
- [Blueprint System Guide](03-Core-Systems/Blueprint/README.md) - Overview
- [Blueprint Format](03-Core-Systems/Blueprint/BLUEPRINT_SYSTEM.md) - JSON schema & components

Features: JSON-based entities, visual editor, component library

#### ECS System 🔧
- [ECS Guide](03-Core-Systems/ECS/README.md) - Entity Component System

Features: Data-oriented design, high performance, scalable

**Perfect for:** Developers implementing game features

---

### 04 - AI Systems 🤖

**Artificial intelligence and behavior systems**

- [AI Systems Overview](04-AI-Systems/README.md) - AI architecture
- [AI Architecture](04-AI-Systems/AI_ARCHITECTURE.md) - Design details
- [NPC AI System](04-AI-Systems/NPC_AI_SYSTEM.md) - Complete NPC AI guide
- [Behavior Trees](04-AI-Systems/BEHAVIOR_TREES.md) - BT system & JSON format
- [BT Visual Architecture](04-AI-Systems/BT_VISUAL_ARCHITECTURE.md) - Visual diagrams
- [BT Developer Guide](04-AI-Systems/BT_DEVELOPER_GUIDE.md) - Creating custom trees

Features: Behavior Trees, state machines, perception system, timesliced AI

**Perfect for:** Developers creating AI behaviors

---

### 05 - Tools 🛠️

**Visual tools for content creation**

- [Tools Overview](05-Tools/README.md) - All tools
- [Blueprint Editor](05-Tools/BLUEPRINT_EDITOR.md) - Visual entity editor
- [Asset Browser](05-Tools/ASSET_BROWSER.md) - Asset management

Features: Visual editing, asset browsing, runtime hot-reload (planned)

**Perfect for:** Game designers, content creators

---

### 06 - Development 🚀

**For contributors and engine developers**

- [Development Guide](06-Development/README.md) - Overview
- [Development Plan](06-Development/DEVELOPMENT_PLAN.md) - Roadmap
- [Phase 1 Summary](06-Development/PHASE1_SUMMARY.md) - Blueprint phase recap
- [Contributing Guide](06-Development/CONTRIBUTING.md) - How to contribute
- [Changelog](06-Development/CHANGELOG.md) - Version history

**Perfect for:** Contributors, maintainers, people extending the engine

---

### 07 - Reference 📚

**Quick reference and supplementary materials**

- [Reference Overview](07-Reference/README.md) - Reference hub
- [Credits](07-Reference/CREDITS.md) - Contributors & acknowledgments
- [SDL Setup](07-Reference/SDL_SETUP.md) - SDL3 installation

**Perfect for:** Everyone, quick lookups


---

## 🔍 Find What You Need

### By Topic

- **Getting Started**: [Section 01](01-Getting-Started/)
- **System Design**: [Section 02](02-Architecture/)
- **Input Handling**: [Section 03, Input](03-Core-Systems/Input/)
- **Entity Creation**: [Section 03, Blueprint](03-Core-Systems/Blueprint/)
- **Component System**: [Section 03, ECS](03-Core-Systems/ECS/)
- **AI & Behavior**: [Section 04](04-AI-Systems/)
- **Visual Tools**: [Section 05](05-Tools/)
- **Contributing**: [Section 06](06-Development/)
- **API Reference**: [Section 07](07-Reference/)

### By File Type

- **Guides** - Comprehensive system documentation
- **Quick Starts** - 5-10 minute tutorials
- **API References** - Detailed function/class documentation
- **Examples** - Practical code samples
- **Architecture** - Design decisions and patterns
- **Troubleshooting** - Common problems and solutions

---

## 🌐 Language Options

- 🇬🇧 **English** - All documentation
- 🇫🇷 **Français** - [Guide Français](01-Getting-Started/GUIDE_FRANCAIS.md)

---

## 📊 Documentation Statistics

- **7 Major Sections** - Thematically organized
- **50+ Documents** - Comprehensive coverage
- **100+ Code Examples** - Learn by doing
- **10,000+ Lines** - Detailed explanations

---

## 🔗 External Resources

### Libraries & Dependencies
- **[SDL3 Documentation](https://wiki.libsdl.org/SDL3)** - Graphics, input, audio
- **[ImGui Documentation](https://github.com/ocornut/imgui)** - GUI library
- **[C++ Reference](https://en.cppreference.com/)** - C++ language reference

### Learning Resources
- **[Game Programming Patterns](https://gameprogrammingpatterns.com/)** - Design patterns
- **[ECS FAQ](https://github.com/SanderMertens/ecs-faq)** - Entity Component Systems
- **[Data-Oriented Design](https://www.dataorienteddesign.com/dodbook/)** - Performance patterns

---

## 🆘 Getting Help

### Documentation Issues

**Found a typo or broken link?**
- Open an issue: [GitHub Issues](https://github.com/Atlasbruce/Olympe-Engine/issues)
- Tag with `documentation`
- We'll fix it ASAP

**Can't find what you're looking for?**
1. Try the search function in your browser (Ctrl+F)
2. Check the [Table of Contents](#-documentation-structure) above
3. Look in [Section 07 - Reference](07-Reference/) for quick lookups

### Engine Issues

**Having trouble with the engine?**
1. Check [Troubleshooting](03-Core-Systems/Input/INPUT_TROUBLESHOOTING.md)
2. Review [Examples](03-Core-Systems/Input/INPUT_EXAMPLES.md)
3. Read the relevant system guide
4. Open a GitHub issue if needed

### Questions & Discussion

- 💬 **GitHub Discussions** - Ask questions, share ideas
- 📧 **Email** - nchereau@gmail.com
- 🐛 **Bug Reports** - [GitHub Issues](https://github.com/Atlasbruce/Olympe-Engine/issues)

---

## 🤝 Contributing to Documentation

We welcome documentation improvements!

### How to Help

1. **Fix typos** - Small improvements are valuable
2. **Add examples** - Share your code
3. **Clarify explanations** - Make it easier to understand
4. **Translate** - Help make docs accessible
5. **Add diagrams** - Visual aids help learning

### Guidelines

- ✅ Clear, concise language
- ✅ Include code examples
- ✅ Add cross-references
- ✅ Test all code samples
- ✅ Follow existing format

See: [Contributing Guide](06-Development/CONTRIBUTING.md)

---

## 📅 Documentation Updates

**Last Major Update:** 2025-01-05  
**Documentation Version:** 2.0  
**Engine Version:** 2.0

### Recent Changes
- ✅ Complete reorganization into 7 themed sections
- ✅ Added Getting Started guides
- ✅ Comprehensive architecture documentation
- ✅ AI systems documentation
- ✅ Tools documentation
- ✅ Contributing guidelines
- ✅ Reference section

See: [Changelog](06-Development/CHANGELOG.md) for complete history

---

## 📜 License

See LICENSE file in the repository.

---

## 🙏 Credits

**Olympe Engine Team - 2025**

- **Nicolas Chereau** - Engine architect & lead developer
- **Contributors** - Thank you!
- **Community** - For feedback and support

See: [Full Credits](07-Reference/CREDITS.md)

---

## 🚀 Ready to Start?

Choose your path:

- 🆕 **New User?** → [Getting Started](01-Getting-Started/README.md)
- 👨‍💻 **Developer?** → [Architecture](02-Architecture/README.md)
- 🎨 **Designer?** → [Blueprint Editor](05-Tools/BLUEPRINT_EDITOR.md)
- 🤝 **Contributor?** → [Contributing](06-Development/CONTRIBUTING.md)

---

**Happy Game Development! 🎮**
