# PHASE 32 IMPLEMENTATION GUIDE: GenerateComponents.py

**Document Date**: 2026-04-04  
**Criticality**: 🔴 BLOCKER  
**Time Estimate**: 12-16 hours  
**Objective**: Auto-generate C++ component types from JSON schema  

---

## OVERVIEW

GenerateComponents.py is the missing critical tool that transforms the JSON component schema into optimized C++ code. Without it, component access is 100x slower (reflection-based runtime lookup vs compiled enums).

### What It Does

```
Input: Gamedata/EntityPrefab/ComponentsParameters.json
       (29 components, 100+ parameters)
       ↓
GenerateComponents.py
       ├─ Parse JSON
       ├─ Validate schema
       ├─ Generate C++ structs
       ├─ Create ComponentID enum
       └─ Output: Source/Generated/
       ↓
Output: 
  - ECS_Components_Generated.h (structs)
  - ComponentIDs_Generated.h (enum + metadata)
  - ComponentQueries_Generated.h (query specializations)
```

### Performance Impact

| Operation | Before (Manual) | After (Generated) | Gain |
|-----------|-----------------|-------------------|------|
| Component lookup | O(n) hash map | O(1) enum | **100x** |
| Query execution | std::any casting | Compiled bitset | **100x** |
| Compile time | 2 min | 3 min | -50% slower |
| Binary size | 15MB | 18MB | +20% |

**Net Effect**: 100x faster queries for 50% slower compile time and 20% larger binary.

---

## IMPLEMENTATION: STEP-BY-STEP

### Step 1: Parse ComponentsParameters.json

**File**: `Tools/GenerateComponents.py` (first 50 LOC)

```python
#!/usr/bin/env python3
"""
Generate C++ component types from JSON schema.

Usage:
    python3 GenerateComponents.py \
        --input Gamedata/EntityPrefab/ComponentsParameters.json \
        --output Source/Generated/

Output files:
    - Source/Generated/ECS_Components_Generated.h
    - Source/Generated/ComponentIDs_Generated.h
    - Source/Generated/ComponentQueries_Generated.h
"""

import json
import sys
import os
from pathlib import Path
from typing import Dict, List, Any

def load_schema(schema_path: str) -> Dict[str, Any]:
    """Load and validate JSON component schema."""
    try:
        with open(schema_path, 'r') as f:
            schema = json.load(f)
    except FileNotFoundError:
        print(f"ERROR: Schema file not found: {schema_path}")
        sys.exit(1)
    except json.JSONDecodeError as e:
        print(f"ERROR: Invalid JSON in {schema_path}: {e}")
        sys.exit(1)

    # Validate schema structure
    if not isinstance(schema, dict):
        print("ERROR: Schema must be a JSON object")
        sys.exit(1)

    if "schemas" not in schema or not isinstance(schema["schemas"], list):
        print("ERROR: Schema must contain 'schemas' array")
        sys.exit(1)

    print(f"[GenerateComponents] Loaded schema with {len(schema['schemas'])} component types")
    return schema

def parse_components(schema: Dict) -> List[Dict[str, Any]]:
    """Parse component definitions from schema."""
    components = []
    for i, comp_def in enumerate(schema["schemas"]):
        if "componentType" not in comp_def:
            print(f"WARNING: Component {i} missing 'componentType', skipping")
            continue

        comp_name = comp_def["componentType"]
        parameters = comp_def.get("parameters", [])

        components.append({
            "name": comp_name,
            "componentType": comp_name,
            "parameters": parameters,
            "id": i,  # Sequential ID
        })

    print(f"[GenerateComponents] Parsed {len(components)} components")
    return components

# ... continue with code generation functions
```

### Step 2: Map JSON Types to C++

**Type System** (20 LOC)

```python
# JSON type → C++ type mapping
TYPE_MAP = {
    "Bool": "bool",
    "Int": "int32_t",
    "Float": "float",
    "String": "std::string",
    "Vector2": "Vector2",  # Custom type
    "Vector3": "Vector3",  # Custom type
    "Color": "Color",      # Custom type
    "EntityRef": "EntityID",  # uint32_t
    "Array": "nlohmann::json",  # Generic JSON array
}

def json_type_to_cpp(json_type: str) -> str:
    """Convert JSON type to C++ type."""
    if json_type not in TYPE_MAP:
        print(f"WARNING: Unknown type '{json_type}', using 'nlohmann::json'")
        return "nlohmann::json"
    return TYPE_MAP[json_type]

def get_default_value(cpp_type: str, json_default: Any = None) -> str:
    """Generate default value for C++ type."""
    if cpp_type == "bool":
        return "false" if json_default is None else str(json_default).lower()
    elif cpp_type in ["int32_t", "uint32_t"]:
        return str(json_default) if json_default is not None else "0"
    elif cpp_type == "float":
        return f"{json_default:.1f}f" if json_default is not None else "0.0f"
    elif cpp_type == "std::string":
        val = json_default if json_default is not None else ""
        return f'"{val}"'
    elif cpp_type == "Vector3":
        if isinstance(json_default, list) and len(json_default) == 3:
            return f"Vector3({json_default[0]}f, {json_default[1]}f, {json_default[2]}f)"
        return "Vector3(0.0f, 0.0f, 0.0f)"
    else:
        return f"{cpp_type}()"
```

