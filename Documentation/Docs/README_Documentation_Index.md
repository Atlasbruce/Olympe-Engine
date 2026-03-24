# Olympe Blueprint Editor — Documentation Index

> **Comprehensive Documentation Suite for Blueprint Editor v4**  
> **Version**: 2.0 (Phase 5-8)  
> **Last Updated**: 2026-03-12  
> **Status**: ✅ Complete — All systems documented

---

## 📚 Quick Navigation

| Document | Purpose | Target Audience | Estimated Read Time |
|----------|---------|-----------------|---------------------|
| **[User Guide v4](Blueprint_Editor_User_Guide_v4.md)** | Step-by-step workflows, CRUD operations | Beginners, Content Creators | 20 min |
| **[Advanced Systems](Blueprint_Editor_Advanced_Systems.md)** | SubGraphs, Profiler, Debugger, Templates | Advanced Users, Developers | 45 min |
| **[Visual Diagrams](Blueprint_Editor_Visual_Diagrams.md)** | Interactive Mermaid flowcharts | Visual learners, System architects | 15 min |
| **[Blueprint Editor Features](Blueprint%20Editor%20Features.md)** | ⚠️ DEPRECATED — Historical BT v2 reference | Legacy system users | 10 min |

---

## 🎯 Learning Paths

### Path 1: Complete Beginner
**Goal**: Create your first Visual Script graph from scratch

1. **[User Guide v4](Blueprint_Editor_User_Guide_v4.md)** — Read sections 1-4
   - Introduction
   - Architecture overview
   - Creating your first graph
   - Basic editing operations
2. **[Visual Diagrams](Blueprint_Editor_Visual_Diagrams.md)** — Review "Graph Creation Workflow"
3. **Practice**: Create a simple AI patrol behavior (EntryPoint → Branch → AtomicTask)

**Time Investment**: ~1 hour  
**Prerequisites**: None

---

### Path 2: Intermediate User
**Goal**: Master node types, Blackboard system, and debugging

1. **[User Guide v4](Blueprint_Editor_User_Guide_v4.md)** — Read sections 5-7
   - CRUD operations
   - Blackboard system
   - Debugging
2. **[Advanced Systems](Blueprint_Editor_Advanced_Systems.md)** — Read:
   - Blackboard System (Phase 2.1)
   - Debug System (Phase 5)
3. **[Visual Diagrams](Blueprint_Editor_Visual_Diagrams.md)** — Review:
   - Data Flow Architecture
   - Debug System State Machine
4. **Practice**: Create a combat AI with health checks, state tracking, and breakpoints

**Time Investment**: ~2 hours  
**Prerequisites**: Completed Path 1

---

### Path 3: Advanced Developer
**Goal**: Use SubGraphs, Templates, and profiling for complex systems

1. **[Advanced Systems](Blueprint_Editor_Advanced_Systems.md)** — Read all sections:
   - SubGraph System (Phase 8)
   - Performance Profiler (Phase 5)
   - Template Manager (Phase 5)
   - Command System (Undo/Redo)
2. **[Visual Diagrams](Blueprint_Editor_Visual_Diagrams.md)** — Review:
   - SubGraph Call Stack
   - Profiler Data Flow
   - Command Stack Operations
3. **Practice**: Build a modular boss AI using SubGraphs, save as template, profile performance

**Time Investment**: ~3 hours  
**Prerequisites**: Completed Path 2

---

### Path 4: System Architect / Contributor
**Goal**: Understand internals for extending or debugging the editor

1. **[Advanced Systems](Blueprint_Editor_Advanced_Systems.md)** — Read entire document
2. **[Visual Diagrams](Blueprint_Editor_Visual_Diagrams.md)** — Study all diagrams
3. **Source Code Review**:
   - `Source/BlueprintEditor/blueprinteditor.h/cpp` — Backend singleton
   - `Source/BlueprintEditor/VisualScriptEditorPanel.h/cpp` — v4 editor implementation
   - `Source/TaskSystem/VSGraphExecutor.h/cpp` — Runtime execution engine
   - `Source/TaskSystem/TaskGraphLoader.cpp` — Multi-version loader
