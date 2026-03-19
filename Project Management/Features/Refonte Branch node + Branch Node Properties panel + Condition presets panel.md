REFONTE Node Branch et Editeur panel pour condition preset

## 🔍 **Nouvelle Compréhension Complète**

### **Q1: Deux pins séparés (Left + Right)**

```
Condition: [mSpeed] == [Pin:1]
├─ Left operand: [mSpeed] = Variable → NO PIN
└─ Right operand: [Pin:1] = Pin → PIN NEEDED

Condition: [Pin:1] <= [Pin:2]
├─ Left operand: [Pin:1] = Pin → PIN #1
└─ Right operand: [Pin:2] = Pin → PIN #2

Condition: [mHealth] == [Pin:1]
├─ Left operand: [mHealth] = Variable → NO PIN
└─ Right operand: [Pin:1] = Pin → PIN #2 (right operand pin)
```

**Donc:** Créer un pin POUR CHAQUE operand qui est de type Pin (left OU right, pas ensemble).

---

### **Q2: ID Global Unique par Pin Instancié**

```
Node: "Is Health Critical?"
├─ Condition #1: [mHealth] <= [2]
│  ├─ Left: Variable → NO PIN
│  └─ Right: Const → NO PIN
│
├─ Condition #3: [mSpeed] <= [100.00]
│  ├─ Left: Variable → NO PIN
│  └─ Right: Const → NO PIN
│
└─ Condition #5: [mSpeed] == [Pin:1]
   ├─ Left: Variable → NO PIN
   └─ Right: Pin → CREATE PIN with UNIQUE ID!
      ├─ ID: "pin_inst_abc123def456"  ← GLOBAL UNIQUE UUID
      ├─ Label: "In #5R: [mSpeed] == [Pin:1]"  ← #5 = cond index, R = Right
      └─ Data type: float
```

**Convention de labeling:** `In #ConditionIndex(L|R): [condition preview]`
- `#5L` = Condition index 5, Left operand
- `#5R` = Condition index 5, Right operand

---

### **Q3: Pin reçoit un Float**

```
Node: "Is Health Critical?"
├─ Condition #5: [mSpeed] == [Pin:1]
   └─ Pin: "In #5R: [mSpeed] == [Pin:1]"
      ├─ Receives: float value
      ├─ Example source: GetBBValue node output (float)
      ├─ Example source: Arithmetic node output (float sum)
      └─ Example source: Any other float output pin

At runtime:
  externalPinData = 150.0  (from connected source)
  leftValue = blackboard.mSpeed
  rightValue = externalPinData (150.0)
  result = (leftValue == rightValue)
```

---

### **Q4: Deux sous-pins avec nom de condition**

```
Condition: [Pin:1] <= [Pin:2]

Créer deux pins:
├─ Pin #1 (Left operand):
│  ├─ ID: "pin_inst_xyz789abc"
│  ├─ Label: "In #5L: [Pin:1] <= [Pin:2]"  ← Full condition name
│  └─ Receives: float (value of Pin:1)
│
└─ Pin #2 (Right operand):
   ├─ ID: "pin_inst_uvw456def"
   ├─ Label: "In #5R: [Pin:1] <= [Pin:2]"  ← Same full condition name
   └─ Receives: float (value of Pin:2)

At runtime:
  leftValue = externalPinData_L (from left pin)
  rightValue = externalPinData_R (from right pin)
  result = (leftValue <= rightValue)
```

**Important:** Both pins show the FULL condition, not just their part.

---

## 🏗️ **Architecture Finale (v4)**

### **1. Operand Structure**

