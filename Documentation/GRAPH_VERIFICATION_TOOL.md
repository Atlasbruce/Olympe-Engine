# Graph Verification Tool - Complete Documentation

## Overview

The **VSGraphVerifier** is a comprehensive validation system for ATS Visual Script graphs (v4 schema). It performs **22+ validation rules** to ensure graph structural integrity, type safety, blackboard consistency, and reachability.

**Access** : Click the **"Verify"** button in the Visual Script editor tab to run all verification rules.

**Output** : A categorized list of issues (Errors, Warnings, Info) with node references and actionable messages.

---

## Verification Rules

### 🔴 Critical Errors (E-Series)

These issues **MUST** be fixed before the graph can execute correctly.

#### **E001 — No EntryPoint Node**
- **Description** : Graph has no EntryPoint node
- **Cause** : EntryPoint was never created or was accidentally deleted
- **Impact** : Graph cannot execute (no entry point for execution)
- **Fix** : Create an EntryPoint node from the node palette

#### **E002 — Dangling Node (No Exec Connections)**
- **Description** : Node has no incoming or outgoing exec connections
- **Cause** : Node is not connected to the execution flow
- **Impact** : Node will never execute
- **Fix** : Connect the node to the execution flow with exec links (white triangular pins)
- **Exception** : Data-pure nodes (GetBBValue, MathOp) are **exempt** — they connect via data pins, not exec pins

#### **E003 — Execution Cycle Detected**
- **Description** : Circular dependency in execution flow (nodeA → nodeB → ... → nodeA)
- **Cause** : Accidentally created a cycle in exec connections
- **Impact** : Infinite loop at runtime
- **Fix** : Remove one of the exec connections forming the cycle

#### **E004 — Circular SubGraph Reference**
- **Description** : SubGraph A references SubGraph B, which directly/indirectly references SubGraph A
- **Cause** : Recursive SubGraph definitions
- **Impact** : Infinite recursion at runtime
- **Fix** : Break the circular reference by changing one SubGraph's target

#### **E005 — Unknown Node in Connection**
- **Description** : Exec connection references a node ID that doesn't exist
- **Cause** : Data corruption or manual JSON editing error
- **Impact** : Graph cannot deserialize/execute
- **Fix** : Manually remove the invalid connection from the JSON file

#### **E006 — Incompatible Data Pin Types**
- **Description** : Data connection connects pins with incompatible types (e.g., Vector → Bool)
- **Cause** : Type mismatch in data flow
- **Impact** : Type error at runtime
- **Fix** : Reconnect to a compatible pin or add a conversion node

#### **E007 — Inverted Pin Direction**
- **Description** : Two output pins connected together, or two input pins connected together
- **Cause** : User error or data corruption
- **Impact** : Data does not flow correctly
- **Fix** : Disconnect and reconnect with correct direction (output → input)

#### **E008 — Unknown Blackboard Key**
- **Description** : GetBBValue or SetBBValue references a blackboard variable that doesn't exist
- **Cause** : Variable was deleted from blackboard schema or typo in key name
- **Impact** : Runtime error when accessing variable
- **Fix** : 
  - Create the missing variable in the graph's blackboard
  - OR correct the variable name to match an existing variable

#### **E009 — Blackboard Type Mismatch**
- **Description** : SetBBValue attempts to write a value with wrong type for the target variable
- **Cause** : Variable type changed, or MathOp output type doesn't match target
- **Impact** : Type error at runtime
- **Fix** : 
  - Change the target variable type to match the data
  - OR add a data conversion node (if supported)

#### **E010 — Switch Node Missing switchVariable**
- **Description** : Switch node has no `switchVariable` configured
- **Cause** : Configuration incomplete
- **Impact** : Runtime error when evaluating switch
- **Fix** : Set the `switchVariable` property in the node's properties panel

#### **E011 — Switch Node Duplicate Cases**
- **Description** : Two case labels have the same value
- **Cause** : User error in configuration
- **Impact** : One case branch is unreachable (dead code)
- **Fix** : Change one of the duplicate case labels to a unique value