4. **Documentation Reference**:
   - `Documentation/Olympe_ATS_VisualScript_Complete_Doc.md` — ATS grammar & node reference
   - `Documentation/ATS_VS_Phase8_Subgraphs.md` — SubGraph technical spec
5. **Practice**: Implement a custom node type or extend validation rules

**Time Investment**: ~6 hours  
**Prerequisites**: C++14, ImGui, JSON experience

---

## 📖 Document Breakdown

### 1. User Guide v4 (Blueprint_Editor_User_Guide_v4.md)

**Primary documentation** for creating and editing Visual Script v4 graphs.

#### Contents:
- **Introduction** (1.5 pages)
  - Overview of ATS Visual Script system
  - Comparison: v2 BehaviorTree vs v4 VisualScript
  - When to use each system
- **Architecture** (2 pages)
  - Backend (BlueprintEditor singleton)
  - Frontend (BlueprintEditorGUI)
  - Core Systems (TaskGraphLoader, VSGraphExecutor)
- **Creating Graphs** (3 pages)
  - New graph workflow (step-by-step)
  - Opening existing graphs
  - Using templates
- **Editing Graphs** (4 pages)
  - Node palette
  - Adding nodes (EntryPoint, Branch, AtomicTask, etc.)
  - Connecting nodes (Exec vs Data pins)
  - Properties panel
  - Canvas navigation
- **CRUD Operations** (3 pages)
  - CreateNode, ConnectExec, ConnectData (with code examples)
  - RemoveNode, DisconnectPin
  - Undo/Redo (Ctrl+Z / Ctrl+Y)
- **Blackboard System** (2 pages)
  - Creating variables (Int, Float, Bool, String, Vector3)
  - GetBBValue / SetBBValue nodes
  - Scope: local: vs global:
- **Debugging** (3 pages)
  - Setting breakpoints (F9)
  - Step controls (F5 Continue, F10 Step Over, F11 Step Into)
  - Watch variables
  - Call stack inspection
- **Keyboard Shortcuts** (1 page)
  - Quick reference table (F5, F9, Ctrl+Z, etc.)
- **File Formats** (2 pages)
  - JSON v4 structure
  - Schema migration (v2 → v4)
- **Quick Start** (1 page)
  - 5-minute workflow summary

**Total Length**: ~20,000 tokens (~40 pages)  
**Format**: Markdown with code examples, tables, and step-by-step instructions

---

### 2. Advanced Systems (Blueprint_Editor_Advanced_Systems.md)

**Deep dive** into advanced features for power users and developers.

#### Contents:
- **Visual Pipeline Diagrams** (5 pages)
  - ASCII art flowcharts for entire pipeline
  - Creation → Editing → Persistence → Loading → Execution → Debugging → Profiling
  - SubGraph execution pipeline with cycle detection
  - Data flow architecture (DataPinCache propagation)
  - Blackboard scoping (local: / global: / input: / output:)
- **SubGraph System (Phase 8)** (4 pages)
  - Schema migration (v4 → v5)
  - SubGraph node type and properties
  - Execution rules (recursion limit, cycle detection)
  - Tab-based navigation (multi-tab editing)
  - Validation (UUID existence, circular dependencies)
  - Code examples (HandleSubGraph implementation)
- **Performance Profiler (Phase 5)** (3 pages)
  - Architecture (NodeExecutionMetrics, FrameProfile)
  - Usage pattern (BeginProfiling → BeginFrame → BeginNodeExecution → EndNodeExecution)
  - ProfilerPanel UI (frame timeline, hotspot table, CSV export)
  - Performance tips (typical node execution times)