```cpp
enum class OperandMode {
    Variable,   // References blackboard
    Const,      // Literal value
    Pin         // External data input
};

struct Operand {
    OperandMode mode;
    std::variant<std::string, double, std::string> value;
    // If Variable: value = variableID (string)
    // If Const: value = constValue (double)
    // If Pin: value = pinLabel (string, e.g., "Pin:1")
    
    double Evaluate(Blackboard& bb, void* externalPinData_left, void* externalPinData_right) const {
        if (mode == OperandMode::Variable) {
            return bb.GetVariable(std::get<std::string>(value)).value;
        } else if (mode == OperandMode::Const) {
            return std::get<double>(value);
        } else if (mode == OperandMode::Pin) {
            // Which pin data to use? Determined by caller context (left vs right)
            return externalPinData_left != nullptr ? *(float*)externalPinData_left 
                                                   : *(float*)externalPinData_right;
        }
        return 0.0;
    }
};
```

### **2. Condition Preset (unchanged)**

```cpp
struct ConditionPreset {
    std::string id;           // Preset ID (e.g., "preset_001")
    std::string name;         // Display name (e.g., "Condition #5")
    Operand left;
    ComparisonOp op;
    Operand right;
    
    std::string GetPreview() const {
        // Returns: "[mSpeed] == [Pin:1]"
    }
};
```

### **3. Dynamic Data Pin (NEW & REVISED)**

```cpp
enum class OperandPosition {
    Left,
    Right
};

struct DynamicDataPin {
    std::string id;                    // GLOBAL UNIQUE UUID (e.g., "pin_inst_abc123")
    int conditionIndex;                // Index in node.conditions (0-based)
    OperandPosition position;          // Left or Right operand
    std::string label;                 // "In #5L: [mSpeed] == [Pin:1]"
    
    // Pin properties for graph rendering
    std::string nodePinID;             // Link to actual ImGui pin
    float dataValue;                   // Runtime: received data
    
    std::string GetDisplayLabel() const {
        std::string posChar = (position == OperandPosition::Left) ? "L" : "R";
        return "In #" + std::to_string(conditionIndex + 1) + posChar + ": [condition_preview]";
    }
};
```

### **4. Node Condition Reference**

```cpp
struct NodeConditionRef {
    std::string presetID;
    LogicalOp logicalOp;           // And, Or
    std::string leftPinID;          // If left operand is Pin: pin instance ID
    std::string rightPinID;         // If right operand is Pin: pin instance ID
};
```

### **5. Node Branch (REVISED)**

```cpp
struct NodeBranch : public Node {
    std::string name;
    std::vector<NodeConditionRef> conditions;
    bool breakpoint;
    
    // All dynamic pins for this node
    std::vector<DynamicDataPin> dynamicPins;  // May have multiple pins per condition
    
    // ============ Methods ============
    
    void UpdateDynamicPins(ConditionPresetRegistry& registry) {
        // For each condition:
        // 1. Check left operand → if Pin, create/update left pin
        // 2. Check right operand → if Pin, create/update right pin
        // 3. If removed, delete pins
    }
    
    std::pair<bool, bool> GetPinNeeds(
        int conditionIndex,
        ConditionPresetRegistry& registry) const {
        // Returns {needsLeftPin, needsRightPin}
    }
    
    std::string GetConditionPreview(
        int conditionIndex,
        ConditionPresetRegistry& registry) const {
        // "[mSpeed] == [Pin:1]"
    }
};
```

---

## 🔧 **Dynamic Pin Generation Logic (FINAL)**

