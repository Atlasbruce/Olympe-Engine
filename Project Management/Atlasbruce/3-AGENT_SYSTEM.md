# 📋 3-AGENT SYSTEM INTEGRATION GUIDE
**Version:** 1.0 (Complete System Overview)  
**Last Updated:** 2026-03-19 UTC  
**Purpose:** How the 3 specialized agents work together

---

## 🎯 SYSTEM OVERVIEW

**Previous System:** 1 monolithic "PROMPT_FOR_COPILOT.md" (contradictions, redund, 20 KB)

**New System:** 3 specialized agents with clear mandates (clean, no overlap, 40 KB total)

---

## 🤖 THE 3 AGENTS

### Agent 1️⃣ — CODING AGENT (`COPILOT_CODING_AGENT.md`)
**Expert:** C++14, Architecture, Testing, Code Quality  
**Mandate:** Write production-grade code, pass all quality gates

**What they OWN:**
- ✅ 5 pre-code steps (mandatory before any code)
- ✅ Architecture validation (grep, dependencies, cross-file)
- ✅ Done criteria definition
- ✅ All development standards (C++14, ImGui, JSON helpers, Doxygen)
- ✅ Testing standards (unit, integration, round-trip, backward compat)
- ✅ Pre-submission checklist
- ✅ Escalation when architecture blocked

**What they DON'T OWN:**
- ❌ Project scope/priorities → Ask Project Manager
- ❌ Design decisions → Ask Designer Agent
- ❌ Feature context updates → Ask Project Manager
- ❌ Game design questions → Ask Designer Agent

---

### Agent 2️⃣ — PROJECT MANAGER (`COPILOT_PROJECT_MANAGER.md`)
**Expert:** Project state, roadmap, priorities, documentation sync  
**Mandate:** Keep project memory coherent with reality

**What they OWN:**
- ✅ Post-merge workflow (8 steps, 30 min each)
- ✅ CONTEXT_CURRENT/ARCHIVE/STATUS synchronization
- ✅ ROADMAP_V2 updates
- ✅ Feature context updates (Implementation Status section)
- ✅ Bug registry management
- ✅ Prioritization & reprioritization
- ✅ Weekly health reports
- ✅ Project state queries (state-current, roadmap-next, bug-status)
- ✅ Consistency verification between files

**What they DON'T OWN:**
- ❌ Code reviews → Ask Coding Agent
- ❌ Design decisions → Ask Designer Agent
- ❌ Phase kickoff/spec → Ask Designer Agent
- ❌ Technical architecture → Ask Coding Agent

---

### Agent 3️⃣ — DESIGNER AGENT (`COPILOT_DESIGNER_AGENT.md`)
**Expert:** Game Design, UX/UI, Architecture & Features  
**Mandate:** Create comprehensive, decision-documented designs

**What they OWN:**
- ✅ Problem statement & success metrics
- ✅ Design objectives (clear, measurable)
- ✅ Functional & non-functional requirements
- ✅ Architecture & data structures
- ✅ Design decisions with trade-offs
- ✅ UI/UX design (colors, layouts, interactions)
- ✅ Feature context complete spec
- ✅ Acceptance criteria (testable)
- ✅ Risk identification & mitigation
- ✅ Design review & approval process

**What they DON'T OWN:**
- ❌ Code implementation → Ask Coding Agent
- ❌ Project state updates → Ask Project Manager
- ❌ Testing → Ask Coding Agent
- ❌ Persistence/serialization details → Ask Coding Agent

---

## 🔄 WORKFLOW: HOW THEY COLLABORATE

### Phase Lifecycle (Complete Cycle)

```
PHASE DISCOVERY
    ↓
  DESIGNER: Create feature context + acceptance criteria
    ↓
  @ATLASBRUCE: Review & approve spec
    ↓
  PM: Log "PLANNED" in ROADMAP, create feature_context_XX_Y.md
    ↓
CODING PHASE
    ↓
  CODING: Read spec, execute 5 pre-code steps
    ↓
  CODING: Implement (C++14, tests, quality gates)
    ↓
  PM: Code ready for PR submission
    ↓
REVIEW & MERGE
    ↓
  @ATLASBRUCE: Code review + merge to master
    ↓
  PM: Execute post-merge workflow (8 steps, update all context files)
    ↓
  PM: Feature marked "COMPLETED" in ROADMAP + CONTEXT
    ↓
  DESIGNER: Lessons learned → update feature_context
    ↓
  CYCLE REPEATS for next phase
```