- **Debug System (Phase 5)** (3 pages)
  - DebugController (singleton state machine)
  - Breakpoint workflow (F9 toggle, runtime check)
  - DebugPanel UI (toolbar, breakpoints list, call stack, watch variables)
  - Keyboard shortcuts (F5/F9/F10/F11)
- **Blackboard System (Phase 2.1)** (2 pages)
  - BlackboardPanel (CRUD operations)
  - Supported types (Int, Float, Bool, String, Vector3)
  - Variable scoping (local/global/input/output)
  - Usage in graphs (GetBBValue/SetBBValue nodes)
- **Template Manager (Phase 5)** (3 pages)
  - BlueprintTemplate structure (metadata + data)
  - Template catalog (storage location, file format)
  - TemplateManager API (CRUD, queries, application)
  - Usage workflows (save as template, apply template, browse catalog)
- **Validation System** (2 pages)
  - BlueprintValidator rules (unreachable nodes, type mismatches, cycles)
  - ValidationError structure (severity levels)
  - ValidationPanel UI (real-time validation, error navigation)
- **Command System (Undo/Redo)** (3 pages)
  - CommandStack architecture (ICommand interface)
  - Command types (CreateNode, DeleteNode, CreateLink)
  - Command batching (CompositeCommand)
  - History panel (visual timeline)

**Total Length**: ~25,000 tokens (~50 pages)  
**Format**: Markdown with ASCII art, code examples, tables, and detailed technical explanations

---

### 3. Visual Diagrams (Blueprint_Editor_Visual_Diagrams.md)

**Interactive Mermaid diagrams** for visual learners and system architects.

#### Contents:
- **Complete Editor Architecture** (1 diagram)
  - Class hierarchy (BlueprintEditor, VisualScriptEditorPanel, VSGraphExecutor, etc.)
  - Relationships and dependencies
- **Asset Loading Pipeline** (1 flowchart)
  - User action → AssetBrowser → TaskGraphLoader → Editor
  - Multi-version migration (v2/v3/v4)
- **Graph Creation Workflow** (1 sequence diagram)
  - User interactions → Menu → Panel → CommandStack → Template
- **Runtime Execution Flow** (1 state diagram)
  - Node-by-node execution (EntryPoint → Branch → AtomicTask → SubGraph)
  - State transitions (Running, Success, Error)
- **SubGraph Call Stack** (1 flowchart)
  - Cycle detection, depth limiting
  - Frame-by-frame execution
- **Data Flow Architecture** (1 graph)
  - Blackboard (local/global) → GetBBValue → DataPinCache → Branch
- **Debug System State Machine** (1 state diagram)
  - NotDebugging ↔ Running ↔ Paused
  - Step controls (F5/F10/F11)
- **Command Stack Operations** (1 flowchart)
  - Execute → Undo → Redo → Branch
  - CurrentIndex tracking
- **Validation Pipeline** (1 flowchart)
  - Auto-validate → Check rules → Display errors → Navigate to node
- **Profiler Data Flow** (1 sequence diagram)
  - BeginProfiling → BeginFrame → BeginNodeExecution → EndNodeExecution → Render
- **Blackboard Scoping Hierarchy** (1 graph)
  - World → Entity → TaskRunner → LocalBlackboard
  - DataPinCache relationships
- **Template System Workflow** (1 flowchart)
  - Save as template → Browse catalog → Apply template → Load graph
- **Multi-Tab SubGraph Navigation** (1 state diagram)
  - RootTab ↔ SubGraphTab1 ↔ SubGraphTab2
- **Type System & Data Pin Validation** (1 graph)
  - Type compatibility checks (Float → Float ✅, Float → Int ❌)

**Total Length**: ~5,000 tokens (~10 pages of diagrams)  
**Format**: Mermaid diagrams (render in GitHub, VS Code, or Markdown viewers)

---

### 4. Blueprint Editor Features (DEPRECATED)

**Historical reference** for legacy BT v2 system. **DO NOT USE for new projects.**

#### Contents:
- ⚠️ Deprecation warning at top
- Redirect to User Guide v4
- Comparison table (v2 vs v4)
- Historical BT v2 reference (for archives only)

