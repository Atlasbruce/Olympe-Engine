# 2-Minute Brief: Animation Graph Planning
**Date**: 2026-04-16 | **Status**: ✅ PLANNING COMPLETE | **Recommendation**: GO

---

## THE ASK
Enrichir le framework (Canvas/Renderer) pour supporter un nouveau type de graphe **"Animation"** (timeline + keyframes).

---

## THE ANSWER

### Current State (3 Types)
```
┌─ VisualScript    (ImNodes, Adapter pattern)
├─ BehaviorTree    (ImNodes, Adapter pattern)
└─ EntityPrefab    (Custom Canvas, Direct pattern) ← We follow THIS pattern
```

### Animation System Proposed
```
┌─ AnimationGraphDocument       (IGraphDocument impl)
├─ AnimationGraphRenderer       (IGraphRenderer impl)
├─ TimelineCanvasEditor        (ICanvasEditor impl, timeline-specific)
└─ Supporting utilities         (Keyframe/Track renderers, playback)
```

### Why This Works
✅ **Pattern Proven**: EntityPrefab (Phase 27+) follows same architecture  
✅ **Extensible**: All abstractions in place (IGraphDocument, IGraphRenderer, ICanvasEditor)  
✅ **Low Risk**: Coordinate math tested in Phases 28-30, lessons applied  
✅ **Testable**: Clear data model, isolated phases  

---

## THE SCOPE

### Files to Create (8 new)
```
AnimationEditor/
├── AnimationTimelineData.h/cpp        (Keyframes, Tracks)
├── AnimationLoader.h/cpp              (JSON → Data)
├── AnimationSerializer.h/cpp          (Data → JSON)
├── AnimationGraphDocument.h/cpp       (IGraphDocument)
├── AnimationGraphRenderer.h/cpp       (IGraphRenderer)
├── TimelineCanvasEditor.h/cpp         (ICanvasEditor + timeline zoom)
├── KeyframeTrackRenderer.h/cpp        (Visual rendering)
└── TimelinePlaybackController.h/cpp   (Playback state)
```

### Files to Modify (3 existing)
```
TabManager.h/cpp                       (+ANIMATION case)
BlueprintEditorGUI.cpp                 (+File→New Animation menu)
IGraphDocument.h                       (+ANIMATION enum)
```

### Total
- **New**: ~1800 LOC
- **Modified**: ~150 LOC modified (very surgical)
- **No Breaking Changes**: Existing graphs unaffected

---

## THE PHASES

| Phase | Name | Duration | Deliverable |
|-------|------|----------|-------------|
| **1** | Data Model | 1 week | JSON serialization round-trip ✓ |
| **2** | Document | 1 week | Load/Save/Dirty tracking ✓ |
| **3** | Rendering | 1 week | Visual timeline + playback ✓ |
| **4** | Integration | 1 week | File menu → tab → working |
| **5** | Advanced | optional | Undo/Redo, curve editor, etc. |

**Total**: 4 weeks (1 FTE)

---

## THE PATTERNS

### ✅ Follow (Proven from Phases 44.4-52)
1. **Initialization Completeness** (Phase 52): All objects init'd before Render()
2. **Move Semantics Safety** (Phase 51): Never access after std::move()
3. **Logging Discipline** (Phase 44.4): Logs ≠ render loops
4. **Frame Cycle Timing** (Phase 45): Modals render AFTER content
5. **Coordinate Math** (Phases 28-30): Formula `(screen - pos - offset) / zoom`
6. **IGraph* Compliance** (Phase 35+): 100% interface implementation

### ❌ Avoid
- Using ImNodes for timeline (not designed for 1D temporal)
- Over-complicated coordinate math (test exhaustively)
- Frame-by-frame logging (console spam)
- Incomplete initialization (use Phase 52 pattern)

---

## THE RISKS

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Coordinate bugs | HIGH | Test formulas Phase 1+2 |
| Timeline lag (many keyframes) | MEDIUM | Implement culling Phase 3 |
| ImGui context issues | MEDIUM | Follow Phase 45 modal timing |
| JSON schema issues | MEDIUM | Validate schema upfront |

**Overall Risk**: LOW (established patterns, clear spec, EntityPrefab precedent)

---

## THE DECISION

```
╔══════════════════════════════════════════════════════════════╗
║  ✅ RECOMMENDATION: GO - PROCEED WITH IMPLEMENTATION         ║
╠══════════════════════════════════════════════════════════════╣
║  • Architecture is sound (follows EntityPrefab pattern)     ║
║  • Design is complete (no unknowns remain)                  ║
║  • Patterns are proven (Phases 44.4-52)                     ║
║  • Timeline is realistic (4 weeks)                          ║
║  • Risks are manageable (mitigation strategies)             ║
╚══════════════════════════════════════════════════════════════╝
```

---

## THE DELIVERABLES (5 Documents)

| Doc | Purpose | Audience | Size |
|-----|---------|----------|------|
| **Audit** | Framework inventory | Architects | 2000 lines |
| **Guide** | Tech specification | Developers | 2500 lines |
| **Summary** | Decision document | Leadership | 1500 lines |
| **Checklist** | Step-by-step verify | QA/Reviewers | 2000 lines |
| **Index** | Navigation guide | Everyone | 800 lines |

📁 All in: `Source/BlueprintEditor/`

---

## NEXT ACTIONS

**Immediately** (Today)
- [ ] Share these 5 documents
- [ ] 30 min: Read Executive Summary
- [ ] Decision: Go or No-Go?

**Before Phase 1** (This Week)
- [ ] Team architecture discussion
- [ ] Assign Phase 1 developer
- [ ] Confirm JSON schema

**Week 1** (Phase 1 Starts)
- [ ] Developer writes AnimationTimelineData
- [ ] Unit tests for serialization
- [ ] Code review approved

---

## QUESTIONS?

**Q: Why not adapt existing code?**  
A: Timeline ≠ node graph. EntityPrefab (Direct pattern) already proved this approach.

**Q: How different from EntityPrefab?**  
A: Same architecture, different rendering (timeline vs spatial canvas).

**Q: What about undo/redo?**  
A: Phase 5 (optional). Phase 1-4 delivers core functionality.

**Q: How tested?**  
A: Unit tests (serialization), integration tests (TabManager), E2E (user workflows).

**Q: Risk to existing graphs?**  
A: Zero. 3 files modified (isolated cases), no breaking changes.

**Q: Timeline realistic?**  
A: Yes. EntityPrefab took 3 phases, Animation follows same pattern.

---

## KEY FACTS

✅ **Framework Ready**: IGraphDocument, IGraphRenderer, ICanvasEditor abstractions exist  
✅ **Pattern Proven**: EntityPrefab (Phase 27+) demonstrates this works  
✅ **Risk Low**: Coordinate math tested, patterns documented, phases isolated  
✅ **Scope Clear**: 8 new files, 3 modified, ~2000 LOC total  
✅ **Timeline 4 Weeks**: 1 FTE, clearly phased, well-understood work  
✅ **Documentation Complete**: 5 comprehensive guides ready  

---

**Status**: 🟢 **READY FOR APPROVAL**  
**Next Meeting**: Architecture Review (1 hour, 5+ docs provided)  
**Go Decision**: Pending team consensus  
**Estimated Start**: Week 1 upon approval  

---

*For detailed info: See full documentation suite in Source/BlueprintEditor/*