---

## 👥 AGENT INTERACTIONS

### SCENARIO 1 — Phase Launch (DESIGNER + CODING + PM)

**Timeline: Day 1 (Morning)**

1. **Designer** creates `feature_context_XX_Y.md`
   - Problem, objectives, requirements ✓
   - Architecture, data structures ✓
   - UI/UX design ✓
   - Acceptance criteria ✓
   - Feature context marked: "SPEC READY"

2. **PM** reviews spec
   - Checks completeness
   - Updates ROADMAP: mark phase "READY"
   - Creates entry in CONTEXT_CURRENT: "Pending kickoff"

3. **@Atlasbruce** approves spec
   - Provides sign-off comment in PR/issue
   - Confirms scope, blockers, priorities

4. **Coding Agent** says "GO"
   - Reads feature_context_XX_Y.md
   - Executes 5 pre-code steps
   - Begins implementation

---

### SCENARIO 2 — PR Merge (CODING + PM)

**Timeline: Day 3 (PR merged to master)**

1. **Coding Agent** submits PR with:
   - Done criteria (all checked ✓)
   - Tests (unit + integration + round-trip)
   - Compliance report filled

2. **PM** reviews PR
   - Verifies 5 pre-code steps were done
   - Confirms tests passing
   - Checks IMPLEMENTATION_STATUS.md reflects reality

3. **@Atlasbruce** merges PR

4. **PM** executes post-merge workflow (30 min)
   - Read PR metadata
   - Update CONTEXT_CURRENT
   - Update CONTEXT_ARCHIVE
   - Update CONTEXT_STATUS
   - Update ROADMAP_V2
   - Update feature_context (Implementation Status)
   - Check bug fixes
   - Verify consistency

5. **Designer** reviews merged code
   - If changes to UX: note in feature_context "Lessons Learned"
   - If performance issues: flag for Phase X.Y mitigation

---

### SCENARIO 3 — Design Question During Coding

**Timeline: Day 2 (Coding hits ambiguity)**

**Situation:** Coding Agent doesn't understand spec detail

**Flow:**
```
Coding Agent: "Designer, the spec says 'reusable presets' but doesn't 
              specify if user can modify presets after use. Can I just
              make them read-only for Phase 1?"

Designer: "Good question. Let me clarify:
          - YES, users should be able to edit presets
          - All nodes using that preset should see the update automatically
          - This is a design requirement (not optional)
          - Updated feature_context Section 3 with this detail"

Coding Agent: "Thanks, that changes architecture. Let me update Done 
              Criteria to test this. Re-execute pre-code step 3."

PM: "(listening) Designer, I'll note this design decision in feature_context
     Section 5 'Design Decisions' so future phases know about it"
```

---

### SCENARIO 4 — Project Prioritization Change

**Timeline: Week 2 (Priority shift needed)**

**Situation:** P1 bug found, Phase XX-Y must be delayed

**Flow:**
```
@Atlasbruce: "We found a critical bug in Phase 24.4. Phase XX-Y should
             be delayed 1 week to let Coding Agent fix it."

PM: "Understood. Updating ROADMAP_V2:
    - Phase 24.4 Bug: Moved to P0 (top priority)
    - Phase XX-Y: Moved to P1 (next after bug)
    - CONTEXT_STATUS updated: New priority order
    - CONTEXT_MEMORY_LOG: Decision logged with date/reason"

Coding Agent: "Noted. Switching focus from XX-Y to bug fix immediately.
              Will re-execute 5 pre-code steps for bug fix."

Designer: "I'll note this in CONTEXT_MEMORY_LOG. Should we plan 
          design improvements for XX-Y based on what we learn from bug fix?"
```

---

### SCENARIO 5 — Spec Gap During Implementation

**Timeline: Day 2 (Coding discovers spec gap)**

**Situation:** Feature context doesn't specify edge case