```cpp
void NodeBranch::UpdateDynamicPins(ConditionPresetRegistry& registry) {
    std::vector<DynamicDataPin> newPins;
    
    // Step 1: Process each condition
    for (int condIndex = 0; condIndex < conditions.size(); condIndex++) {
        const NodeConditionRef& condRef = conditions[condIndex];
        const ConditionPreset* preset = registry.GetPreset(condRef.presetID);
        
        if (!preset) continue;
        
        std::string conditionPreview = GetConditionPreview(condIndex, registry);
        
        // Check LEFT operand
        if (preset->left.mode == OperandMode::Pin) {
            // Need a left pin
            DynamicDataPin pin;
            pin.id = GenerateUniqueID();  // e.g., "pin_inst_abc123"
            pin.conditionIndex = condIndex;
            pin.position = OperandPosition::Left;
            pin.label = "In #" + std::to_string(condIndex + 1) + "L: " + conditionPreview;
            
            // Check if this pin already exists (update) or new (create)
            auto existing = FindPinByConditionAndPosition(condIndex, OperandPosition::Left);
            if (existing) {
                pin.id = existing->id;  // Keep same ID, update data
                pin.nodePinID = existing->nodePinID;
                RemoveOldPin(existing);  // Will re-add below
            } else {
                pin.nodePinID = AddInputPin(pin.label, PinType::Float);
            }
            
            newPins.push_back(pin);
            condRef.leftPinID = pin.id;
        }
        
        // Check RIGHT operand
        if (preset->right.mode == OperandMode::Pin) {
            // Need a right pin
            DynamicDataPin pin;
            pin.id = GenerateUniqueID();
            pin.conditionIndex = condIndex;
            pin.position = OperandPosition::Right;
            pin.label = "In #" + std::to_string(condIndex + 1) + "R: " + conditionPreview;
            
            auto existing = FindPinByConditionAndPosition(condIndex, OperandPosition::Right);
            if (existing) {
                pin.id = existing->id;
                pin.nodePinID = existing->nodePinID;
                RemoveOldPin(existing);
            } else {
                pin.nodePinID = AddInputPin(pin.label, PinType::Float);
            }
            
            newPins.push_back(pin);
            condRef.rightPinID = pin.id;
        }
    }
    
    // Step 2: Remove pins that are no longer needed
    for (const auto& oldPin : dynamicPins) {
        auto found = std::find_if(newPins.begin(), newPins.end(),
            [&oldPin](const DynamicDataPin& p) { return p.id == oldPin.id; });
        
        if (found == newPins.end()) {
            // Pin no longer needed
            RemoveInputPin(oldPin.nodePinID);
        }
    }
    
    // Step 3: Replace with new pins
    dynamicPins = newPins;
}

std::pair<bool, bool> NodeBranch::GetPinNeeds(
    int conditionIndex,
    ConditionPresetRegistry& registry) const {
    
    if (conditionIndex >= conditions.size()) return {false, false};
    
    const ConditionPreset* preset = registry.GetPreset(conditions[conditionIndex].presetID);
    if (!preset) return {false, false};
    
    bool needsLeft = (preset->left.mode == OperandMode::Pin);
    bool needsRight = (preset->right.mode == OperandMode::Pin);
    
    return {needsLeft, needsRight};
}
```

---

## 📊 **Pin Mapping Example**

### **Scenario: Multiple Conditions**

```
Node: "Is Health Critical?"

Condition #1: [mHealth] <= [2]
├─ Left: Variable (mHealth) → NO PIN
└─ Right: Const (2) → NO PIN

Condition #2: [mSpeed] <= [100.00]
├─ Left: Variable (mSpeed) → NO PIN
└─ Right: Const (100) → NO PIN

Condition #3: [mSpeed] == [Pin:1]
├─ Left: Variable (mSpeed) → NO PIN
└─ Right: Pin (Pin:1) → PIN CREATED
   ├─ ID: "pin_inst_1"
   ├─ Label: "In #3R: [mSpeed] == [Pin:1]"
   └─ Receives: float

Condition #4: [Pin:1] <= [Pin:2]
├─ Left: Pin (Pin:1) → PIN CREATED
│  ├─ ID: "pin_inst_2"
│  ├─ Label: "In #4L: [Pin:1] <= [Pin:2]"
│  └─ Receives: float
└─ Right: Pin (Pin:2) → PIN CREATED
   ├─ ID: "pin_inst_3"
   ├─ Label: "In #4R: [Pin:1] <= [Pin:2]"
   └─ Receives: float

RENDERED NODE:
╔════════════════════════════════════════╗
║  Is Health Critical?                   ║
╠════════════════════════════════════════╣
║ ➤ In                        Then ➤     ║
║                              Else ➤    ║
║                                        ║
║ [mHealth] <= [2]                       ║
║ And [mSpeed] <= [100.00]               ║
║ Or [mSpeed] == [Pin : 1]               ║
║ And [Pin:1] <= [Pin:2]                 ║
║                                        ║
║ 🟡 In #3R: [mSpeed] == [Pin : 1]       ║
║ 🟡 In #4L: [Pin:1] <= [Pin:2]          ║
║ 🟡 In #4R: [Pin:1] <= [Pin:2]          ║
╚════════════════════════════════════════╝
```

