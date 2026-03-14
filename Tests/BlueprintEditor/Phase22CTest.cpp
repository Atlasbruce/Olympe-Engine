/**
 * @file Phase22CTest.cpp
 * @brief Unit tests for Phase 22-C — Parameter Dropdowns & Registries.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * Tests operate directly on the registry classes and undo/redo commands —
 * no SDL3, ImGui, or ImNodes dependency.
 *
 * Test cases:
 *   === OperatorRegistry (4) ===
 *   1. Test_Math_OperatorList         — GetMathOperators() returns +,-,*,/,%
 *   2. Test_Comparison_OperatorList   — GetComparisonOperators() returns ==,!=,<,<=,>,>=
 *   3. Test_Math_IsValid              — IsValidMathOperator() correct results
 *   4. Test_DisplayName               — GetDisplayName() formats correctly
 *
 *   === AtomicTaskUIRegistry (5) ===
 *   5. Test_BuiltIn_Tasks_Present     — built-in tasks are registered
 *   6. Test_Categories                — GetAllCategories() returns expected categories
 *   7. Test_TasksByCategory           — GetTasksByCategory() filters correctly
 *   8. Test_GetSortedForUI            — GetSortedForUI() is sorted by category+name
 *   9. Test_GetTaskSpec               — GetTaskSpec() returns correct display name
 *
 *   === ConditionRegistry (5) ===
 *  10. Test_BuiltIn_Conditions        — built-in conditions present
 *  11. Test_CompareValue_Params       — CompareValue has Key, Operator, Value params
 *  12. Test_IsSet_Params              — IsSet has one param (Key)
 *  13. Test_InRange_Params            — InRange has Key, Min, Max params
 *  14. Test_RandomChance_Params       — RandomChance has Probability param
 *
 *   === BBVariableRegistry (6) ===
 *  15. Test_LoadFromTemplate_Empty    — empty template → empty registry
 *  16. Test_LoadFromTemplate_Entries  — entries correctly loaded
 *  17. Test_GetVariablesByType        — type filter works
 *  18. Test_GetLocalVars              — scope filter works
 *  19. Test_HasVariable               — HasVariable() returns correct results
 *  20. Test_FormatDisplayLabel        — FormatDisplayLabel() formats correctly
 *
 *   === ParameterBindingType (3) ===
 *  21. Test_NewEnumValues             — new enum values have correct uint8_t values
 *  22. Test_Enum_Distinct             — all enum values are distinct
 *  23. Test_Enum_AtomicTaskID         — AtomicTaskID == 2
 *
 *   === EditParameterCommand (5) ===
 *  24. Test_EditParam_Execute         — Execute() sets parameter binding
 *  25. Test_EditParam_Undo            — Undo() restores old binding
 *  26. Test_EditParam_Redo            — Undo+Redo restores new binding
 *  27. Test_EditParam_Description     — GetDescription() is non-empty
 *  28. Test_EditNodeProps_Command     — EditNodePropertiesCommand works
 *
 *   === ParameterEditorRegistry (4) ===
 *  29. Test_NodeParams_AtomicTask     — AtomicTask has "taskType" param
 *  30. Test_NodeParams_MathOp         — MathOp has "operation" param
 *  31. Test_NodeParams_Branch         — Branch has conditionType + condition params
 *  32. Test_NodeParams_GetBBValue     — GetBBValue has "bbKey" param
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/OperatorRegistry.h"
#include "BlueprintEditor/AtomicTaskUIRegistry.h"
#include "BlueprintEditor/ConditionRegistry.h"
#include "BlueprintEditor/BBVariableRegistry.h"
#include "BlueprintEditor/ParameterEditorRegistry.h"
#include "BlueprintEditor/UndoRedoStack.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <algorithm>

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test infrastructure
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(cond, msg)                                              \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::cout << "  FAIL: " << (msg) << std::endl;                 \
            ++s_failCount;                                                  \
        }                                                                   \
    } while (false)

static void ReportTest(const std::string& name, bool passed)
{
    if (passed)
    {
        std::cout << "[PASS] " << name << std::endl;
        ++s_passCount;
    }
    else
    {
        std::cout << "[FAIL] " << name << std::endl;
        ++s_failCount;
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool VecContains(const std::vector<std::string>& vec, const std::string& val)
{
    for (size_t i = 0; i < vec.size(); ++i)
        if (vec[i] == val) return true;
    return false;
}

// ============================================================================
// OperatorRegistry Tests
// ============================================================================

static void Test_Math_OperatorList()
{
    const std::string name = "Math_OperatorList";
    int prevFail = s_failCount;

    const std::vector<std::string>& ops = OperatorRegistry::GetMathOperators();
    TEST_ASSERT(ops.size() == 5u, "Must have 5 math operators");
    TEST_ASSERT(VecContains(ops, "+"), "'+' must be in math ops");
    TEST_ASSERT(VecContains(ops, "-"), "'-' must be in math ops");
    TEST_ASSERT(VecContains(ops, "*"), "'*' must be in math ops");
    TEST_ASSERT(VecContains(ops, "/"), "'/' must be in math ops");
    TEST_ASSERT(VecContains(ops, "%"), "'%' must be in math ops");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_Comparison_OperatorList()
{
    const std::string name = "Comparison_OperatorList";
    int prevFail = s_failCount;

    const std::vector<std::string>& ops = OperatorRegistry::GetComparisonOperators();
    TEST_ASSERT(ops.size() == 6u, "Must have 6 comparison operators");
    TEST_ASSERT(VecContains(ops, "=="), "'==' must be present");
    TEST_ASSERT(VecContains(ops, "!="), "'!=' must be present");
    TEST_ASSERT(VecContains(ops, "<"),  "'<' must be present");
    TEST_ASSERT(VecContains(ops, "<="), "'<=' must be present");
    TEST_ASSERT(VecContains(ops, ">"),  "'>' must be present");
    TEST_ASSERT(VecContains(ops, ">="), "'>=' must be present");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_Math_IsValid()
{
    const std::string name = "Math_IsValid";
    int prevFail = s_failCount;

    TEST_ASSERT(OperatorRegistry::IsValidMathOperator("+"),  "'+' is valid");
    TEST_ASSERT(OperatorRegistry::IsValidMathOperator("-"),  "'-' is valid");
    TEST_ASSERT(OperatorRegistry::IsValidMathOperator("*"),  "'*' is valid");
    TEST_ASSERT(OperatorRegistry::IsValidMathOperator("/"),  "'/' is valid");
    TEST_ASSERT(OperatorRegistry::IsValidMathOperator("%"),  "'%' is valid");
    TEST_ASSERT(!OperatorRegistry::IsValidMathOperator("^"), "'^' is NOT valid");
    TEST_ASSERT(!OperatorRegistry::IsValidMathOperator(""),  "'' is NOT valid");
    TEST_ASSERT(OperatorRegistry::IsValidComparisonOperator("=="), "'==' is valid cmp");
    TEST_ASSERT(!OperatorRegistry::IsValidComparisonOperator("+"), "'+' is NOT valid cmp");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_DisplayName()
{
    const std::string name = "DisplayName";
    int prevFail = s_failCount;

    TEST_ASSERT(!OperatorRegistry::GetDisplayName("+").empty(),  "'+' display name non-empty");
    TEST_ASSERT(!OperatorRegistry::GetDisplayName("==").empty(), "'==' display name non-empty");
    // Fallback: unknown operator returns itself
    TEST_ASSERT(OperatorRegistry::GetDisplayName("??") == "??", "unknown op returns itself");

    ReportTest(name, s_failCount == prevFail);
}

// ============================================================================
// AtomicTaskUIRegistry Tests
// ============================================================================

static void Test_BuiltIn_Tasks_Present()
{
    const std::string name = "BuiltIn_Tasks_Present";
    int prevFail = s_failCount;

    AtomicTaskUIRegistry& reg = AtomicTaskUIRegistry::Get();

    TEST_ASSERT(reg.GetTaskSpec("move_to_goal")    != nullptr, "move_to_goal registered");
    TEST_ASSERT(reg.GetTaskSpec("attack_if_close") != nullptr, "attack_if_close registered");
    TEST_ASSERT(reg.GetTaskSpec("play_animation")  != nullptr, "play_animation registered");
    TEST_ASSERT(reg.GetTaskSpec("play_sound")      != nullptr, "play_sound registered");
    TEST_ASSERT(reg.GetTaskSpec("log_message")     != nullptr, "log_message registered");
    TEST_ASSERT(reg.GetTaskSpec("nonexistent_xyz") == nullptr, "nonexistent not found");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_Categories()
{
    const std::string name = "Categories";
    int prevFail = s_failCount;

    std::vector<std::string> cats = AtomicTaskUIRegistry::Get().GetAllCategories();
    TEST_ASSERT(VecContains(cats, "Movement"),  "Movement category present");
    TEST_ASSERT(VecContains(cats, "Combat"),    "Combat category present");
    TEST_ASSERT(VecContains(cats, "Animation"), "Animation category present");
    TEST_ASSERT(VecContains(cats, "Audio"),     "Audio category present");
    TEST_ASSERT(VecContains(cats, "Misc"),      "Misc category present");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_TasksByCategory()
{
    const std::string name = "TasksByCategory";
    int prevFail = s_failCount;

    std::vector<TaskSpec> movement = AtomicTaskUIRegistry::Get().GetTasksByCategory("Movement");
    TEST_ASSERT(movement.size() == 3u, "Movement has 3 tasks");

    bool hasMoveToGoal = false;
    for (size_t i = 0; i < movement.size(); ++i)
        if (movement[i].id == "move_to_goal") { hasMoveToGoal = true; break; }
    TEST_ASSERT(hasMoveToGoal, "move_to_goal in Movement category");

    std::vector<TaskSpec> empty = AtomicTaskUIRegistry::Get().GetTasksByCategory("NonExistent");
    TEST_ASSERT(empty.empty(), "NonExistent category returns empty");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetSortedForUI()
{
    const std::string name = "GetSortedForUI";
    int prevFail = s_failCount;

    std::vector<TaskSpec> sorted = AtomicTaskUIRegistry::Get().GetSortedForUI();
    TEST_ASSERT(!sorted.empty(), "GetSortedForUI() returns non-empty list");
    TEST_ASSERT(sorted.size() >= 15u, "At least 15 built-in tasks");

    // Verify sorted by category
    for (size_t i = 1; i < sorted.size(); ++i)
    {
        bool catOk = sorted[i].category >= sorted[i-1].category;
        bool nameOk = (sorted[i].category != sorted[i-1].category) ||
                      (sorted[i].displayName >= sorted[i-1].displayName);
        TEST_ASSERT(catOk,  "Categories must be in ascending order");
        TEST_ASSERT(nameOk, "Names within a category must be in ascending order");
    }

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetTaskSpec()
{
    const std::string name = "GetTaskSpec";
    int prevFail = s_failCount;

    const TaskSpec* spec = AtomicTaskUIRegistry::Get().GetTaskSpec("log_message");
    TEST_ASSERT(spec != nullptr, "log_message spec found");
    TEST_ASSERT(spec->id == "log_message",          "id matches");
    TEST_ASSERT(spec->displayName == "Log Message", "displayName matches");
    TEST_ASSERT(spec->category == "Misc",            "category matches");
    TEST_ASSERT(!spec->description.empty(),          "description non-empty");

    ReportTest(name, s_failCount == prevFail);
}

// ============================================================================
// ConditionRegistry Tests
// ============================================================================

static void Test_BuiltIn_Conditions()
{
    const std::string name = "BuiltIn_Conditions";
    int prevFail = s_failCount;

    ConditionRegistry& reg = ConditionRegistry::Get();

    TEST_ASSERT(reg.GetConditionSpec("CompareValue")  != nullptr, "CompareValue registered");
    TEST_ASSERT(reg.GetConditionSpec("IsSet")         != nullptr, "IsSet registered");
    TEST_ASSERT(reg.GetConditionSpec("IsNotSet")      != nullptr, "IsNotSet registered");
    TEST_ASSERT(reg.GetConditionSpec("InRange")       != nullptr, "InRange registered");
    TEST_ASSERT(reg.GetConditionSpec("RandomChance")  != nullptr, "RandomChance registered");
    TEST_ASSERT(reg.GetConditionSpec("Unknown_XYZ")   == nullptr, "Unknown not found");

    std::vector<std::string> ids = reg.GetAllConditionIds();
    TEST_ASSERT(ids.size() >= 5u, "At least 5 conditions registered");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_CompareValue_Params()
{
    const std::string name = "CompareValue_Params";
    int prevFail = s_failCount;

    const ConditionSpec* spec = ConditionRegistry::Get().GetConditionSpec("CompareValue");
    TEST_ASSERT(spec != nullptr, "CompareValue must exist");
    TEST_ASSERT(spec->parameters.size() == 3u, "CompareValue has 3 params");

    // Check param names
    bool hasKey = false, hasOp = false, hasValue = false;
    for (size_t i = 0; i < spec->parameters.size(); ++i)
    {
        if (spec->parameters[i].name == "Key")      hasKey   = true;
        if (spec->parameters[i].name == "Operator") hasOp    = true;
        if (spec->parameters[i].name == "Value")    hasValue = true;
    }
    TEST_ASSERT(hasKey,   "CompareValue has 'Key' param");
    TEST_ASSERT(hasOp,    "CompareValue has 'Operator' param");
    TEST_ASSERT(hasValue, "CompareValue has 'Value' param");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_IsSet_Params()
{
    const std::string name = "IsSet_Params";
    int prevFail = s_failCount;

    const ConditionSpec* spec = ConditionRegistry::Get().GetConditionSpec("IsSet");
    TEST_ASSERT(spec != nullptr, "IsSet must exist");
    TEST_ASSERT(spec->parameters.size() == 1u, "IsSet has 1 param");
    TEST_ASSERT(spec->parameters[0].name == "Key", "IsSet param is 'Key'");
    TEST_ASSERT(spec->parameters[0].required, "Key is required");
    TEST_ASSERT(spec->parameters[0].bindingType == ParameterBindingType::LocalVariable,
                "Key bindingType is LocalVariable");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_InRange_Params()
{
    const std::string name = "InRange_Params";
    int prevFail = s_failCount;

    const ConditionSpec* spec = ConditionRegistry::Get().GetConditionSpec("InRange");
    TEST_ASSERT(spec != nullptr, "InRange must exist");
    TEST_ASSERT(spec->parameters.size() == 3u, "InRange has 3 params (Key, Min, Max)");

    bool hasKey = false, hasMin = false, hasMax = false;
    for (size_t i = 0; i < spec->parameters.size(); ++i)
    {
        if (spec->parameters[i].name == "Key") hasKey = true;
        if (spec->parameters[i].name == "Min") hasMin = true;
        if (spec->parameters[i].name == "Max") hasMax = true;
    }
    TEST_ASSERT(hasKey, "InRange has 'Key'");
    TEST_ASSERT(hasMin, "InRange has 'Min'");
    TEST_ASSERT(hasMax, "InRange has 'Max'");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_RandomChance_Params()
{
    const std::string name = "RandomChance_Params";
    int prevFail = s_failCount;

    const ConditionSpec* spec = ConditionRegistry::Get().GetConditionSpec("RandomChance");
    TEST_ASSERT(spec != nullptr, "RandomChance must exist");
    TEST_ASSERT(spec->parameters.size() == 1u, "RandomChance has 1 param");
    TEST_ASSERT(spec->parameters[0].name == "Probability", "Param name is 'Probability'");
    TEST_ASSERT(spec->parameters[0].bindingType == ParameterBindingType::Literal,
                "Probability bindingType is Literal");

    ReportTest(name, s_failCount == prevFail);
}

// ============================================================================
// BBVariableRegistry Tests
// ============================================================================

static void Test_LoadFromTemplate_Empty()
{
    const std::string name = "LoadFromTemplate_Empty";
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl;
    BBVariableRegistry reg;
    reg.LoadFromTemplate(tmpl);

    TEST_ASSERT(reg.GetCount() == 0u, "Empty template → empty registry");
    TEST_ASSERT(reg.GetAllVariables().empty(), "GetAllVariables() empty");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_LoadFromTemplate_Entries()
{
    const std::string name = "LoadFromTemplate_Entries";
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl;
    BlackboardEntry e1; e1.Key = "health";    e1.Type = VariableType::Float;  e1.IsGlobal = false;
    BlackboardEntry e2; e2.Key = "target";    e2.Type = VariableType::EntityID; e2.IsGlobal = false;
    BlackboardEntry e3; e3.Key = "globalVar"; e3.Type = VariableType::Int;    e3.IsGlobal = true;
    tmpl.Blackboard.push_back(e1);
    tmpl.Blackboard.push_back(e2);
    tmpl.Blackboard.push_back(e3);

    BBVariableRegistry reg;
    reg.LoadFromTemplate(tmpl);

    TEST_ASSERT(reg.GetCount() == 3u, "Registry has 3 entries");
    TEST_ASSERT(reg.HasVariable("health"),    "health present");
    TEST_ASSERT(reg.HasVariable("target"),    "target present");
    TEST_ASSERT(reg.HasVariable("globalVar"), "globalVar present");
    TEST_ASSERT(!reg.HasVariable("missing"),  "missing not present");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetVariablesByType()
{
    const std::string name = "GetVariablesByType";
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl;
    BlackboardEntry f1; f1.Key = "speed"; f1.Type = VariableType::Float;
    BlackboardEntry f2; f2.Key = "hp";    f2.Type = VariableType::Float;
    BlackboardEntry i1; i1.Key = "count"; i1.Type = VariableType::Int;
    tmpl.Blackboard.push_back(f1);
    tmpl.Blackboard.push_back(f2);
    tmpl.Blackboard.push_back(i1);

    BBVariableRegistry reg;
    reg.LoadFromTemplate(tmpl);

    std::vector<VarSpec> floats = reg.GetVariablesByType(VariableType::Float);
    TEST_ASSERT(floats.size() == 2u, "Two Float variables");

    std::vector<VarSpec> ints = reg.GetVariablesByType(VariableType::Int);
    TEST_ASSERT(ints.size() == 1u, "One Int variable");
    TEST_ASSERT(ints[0].name == "count", "Int variable is 'count'");

    std::vector<VarSpec> bools = reg.GetVariablesByType(VariableType::Bool);
    TEST_ASSERT(bools.empty(), "No Bool variables");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetLocalVars()
{
    const std::string name = "GetLocalVars";
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl;
    BlackboardEntry e1; e1.Key = "local1"; e1.Type = VariableType::Int; e1.IsGlobal = false;
    BlackboardEntry e2; e2.Key = "global1"; e2.Type = VariableType::Int; e2.IsGlobal = true;
    BlackboardEntry e3; e3.Key = "local2"; e3.Type = VariableType::Float; e3.IsGlobal = false;
    tmpl.Blackboard.push_back(e1);
    tmpl.Blackboard.push_back(e2);
    tmpl.Blackboard.push_back(e3);

    BBVariableRegistry reg;
    reg.LoadFromTemplate(tmpl);

    std::vector<VarSpec> locals = reg.GetLocalVariables();
    TEST_ASSERT(locals.size() == 2u, "Two local variables");

    std::vector<VarSpec> globals = reg.GetGlobalVariables();
    TEST_ASSERT(globals.size() == 1u, "One global variable");
    TEST_ASSERT(globals[0].name == "global1", "Global variable is 'global1'");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_HasVariable()
{
    const std::string name = "HasVariable";
    int prevFail = s_failCount;

    TaskGraphTemplate tmpl;
    BlackboardEntry e; e.Key = "myKey"; e.Type = VariableType::Bool;
    tmpl.Blackboard.push_back(e);

    BBVariableRegistry reg;
    reg.LoadFromTemplate(tmpl);

    TEST_ASSERT(reg.HasVariable("myKey"),    "myKey found");
    TEST_ASSERT(!reg.HasVariable("noKey"),   "noKey not found");
    TEST_ASSERT(!reg.HasVariable(""),        "empty string not found");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_FormatDisplayLabel()
{
    const std::string name = "FormatDisplayLabel";
    int prevFail = s_failCount;

    std::string label = BBVariableRegistry::FormatDisplayLabel("health", VariableType::Float, false);
    TEST_ASSERT(!label.empty(), "Label is non-empty");
    // Should contain the variable name and type
    TEST_ASSERT(label.find("health") != std::string::npos, "Label contains variable name");
    TEST_ASSERT(label.find("Float")  != std::string::npos, "Label contains type name");
    TEST_ASSERT(label.find("local")  != std::string::npos, "Label contains scope");

    std::string globalLabel = BBVariableRegistry::FormatDisplayLabel("x", VariableType::Int, true);
    TEST_ASSERT(globalLabel.find("global") != std::string::npos, "Global label contains 'global'");

    ReportTest(name, s_failCount == prevFail);
}

// ============================================================================
// ParameterBindingType enum Tests
// ============================================================================

static void Test_NewEnumValues()
{
    const std::string name = "NewEnumValues";
    int prevFail = s_failCount;

    TEST_ASSERT(static_cast<uint8_t>(ParameterBindingType::Literal)       == 0u, "Literal == 0");
    TEST_ASSERT(static_cast<uint8_t>(ParameterBindingType::LocalVariable)  == 1u, "LocalVariable == 1");
    TEST_ASSERT(static_cast<uint8_t>(ParameterBindingType::AtomicTaskID)   == 2u, "AtomicTaskID == 2");
    TEST_ASSERT(static_cast<uint8_t>(ParameterBindingType::ConditionID)    == 3u, "ConditionID == 3");
    TEST_ASSERT(static_cast<uint8_t>(ParameterBindingType::MathOperator)   == 4u, "MathOperator == 4");
    TEST_ASSERT(static_cast<uint8_t>(ParameterBindingType::ComparisonOp)   == 5u, "ComparisonOp == 5");
    TEST_ASSERT(static_cast<uint8_t>(ParameterBindingType::SubGraphPath)   == 6u, "SubGraphPath == 6");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_Enum_Distinct()
{
    const std::string name = "Enum_Distinct";
    int prevFail = s_failCount;

    uint8_t vals[7] = {
        static_cast<uint8_t>(ParameterBindingType::Literal),
        static_cast<uint8_t>(ParameterBindingType::LocalVariable),
        static_cast<uint8_t>(ParameterBindingType::AtomicTaskID),
        static_cast<uint8_t>(ParameterBindingType::ConditionID),
        static_cast<uint8_t>(ParameterBindingType::MathOperator),
        static_cast<uint8_t>(ParameterBindingType::ComparisonOp),
        static_cast<uint8_t>(ParameterBindingType::SubGraphPath)
    };

    for (int i = 0; i < 7; ++i)
        for (int j = i+1; j < 7; ++j)
            TEST_ASSERT(vals[i] != vals[j], "All enum values must be distinct");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_Enum_AtomicTaskID()
{
    const std::string name = "Enum_AtomicTaskID";
    int prevFail = s_failCount;

    ParameterBindingType t = ParameterBindingType::AtomicTaskID;
    TEST_ASSERT(t != ParameterBindingType::Literal,       "AtomicTaskID != Literal");
    TEST_ASSERT(t != ParameterBindingType::LocalVariable, "AtomicTaskID != LocalVariable");
    TEST_ASSERT(static_cast<uint8_t>(t) == 2u,            "AtomicTaskID value is 2");

    ReportTest(name, s_failCount == prevFail);
}

// ============================================================================
// EditParameterCommand / EditNodePropertiesCommand Tests
// ============================================================================

static TaskNodeDefinition MakeNode(int id, TaskNodeType type, const std::string& name)
{
    TaskNodeDefinition node;
    node.NodeID   = id;
    node.Type     = type;
    node.NodeName = name;
    return node;
}

static void Test_EditParam_Execute()
{
    const std::string name = "EditParam_Execute";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    TaskNodeDefinition node = MakeNode(1, TaskNodeType::AtomicTask, "Task");
    graph.Nodes.push_back(node);
    graph.BuildLookupCache();

    ParameterBinding oldB;
    oldB.Type         = ParameterBindingType::AtomicTaskID;
    oldB.VariableName = "move_to_goal";

    ParameterBinding newB;
    newB.Type         = ParameterBindingType::AtomicTaskID;
    newB.VariableName = "attack_if_close";

    UndoRedoStack stack;
    stack.PushCommand(
        std::unique_ptr<ICommand>(new EditParameterCommand(1, "taskType", oldB, newB)),
        graph);

    const TaskNodeDefinition* n = graph.GetNode(1);
    TEST_ASSERT(n != nullptr, "Node must exist");
    auto it = n->Parameters.find("taskType");
    TEST_ASSERT(it != n->Parameters.end(), "Parameter 'taskType' must be set");
    TEST_ASSERT(it->second.VariableName == "attack_if_close",
                "Parameter value must be 'attack_if_close'");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_EditParam_Undo()
{
    const std::string name = "EditParam_Undo";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    TaskNodeDefinition node = MakeNode(1, TaskNodeType::AtomicTask, "Task");
    ParameterBinding initial;
    initial.Type         = ParameterBindingType::AtomicTaskID;
    initial.VariableName = "move_to_goal";
    node.Parameters["taskType"] = initial;
    graph.Nodes.push_back(node);
    graph.BuildLookupCache();

    ParameterBinding newB;
    newB.Type         = ParameterBindingType::AtomicTaskID;
    newB.VariableName = "attack_if_close";

    UndoRedoStack stack;
    stack.PushCommand(
        std::unique_ptr<ICommand>(new EditParameterCommand(1, "taskType", initial, newB)),
        graph);
    stack.Undo(graph);

    const TaskNodeDefinition* n = graph.GetNode(1);
    TEST_ASSERT(n != nullptr, "Node must exist after undo");
    auto it = n->Parameters.find("taskType");
    TEST_ASSERT(it != n->Parameters.end(), "Parameter must still exist after undo");
    TEST_ASSERT(it->second.VariableName == "move_to_goal",
                "Parameter must be restored to 'move_to_goal' after undo");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_EditParam_Redo()
{
    const std::string name = "EditParam_Redo";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    TaskNodeDefinition node = MakeNode(1, TaskNodeType::AtomicTask, "Task");
    ParameterBinding initial;
    initial.Type         = ParameterBindingType::AtomicTaskID;
    initial.VariableName = "move_to_goal";
    node.Parameters["taskType"] = initial;
    graph.Nodes.push_back(node);
    graph.BuildLookupCache();

    ParameterBinding newB;
    newB.Type         = ParameterBindingType::AtomicTaskID;
    newB.VariableName = "attack_if_close";

    UndoRedoStack stack;
    stack.PushCommand(
        std::unique_ptr<ICommand>(new EditParameterCommand(1, "taskType", initial, newB)),
        graph);
    stack.Undo(graph);
    stack.Redo(graph);

    const TaskNodeDefinition* n = graph.GetNode(1);
    TEST_ASSERT(n != nullptr, "Node must exist after redo");
    auto it = n->Parameters.find("taskType");
    TEST_ASSERT(it != n->Parameters.end(), "Parameter must exist after redo");
    TEST_ASSERT(it->second.VariableName == "attack_if_close",
                "Parameter must be 'attack_if_close' after redo");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_EditParam_Description()
{
    const std::string name = "EditParam_Description";
    int prevFail = s_failCount;

    ParameterBinding a, b;
    a.Type = ParameterBindingType::Literal;
    b.Type = ParameterBindingType::Literal;

    EditParameterCommand cmd(42, "myParam", a, b);
    std::string desc = cmd.GetDescription();

    TEST_ASSERT(!desc.empty(), "GetDescription() must be non-empty");
    TEST_ASSERT(desc.find("myParam") != std::string::npos, "Description mentions param name");
    TEST_ASSERT(desc.find("42") != std::string::npos,       "Description mentions node ID");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_EditNodeProps_Command()
{
    const std::string name = "EditNodeProps_Command";
    int prevFail = s_failCount;

    TaskGraphTemplate graph;
    TaskNodeDefinition node = MakeNode(1, TaskNodeType::Branch, "Branch");
    graph.Nodes.push_back(node);
    graph.BuildLookupCache();

    EditNodePropertiesCommand::ParameterMap oldMap, newMap;

    ParameterBinding condOld;
    condOld.Type         = ParameterBindingType::ConditionID;
    condOld.VariableName = "IsSet";
    oldMap["conditionType"] = condOld;

    ParameterBinding condNew;
    condNew.Type         = ParameterBindingType::ConditionID;
    condNew.VariableName = "CompareValue";
    newMap["conditionType"] = condNew;

    UndoRedoStack stack;
    stack.PushCommand(
        std::unique_ptr<ICommand>(new EditNodePropertiesCommand(1, oldMap, newMap)),
        graph);

    const TaskNodeDefinition* n = graph.GetNode(1);
    TEST_ASSERT(n != nullptr, "Node must exist after execute");
    auto it = n->Parameters.find("conditionType");
    TEST_ASSERT(it != n->Parameters.end(), "conditionType must be set");
    TEST_ASSERT(it->second.VariableName == "CompareValue", "Value is 'CompareValue'");

    // Undo
    stack.Undo(graph);
    n = graph.GetNode(1);
    it = n->Parameters.find("conditionType");
    TEST_ASSERT(it != n->Parameters.end(), "conditionType still present after undo");
    TEST_ASSERT(it->second.VariableName == "IsSet", "Value restored to 'IsSet' after undo");

    // GetDescription
    EditNodePropertiesCommand descCmd(1, oldMap, newMap);
    TEST_ASSERT(!descCmd.GetDescription().empty(), "GetDescription() non-empty");

    ReportTest(name, s_failCount == prevFail);
}

// ============================================================================
// ParameterEditorRegistry Tests
// ============================================================================

static void Test_NodeParams_AtomicTask()
{
    const std::string name = "NodeParams_AtomicTask";
    int prevFail = s_failCount;

    ParameterEditorRegistry& reg = ParameterEditorRegistry::Get();
    const std::vector<ParameterDescriptor>& params = reg.GetNodeParameters(TaskNodeType::AtomicTask);

    TEST_ASSERT(!params.empty(), "AtomicTask has parameter descriptors");

    const ParameterDescriptor* d = reg.GetParameterDescriptor(TaskNodeType::AtomicTask, "taskType");
    TEST_ASSERT(d != nullptr, "AtomicTask has 'taskType' descriptor");
    TEST_ASSERT(d->bindingType == ParameterBindingType::AtomicTaskID,
                "taskType bindingType is AtomicTaskID");
    TEST_ASSERT(d->required, "taskType is required");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_NodeParams_MathOp()
{
    const std::string name = "NodeParams_MathOp";
    int prevFail = s_failCount;

    ParameterEditorRegistry& reg = ParameterEditorRegistry::Get();
    const ParameterDescriptor* d = reg.GetParameterDescriptor(TaskNodeType::MathOp, "operation");

    TEST_ASSERT(d != nullptr, "MathOp has 'operation' descriptor");
    TEST_ASSERT(d->bindingType == ParameterBindingType::MathOperator,
                "operation bindingType is MathOperator");
    TEST_ASSERT(d->required, "operation is required");

    // Missing parameter returns nullptr
    const ParameterDescriptor* missing = reg.GetParameterDescriptor(TaskNodeType::MathOp, "noSuchParam");
    TEST_ASSERT(missing == nullptr, "noSuchParam returns nullptr");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_NodeParams_Branch()
{
    const std::string name = "NodeParams_Branch";
    int prevFail = s_failCount;

    ParameterEditorRegistry& reg = ParameterEditorRegistry::Get();
    const std::vector<ParameterDescriptor>& params = reg.GetNodeParameters(TaskNodeType::Branch);

    TEST_ASSERT(!params.empty(), "Branch has parameter descriptors");

    const ParameterDescriptor* cond = reg.GetParameterDescriptor(TaskNodeType::Branch, "conditionType");
    TEST_ASSERT(cond != nullptr, "Branch has 'conditionType' descriptor");
    TEST_ASSERT(cond->bindingType == ParameterBindingType::ConditionID,
                "conditionType bindingType is ConditionID");
    TEST_ASSERT(cond->required, "conditionType is required");

    // While shares the same descriptors as Branch
    const std::vector<ParameterDescriptor>& whileParams = reg.GetNodeParameters(TaskNodeType::While);
    TEST_ASSERT(!whileParams.empty(), "While also has parameter descriptors");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_NodeParams_GetBBValue()
{
    const std::string name = "NodeParams_GetBBValue";
    int prevFail = s_failCount;

    ParameterEditorRegistry& reg = ParameterEditorRegistry::Get();

    const ParameterDescriptor* d = reg.GetParameterDescriptor(TaskNodeType::GetBBValue, "bbKey");
    TEST_ASSERT(d != nullptr, "GetBBValue has 'bbKey' descriptor");
    TEST_ASSERT(d->bindingType == ParameterBindingType::LocalVariable,
                "bbKey bindingType is LocalVariable");

    // Nodes with no registry entry return empty
    const std::vector<ParameterDescriptor>& empty = reg.GetNodeParameters(TaskNodeType::EntryPoint);
    TEST_ASSERT(empty.empty(), "EntryPoint has no parameter descriptors");

    ReportTest(name, s_failCount == prevFail);
}

// ============================================================================
// main
// ============================================================================

int main()
{
    std::cout << "=== Phase 22-C Tests — Parameter Dropdowns & Registries ===" << std::endl;

    // OperatorRegistry
    Test_Math_OperatorList();
    Test_Comparison_OperatorList();
    Test_Math_IsValid();
    Test_DisplayName();

    // AtomicTaskUIRegistry
    Test_BuiltIn_Tasks_Present();
    Test_Categories();
    Test_TasksByCategory();
    Test_GetSortedForUI();
    Test_GetTaskSpec();

    // ConditionRegistry
    Test_BuiltIn_Conditions();
    Test_CompareValue_Params();
    Test_IsSet_Params();
    Test_InRange_Params();
    Test_RandomChance_Params();

    // BBVariableRegistry
    Test_LoadFromTemplate_Empty();
    Test_LoadFromTemplate_Entries();
    Test_GetVariablesByType();
    Test_GetLocalVars();
    Test_HasVariable();
    Test_FormatDisplayLabel();

    // ParameterBindingType enum
    Test_NewEnumValues();
    Test_Enum_Distinct();
    Test_Enum_AtomicTaskID();

    // EditParameterCommand / EditNodePropertiesCommand
    Test_EditParam_Execute();
    Test_EditParam_Undo();
    Test_EditParam_Redo();
    Test_EditParam_Description();
    Test_EditNodeProps_Command();

    // ParameterEditorRegistry
    Test_NodeParams_AtomicTask();
    Test_NodeParams_MathOp();
    Test_NodeParams_Branch();
    Test_NodeParams_GetBBValue();

    std::cout << "\n=== Results: "
              << s_passCount << " passed, "
              << s_failCount << " failed ===" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