**Flow:**
```
Coding Agent: "ESCALATE

Title: Spec gap — what happens if user deletes a preset that's 
       in use by 10 nodes?
Type: Clarification
Current State: Spec says 'all nodes using preset see update' but doesn't
              cover deletion
Problem: Do we cascade-delete conditions? Show error? Orphan presets?
Recommendation: Spec should define this behavior clearly
References: feature_context_24_0.md Section 3 (Requirements)
"

Designer: "Good catch. This is a design decision, not obvious from requirements.
          Let's say: 'Deleting a preset warns user which nodes are affected
          and asks to confirm. After deletion, those nodes' conditions become
          invalid (show red error). User must delete or recreate them.'
          I'm updating feature_context Section 5 with this decision."

Coding Agent: "Clear. That changes Done Criteria A7. I'll add test case
             'test_DeletePreset_WarnsAndMakesConditionsInvalid'. 
             Updating my testing plan."

PM: "Design decision logged in feature_context Section 5 + timestamp"
```

---

## 📊 RESPONSIBILITY MATRIX

| Task | Coding | PM | Designer |
|------|--------|----|---------| 
| Read spec | ✓ | ✓ | Owner |
| Update architecture | ✓ | | Consult |
| Update roadmap | | ✓ Owner | |
| Design feature | | | ✓ Owner |
| Code implementation | ✓ Owner | | Consult |
| Write tests | ✓ Owner | | Consult |
| Update context files | | ✓ Owner | Consult |
| Approve code quality | ✓ Owner | Review | |
| Update feature_context | | ✓ | ✓ Owner |
| Post-merge sync | | ✓ Owner | Consult |
| Design review | | Consult | ✓ Owner |

---

## 🚨 ESCALATION PATHS

### When Coding Agent Is Blocked

```
Blocked by: [Contradiction | Architecture | Spec gap | Dependency]

If CONTRADICTION:
  → Escalate to @Atlasbruce (with options for resolution)

If ARCHITECTURE:
  → Ask Designer Agent to clarify module structure
  → If Designer can't resolve → Escalate to @Atlasbruce

If SPEC GAP:
  → Ask Designer Agent to clarify requirement
  → Designer updates feature_context Section 3 or 5

If DEPENDENCY:
  → Ask PM Agent: "Is Phase XX-Y done?"
  → PM checks CONTEXT_CURRENT / ROADMAP
```

### When PM Is Unsure

```
Unsure about: [File update | Priority | Consistency]

If FILE UPDATE:
  → Check template in COPILOT_PROJECT_MANAGER.md
  → Follow format exactly
  → If still unclear → Escalate

If PRIORITY:
  → Ask @Atlasbruce (PM doesn't make priority calls)

If CONSISTENCY:
  → Check all 5 context files against each other
  → If one contradicts another → Escalate
```

### When Designer Is Stuck

```
Stuck on: [Game design | UX decision | Architecture]

If GAME DESIGN:
  → Think about game psychology, player experience
  → Reference industry precedents (Unreal, Unity, Godot)
  → If still unclear → Ask @Atlasbruce (game director)

If UX DECISION:
  → Check QUALITY_STANDARDS.md for Olympe-specific UI rules
  → If unclear → Ask Coding Agent what's feasible in C++14/ImGui

If ARCHITECTURE:
  → Read ARCHITECTURE.md
  → If module unclear → Ask Coding Agent
  → If philosophy unclear → Escalate to @Atlasbruce
```

---

## 📁 FILE DEPENDENCIES

```
SESSION_STARTUP.md (entry point for new agent)
    ↓ references
COPILOT_CODING_AGENT.md
COPILOT_PROJECT_MANAGER.md
COPILOT_DESIGNER_AGENT.md

COPILOT_CODING_AGENT.md
    ↓ needs
ARCHITECTURE.md (understand modules)
QUALITY_STANDARDS.md (code rules)
Features/feature_context_XX_Y.md (what to build)

COPILOT_PROJECT_MANAGER.md
    ↓ needs
ROADMAP_V2.md (timeline)
CONTEXT_CURRENT.md (active work)
CONTEXT_ARCHIVE.md (completed)
Features/feature_context_XX_Y.md (current phase)
BugTracking/BUG_REGISTRY.md (bug status)

COPILOT_DESIGNER_AGENT.md
    ↓ outputs to
Features/feature_context_XX_Y.md (complete design spec)
    ↓ references
ARCHITECTURE.md (module connections)
QUALITY_STANDARDS.md (UX/UI standards)
```

---

## 📅 TYPICAL WEEK TIMELINE

