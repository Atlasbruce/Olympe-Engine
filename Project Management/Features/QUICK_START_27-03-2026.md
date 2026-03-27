# 🚀 QUICK START GUIDE

**Status:** Ready to Launch Phase 0  
**Date:** 2026-03-27  
**Next Step:** Monday, March 31, 2026

---

## ⚡ 5-MINUTE BRIEFING

### What's Happening?

We're consolidating **5 parallel AI systems** (7,710 LOC) into **2 unified systems** with 76% performance gain.

### Why Now?

- Old systems: Complex, hardcoded, slow
- New systems: Declarative VS graphs, data-driven, +50ms/frame
- Foundation ready: LocalBlackboard, GlobalBlackboard, VSGraphExecutor all exist ✅

### What's the Catch?

**3 critical components missing:**
1. EventToBlackboardBridge (2 days to build)
2. SensorSystem (3 days to build)
3. AI_MoveTask (3 days to build)

We MUST build these **FIRST** before archiving legacy systems.

### Timeline

- **Week 0:** Build 3 components (Mar 31 - Apr 4)
- **Weeks 1-5:** Archive legacy + integrate (Apr 7 - May 6)
- **Total:** 6-7 weeks (vs 5-week original plan)

---

## 🎯 3 DECISIONS NEEDED (For @Atlasbruce)

### Decision 1: BehaviorTreeAdapter
**Question:** Keep or archive the BehaviorTreeAdapter?

- ✅ **KEEP:** Use for automatic BT→ATS conversion tool
- ❌ **ARCHIVE:** Write custom converter instead

**Recommendation:** Keep (easier migration)

### Decision 2: State Transitions - Which Blackboard?
**Question:** Store AI mode in LocalBlackboard or GlobalBlackboard?

