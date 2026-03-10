# 📦 Archived Documentation — Olympe Blueprint Editor

> **Purpose**: Historical reference for deprecated systems  
> **Status**: ⚠️ OBSOLETE — Do not use for new projects  
> **Archive Date**: 2026-03-12

---

## ⚠️ Important Notice

**All documentation in this directory is OBSOLETE and has been replaced by the v4 documentation suite.**

### Use Current Documentation Instead

| Archived Document | Replacement | Reason |
|-------------------|-------------|--------|
| **Blueprint Editor Features.md** | [Blueprint_Editor_User_Guide_v4.md](../Blueprint_Editor_User_Guide_v4.md) | Described deprecated BT v2 system (NodeGraphPanel) |
| **BLUEPRINT-REFACTOR-MARCH-2026.md** | [Blueprint_Editor_Advanced_Systems.md](../Blueprint_Editor_Advanced_Systems.md) | Refactor planning doc, now superseded by implemented v4 system |
| **BehaviorTreeDebugger.md** | [Blueprint_Editor_User_Guide_v4.md](../Blueprint_Editor_User_Guide_v4.md) § 7 (Debugging) | Legacy BT v2 debugger, replaced by v4 DebugController |

---

## 📚 Current Documentation (v4)

### Quick Links

🎯 **Start Here**:
- **[User Guide v4](../Blueprint_Editor_User_Guide_v4.md)** — Complete workflows (20 min read)
- **[Quick Start (FR)](../Blueprint_Editor_Quick_Start_FR.md)** — Guide de démarrage rapide en français (5 min)

🔧 **Advanced Features**:
- **[Advanced Systems](../Blueprint_Editor_Advanced_Systems.md)** — SubGraphs, Profiler, Templates (45 min read)
- **[Visual Diagrams](../Blueprint_Editor_Visual_Diagrams.md)** — 15 interactive Mermaid diagrams (15 min read)

📖 **Reference**:
- **[Pipeline Summary](../Blueprint_Editor_Pipeline_Summary.md)** — One-page ASCII art reference
- **[Documentation Index](../README_Documentation_Index.md)** — Master index with learning paths

---

## 🕰️ Archive Contents

### Blueprint Editor Features.md (Archived: 2026-03-12)

**Original Purpose**: Documentation for BehaviorTree v2 editor (NodeGraphPanel)

**Why Archived**:
- Described **NodeGraphPanel** (marked `@deprecated` in source code, Phase 7)
- NodeGraphPanel kept only for **BehaviorTreeDebugWindow** (runtime visualization)
- **VisualScriptEditorPanel** is the current editor (v4, Phase 5)

**What Was Wrong**:
- ❌ Documented deprecated creation workflows (NodeGraphPanel::AddNode)
- ❌ Incorrect file format (BT v2 instead of VS v4)
- ❌ Missing new features (SubGraphs, Profiler, Templates, etc.)

**Current Replacement**: [Blueprint_Editor_User_Guide_v4.md](../Blueprint_Editor_User_Guide_v4.md)

---

### BLUEPRINT-REFACTOR-MARCH-2026.md (Archived: 2026-03-12)

**Original Purpose**: Planning document for Blueprint Editor refactor

**Why Archived**:
- Refactor planning phase (pre-implementation)
- Actual implementation differs from original plan
- Superseded by complete v4 documentation

**What It Contained**:
- Original refactor goals and phases
- Architecture diagrams (now outdated)
- Migration strategies (now implemented)

**Current Replacement**: [Blueprint_Editor_Advanced_Systems.md](../Blueprint_Editor_Advanced_Systems.md)

---

### BehaviorTreeDebugger.md (Archived: 2026-03-12)

**Original Purpose**: Documentation for legacy BT v2 debugger

**Why Archived**:
- Documented old BT v2 debugging system
- **DebugController** (v4) replaced legacy debugger
- Breakpoint system completely rewritten for Visual Script v4

**What Was Wrong**:
- ❌ Documented BT-specific debugging (BT node IDs, BT state)
- ❌ Missing new debug features (Step Into SubGraph, Call Stack, Watch Variables)

**Current Replacement**: [Blueprint_Editor_User_Guide_v4.md](../Blueprint_Editor_User_Guide_v4.md) § 7 (Debugging)

---

## 🔍 Why These Documents Were Archived

### Core Issue: System Evolution

The Olympe Blueprint Editor evolved from **BehaviorTree v2** (2026-01) to **ATS Visual Script v4** (2026-03). This involved:

1. **Editor Change**: NodeGraphPanel (deprecated) → VisualScriptEditorPanel (current)
2. **File Format Change**: BT v2 nested format → VS v4 flat format
3. **New Features**: SubGraphs (Phase 8), Profiler (Phase 5), Templates (Phase 5), Validation
4. **Schema Migration**: v2 → v3 → v4 (automatic on load via TaskGraphLoader)

Old documentation described **NodeGraphPanel workflows** which are no longer the primary system. NodeGraphPanel is now marked `@deprecated` in source code and kept only for runtime BehaviorTree debug visualization.

### What Changed

| System Component | Old (v2) | New (v4) |
|------------------|----------|----------|
| **Editor** | NodeGraphPanel | VisualScriptEditorPanel |
| **File Format** | BT v2 nested | VS v4 flat |
| **Graph Type** | "BehaviorTree" | "VisualScript" |
| **Node Types** | BT-specific (Selector, Sequence, Decorator) | VS flow control (EntryPoint, Branch, While) |
| **Connections** | Implicit parent-child | Explicit ExecConnections + DataConnections |
| **Debugger** | BT-specific tree traversal | Generic graph execution with breakpoints |

---

## 📜 Historical Context

### Timeline

| Date | Event | Documentation Status |
|------|-------|---------------------|
| **2026-01-15** | Initial BT v2 documentation created | "Blueprint Editor Features.md" v1.0 |
| **2026-02-19** | Blackboard System (Phase 2.1) added | Updated legacy doc |
| **2026-03-07** | Refactor planning initiated | "BLUEPRINT-REFACTOR-MARCH-2026.md" created |
| **2026-03-09** | Visual Script v4 implementation complete | Legacy docs now obsolete |
| **2026-03-12** | Complete documentation rewrite | v4 suite created, legacy docs archived |

### Why Not Delete?

These documents are **archived** (not deleted) for:

1. **Historical Reference** — Understanding system evolution
2. **Legacy Support** — Users still on v2 can reference old workflows (deprecated but functional)
3. **Migration Context** — Helps understand v2→v4 migration decisions
4. **Audit Trail** — Documentation version history for compliance

---

## 🚫 Do Not Use These Documents For

❌ **New Projects** — Use [User Guide v4](../Blueprint_Editor_User_Guide_v4.md) instead  
❌ **Learning Blueprint Editor** — Outdated workflows, incorrect file formats  
❌ **Reference Documentation** — Missing 60% of current features (SubGraphs, Profiler, etc.)  
❌ **Troubleshooting** — Error descriptions don't match v4 system  

---

## ✅ When to Reference Archived Docs

### Valid Use Cases

1. **Understanding Historical Decisions**
   - Why was NodeGraphPanel deprecated?
   - What was the original BT v2 architecture?

2. **Legacy System Maintenance**
   - Working on a codebase still using BT v2 (pre-migration)
   - Debugging legacy `.bt` files (v2 format)

3. **Migration Planning**
   - Understanding differences between v2 and v4
   - Planning custom migration scripts

4. **Academic Research**
   - Studying evolution of visual scripting systems
   - Comparing architectural approaches

### How to Use

1. **Read archived doc** to understand old system
2. **Compare with current doc** to see what changed
3. **Use migration guide** in [Advanced Systems](../Blueprint_Editor_Advanced_Systems.md) to update your workflow

---

## 📞 Questions?

If you need clarification about archived documentation:

1. Check the **replacement document** (see table at top)
2. Review **[Documentation Index](../README_Documentation_Index.md)** for current navigation
3. Read **migration sections** in current docs for v2→v4 transition details

For technical support:
- **GitHub Issues**: Report bugs or request clarifications
- **Email**: docs@olympe-engine.com (if available)

---

## 📝 Archive Management

### Adding New Documents to Archive

When deprecating documentation:

1. Add entry to the **table at top** of this README
2. Explain **why** it was archived (system change, refactor, etc.)
3. Link to **replacement documentation**
4. Update **[Documentation Index](../README_Documentation_Index.md)** to mark as archived
5. Move file to this directory using `Move-Item` (PowerShell) or `mv` (bash)

### Archive Retention Policy

**Retention**: Indefinite (unless storage constraints require cleanup)

**Removal Criteria**: Only if document is 100% irrelevant (e.g., describes removed features with no historical value)

---

**Archive Created**: 2026-03-12  
**Maintained By**: Olympe Engine Documentation Team  
**Last Updated**: 2026-03-12
