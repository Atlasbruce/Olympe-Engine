# Recent Completions & Active Development

**Last Updated:** 2026-03-15 09:51:59 UTC

---

## Just Completed ✅

### Phase 22-C — Parameter Dropdowns & Registries
- **PR:** #386 — Merged 2026-03-14 21:59:19 UTC
- 5 centralized registries (AtomicTask, Condition, BBVar, Operator, Master)
- ParameterBindingType extended to 7 types
- Dropdown UI for 8+ node types (no more text fields)
- 8 new validation rules (E020-E025, W010-W011)
- 30+ test cases passing

### Phase 22-A — Switch Node Enhancement
- **PR:** #384 — Merged 2026-03-14 18:46:14 UTC
- Dynamic case labels ("Combat", "Patrol" instead of "Case_5")
- Right-aligned pin labels
- Runtime variable display
- 10/10 tests passing

### Phase 21-D — Dynamic Pins for Sequence/Switch
- **PR:** #382 — Merged 2026-03-14 16:54:08 UTC
- Buttons [+]/[-] to add/remove exec out pins
- AddExecPinCommand + RemoveExecPinCommand (full Undo/Redo)
- Pin de base (Out/Case_0) non supprimable
- RebuildLinks() synchronization

### Phase 21-B — GVS Panel Validation UI
- **PR:** #381 — Merged 2026-03-14 10:03:09 UTC
- Scrollable issue panel with color icons
- Click-to-select faulting node on canvas
- Red badge on toolbar when errors present
- Toggle Auto-verify after each PushCommand

---

## Next Priority (P1)

### Phase 21-C — GVS Pre-save/Pre-exec Validation
- **Status:** ⏳ NEXT
- **Priority:** P1
- **Scope:**
  - Pre-save: confirmation dialog if errors present
  - Pre-exec: block execution if ERROR, log warning if WARNING only
- **Estimated Duration:** ~0.5 session
- **Depends on:** Phase 21-B (✅ merged)

---

## Prioritization

**P1 (HIGH):**
- Phase 21-C — Pre-save/Pre-exec Validation (NEXT)

**P2 (MEDIUM):**
- Phase 22-B — Font Awesome Icons (awaiting design spec)
- Phase 23-A — AnimGraph Infrastructure (future)

**P3 (LOW):**
- Phase 24 — Runtime Execution & Debugger Multi-Instances (future)