**Status**: Obsolete — kept for historical reference  
**Replacement**: [User Guide v4](Blueprint_Editor_User_Guide_v4.md)

---

## 🔍 Search by Topic

### Graph Creation
- **User Guide v4**: Section 3 ("Creating Graphs")
- **Visual Diagrams**: "Graph Creation Workflow"
- **Advanced Systems**: "Visual Pipeline Diagrams" → "PHASE 2: GRAPH CREATION"

### Node Types
- **User Guide v4**: Section 4 ("Editing Graphs") → Table of node types
- **ATS Documentation**: `Documentation/Olympe_ATS_VisualScript_Complete_Doc.md` → Full node reference

### Blackboard System
- **User Guide v4**: Section 6 ("Blackboard System")
- **Advanced Systems**: "Blackboard System (Phase 2.1)"
- **Visual Diagrams**: "Blackboard Scoping Hierarchy"

### Debugging
- **User Guide v4**: Section 7 ("Debugging")
- **Advanced Systems**: "Debug System (Phase 5)"
- **Visual Diagrams**: "Debug System State Machine"

### SubGraphs
- **Advanced Systems**: "SubGraph System (Phase 8)" — Primary documentation
- **Visual Diagrams**: "SubGraph Call Stack"
- **Technical Spec**: `Documentation/ATS_VS_Phase8_Subgraphs.md`

### Performance Profiling
- **Advanced Systems**: "Performance Profiler (Phase 5)"
- **Visual Diagrams**: "Profiler Data Flow"

### Templates
- **Advanced Systems**: "Template Manager (Phase 5)"
- **Visual Diagrams**: "Template System Workflow"

### Undo/Redo
- **User Guide v4**: Section 5 ("CRUD Operations") → Undo/Redo subsection
- **Advanced Systems**: "Command System (Undo/Redo)"
- **Visual Diagrams**: "Command Stack Operations"

### Validation
- **Advanced Systems**: "Validation System"
- **Visual Diagrams**: "Validation Pipeline"

### File Formats
- **User Guide v4**: Section 9 ("File Formats")
- **Advanced Systems**: "SubGraph System" → "Schema Migration (v4 → v5)"

### Keyboard Shortcuts
- **User Guide v4**: Section 8 ("Keyboard Shortcuts")
- **Advanced Systems**: "Debug System" → Keyboard shortcuts table

---

## 🛠️ For Contributors

### Adding New Documentation
1. Create new `.md` file in `Docs/` directory
2. Add entry to this index under "Quick Navigation" and "Document Breakdown"
3. Link from related documents (cross-references)
4. Update "Search by Topic" section if introducing new topics

### Updating Existing Documentation
1. Modify document in place
2. Update "Last Updated" date in document header
3. Update version number if major changes
4. Update this index's "Last Updated" date

