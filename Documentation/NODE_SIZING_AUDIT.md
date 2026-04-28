# Node Frame Sizing System Audit & Enhancement Plan

## Executive Summary
The Switch node (and all nodes) currently prioritize **title bar width** over **content width** (pin labels). This causes pin labels like `Case_0 [Idle(100)]` to extend beyond the node frame when the title is short.

**Current Problem**:
- Node frame width = title bar width + padding (ImGui/ImNodes default)
- Column widths are hardcoded: left=80px, right=remaining space
- Pin labels can exceed both column widths
- Result: Visual overflow, poor UX, text truncation

---

## Current Implementation Analysis

### Node Rendering Pipeline
**File**: `Source/BlueprintEditor/VisualScriptNodeRenderer.cpp`

**Key Functions**:
1. `RenderNode()` - Basic overload (lines ~210-293)
2. `RenderNode()` - Extended overload (lines ~299-560+)

### Current Sizing Logic (Both Overloads)

```cpp
// Line 225 (basic) & Line 344 (extended)
ImGui::Columns(2, "node_pins", false);  // or "node_pins_extended"
ImGui::SetColumnWidth(0, 80.0f);        // HARDCODED LEFT COLUMN

// Left column: input pins
// ... rendering loop ...

// Right column: output pins
ImGui::NextColumn();
// ... rendering loop with labels like "Case_0 [Idle(100)]" ...

ImGui::Columns(1);  // End columns
ImNodes::EndNode();
```

### Root Causes

| Issue | Location | Impact | Severity |
|-------|----------|--------|----------|
| Hardcoded left column width | `SetColumnWidth(0, 80.0f)` | Pins truncated if > 80px | HIGH |
| No right column width control | Implicit (remaining space) | Right pins overflow | HIGH |
| Title-driven node sizing | ImNodes::BeginNode() behavior | Width = title width + padding | CRITICAL |
| No pre-rendering width measurement | Content rendered inline | No width calculation before render | CRITICAL |
| Dynamic pin labels unbounded | Lines 419-431, 458-470 | Switch case labels can be very long | HIGH |

---

## Problem Manifestation

### Scenario 1: Short Title ("Switch")
```
┌─────────────────┐
│     Switch      │
├─────────────────┤
│ In │ Case_0... │ ← OVERFLOW (label truncated/cut off)
│    │ Case_1... │ ← OVERFLOW
└─────────────────┘
```

### Scenario 2: Long Title ("Switch long teste de titre pour rallonger le cadre du node")
```
┌──────────────────────────────────────────────────────────┐
│ Switch long teste de titre pour rallonger le cadre du... │
├──────────────────────────────────────────────────────────┤
│ In │ Case_0 [Idle(100)]      │ ← VISIBLE (frame expanded)
│    │ Case_1 [Walking(200)]   │ ← VISIBLE
└──────────────────────────────────────────────────────────┘
```

**Observation**: Title width directly correlates with node frame width. Pin labels visible only when title forces expansion.

---

## ImGui/ImNodes Constraints

### ImNodes::BeginNode() Behavior
- **Default**: Node width = max(title_text_width, content_width)
- **Issue**: Title typically measured first, content rendered inline
- **No Direct Size Control**: ImNodes doesn't expose pre-sizing
- **Column System**: ImGui::Columns() within ImNodes node inherits available space

### ImGui::Columns() Inside ImNodes
- Column widths are LOCAL to the node's content area
- SetColumnWidth(0, 80.0f) = left column = 80px fixed
- Right column = remaining_width (all remaining space)
- **Total node width** still driven by title + padding

### Text Width Measurement
- `ImGui::CalcTextSize(text)` returns approximate text dimensions
- Can be called BEFORE rendering to measure content width
- Requires access to ImGui context (available during Render)

---

## Solution Design

### Approach: Content-Driven Width Measurement

**Key Principle**: Measure all content before rendering, set node width to accommodate all content.

**Steps**:
1. **Measure Phase** (Pre-render):
   - Iterate through all output pin labels
   - Iterate through all input pin labels
   - Calculate max width needed for left column
   - Calculate max width needed for right column
   - Account for title bar width
   - Calculate min node width = max(title_width, left_width + right_width + spacing)

2. **Render Phase**:
   - Set column widths based on measurements
   - Render node with ImGui::SetNextItemWidth() or similar
   - ImNodes should respect measured widths

3. **Column Width Strategy**:
   - Left column: measured_left_width (was hardcoded 80.0f)
   - Right column: measured_right_width
   - Minimum widths to ensure readability

### Implementation Strategy

