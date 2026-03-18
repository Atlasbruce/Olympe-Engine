# Quality Standards — Branch/While Node UI/UX (Phase 24)

**Created:** 2026-03-18 UTC  
**Applies to:** All Phase 24 PRs (Panel + Canvas rendering)  
**Status:** 📋 Active — enforced from Phase 2 onwards

---

## 1. Code Quality Rules

### 1.1 Language & Compilation
- C++14 strict — no C++17 features (`std::optional`, structured bindings, etc.)
- Zero compile errors, zero compile warnings on headless build
- All new symbols inside `namespace Olympe { }`
- Use `SYSTEM_LOG` for all logging (not `std::cout`)
- Use `json_get_*()` helpers for JSON serialization/deserialization
- All structs initialized with explicit default values

### 1.2 ImGui Rules
- **No modal popups** for condition editing — use inline `CollapsingHeader`
- `ImGui::PushStyleColor` must always be paired with `ImGui::PopStyleColor`
- `ImGui::PushID` / `ImGui::PopID` required around every condition row to avoid ID collisions
- `ImGui::Indent()` / `ImGui::Unindent()` must be balanced
- No emoji or extended UTF-8 characters in `ImGui::Text()` calls

### 1.3 Naming Conventions
- Methods: `RenderXxx()`, `GetXxx()`, `SetXxx()`, `OnXxx()`
- Member variables: `m_camelCase`
- Constants: `k_SCREAMING_SNAKE_CASE`
- UUIDs: generated via existing `UUID::Generate()` helper; never hard-coded strings

---

## 2. UI/UX Rendering Requirements

### 2.1 Color Constants

All colors must match the specification exactly:

| Element | RGBA | Hex |
|---------|------|-----|
| Node title (canvas) | `(0.2, 0.8, 1.0, 1.0)` | `#33CCFF` |
| Properties title (panel) | `(0.2, 0.8, 1.0, 1.0)` | `#33CCFF` |
| Conditions text (canvas) | `(0.2, 0.9, 0.2, 1.0)` | `#33E633` |
| Dynamic pin slots (canvas) | `(1.0, 0.9, 0.2, 1.0)` | `#FFE633` |

### 2.2 Layout Rules
- Title must appear **exactly once** in the Properties panel
- Condition rows must be visually distinct (use `ImGui::Separator()` or padding)
- Dynamic pin labels must be spatially aligned with their pin connectors on the canvas
- Exec pins (In/Then/Else) must be rendered **before** conditions (Section 2 before Section 3)

### 2.3 Interaction Requirements
- Condition list must support: **add**, **edit** (via dropdowns), **delete** (X button), **reorder** (stretch goal)
- Panel must reload automatically when `m_selectedNodeID` changes
- `IsDirty()` must be set to `true` whenever any condition field is modified
- Logical operator dropdown must be **hidden** (or grayed out) for the first condition row

---

## 3. Functional Acceptance Tests

Each PR must demonstrate these scenarios passing (manual verification + headless test):

### Scenario 1 — Add a Simple Condition
1. Select a Branch node
2. In Properties panel, click `[+ Add Condition]`
3. Set left: `mHealth`, operator: `<=`, right: `2`
4. **Expected:** Condition appears in panel; canvas Section 3 shows `[mHealth] <= [2]` in green

### Scenario 2 — Add a Pin-Mode Condition
1. Add a condition, set right operand to **Pin** mode
2. **Expected:**
   - A dynamic pin slot appears on the node canvas (yellow, Section 4)
   - `def.dynamicPins` contains exactly one new entry
   - `NodeConditionRef::rightPinID` matches the new pin's UUID

### Scenario 3 — Delete a Condition
1. Click `[X]` on an existing condition row
2. **Expected:**
   - Condition removed from `def.conditionRefs`
   - If it had a Pin operand, the corresponding `DynamicDataPin` is removed
   - Canvas Section 3 and Section 4 update immediately

### Scenario 4 — Logical Operators
1. Add two or more conditions
2. Set logical operator to `Or` on the second condition
3. **Expected:** Panel shows `Or` label before second condition row; canvas text reads `Or [mSpeed] ...`

### Scenario 5 — Node Selection Change
1. Select a Branch node with conditions
2. Select a different node (or no node)
3. Re-select the Branch node
4. **Expected:** Panel reloads correctly; no stale data; no duplicate title

### Scenario 6 — No Regression on Other Node Types
1. Select a non-Branch/While node (e.g., Action, Sequence)
2. **Expected:** Properties panel renders normally; no green/yellow sections appear; no crashes

---

## 4. PR Review Checklist

Every Phase 24 PR must satisfy **all** of the following before merge:

### Code
- [ ] No new compile errors or warnings
- [ ] No C++17 features used
- [ ] All new ImGui Push/Pop calls balanced
- [ ] No hard-coded UUID strings
- [ ] All structs initialized with defaults

### Visual Correctness
- [ ] Title appears exactly once in Properties panel
- [ ] Section 3 (conditions) visible and green on canvas
- [ ] Section 4 (dynamic pins) visible and yellow on canvas when applicable
- [ ] Condition editing is inline (not modal)

### Functional Correctness
- [ ] `IsDirty()` propagation verified (condition change → template sync)
- [ ] Pin generation verified (Pin-mode operand → DynamicDataPin created)
- [ ] Panel reloads on node selection change
- [ ] Delete button removes condition AND its pins

### Tests
- [ ] All existing Phase 24 headless tests pass (37+ minimum)
- [ ] New test(s) added for new behavior where applicable
- [ ] Zero test failures

### Documentation
- [ ] `IMPLEMENTATION_STATUS.md` updated with new component statuses
- [ ] Version stamp updated (if applicable)

---

## 5. Definition of "DONE"

A component is **DONE** when ALL of the following are true:

1. ✅ Implementation matches the spec in `PHASE_24_SPECIFICATION.md`
2. ✅ All acceptance tests in Section 3 pass (manual + headless)
3. ✅ PR review checklist in Section 4 fully satisfied
4. ✅ `IMPLEMENTATION_STATUS.md` shows `✅ Yes` / `✅ Yes` / `100%` for the component
5. ✅ No regressions introduced in other components

A Phase is **DONE** when ALL its components are DONE and `IMPLEMENTATION_STATUS.md` shows "Overall Status: ✅ READY".

---

*See also: [PHASE_24_SPECIFICATION.md](./PHASE_24_SPECIFICATION.md) | [IMPLEMENTATION_STATUS.md](./IMPLEMENTATION_STATUS.md)*