- ✅ **LocalBlackboard:** Per-entity state (Guard#1 ≠ Guard#2)
- ❌ **GlobalBlackboard:** Shared state (all see same mode)

**Recommendation:** LocalBlackboard (better isolation)

### Decision 3: AI_MoveTask Design
**Question:** How complex should the pathfinding task be?

- ✅ **Lightweight:** Thin wrapper around PathfindingSystem
- ❌ **Heavy:** Full navmesh + complex state management

**Recommendation:** Lightweight (fewer bugs)

---

## 📋 PHASE 0: WEEK-BY-WEEK

### Monday-Tuesday (Mar 31 - Apr 01): P0.1

**Who:** @Dev1  
**What:** EventToBlackboardBridge

```
Mon Mar 31:
├─ 8:00  Standup: Explain P0.1
├─ 9:00  Code: EventToBlackboardBridge.h
├─ 11:00 Code: EventToBlackboardBridge.cpp
├─ 13:00 Lunch
├─ 14:00 Tests: EventToBlackboardBridgeTests.cpp
└─ 17:00 Review + commit

Tue Apr 01:
├─ 8:00  Fix test failures (if any)
├─ 10:00 Integration test
├─ 12:00 Code review
└─ 14:00 DONE: P0.1 complete & tested
```

**Files Created:**
- Source/TaskSystem/EventToBlackboardBridge.h
- Source/TaskSystem/EventToBlackboardBridge.cpp
- Tests/TaskSystem/EventToBlackboardBridgeTests.cpp

**Success:** All unit tests pass ✅

---

### Tuesday-Wednesday (Apr 01 - Apr 03): P0.2

**Who:** @Dev2  
**What:** SensorSystem

```
Tue Apr 01:
├─ 9:00  Code: SensorComponent.h + SensorSystem.h
├─ 11:00 Code: SensorSystem.cpp (begin)
└─ 17:00 Commit partial

Wed Apr 02:
├─ 8:00  Code: SensorSystem.cpp (continue)
├─ 11:00 Tests: SensorSystemTests.cpp
├─ 14:00 Debug + fix
└─ 17:00 Integration test

Thu Apr 03:
├─ 8:00  Performance verify (timesliced @ 5Hz)
├─ 10:00 Code review
└─ 12:00 DONE: P0.2 complete & tested
```

**Files Created:**
- Source/ECS/Systems/SensorSystem.h
- Source/ECS/Systems/SensorSystem.cpp
- Source/ECS/Components/SensorComponent.h
- Tests/ECS/Systems/SensorSystemTests.cpp

**Success:** Vision + hearing queries work ✅

---

### Wednesday-Friday (Apr 02 - Apr 04): P0.3

**Who:** @Dev3  
**What:** AI_MoveTask

```
Wed Apr 02:
├─ 9:00  Code: AI_MoveTask.h
├─ 11:00 Code: AI_MoveTask.cpp (begin)
└─ 17:00 Commit partial

Thu Apr 03:
├─ 8:00  Code: AI_MoveTask.cpp (continue)
├─ 11:00 Tests: AI_MoveTaskTests.cpp
├─ 14:00 Debug + fix
└─ 17:00 Integration test

Fri Apr 04:
├─ 8:00  Pathfinding integration verify
├─ 10:00 Code review
├─ 12:00 DONE: P0.3 complete & tested
└─ 14:00 Phase 0 sign-off meeting
```

**Files Created:**
- Source/TaskSystem/AtomicTasks/AI_MoveTask.h
- Source/TaskSystem/AtomicTasks/AI_MoveTask.cpp
- Tests/TaskSystem/AtomicTasks/AI_MoveTaskTests.cpp

**Success:** Pathfinding + movement work ✅

---

## 📊 PHASE 0 DEFINITION OF DONE

### P0.1 ✅

- [ ] EventToBlackboardBridge.h/cpp compiled
- [ ] RegisterEventMapping() validates inputs
- [ ] ProcessEvents() reads EventQueue + writes LocalBB
- [ ] Unit tests: 100% pass
- [ ] Code review: Approved
- [ ] No compiler warnings
- [ ] Committed + pushed

### P0.2 ✅

- [ ] SensorSystem + SensorComponent compiled
- [ ] Vision queries with angle + distance
- [ ] Raycast line-of-sight checking
- [ ] Hearing range queries
- [ ] Timesliced updates (5 Hz)
- [ ] LocalBlackboard outputs verified
- [ ] Unit tests: 100% pass
- [ ] Performance: < 2ms for 100 entities
- [ ] Code review: Approved
- [ ] Committed + pushed

### P0.3 ✅

- [ ] AI_MoveTask.h/cpp compiled
- [ ] Pathfinding system integration
- [ ] Waypoint following logic
- [ ] Arrival detection (threshold)
- [ ] Status returns (Running/Success/Failure)
- [ ] Abort stops movement
- [ ] Unit tests: 100% pass
- [ ] Code review: Approved
- [ ] No compiler warnings
- [ ] Committed + pushed

### Phase 0 FINAL ✅

- [ ] All 3 components have 100% test pass rate
- [ ] Clean build: `msbuild OlympeEngine.sln`
- [ ] Zero compilation warnings
- [ ] All files in version control
- [ ] Project file (.vcxproj) updated
- [ ] Standup: "Phase 0 complete, ready for Phase 1"

---

## 🎬 MONDAY START CHECKLIST (Mar 31)

### Everyone
- [ ] Read: CLEANUP_EXECUTIVE_SUMMARY_27-03-2026.md (5 min)
- [ ] Read: TECHNICAL_CHECKLIST_27-03-2026.md (30 min, your phase)
- [ ] Setup: Local dev environment
- [ ] Verify: Can build existing code

### @Atlasbruce (Decision Maker)
- [ ] Make 3 decisions above
- [ ] Approve Phase 0 start
- [ ] Assign: @Dev1, @Dev2, @Dev3

### @Dev1 (EventToBlackboardBridge)
- [ ] Read: cleaning_redundancy_systems_27-03-2026.md → P0.1 section
- [ ] Setup: TextEditor for .h and .cpp files
- [ ] Plan: 2-day schedule
- [ ] Ready: 8:00 AM Monday

### @Dev2 (SensorSystem)
- [ ] Read: cleaning_redundancy_systems_27-03-2026.md → P0.2 section
- [ ] Review: LocalBlackboard API
- [ ] Review: SpatialPartitioning API
- [ ] Ready: 9:00 AM Tuesday (after P0.1 start)

### @Dev3 (AI_MoveTask)
- [ ] Read: cleaning_redundancy_systems_27-03-2026.md → P0.3 section
- [ ] Review: PathfindingSystem API
- [ ] Review: IAtomicTask interface
- [ ] Ready: 9:00 AM Wednesday (after P0.1 start)

### @QA1 (Testing)
- [ ] Read: TECHNICAL_CHECKLIST_27-03-2026.md (all phases)
- [ ] Setup: Unit test framework
- [ ] Prepare: Test cases for P0.1, P0.2, P0.3
- [ ] Ready: Support devs daily

### @Tech Lead
- [ ] Read: All 5 documentation files
- [ ] Setup: Daily standup template
- [ ] Prepare: Escalation process for blockers
- [ ] Ready: Lead team calls

---

## 📞 DAILY STANDUP FORMAT

**Time:** 9:00 AM daily (Monday-Friday)  
**Duration:** 10-15 minutes  
**Location:** TBD (Zoom/Teams/In-person)

**Agenda:**
1. **Yesterday:** What got done?
   - @Dev1: "EventToBlackboardBridge 80% done, testing today"
   - @Dev2: "SensorSystem prep complete, starting implementation"
2. **Today:** What's next?
   - @Dev1: "Finish tests, code review, submit"
   - @Dev2: "Implement core logic (QueryVisibilityRay)"
3. **Blockers:** Any stuck?
   - Any: "Need clarification on X" → @Tech Lead helps
   - Any: "PathfindingSystem API unclear" → escalate 30min before standup

---

## 🔴 IMMEDIATE BLOCKERS (Pre-Flight Check)

### Do We Have?

- [ ] ✅ LocalBlackboard (exists: Source/TaskSystem/LocalBlackboard.h)
- [ ] ✅ GlobalBlackboard (exists: Source/NodeGraphCore/GlobalBlackboard.h)
- [ ] ✅ EventQueue (exists: Source/EventSystem.h)
- [ ] ✅ SpatialPartitioning (exists: Source/Spatial/SpatialPartitioning.h)
- [ ] ✅ PathfindingSystem (exists: Source/Pathfinding/PathfindingSystem.h)
- [ ] ✅ TaskSystem (exists: Source/TaskSystem/)
- [ ] ✅ AtomicTask interface (exists: Source/TaskSystem/IAtomicTask.h)
- [ ] ✅ TaskRunnerComponent (exists: Source/ECS/Components/TaskRunnerComponent.h)

**All Prerequisites Exist!** ✅ We can start Monday.

---

## 📈 VELOCITY TARGET

### Phase 0 (5-6 days)
- Lines of code: ~2700 (code + tests)
- **Velocity:** ~450-540 LOC/day (sustainable)
- Bug injection: Expect ~5-10 bugs/phase (normal)
- Test coverage: Aim for 90%+

### Phases 1-4 (24 days)
- Archive operations: ~20 files
- Integration: ~30% new code, ~70% refactoring/testing
- **Expected:** Smoother than P0 (known systems)

---

## 🎯 WEEK 1 PREP (After Phase 0)

### Monday Apr 7: Phase 1 Begins

**Parallel Tracks:**
- Track A: @Dev1 on P1.1 CompatibilityLayer
- Track B: @Dev2 on P1.3 AIStimuliSystem archive
- Track C: @Dev3 on P1.4 BehaviorTreeRuntime_data

### Week 1 Goal
- P1.1: CompatibilityLayer done + tested
- P1.2-P1.4: 50% progress (can parallelize)
- No blockers from Phase 0

---

## ✅ FINAL PRE-START CHECKLIST

**Monday 8:00 AM (Mar 31):**

- [ ] All devs present + ready
- [ ] Phase 0 documentation understood
- [ ] Decisions approved (BehaviorTreeAdapter, GlobalBB, AI_MoveTask)
- [ ] Tools setup (VS, Git, build system)
- [ ] First standup scheduled
- [ ] Escalation contacts known
- [ ] Project file updated (new folders created)

**Then:** "Ready... set... code!" 🚀

---

## 📚 HELPFUL REFERENCES

### Code Examples Already in Codebase

```cpp
// LocalBlackboard usage
LocalBlackboard* bb = GetLocalBlackboard(entity);
bb->SetValue("local:key", TaskValue(value));
auto val = bb->GetValue("local:key");

// GlobalBlackboard usage
GlobalBlackboard& gbb = GlobalBlackboard::Get();
gbb.SetValue("global:key", value);

// TaskValue types
TaskValue intVal(42);
TaskValue floatVal(3.14f);
TaskValue boolVal(true);
TaskValue vectorVal(Vector(1, 2, 3));
TaskValue entityVal(entity_id);
```

### Key Files to Reference

```
Source/TaskSystem/LocalBlackboard.h          → API reference
Source/NodeGraphCore/GlobalBlackboard.h      → API reference
Source/ECS/Components/TaskRunnerComponent.h → Per-entity state
Source/TaskSystem/VSGraphExecutor.h         → Graph execution
Source/Spatial/SpatialPartitioning.h        → Spatial queries
Source/Pathfinding/PathfindingSystem.h      → Pathfinding API
Source/EventSystem.h                        → Event queue
```

---

## 🎬 READY TO LAUNCH?

**If YES:** → Proceed to Monday morning  
**If NO:** → Ask questions now (Friday before start)  
**If DECISION NEEDED:** → @Atlasbruce decides by Friday EOD

---

**Next Document:** TECHNICAL_CHECKLIST_27-03-2026.md (detailed daily tasks)  
**Timeline Lock-in:** Monday, March 31, 2026  
**Status:** 🟢 GO / 🔴 NO-GO (awaiting decisions)

---

**Questions Before We Start?** Ask now. Once Phase 0 begins → all hands on deck.