**Function Signature**:
```cpp
// Helper function to measure node content width
struct NodeContentMeasurement {
    float leftColumnWidth;   // Width needed for left pins
    float rightColumnWidth;  // Width needed for right pins
    float titleWidth;        // Width of node title
    float minimumNodeWidth;  // Total node width needed
};

NodeContentMeasurement MeasureNodeContent(
    const TaskNodeDefinition& def,
    const std::vector<std::string>& execInputPins,
    const std::vector<std::string>& execOutputPins,
    const std::vector<std::pair<std::string, VariableType>>& dataInputPins,
    const std::vector<std::pair<std::string, VariableType>>& dataOutputPins);
```

**Measurement Algorithm**:
```cpp
1. Measure title: ImGui::CalcTextSize(def.NodeName.c_str())
2. For each left pin (exec input + data input):
   - Measure pin label text
   - Track maximum width
3. For each right pin (exec output + data output):
   - Measure pin label text
   - For Switch: add [customLabel(value)] suffix
   - Track maximum width
4. Add padding (spacing, icons):
   - Left: text_width + pin_icon_width + spacing
   - Right: pin_icon_width + text_width + spacing
5. Return measurements
```

### Proposed Minimum Widths
- Minimum left column: 70px (label + triangle icon + spacing)
- Minimum right column: 70px (triangle icon + label + spacing)
- Minimum title width: 100px
- **Minimum node width**: max(title_width, left_width + right_width + 20px padding)

---

## Implementation Changes Required

### File: `Source/BlueprintEditor/VisualScriptNodeRenderer.cpp`

**Changes**:

1. **Add measurement helper function** (after existing helpers):
   ```cpp
   NodeContentMeasurement MeasureNodeContent(
       const TaskNodeDefinition& def,
       const std::vector<std::string>& execInputPins,
       const std::vector<std::string>& execOutputPins,
       const std::vector<std::pair<std::string, VariableType>>& dataInputPins,
       const std::vector<std::pair<std::string, VariableType>>& dataOutputPins);
   ```

2. **Add struct definition** (before implementation):
   ```cpp
   struct NodeContentMeasurement {
       float leftColumnWidth;
       float rightColumnWidth;
       float titleWidth;
       float minimumNodeWidth;
   };
   ```

3. **Update Basic RenderNode()** (lines ~225-226):
   - Call MeasureNodeContent()
   - Use measured widths instead of hardcoded 80.0f

4. **Update Extended RenderNode()** (lines ~344-345):
   - Call MeasureNodeContent()
   - Use measured widths instead of hardcoded 80.0f
   - Account for Switch node case labels

---

## Backward Compatibility

**No Breaking Changes**:
- Same function signatures
- Same rendering output (just sized better)
- All existing node types supported
- Column system unchanged (just sizes adjusted)

**Benefits**:
- All nodes benefit from improved sizing
- VSSequence with many outputs
- Switch nodes with long case labels
- Nodes with long parameter names

---

## Testing Strategy

### Unit Testing
1. Verify MeasureNodeContent() returns reasonable values
2. Test with various pin label lengths
3. Test with title of various lengths

### Visual Testing
1. Switch node with short title + long case labels
   - Expected: Frame expands to contain all labels
   - Current: Labels overflow

2. VSSequence with many output pins
   - Expected: Frame wide enough for all outputs
   - Current: May truncate if many outputs

3. AtomicTask with long parameter names
   - Expected: Frame wide enough for parameters
   - Current: May truncate

### Performance Testing
1. Measure frame time with many nodes
2. Verify no regression vs current implementation
3. Check memory usage unchanged

---

## Expected Outcomes

### Before Fix
```
┌───────────────────┐
│    Switch         │
├───────────────────┤
│ In │ Case_0 [Idle  │ ← TRUNCATED
│    │ Case_1 [Walk  │ ← TRUNCATED
└───────────────────┘
```

### After Fix
```
┌────────────────────────────────────┐
│          Switch                    │
├────────────────────────────────────┤
│ In │ Case_0 [Idle(100)]            │ ← VISIBLE
│    │ Case_1 [Walking(200)]         │ ← VISIBLE
└────────────────────────────────────┘
```

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|-----------|
| ImNodes doesn't respect width hints | Low | Width still title-driven | Fallback: enforce min width |
| Performance degradation | Very Low | Frame drops | Add caching if needed |
| Text width calculation inaccurate | Low | Sizing mismatch | Tune padding/margins |
| Regression in other node types | Low | Visual issues | Comprehensive testing |

---

## Phase Alignment

**Related to Phase 26 UX Enhancements**: ✅
- Phase 26-A completed: Modal UX improvements
- Phase 26-A Refinements: Layout & scrolling fixes
- Phase 26-B (This): Node frame sizing fixes

This work represents the visual presentation layer improvements, following the data layer (Phase 1-7) and modal layer (Phase 26-A) fixes.

---

## Deliverables

1. ✅ Audit document (this file)
2. ⏳ Measurement helper function
3. ⏳ Updated RenderNode() overloads
4. ⏳ Build verification
5. ⏳ Visual testing
6. ⏳ Performance validation