---

## 🎨 **UI Rendering**

```cpp
class NodeBranchRenderer {
    void Render(NodeBranch& node, ConditionPresetRegistry& registry) {
        // Section 1: Title
        RenderTitleBar(node.name);
        
        // Section 2: Exec pins
        ImGui::Text("➤ In");
        ImGui::SameLine(GetNodeWidth() - 100);
        ImGui::Text("Then ➤");
        ImGui::Text("Else ➤");
        
        ImGui::Separator();
        
        // Section 3: Conditions preview
        for (int i = 0; i < node.conditions.size(); i++) {
            if (i > 0) {
                ImGui::TextColored(
                    ImVec4(0, 1, 0, 1),
                    "%s",
                    LogicalOpToString(node.conditions[i].logicalOp).c_str()
                );
                ImGui::SameLine();
            }
            
            ImGui::TextColored(
                ImVec4(0, 1, 0, 1),
                "%s",
                node.GetConditionPreview(i, registry).c_str()
            );
        }
        
        ImGui::Separator();
        
        // Section 4: Dynamic data pins
        for (const auto& pin : node.dynamicPins) {
            ImGui::TextColored(
                ImVec4(1, 1, 0, 1),
                "🟡 %s",
                pin.label.c_str()
            );
            // This is connectable as a pin in the graph
        }
    }
};
```

---

## ⚡ **Runtime Evaluation**

```cpp
bool EvaluateNodeConditions(
    NodeBranch& node,
    ConditionPresetRegistry& registry,
    Blackboard& blackboard,
    std::map<std::string, float>& pinDataMap) {  // pinID → float value
    
    bool result = false;
    bool firstCondition = true;
    
    for (const auto& condRef : node.conditions) {
        const ConditionPreset* preset = registry.GetPreset(condRef.presetID);
        if (!preset) continue;
        
        // Evaluate left operand
        double leftValue;
        if (preset->left.mode == OperandMode::Variable) {
            leftValue = blackboard.GetVariable(std::get<std::string>(preset->left.value)).value;
        } else if (preset->left.mode == OperandMode::Const) {
            leftValue = std::get<double>(preset->left.value);
        } else if (preset->left.mode == OperandMode::Pin) {
            // Get from dynamic pin data
            leftValue = pinDataMap[condRef.leftPinID];
        }
        
        // Evaluate right operand
        double rightValue;
        if (preset->right.mode == OperandMode::Variable) {
            rightValue = blackboard.GetVariable(std::get<std::string>(preset->right.value)).value;
        } else if (preset->right.mode == OperandMode::Const) {
            rightValue = std::get<double>(preset->right.value);
        } else if (preset->right.mode == OperandMode::Pin) {
            // Get from dynamic pin data
            rightValue = pinDataMap[condRef.rightPinID];
        }
        
        // Evaluate comparison
        bool conditionResult = EvaluateComparison(leftValue, preset->op, rightValue);
        
        // Apply logical operator
        if (firstCondition) {
            result = conditionResult;
            firstCondition = false;
        } else {
            if (condRef.logicalOp == LogicalOp::And) {
                result = result && conditionResult;
            } else if (condRef.logicalOp == LogicalOp::Or) {
                result = result || conditionResult;
            }
        }
    }
    
    return result;
}
```

---

## 🧪 **Testing (FINAL)**