#### **E012 — Switch Node Empty Case Label**
- **Description** : One of the switch cases has an empty pin name
- **Cause** : Configuration incomplete
- **Impact** : That case cannot be triggered
- **Fix** : Set a meaningful label for the case

#### **E021 — Unknown Condition ID**
- **Description** : Branch or While node references a condition that doesn't exist in ConditionRegistry
- **Cause** : Condition was deleted or custom condition not registered
- **Impact** : Condition cannot be evaluated at runtime
- **Fix** : 
  - Register the condition in the ConditionRegistry
  - OR change the node to use an existing condition

#### **E040 — Condition Pin Mode with Empty Reference**
- **Description** : Condition operand set to "Pin mode" but no pin reference is specified
- **Cause** : Configuration incomplete
- **Impact** : Runtime error
- **Fix** : Either change mode to "Const" or "Variable", OR specify a pin reference

#### **E041 — Condition Variable Not in Blackboard**
- **Description** : Condition operand set to "Variable mode" but variable doesn't exist
- **Cause** : Variable was deleted or typo in name
- **Impact** : Runtime error when evaluating condition
- **Fix** : Create the variable or correct the variable name

#### **E042 — Condition Type Mismatch**
- **Description** : Left and right operands of condition have incompatible types
- **Cause** : Type error in condition configuration
- **Impact** : Runtime type error
- **Fix** : Ensure both operands are compatible types

---

### 🟡 Warnings (W-Series)

These issues **should** be fixed for code quality and performance, but graph will still execute.

#### **W001 — AtomicTask with Empty AtomicTaskID**
- **Description** : AtomicTask node has no TaskID assigned
- **Cause** : Node created but not configured
- **Impact** : Node does nothing at runtime (silent failure)
- **Fix** : Set the `AtomicTaskID` property to a valid task type

#### **W002 — Delay with DelaySeconds ≤ 0**
- **Description** : Delay node has duration ≤ 0 seconds
- **Cause** : Invalid configuration
- **Impact** : Delay has no effect (node completes immediately)
- **Fix** : Set DelaySeconds to a positive value

#### **W003 — SubGraph with Empty Path**
- **Description** : SubGraph node has no SubGraphPath configured
- **Cause** : Node created but not configured
- **Impact** : SubGraph cannot be loaded at runtime
- **Fix** : Set the `SubGraphPath` property to point to a valid graph JSON file

#### **W004 — MathOp with Empty Operator**
- **Description** : MathOp node has no operator assigned (not +, -, *, /, %, ^)
- **Cause** : Configuration incomplete
- **Impact** : MathOp produces undefined result
- **Fix** : Set the operator in the node's properties panel