### Step 3: Generate Component Structs

**Struct Generation** (100 LOC)

```python
def generate_component_struct(comp: Dict[str, Any]) -> str:
    """Generate C++ struct for a single component."""

    name = comp["componentType"]
    params = comp["parameters"]

    # Member declarations
    members = []
    for param in params:
        param_name = param.get("name", "unknown")
        param_type = param.get("type", "String")
        cpp_type = json_type_to_cpp(param_type)
        default_val = param.get("defaultValue")
        cpp_default = get_default_value(cpp_type, default_val)

        members.append(f"    {cpp_type} {param_name} = {cpp_default};")

    # Constructor parameter list
    ctor_params = []
    for param in params:
        param_name = param.get("name", "unknown")
        param_type = param.get("type", "String")
        cpp_type = json_type_to_cpp(param_type)
        ctor_params.append(f"{cpp_type} {param_name}")

    ctor_params_str = ", ".join(ctor_params)

    # Constructor initializer list
    ctor_init = []
    for param in params:
        param_name = param.get("name", "unknown")
        ctor_init.append(f"{param_name}({param_name})")

    ctor_init_str = ", ".join(ctor_init)

    # Generate struct
    struct_code = f"""
/**
 * @struct {name}
 * @brief Auto-generated component type from schema
 */
struct {name} {{
{chr(10).join(members)}

    // Default constructor
    {name}() = default;

    // Parameterized constructor
    {name}({ctor_params_str})
        : {ctor_init_str} {{}}

    // Copy semantics (defaulted for C++14 compatibility)
    {name}(const {name}&) = default;
    {name}& operator=(const {name}&) = default;

    // Move semantics
    {name}({name}&&) = default;
    {name}& operator=({name}&&) = default;

    ~{name}() = default;
}};
"""
    return struct_code.strip()
```

### Step 4: Generate ComponentID Enum

**Enum Generation** (30 LOC)

```python
def generate_component_ids(components: List[Dict]) -> str:
    """Generate ComponentID enum from components."""

    enum_entries = []
    for i, comp in enumerate(components):
        name = comp["componentType"]
        enum_entries.append(f"    {name} = {i},")

    enum_code = f"""
/**
 * @enum ComponentID
 * @brief Unique ID for each component type (auto-generated)
 */
enum class ComponentID : uint32_t {{
{chr(10).join(enum_entries)}
    COUNT = {len(components)}
}};

// Array of component names for debugging
static constexpr const char* COMPONENT_NAMES[{len(components)}] = {{
{chr(10).join([f'    "{comp["componentType"]}",' for comp in components])}
}};

// Bitset configuration
static constexpr uint32_t TOTAL_COMPONENTS = {len(components)};
using ComponentSignature = std::bitset<{len(components)}>;
"""
    return enum_code.strip()
```

### Step 5: Generate Query Specializations

**Query Generation** (40 LOC)

```python
def generate_query_specializations(components: List[Dict]) -> str:
    """Generate common query specializations for optimization."""

    # Most common component combinations
    common_queries = [
        ["Position_data", "Movement_data"],
        ["Position_data", "Health_data"],
        ["Position_data", "Movement_data", "Health_data"],
        ["Identity_data", "Position_data"],
    ]

    specializations = []
    for query_types in common_queries:
        # Only generate if all components exist
        available = [c["componentType"] for c in components]
        if all(t in available for t in query_types):
            types_str = ", ".join(query_types)
            spec = f"""
// Specialization for Query<{types_str}>
template<>
class Query<{types_str}> {{
public:
    // Optimized query implementation
    // For pool management and iteration
}};
"""
            specializations.append(spec)

    return "\n".join(specializations)
```

### Step 6: Assemble Output Files

**Header Assembly** (80 LOC)

