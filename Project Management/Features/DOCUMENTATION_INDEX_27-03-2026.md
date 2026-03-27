# 📑 INDEX: AI System Cleanup Documentation

**Date:** 2026-03-27  
**Status:** COMPLETE DOCUMENTATION SET  
**All Documents:** Ready for Review & Implementation

---

## 📚 Document Map

### 1. 🎯 **START HERE** — Executive Summary (5 min read)

**File:** `CLEANUP_EXECUTIVE_SUMMARY_27-03-2026.md`

**What:** High-level overview for decision makers  
**Who:** @Atlasbruce, Project Managers  
**Why:** Understand key differences from original plan  
**Contains:**
- 3 blocking issues identified
- Timeline comparison (5 weeks → 6-7 weeks)
- 3 decision points requiring approval
- Expected outcomes (code, architecture, performance)

**Key Takeaway:** Phase 0 (5-6 days) MUST be completed before P1-P4 can begin

---

### 2. 📋 **FULL PLAN** — Detailed Implementation Strategy (60 min read)

**File:** `cleaning_redundancy_systems_27-03-2026.md`

**What:** Complete breakdown of all 5 phases (0-4)  
**Who:** Development Team Leads  
**Why:** Understand full scope + dependencies  
**Contains:**
- Analysis comparative (original vs revised)
- Phase 0 detailed specifications (P0.1, P0.2, P0.3)
- Phases 1-4 complete breakdown
- Detailed calendar (week by week)
- Comprehensive risk mitigation
- Full checklist with success criteria

**Key Sections:**
- P0.1: EventToBlackboardBridge (impl + tests)
- P0.2: SensorSystem (impl + tests)
- P0.3: AI_MoveTask (impl + tests)
- P1-P4: Archive + integration phases

**When:** Read BEFORE starting implementation

---

### 3. 🏗️ **ARCHITECTURE** — Diagrams & Data Flows (45 min read)

**File:** `ARCHITECTURE_DIAGRAMS_27-03-2026.md`

**What:** Visual representations of system changes  
**Who:** Architects, Integration Engineers  
**Why:** Understand how pieces connect  
**Contains:**
- Dependency graph (Phase 0-4)
- Data flow: Legacy vs New
- Component architecture: Before → After
- System lifecycle diagrams
- State machine transformation
- Integration flow (TaskRunnerComponent)
- AI entity lifecycle
- Memory & performance comparison
- Implementation dependency graph (detailed)

**Key Diagrams:**
- 9 detailed ASCII diagrams showing:
  - System dependencies
  - Data transformations
  - Component consolidation
  - Execution flow
  - Memory savings (-32%)
  - Performance gains (-76%)

**When:** Read for technical understanding

---

### 4. ✅ **CHECKLIST** — Day-by-Day Execution Guide (120 min read + ongoing)

**File:** `TECHNICAL_CHECKLIST_27-03-2026.md`

**What:** Actionable task breakdown for developers  
**Who:** Development Team (Primary)  
**Why:** Know exactly what to build each day  
**Contains:**
- 60-second quick start
- P0.1-P0.3 detailed checklists (files, code structure, tests)
- P1.1-P1.4 detailed checklists
- P2.1-P2.3 detailed checklists
- P3.1-P3.2 detailed checklists
- P4.1-P4.4 detailed checklists
- Daily standup template
- Blocker resolution guide
- Final success criteria

**Key Features:**
- [ ] Boxes for each task
- [ ] Time estimates
- [ ] Responsible person assignment
- [ ] Definition of Done for each phase
- [ ] Code snippets ready to implement
- [ ] Test names + requirements

**When:** Use EVERY DAY during development

---

## 🎯 READING PATHS

### Path A: "I'm The Project Lead" (15 min)

1. **CLEANUP_EXECUTIVE_SUMMARY_27-03-2026.md** (5 min)
   - Understand the 3 blockers
   - Make 3 decisions (BehaviorTreeAdapter, GlobalBB vs LocalBB, AI_MoveTask design)

2. **cleaning_redundancy_systems_27-03-2026.md** (10 min)
   - Phase 0 overview
   - Timeline lock-in dates
   - Skim P1-P4 summary

**Output:** Approval to start P0

---

### Path B: "I'm The Tech Lead" (60 min)

1. **CLEANUP_EXECUTIVE_SUMMARY_27-03-2026.md** (5 min)
   - Understand blockers + decisions

2. **ARCHITECTURE_DIAGRAMS_27-03-2026.md** (20 min)
   - Study data flows
   - Understand component changes
   - Review dependency graph

3. **cleaning_redundancy_systems_27-03-2026.md** (30 min)
   - P0 detailed specs
   - P1-P4 architecture
   - Risk mitigation

4. **TECHNICAL_CHECKLIST_27-03-2026.md** (5 min)
   - Skim task structure
   - Note test requirements

**Output:** Ready to lead development

---

### Path C: "I'm The Developer" (120 min + ongoing)