```cpp
TEST(DynamicPins, CreatesLeftAndRightPinsSeparately) {
    NodeBranch node;
    ConditionPresetRegistry registry;
    
    // Condition: [Pin:1] <= [Pin:2]
    auto preset = CreatePreset("preset", 
        {OperandMode::Pin, "Pin:1"},
        ComparisonOp::Less,
        {OperandMode::Pin, "Pin:2"}
    );
    registry.CreatePreset(preset);
    
    node.conditions.push_back({preset.id});
    node.UpdateDynamicPins(registry);
    
    // Should create 2 pins
    ASSERT_EQ(node.dynamicPins.size(), 2);
    
    // Pin 1: Left
    ASSERT_EQ(node.dynamicPins[0].position, OperandPosition::Left);
    ASSERT_EQ(node.dynamicPins[0].label, "In #1L: [Pin:1] <= [Pin:2]");
    
    // Pin 2: Right
    ASSERT_EQ(node.dynamicPins[1].position, OperandPosition::Right);
    ASSERT_EQ(node.dynamicPins[1].label, "In #1R: [Pin:1] <= [Pin:2]");
    
    // Both have unique IDs
    ASSERT_NE(node.dynamicPins[0].id, node.dynamicPins[1].id);
}

TEST(DynamicPins, MixedOperands) {
    NodeBranch node;
    ConditionPresetRegistry registry;
    
    // Condition: [mHealth] == [Pin:1]
    auto preset = CreatePreset("preset",
        {OperandMode::Variable, "mHealth"},
        ComparisonOp::Equal,
        {OperandMode::Pin, "Pin:1"}
    );
    registry.CreatePreset(preset);
    
    node.conditions.push_back({preset.id});
    node.UpdateDynamicPins(registry);
    
    // Only 1 pin (right operand)
    ASSERT_EQ(node.dynamicPins.size(), 1);
    ASSERT_EQ(node.dynamicPins[0].position, OperandPosition::Right);
    ASSERT_EQ(node.dynamicPins[0].label, "In #1R: [mHealth] == [Pin:1]");
}

TEST(Evaluation, ReceivesPinDataCorrectly) {
    NodeBranch node;
    ConditionPresetRegistry registry;
    Blackboard blackboard;
    
    // Setup
    auto preset = CreatePreset("preset",
        {OperandMode::Pin, "Pin:1"},
        ComparisonOp::Less,
        {OperandMode::Pin, "Pin:2"}
    );
    registry.CreatePreset(preset);
    
    node.conditions.push_back({preset.id});
    node.UpdateDynamicPins(registry);
    
    // Pin data mapping
    std::map<std::string, float> pinData;
    pinData[node.dynamicPins[0].id] = 50.0f;   // Left pin
    pinData[node.dynamicPins[1].id] = 100.0f;  // Right pin
    
    // Evaluate
    bool result = EvaluateNodeConditions(node, registry, blackboard, pinData);
    
    // 50.0 < 100.0 = true
    ASSERT_TRUE(result);
}

TEST(DynamicPins, MultiConditionWithMultiplePins) {
    NodeBranch node;
    ConditionPresetRegistry registry;
    
    // Condition 1: [mHealth] <= [2] → 0 pins
    auto preset1 = CreatePreset("preset1",
        {OperandMode::Variable, "mHealth"},
        ComparisonOp::LessEqual,
        {OperandMode::Const, 2.0}
    );
    
    // Condition 2: [Pin:1] <= [Pin:2] → 2 pins
    auto preset2 = CreatePreset("preset2",
        {OperandMode::Pin, "Pin:1"},
        ComparisonOp::LessEqual,
        {OperandMode::Pin, "Pin:2"}
    );
    
    registry.CreatePreset(preset1);
    registry.CreatePreset(preset2);
    
    node.conditions.push_back({preset1.id, LogicalOp::START});
    node.conditions.push_back({preset2.id, LogicalOp::And});
    
    node.UpdateDynamicPins(registry);
    
    // Total: 2 pins (from condition 2)
    ASSERT_EQ(node.dynamicPins.size(), 2);
    
    // Both pins reference condition index 1 (2nd condition)
    ASSERT_EQ(node.dynamicPins[0].conditionIndex, 1);
    ASSERT_EQ(node.dynamicPins[1].conditionIndex, 1);
    
    // Labels include condition index +1
    ASSERT_EQ(node.dynamicPins[0].label, "In #2L: [Pin:1] <= [Pin:2]");
    ASSERT_EQ(node.dynamicPins[1].label, "In #2R: [Pin:1] <= [Pin:2]");
}
```