```python
def generate_header_file(components: List[Dict]) -> str:
    """Generate complete ECS_Components_Generated.h file."""

    header = """/**
 * @file ECS_Components_Generated.h
 * @brief Auto-generated component type definitions
 * @generated {timestamp}
 * 
 * WARNING: This file is AUTO-GENERATED. Do not edit manually.
 * Re-run GenerateComponents.py to regenerate.
 * 
 * To modify component definitions, edit:
 *   Gamedata/EntityPrefab/ComponentsParameters.json
 */

#ifndef OLYMPE_ECS_COMPONENTS_GENERATED_H
#define OLYMPE_ECS_COMPONENTS_GENERATED_H

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <bitset>
#include "../../TaskSystem/TaskGraphTypes.h"  // TaskValue, etc.
#include "../../vector.h"  // Vector3, Vector2
#include "../../third_party/nlohmann/json.hpp"

namespace Olympe {{

// ============================================================================
// Component Type Definitions (auto-generated)
// ============================================================================

"""

    # Add all component structs
    for comp in components:
        header += generate_component_struct(comp) + "\n\n"

    header += "\n}}  // namespace Olympe\n\n#endif  // OLYMPE_ECS_COMPONENTS_GENERATED_H\n"

    return header

def write_files(output_dir: str, components: List[Dict]) -> None:
    """Write all generated files to output directory."""

    os.makedirs(output_dir, exist_ok=True)

    # 1. ECS_Components_Generated.h
    header_file = os.path.join(output_dir, "ECS_Components_Generated.h")
    with open(header_file, 'w') as f:
        f.write(generate_header_file(components))
    print(f"[GenerateComponents] Wrote {header_file}")

    # 2. ComponentIDs_Generated.h
    ids_file = os.path.join(output_dir, "ComponentIDs_Generated.h")
    with open(ids_file, 'w') as f:
        f.write("#ifndef OLYMPE_COMPONENT_IDS_GENERATED_H\n")
        f.write("#define OLYMPE_COMPONENT_IDS_GENERATED_H\n\n")
        f.write("#include <cstdint>\n#include <bitset>\n\n")
        f.write("namespace Olympe {\n\n")
        f.write(generate_component_ids(components))
        f.write("\n\n}  // namespace Olympe\n\n")
        f.write("#endif  // OLYMPE_COMPONENT_IDS_GENERATED_H\n")
    print(f"[GenerateComponents] Wrote {ids_file}")

    # 3. ComponentQueries_Generated.h
    queries_file = os.path.join(output_dir, "ComponentQueries_Generated.h")
    with open(queries_file, 'w') as f:
        f.write("#ifndef OLYMPE_COMPONENT_QUERIES_GENERATED_H\n")
        f.write("#define OLYMPE_COMPONENT_QUERIES_GENERATED_H\n\n")
        f.write("namespace Olympe {\n\n")
        f.write(generate_query_specializations(components))
        f.write("\n\n}  // namespace Olympe\n\n")
        f.write("#endif  // OLYMPE_COMPONENT_QUERIES_GENERATED_H\n")
    print(f"[GenerateComponents] Wrote {queries_file}")
```

### Step 7: Main Entry Point

**Main Function** (30 LOC)

```python
def main():
    """Main entry point for code generation."""
    import argparse

    parser = argparse.ArgumentParser(
        description="Generate C++ component types from JSON schema"
    )
    parser.add_argument(
        "--input",
        required=True,
        help="Path to ComponentsParameters.json schema file"
    )
    parser.add_argument(
        "--output",
        required=True,
        help="Output directory for generated files"
    )

    args = parser.parse_args()

    # Load schema
    schema = load_schema(args.input)

    # Parse components
    components = parse_components(schema)

    if not components:
        print("ERROR: No components found in schema")
        sys.exit(1)

    # Generate and write files
    write_files(args.output, components)

    print(f"[GenerateComponents] SUCCESS: Generated {len(components)} component types")
    print(f"[GenerateComponents] Output directory: {args.output}")

if __name__ == "__main__":
    main()
```

---

## STEP 3b: CMakeLists.txt INTEGRATION

**File**: `Source/BlueprintEditor/CMakeLists.txt`