#### **W005 — Unknown AtomicTaskID**
- **Description** : AtomicTaskID is not registered in AtomicTaskUIRegistry
- **Cause** : Custom task not registered, or typo in ID
- **Impact** : No UI metadata available (won't affect execution if runtime handler exists)
- **Fix** : Register the task or correct the ID

#### **W015 — Condition Always True/False**
- **Description** : Branch/While condition is Const vs Const (always evaluates to same value)
- **Cause** : Configuration optimization hint
- **Impact** : One branch of the if/while is dead code
- **Fix** : Consider simplifying by removing the branch, or change to dynamic condition

#### **W016 — Condition Pin Mode with No DataConnection**
- **Description** : Condition operand set to "Pin mode" but no data connection exists for that pin
- **Cause** : Pin reference set but connection not created
- **Impact** : Pin gets default value (0 or false) at runtime
- **Fix** : Create the data connection, OR change the mode

---

### ℹ️ Informational (I-Series)

These issues provide helpful information but don't prevent execution.

#### **I001 — Node Not Reachable from EntryPoint**
- **Description** : Node is not reachable via execution flow from EntryPoint
- **Cause** : Node is disconnected from main execution path
- **Impact** : Node will never execute (dead code)
- **Fix** : Connect the node to the execution flow, OR intentionally leave unreachable (for future expansion)
- **Exception** : Data-pure nodes (GetBBValue, MathOp) are **exempt** — they connect via data pins, not exec flow

---

## Phase 24.3 Updates - Data-Pure Node Support

### Key Changes

The verification tool was updated to **understand data-pure nodes** (GetBBValue, MathOp):

#### **Rule E002 - Dangling Node**
- **Before** : Data-pure nodes flagged as dangling/orphan (ERROR)
- **After** : Data-pure nodes are **exempt** — they're connected via data pins, not exec pins ✅

#### **Rule I001 - Node Reachability**
- **Before** : Data-pure nodes flagged as unreachable (INFO)
- **After** : Data-pure nodes are **exempt** — they don't need exec-flow connectivity ✅

#### **Behavior**
Data-pure nodes (GetBBValue, MathOp) are validated via:
- **Data Pins** : Must be connected via `DataPinConnection` to receive inputs
- **Computation** : Execute during `ResolveDataPins()` phase (before parent node executes)
- **Output** : Results cached and used by parent nodes

---

## How to Use the Verification Tool

### Step 1: Run Verification
1. Open a Visual Script graph in the editor
2. Click the **"Verify"** button in the tab bar
3. Verification runs automatically and displays results in the **"Verification Output"** panel

### Step 2: Read Results
```
Verification Output
├─ Issues found: 7
├─ [ERROR] Graph has 7 error(s)
├─ [WARNING] Graph has X warning(s)
└─ [INFO] Graph has Y info(s)
    ├─ [ERROR] Node #14 ('mHealth'): has no exec connections
    ├─ [INFO] Node #15 ('mFoodPortion'): is not reachable from EntryPoint
    └─ [WARNING] Node #16 ('MathOp'): is a MathOp node with no MathOperator assigned
```

### Step 3: Fix Issues
Click on each issue message to:
- Navigate to the problematic node
- Highlight the issue in red/yellow
- Follow the "Fix" recommendations

### Step 4: Re-verify
After making changes, click **"Verify"** again to confirm all issues are resolved.

---

## Quick Reference - Error Categories

| Category | Count | Impact | Urgency |
|----------|-------|--------|---------|
| **Errors (E)** | 16 | Graph won't execute or will crash | 🔴 HIGH |
| **Warnings (W)** | 5 | Graph executes but with issues | 🟡 MEDIUM |
| **Info (I)** | 1 | Helpful feedback | 🔵 LOW |
| **TOTAL** | **22** | Comprehensive validation | - |

---

## Performance Considerations

- **Verification Time** : O(N + E) where N = nodes, E = connections
- **Typical Graphs** : < 100ms for graphs with < 50 nodes
- **Large Graphs** : May take seconds for very large graphs (1000+ nodes)

**Tip** : Run verification before saving to catch issues early!

---

## Best Practices

### ✅ DO

- Run verification after major graph changes
- Fix all **Errors** before shipping
- Address **Warnings** for code quality
- Pay attention to **Info** messages for optimization hints
- Use EntryPoint as the single entry point (mandatory)
- Keep exec flow linear when possible (fewer cycles = clearer logic)

### ❌ DON'T

- Ignore Error issues (they'll cause runtime failures)
- Create circular exec flows (use Sequence for chaining instead)
- Leave unreachable nodes without intention
- Have dangling (disconnected) nodes in production graphs
- Mix data and exec connections incorrectly

---

## Troubleshooting

### "Node has no exec connections" but node IS connected
**Cause** : Node is connected via data pins, not exec pins
**Solution** : This is normal for data-pure nodes (GetBBValue, MathOp). The error should be suppressed. ✓ FIXED in Phase 24.3

### Graph verifies as valid but crashes at runtime
**Cause** : Verification found no structural errors, but logic is wrong (e.g., bad condition)
**Solution** : Add runtime logging to trace execution flow

### "Unknown Blackboard Key" after adding variable
**Cause** : Variable added to graph but verification cached old schema
**Solution** : Click "Verify" again to refresh

---

## Summary

The VSGraphVerifier is your **comprehensive graph health check** :

✅ Structural integrity (connections, nodes exist)
✅ Type safety (compatible pin types)
✅ Blackboard consistency (variables exist, types match)
✅ Execution flow (cycles, reachability)
✅ Configuration completeness (all required fields set)
✅ **Phase 24.3** : Data-pure node support ✓

Use it **before saving** to catch issues early!