1. **CLEANUP_EXECUTIVE_SUMMARY_27-03-2026.md** (5 min)
   - Context

2. **ARCHITECTURE_DIAGRAMS_27-03-2026.md** (20 min)
   - Understand your component in context

3. **cleaning_redundancy_systems_27-03-2026.md** (30 min)
   - Find your assigned phase
   - Read detailed specs

4. **TECHNICAL_CHECKLIST_27-03-2026.md** (60 min + ongoing)
   - Deep dive into your phase's checklist
   - Use as daily reference
   - Check off items as complete

**Output:** Ready to code + test

---

### Path D: "I'm A QA Engineer" (45 min + ongoing)

1. **CLEANUP_EXECUTIVE_SUMMARY_27-03-2026.md** (5 min)
   - Understand scope

2. **ARCHITECTURE_DIAGRAMS_27-03-2026.md** (15 min)
   - Study behavior changes

3. **TECHNICAL_CHECKLIST_27-03-2026.md** (25 min)
   - Review test requirements
   - Study regression test matrix
   - Plan test cases

**Output:** Test plan for each phase

---

## 📅 TIMELINE AT A GLANCE

```
Week 0 (Mar 31-Apr 04): Phase 0 ← CRITICAL PATH
├─ P0.1: EventToBlackboardBridge (2 days, @Dev1)
├─ P0.2: SensorSystem (3 days, @Dev2)
└─ P0.3: AI_MoveTask (3 days, @Dev3)

Week 1 (Apr 07-Apr 11): Phase 1
├─ P1.1: CompatibilityLayer (2 days, @Dev1)
├─ P1.2: Archive AIBlackboard_data (1 day, @Dev1)
├─ P1.3: Archive AIStimuliSystem (2 days, @Dev2)
└─ P1.4: Archive BehaviorTreeRuntime_data (1 day, @Dev3)

Weeks 2-3 (Apr 14-Apr 22): Phase 2
├─ P2.1: SensorSystem Integration (3 days, @Dev1)
├─ P2.2: State Transitions → VS Graph (3 days, @Dev2)
└─ P2.3: Archive AIPerceptionSystem (1 day, @Dev3)

Weeks 3-4 (Apr 23-Apr 29): Phase 3
├─ P3.1: AI_MoveTask Integration (3 days, @Dev1)
└─ P3.2: Archive BehaviorTreeSystem (3 days, @Dev2 + @QA1)

Week 5 (Apr 30-May 06): Phase 4
├─ P4.1: Remove BehaviorTreeDebugWindow (2 days, @Dev1)
├─ P4.2: Archive BehaviorTree Core (2 days, @Dev2)
├─ P4.3: Archive BehaviorTreeManager (1 day, @Dev3)
└─ P4.4: Final Verification + Docs (2 days, Team)

TOTAL: 6-7 weeks (29-31 days)
```

---

## 🔗 CROSS-REFERENCES

### By Topic

**EventToBlackboardBridge:**
- Design: `cleaning_redundancy_systems_27-03-2026.md` → P0.1
- Architecture: `ARCHITECTURE_DIAGRAMS_27-03-2026.md` → Section 4
- Implementation: `TECHNICAL_CHECKLIST_27-03-2026.md` → P0.1

**SensorSystem:**
- Design: `cleaning_redundancy_systems_27-03-2026.md` → P0.2
- Architecture: `ARCHITECTURE_DIAGRAMS_27-03-2026.md` → Section 2, 4
- Implementation: `TECHNICAL_CHECKLIST_27-03-2026.md` → P0.2

**AI_MoveTask:**
- Design: `cleaning_redundancy_systems_27-03-2026.md` → P0.3
- Architecture: `ARCHITECTURE_DIAGRAMS_27-03-2026.md` → Section 7
- Implementation: `TECHNICAL_CHECKLIST_27-03-2026.md` → P0.3

**Phase 0 (Critical):**
- Overview: `CLEANUP_EXECUTIVE_SUMMARY_27-03-2026.md` → TL;DR
- Detailed: `cleaning_redundancy_systems_27-03-2026.md` → PHASE 0
- Implementation: `TECHNICAL_CHECKLIST_27-03-2026.md` → PHASE 0

**LocalBlackboard:**
- Benefits: `CLEANUP_EXECUTIVE_SUMMARY_27-03-2026.md` → Key Decision 2
- Architecture: `ARCHITECTURE_DIAGRAMS_27-03-2026.md` → Sections 2, 4
- Migration: `cleaning_redundancy_systems_27-03-2026.md` → P1.1, P1.2

---

## 📊 METRICS & SUCCESS

### Phase-by-Phase Success Criteria

| Phase | Metric | Target | Verification |
|-------|--------|--------|---------------|
| **0** | Unit tests pass | 100% | All P0.1/P0.2/P0.3 tests pass |
| **1** | Build clean | 0 errors | `msbuild` succeeds |
| **2** | Integration tests | 100% | Perception + state transitions work |
| **3** | Regression tests | 100% | AI behavior identical |
| **4** | Final build | 0 warnings | Release build clean |