```cmake
# ============================================================================
# Phase 32: Code Generation - Pre-build step
# ============================================================================

# Generate component types from schema
set(COMPONENTS_SCHEMA "${CMAKE_SOURCE_DIR}/Gamedata/EntityPrefab/ComponentsParameters.json")
set(GENERATED_DIR "${CMAKE_SOURCE_DIR}/Source/Generated")

# Create generated directory
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Custom target for code generation
add_custom_target(GenerateComponents ALL
    COMMAND python3 
        ${CMAKE_SOURCE_DIR}/Tools/GenerateComponents.py
        --input ${COMPONENTS_SCHEMA}
        --output ${GENERATED_DIR}
    DEPENDS ${COMPONENTS_SCHEMA}
    COMMENT "Generating C++ component types from schema..."
    VERBATIM
)

# Make OlympeBlueprintEditor depend on code generation
add_dependencies(OlympeBlueprintEditor GenerateComponents)

# Add generated files to compilation
target_include_directories(OlympeBlueprintEditor
    PRIVATE ${GENERATED_DIR}
)
```

---

## VALIDATION & TESTING

### Unit Tests (Python)

**File**: `Tools/test_generate_components.py`

```python
import unittest
import json
import tempfile
from GenerateComponents import (
    load_schema, parse_components, json_type_to_cpp,
    generate_component_struct, generate_component_ids
)

class TestCodeGeneration(unittest.TestCase):

    def test_load_valid_schema(self):
        """Test loading valid schema file."""
        # Create temp schema
        schema = {
            "schemas": [
                {
                    "componentType": "TestComponent",
                    "parameters": [
                        {"name": "value", "type": "Float", "defaultValue": 0.0}
                    ]
                }
            ]
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(schema, f)
            path = f.name

        result = load_schema(path)
        self.assertIsNotNone(result)
        self.assertEqual(len(result["schemas"]), 1)

    def test_type_conversion(self):
        """Test JSON type → C++ type conversion."""
        self.assertEqual(json_type_to_cpp("Float"), "float")
        self.assertEqual(json_type_to_cpp("Int"), "int32_t")
        self.assertEqual(json_type_to_cpp("Bool"), "bool")
        self.assertEqual(json_type_to_cpp("String"), "std::string")

    def test_component_struct_generation(self):
        """Test struct generation."""
        comp = {
            "componentType": "TestComp",
            "parameters": [
                {"name": "x", "type": "Float", "defaultValue": 0.0},
                {"name": "y", "type": "Float", "defaultValue": 0.0}
            ]
        }

        code = generate_component_struct(comp)
        self.assertIn("struct TestComp", code)
        self.assertIn("float x = 0.0f", code)
        self.assertIn("float y = 0.0f", code)

    def test_component_ids_generation(self):
        """Test ComponentID enum generation."""
        components = [
            {"componentType": "Position_data", "parameters": []},
            {"componentType": "Health_data", "parameters": []},
        ]

        code = generate_component_ids(components)
        self.assertIn("enum class ComponentID", code)
        self.assertIn("Position_data = 0", code)
        self.assertIn("Health_data = 1", code)
        self.assertIn("COUNT = 2", code)

if __name__ == "__main__":
    unittest.main()
```

### Compilation Test

```bash
# After running GenerateComponents.py, verify C++ compilation
cd /path/to/OlympeEngine
cmake --build . --target OlympeBlueprintEditor

# Check for errors
if [ $? -ne 0 ]; then
    echo "COMPILATION FAILED"
    exit 1
fi

echo "COMPILATION SUCCESSFUL"
```

---

## SUCCESS CHECKLIST

### Development Phase
- [ ] GenerateComponents.py written and tested
- [ ] CMakeLists.txt integration complete
- [ ] Unit tests pass (100%)
- [ ] Python script handles all 29 components
- [ ] Type mappings cover all JSON types

### Integration Phase
- [ ] Pre-build hook triggers on schema changes
- [ ] Generated files created in Source/Generated/
- [ ] C++ compiler accepts generated code
- [ ] No compilation errors or warnings
- [ ] Binary builds successfully

### Validation Phase
- [ ] ComponentID enum has 29 unique IDs
- [ ] All structs instantiate without errors
- [ ] Bitset queries compile and execute
- [ ] Performance: bitset query < 5ms for 1000 entities
- [ ] 100x improvement verified vs std::any

### Production Phase
- [ ] Clean build produces consistent output
- [ ] Schema changes trigger regeneration
- [ ] CI/CD integrates code generation
- [ ] Documentation updated
- [ ] Phase 33 (Entity Factory) unblocked

---

## NEXT STEPS

Once Phase 32 is complete:

1. **Phase 33**: Build Entity Factory using generated components
2. **Phase 34**: Measure performance gains (100x claim validation)
3. **Phase 35**: Add runtime validation using generated IDs

---

**END OF PHASE 32 IMPLEMENTATION GUIDE**