### Documentation Standards
- **Format**: Markdown with Mermaid diagrams
- **Code Examples**: Always include language specifier (\`\`\`cpp, \`\`\`json)
- **Cross-References**: Use relative links (`[Text](file.md)`)
- **Deprecation Notices**: Use ⚠️ emoji and redirect to replacement docs
- **Status Tags**: ✅ Current, ⚠️ Deprecated, 🚧 WIP

---

## 📦 Additional Resources

### Technical Specifications
| Document | Purpose |
|----------|---------|
| `Documentation/Olympe_ATS_VisualScript_Complete_Doc.md` | Complete ATS grammar, node reference, blackboard spec |
| `Documentation/UserManual_ATS_VisualScripting.md` | User manual with examples |
| `Documentation/ATS_VS_Phase8_Subgraphs.md` | SubGraph system technical spec |
| `Documentation/ATS_VS_Phase5_VisualEditor.md` | VisualScriptEditorPanel implementation details |
| `Docs/Atomic-Task-System-TechSpec.md` | AtomicTask system architecture |

### Source Code Reference
| File | Purpose |
|------|---------|
| `Source/BlueprintEditor/blueprinteditor.h/cpp` | Backend singleton (asset management, lifecycle) |
| `Source/BlueprintEditor/BlueprintEditorGUI.h/cpp` | Frontend UI (menu bar, panels) |
| `Source/BlueprintEditor/VisualScriptEditorPanel.h/cpp` | v4 Visual Script editor |
| `Source/TaskSystem/VSGraphExecutor.h/cpp` | Runtime execution engine |
| `Source/TaskSystem/TaskGraphLoader.cpp` | Multi-version loader (v2/v3/v4) |
| `Source/BlueprintEditor/PerformanceProfiler.h/cpp` | Profiler system |
| `Source/BlueprintEditor/DebugController.h/cpp` | Debug system |
| `Source/BlueprintEditor/TemplateManager.h/cpp` | Template catalog |

### Video Tutorials (If Available)
- [ ] Creating Your First Graph (5 min)
- [ ] Using Blackboard Variables (8 min)
- [ ] Debugging with Breakpoints (10 min)
- [ ] Building Modular AI with SubGraphs (15 min)
- [ ] Performance Profiling Best Practices (12 min)

---

## 🎓 Certification Checklist

### Beginner (Content Creator)
- ✅ Create a simple graph (EntryPoint + 3 nodes)
- ✅ Add a Blackboard variable (Int or Float)
- ✅ Use GetBBValue and SetBBValue nodes
- ✅ Connect nodes with Exec and Data pins
- ✅ Save and load a graph file
- ✅ Use Ctrl+Z to undo an action

**Recommended Documents**: User Guide v4 (Sections 1-6)

### Intermediate (AI Designer)
- ✅ Create a graph with Branch, While, and Sequence nodes
- ✅ Use local: and global: scoped Blackboard variables
- ✅ Set a breakpoint and use F10 to step through execution
- ✅ Use the Validation Panel to fix errors
- ✅ Save a graph as a template
- ✅ Apply a template to a new graph

**Recommended Documents**: User Guide v4 (all), Advanced Systems (Blackboard, Debug, Templates)

### Advanced (Systems Programmer)
- ✅ Create a modular AI using SubGraphs (3+ levels deep)
- ✅ Profile a graph and identify performance hotspots
- ✅ Implement cycle detection testing (SubGraph A → B → A)
- ✅ Use CompositeCommand for batch operations
- ✅ Write custom validation rules (extend BlueprintValidator)
- ✅ Contribute a new node type to the editor

**Recommended Documents**: Advanced Systems (all), Visual Diagrams (all), Source Code

---

## 📞 Support

### Reporting Issues
1. Check existing documentation (use "Search by Topic" above)
2. Review [Blueprint Editor Features (DEPRECATED)](Blueprint%20Editor%20Features.md) for legacy v2 issues
3. Create GitHub issue with:
   - Documentation version (e.g., "User Guide v4 2.0")
   - Section/page number
   - Expected vs actual behavior
   - Screenshot (if applicable)

### Contributing Documentation
1. Fork repository
2. Create branch: `docs/feature-name`
3. Follow [Documentation Standards](#documentation-standards)
4. Submit Pull Request with summary of changes
5. Tag reviewers: @olympe-engine-docs

---

## 📜 Version History

| Version | Date | Changes |
|---------|------|---------|
| **2.0** | 2026-03-12 | Complete rewrite for v4 system: User Guide v4, Advanced Systems, Visual Diagrams |
| 1.5 | 2026-02-19 | Added Blackboard System (Phase 2.1) |
| 1.0 | 2026-01-15 | Initial documentation for BT v2 system (now deprecated) |

---

## 📝 License

Documentation licensed under MIT License.  
Copyright © 2026 Olympe Engine Team.

---

**Happy Building! 🚀**

If you have questions or suggestions for improving this documentation, please open an issue on GitHub or contact the Olympe Engine documentation team.