### Overall Targets

| Metric | Before | After | Gain |
|--------|--------|-------|------|
| LOC Legacy | 7710 | ~1500 | -6210 (-81%) |
| AI Systems | 5 | 2 | -3 (-60%) |
| AI Components | 4 | 1 | -3 (-75%) |
| Execution Time | ~23ms | ~5.5ms | -17.5ms (-76%) |
| Memory/Entity | 164B | 112B | -52B (-32%) |

---

## 🚀 HOW TO USE THIS DOCUMENTATION

### Week-by-Week Workflow

**Week 0 (Before Phase 0):**
1. [ ] @Atlasbruce reads Executive Summary
2. [ ] Team makes 3 decisions
3. [ ] Approval given
4. [ ] Developers read their P0 phase details

**During Week 0 (Phase 0):**
1. [ ] Developers use TECHNICAL_CHECKLIST
2. [ ] Daily standup using standup template
3. [ ] Each dev checks off completed items
4. [ ] QA tests each component

**After Week 0 (Before Week 1):**
1. [ ] @Dev Lead reviews all P0 tests
2. [ ] Phase 1 teams read their specs
3. [ ] No approval needed (path clear)

**During Weeks 1-5:**
1. [ ] Developers execute checklist items
2. [ ] QA validates each phase
3. [ ] Daily updates vs timeline
4. [ ] Escalate blockers immediately

---

## 📞 COMMUNICATION PROTOCOL

### Daily Standup
- **Time:** 10:00 AM daily
- **Duration:** 10 minutes
- **Template:** See TECHNICAL_CHECKLIST_27-03-2026.md
- **Report:** What done, what's next, any blockers

### Weekly Review
- **Time:** Friday 4 PM
- **Duration:** 30 minutes
- **Items:**
  - Phase progress vs timeline
  - Tests passing/failing
  - Next week's prep
  - Risk assessment

### Escalation Path
**Blocker Found:**
1. Report to your Phase Lead immediately
2. If unresolved after 2 hours → @Tech Lead
3. If unresolved after 4 hours → @Atlasbruce

---

## 🎓 LEARNING RESOURCES

### Additional Context

**LocalBlackboard Documentation:**
- File: `Source/TaskSystem/LocalBlackboard.h`
- Shows: Per-entity key-value storage API

**GlobalBlackboard Documentation:**
- File: `Source/NodeGraphCore/GlobalBlackboard.h`
- Shows: Game-wide state storage API

**TaskRunnerComponent:**
- File: `Source/ECS/Components/TaskRunnerComponent.h`
- Shows: Per-entity VS graph execution state

**VSGraphExecutor:**
- File: `Source/TaskSystem/VSGraphExecutor.h`
- Shows: Frame-by-frame graph execution logic

**Phase 24 Condition Presets:**
- Reference: Copilot Instructions (`.github/copilot-instructions.md`)
- Shows: Condition preset embedding in v4 schema

---

## ✅ DOCUMENT VALIDATION CHECKLIST

- [x] All 4 main documents created
- [x] Cross-references verified
- [x] Timeline consistent (29-31 days)
- [x] Task assignments clear
- [x] Success criteria defined
- [x] Risk mitigation planned
- [x] No conflicting requirements
- [x] C++14 compliance noted
- [x] Test requirements specified
- [x] Decision points documented

---

## 📝 REVISION HISTORY

| Date | Version | Author | Changes |
|------|---------|--------|---------|
| 2026-03-27 | 1.0 | Copilot | Initial complete documentation set |

---

## 🎯 FINAL CHECKLIST BEFORE START

**@Atlasbruce (Project Lead):**
- [ ] Read: CLEANUP_EXECUTIVE_SUMMARY_27-03-2026.md
- [ ] Make decisions: GlobalBB vs LocalBB, BehaviorTreeAdapter, AI_MoveTask design
- [ ] Approve: Phase 0 start
- [ ] Assign: Dev resources (3 devs for P0)

**@Tech Lead:**
- [ ] Read: All 4 documents
- [ ] Review: Architecture diagrams
- [ ] Prepare: Daily standup template
- [ ] Set up: Test infrastructure for P0

**@Developers:**
- [ ] Read: Full plan for assigned phase
- [ ] Study: Technical checklist for your phase
- [ ] Setup: Local dev environment
- [ ] Ready: To start coding

**@QA Lead:**
- [ ] Read: Technical checklist
- [ ] Prepare: Test cases per phase
- [ ] Setup: Test automation infrastructure
- [ ] Verify: Success criteria for each phase

---

**Questions?** → Review cross-references above  
**Ready to start?** → Proceed to Phase 0 checklist  
**Need clarification?** → Refer to specific document sections

---

**Documentation Set Complete:** 2026-03-27  
**Status:** ✅ READY FOR IMPLEMENTATION  
**Approval Required:** Phase 0 start  
**Next Step:** Begin P0.1 (Mon Mar 31)