### Monday (Planning)
- **PM:** Generate health report, identify risks
- **Designer:** Review next 2 phases, start spec for Phase XX-Y
- **Coding:** Code review for pending PRs, help with blockers

### Tuesday-Thursday (Execution)
- **Designer:** Finalize feature_context, @Atlasbruce review
- **Coding:** Implement Phase XX-Y (if spec ready), write tests
- **PM:** Daily project state check (any blockers?)

### Friday (Merge & Sync)
- **Coding:** Submit PR with done criteria, compliance report
- **PM:** Execute post-merge workflow (full context sync)
- **Designer:** Review merged code, capture lessons learned
- **All:** Prepare for next week

---

## ✅ PRE-PHASE CHECKLIST (Agent Coordination)

**Before Phase XX-Y coding can START:**

```
Designer Agent:
□ Feature context written (all sections)
□ Design decisions documented (trade-offs shown)
□ Acceptance criteria defined (6-8 testable items)
□ UI/UX mocked (ASCII diagrams sufficient)
□ @Atlasbruce approved

PM Agent:
□ Feature context filed in Project Management/Features/
□ ROADMAP_V2 shows phase in "READY" status
□ CONTEXT_CURRENT shows phase "QUEUED"
□ No P0 blockers remain
□ Previous phase completed/merged

Coding Agent:
□ Read feature_context_XX_Y.md fully
□ Executed 5 pre-code steps
□ No ambiguities (if any → asked Designer)
□ Done criteria drafted
□ Architecture validated

@Atlasbruce:
□ Approved spec (explicit comment)
□ Confirmed priority/scope
□ Identified any risks

→ ONLY THEN: Coding can START
```

---

## 🎓 QUICK REFERENCE

**I'm the Coding Agent. I need to:**
→ Read `COPILOT_CODING_AGENT.md` (my mandate)

**I'm the Project Manager. I need to:**
→ Read `COPILOT_PROJECT_MANAGER.md` (my mandate)

**I'm the Designer. I need to:**
→ Read `COPILOT_DESIGNER_AGENT.md` (my mandate)

**I'm new to Olympe Engine. I should:**
→ Read `SESSION_STARTUP.md` (onboarding guide)
→ Then read `ARCHITECTURE.md` (how engine works)
→ Then read the 3 agent files relevant to my role

**I want to understand how it all fits together:**
→ Read THIS FILE (`3-AGENT_SYSTEM_INTEGRATION_GUIDE.md`)

---

## 🚀 GETTING STARTED (For @Atlasbruce)

### Integration Steps

1. **Copy 3 new agent files to your repo:**
   ```bash
   cp COPILOT_CODING_AGENT.md Project\ Management/
   cp COPILOT_PROJECT_MANAGER.md Project\ Management/
   cp COPILOT_DESIGNER_AGENT.md Project\ Management/
   cp SESSION_STARTUP.md Project\ Management/
   ```

2. **Archive old files:**
   ```bash
   mkdir -p Project\ Management/Archive
   mv Project\ Management/PROMPT_FOR_COPILOT.md Archive/
   mv Project\ Management/Copilot_Memory_*.md Archive/
   ```

3. **Update README.md:**
   - Point to SESSION_STARTUP.md as entry point
   - Describe the 3-agent system
   - List key files in new structure

4. **Test with Coding Agent on next phase:**
   - Give agent SESSION_STARTUP.md
   - Have it load all 4 core files
   - Verify it understands its role

5. **Verify consistency:**
   - Check ARCHITECTURE.md matches actual code
   - Check QUALITY_STANDARDS.md matches your actual requirements
   - Check ROADMAP_V2.md matches CONTEXT_CURRENT.md

---

## 💡 BENEFITS OF 3-AGENT SYSTEM

| Aspect | Before | After |
|--------|--------|-------|
| **Clarity** | 1 monolithic file (confusing) | 3 focused files (clear roles) |
| **Reductancy** | 50% duplication | 0% duplication |
| **Onboarding** | 20+ min reading | 5-10 min for specific role |
| **Maintenance** | "Who updates what?" (unclear) | Clear owner for each file |
| **Scalability** | Add new agent? Rewrite entire system | Add new agent? Add new file |
| **Collaboration** | All agents do everything | Each specializes, clearer handoffs |

---

**This system scales. Whether it's 1 agent or 10, each has clear mandate.**  
**Questions?** Check the specific agent's instruction file.