---

## 📋 **Files à Modifier/Créer (FINAL)**

### **NEW FILES**

```cpp
// Source/Editor/ConditionPreset/Operand.h
struct Operand { /* ... */ };

// Source/Editor/ConditionPreset/ConditionPreset.h/cpp
struct ConditionPreset { /* ... */ };

// Source/Editor/ConditionPreset/DynamicDataPin.h
enum class OperandPosition { Left, Right };
struct DynamicDataPin { /* ... */ };

// Source/Editor/ConditionPreset/ConditionPresetRegistry.h/cpp
class ConditionPresetRegistry { /* CRUD + serialization */ };

// Source/Editor/Panels/ConditionPresetLibraryPanel.h/cpp
class ConditionPresetLibraryPanel { /* Global preset UI */ };

// Source/Editor/Panels/NodeConditionsPanel.h/cpp
class NodeConditionsPanel { /* Node properties UI */ };

// Source/Editor/Nodes/NodeBranchRenderer.h/cpp
class NodeBranchRenderer { /* 4-section rendering */ };

// Source/Engine/Utils/PinIDGenerator.h/cpp
class PinIDGenerator { /* Generate unique pin IDs */ };
```

### **MODIFIED FILES**

```cpp
// NodeBranch.h - COMPLETE REWRITE
// - Replace Condition vector with NodeConditionRef vector
// - Add DynamicDataPin vector
// - Add UpdateDynamicPins()
// - Add GetPinNeeds()
// - Add GetConditionPreview()

// VisualScriptEditorPanel.cpp
// - Load registry on startup
// - Add ConditionPresetLibraryPanel to UI
// - Pass registry to all components

// VisualScriptEvaluator.cpp
// - Use EvaluateNodeConditions()
// - Map pin data by ID

// PropertyPanel.cpp
// - Use NodeConditionsPanel

// Serialization (VisualScriptData.cpp)
// - Serialize/deserialize presets to condition_presets.json
// - Store pin IDs with conditions
```

---

## 📚 **Documentation (Final Update)**

```markdown
## Phase 24: Condition Preset System

### Key Concept: Per-Operand Pins

Each operand that references a Pin generates a separate data input pin:
- Condition [mSpeed] == [Pin:1] → 1 pin (right operand)
- Condition [Pin:1] <= [Pin:2] → 2 pins (left + right operands)

### Pin Naming Convention

`In #ConditionIndex(L|R): [condition_preview]`

Example:
- `In #3R: [mSpeed] == [Pin:1]` (3rd condition, right operand)
- `In #4L: [Pin:1] <= [Pin:2]` (4th condition, left operand)
- `In #4R: [Pin:1] <= [Pin:2]` (4th condition, right operand)

### Data Flow

1. User connects float output to `In #3R`
2. At runtime, value received into pin_inst_abc123
3. EvaluateNodeConditions retrieves pinData[pin_inst_abc123]
4. Uses as right operand value for condition evaluation

### Pin ID System

- Each pin instance gets a global unique ID (UUID)
- Persisted with the node definition
- Used to map incoming data at runtime
- Survives through undo/redo and save/load
```

---

## ✅ **Summary Table**

| Aspect | Details |
|--------|---------|
| **Pins per operand** | 1 pin per Pin operand (left OR right) |
| **Pin identification** | Global unique UUID per pin instance |
| **Pin naming** | `In #CondIndex(L\|R): [condition_preview]` |
| **Data type** | float (for each pin independently) |
| **Multi-pin conditions** | Create 2 separate pins with same condition name |
| **Rendering** | List all dynamic pins in section 4 of node |
| **Deduplication** | NONE (each operand = separate pin) |
| **Runtime** | Map pin ID → received float value |

---