# PERFORMANCE VALIDATION FRAMEWORK (Phase 34 & Beyond)

**Document Date**: 2026-04-04  
**Purpose**: Establish performance baselines for the 100x performance claim  
**Phase Dependency**: Phase 32 (code generation) must be complete first  

---

## PERFORMANCE METRICS DEFINED

### 1. Component Query Performance

#### Benchmark: "Query 1000 entities with Position + Movement"

**Measurement Device**: CPU performance profiler  
**Execution Environment**: Release build (O3), single-threaded  

**Current State (std::any reflection)**:
```
Expected: ~520ms (100x slower baseline per architecture doc)
Reason: Runtime hash map lookup + std::any_cast overhead
```

**Target State (Generated components)**:
```
Expected: ~5ms (100x improvement = 104x total speed)
Reason: Compiled ComponentID enum + bitset operations
Measurement: Bitset query compiled to CPU instructions
```

**How to Measure**:
```cpp
// Pseudocode for benchmark
const int ITERATIONS = 100;
auto start = high_resolution_clock::now();

for (int i = 0; i < ITERATIONS; ++i) {
    auto query = world.Query<Position_data, Movement_data>();
    for (auto [entity, pos, mov] : query) {
        pos.x += mov.vx * 0.016f;  // Simulate work
    }
}

auto end = high_resolution_clock::now();
auto duration = duration_cast<milliseconds>(end - start);
float avg_ms = duration.count() / (float)ITERATIONS;
printf("Query performance: %.2f ms/frame\n", avg_ms);
```

**Success Threshold**:
- ✅ < 5ms per frame (for 1000 entities)
- ✅ > 100x faster than reflection
- ✅ Scales linearly (2000 entities = ~10ms)

---

### 2. Component Lookup Performance

#### Benchmark: "Get component by type"

**Current State**:
```
Expected: ~100 cycles per lookup (std::unordered_map)
Reason: Hash function, collision resolution
```

**Target State**:
```
Expected: ~3 cycles per lookup (enum + array)
Reason: Direct array indexing via enum ID
```

**How to Measure**:
```cpp
// Pseudocode
const int ITERATIONS = 1000000;
Entity entity = CreateTestEntity();

auto start = high_resolution_clock::now();

for (int i = 0; i < ITERATIONS; ++i) {
    auto pos = world.GetComponent<Position_data>(entity);
    // Do something to prevent optimization
    asm volatile ("" : "+m" (pos));
}

auto end = high_resolution_clock::now();
auto duration = duration_cast<nanoseconds>(end - start);
float ns_per_lookup = duration.count() / (float)ITERATIONS;
printf("Component lookup: %.2f ns/lookup\n", ns_per_lookup);

// Convert to cycles (assume 3.0 GHz CPU)
float cycles = ns_per_lookup * 3.0;
printf("Component lookup: %.1f cycles\n", cycles);
```

**Success Threshold**:
- ✅ < 5 ns per lookup (~15 cycles)
- ✅ O(1) complexity
- ✅ Deterministic (no hash collisions)

---

### 3. Entity Spawn Performance

#### Benchmark: "Spawn 1000 entities"

**Measurement Target**: EntityFactory::CreateEntity()  
**Component Set**: Full Guard prefab (6 components)  

**Current State**:
```
Expected: Unknown (not measured)
Target: < 100ms for 1000 entities = 0.1ms per entity
```

**How to Measure**:
```cpp
// Pseudocode
const int ENTITY_COUNT = 1000;
auto start = high_resolution_clock::now();

for (int i = 0; i < ENTITY_COUNT; ++i) {
    factory.CreateEntity("Guard", Vector{(float)i, 0, 0});
}

auto end = high_resolution_clock::now();
auto duration = duration_cast<milliseconds>(end - start);
printf("Spawn 1000 entities: %.2f ms\n", duration.count());
printf("Per entity: %.3f ms\n", duration.count() / ENTITY_COUNT);
```

**Success Threshold**:
- ✅ < 100ms total (1000 entities)
- ✅ < 0.2ms per entity
- ✅ Linear scaling (no quadratic behavior)

---

### 4. Memory Usage

#### Benchmark: "Memory for 1000 entities"

**Measurement Target**: Total heap usage after spawn  

**Target**:
```
Memory = (components_per_entity * component_size * entity_count) + overhead
       = (6 * 64 bytes * 1000) + 10MB
       = ~50MB max
```

**How to Measure**:
```cpp
// Pseudocode
size_t before = GetHeapUsage();

for (int i = 0; i < 1000; ++i) {
    factory.CreateEntity("Guard", ...);
}

size_t after = GetHeapUsage();
printf("Memory usage: %.2f MB\n", (after - before) / 1024.0 / 1024.0);
printf("Per entity: %.0f bytes\n", (after - before) / 1000.0);
```

**Success Threshold**:
- ✅ < 100MB total
- ✅ < 100KB per entity
- ✅ No memory leaks (verified by repeated cycles)

---

### 5. Graph Execution Performance

#### Benchmark: "Execute graph for 100 entities"

**Measurement Target**: VSGraphExecutor::ExecuteFrame()  
**Graph Complexity**: 20 nodes (medium complexity)  

**Target**:
```
Per frame per entity: < 1ms
Total for 100 entities: < 100ms
```

**How to Measure**:
```cpp
// Pseudocode
const int ITERATIONS = 100;
std::vector<EntityID> entities;
for (int i = 0; i < 100; ++i) {
    entities.push_back(factory.CreateEntity("Guard", ...));
}

auto start = high_resolution_clock::now();

for (int i = 0; i < ITERATIONS; ++i) {
    taskSystem.Process(0.016f);  // 60 FPS delta time
}

auto end = high_resolution_clock::now();
auto duration = duration_cast<milliseconds>(end - start);
printf("Graph execution: %.2f ms/frame\n", duration.count() / ITERATIONS);
```

**Success Threshold**:
- ✅ < 1ms per entity per frame
- ✅ < 100ms total for 100 entities at 60 FPS

---

## PERFORMANCE REGRESSION TEST SUITE

### Automated Tests (CI/CD)

```python
# test_performance_regression.py
import subprocess
import json
import time

BASELINES = {
    "query_1000_entities": 5.0,      # ms
    "component_lookup": 5.0,          # ns
    "spawn_1000_entities": 100.0,    # ms
    "memory_per_entity": 100.0,      # KB
    "graph_execution_100_entities": 100.0  # ms
}

def run_benchmark(name, executable):
    """Run benchmark and return result."""
    result = subprocess.run(
        executable,
        capture_output=True,
        text=True
    )
    # Parse output and extract metric
    return parse_metric(result.stdout)

def check_regression(name, current, baseline):
    """Check if performance regressed."""
    ratio = current / baseline
    if ratio > 1.1:  # 10% regression threshold
        return f"REGRESSION: {name} was {baseline:.2f}, now {current:.2f} ({ratio:.1f}x)"
    elif ratio > 1.05:  # 5% warning threshold
        return f"WARNING: {name} may have regressed ({ratio:.1f}x)"
    else:
        return f"OK: {name}"

def main():
    results = {}
    for test_name, baseline in BASELINES.items():
        executable = f"./bin/perf_test_{test_name}"
        try:
            current = run_benchmark(test_name, executable)
            results[test_name] = current
            status = check_regression(test_name, current, baseline)
            print(status)
        except Exception as e:
            print(f"ERROR running {test_name}: {e}")

    # Save results for tracking
    with open("perf_results.json", "w") as f:
        json.dump(results, f, indent=2)

if __name__ == "__main__":
    main()
```

---

## PROFILING POINTS (When to Measure)

### Mandatory Profiling Points

1. **After Phase 32** (Code Generation Complete)
   - Measure: ComponentID enum compilation
   - Purpose: Verify bitset operations work
   - Action: If fails, debug code generation

2. **After Phase 33** (Entity Factory Complete)
   - Measure: Entity spawn performance
   - Purpose: Verify factory efficiency
   - Action: If exceeds 100ms, optimize

3. **After Phase 34** (Performance Baselines Established)
   - Measure: All 5 metrics
   - Purpose: Validate 100x claim
   - Action: If target missed, debug hot paths

4. **Before Release** (Production Ready)
   - Measure: All metrics + memory profiling
   - Purpose: Final validation
   - Action: Release only if all targets met

---

## PROFILING METHODOLOGY

### CPU Profiling (for Phase 34)

**Tool**: Visual Studio Performance Profiler or perf  
**Duration**: 30-second trace  
**Output**: 
- Flame graph (hot paths)
- Call tree (function breakdown)
- Sample counts (per-function time)

**Analysis**:
```
Top hotspots should be:
1. Component array access (expected)
2. Data pin evaluation (expected)
3. Other system code (not in query hot path)

Red flags:
- std::any_cast in top 10
- unordered_map::find in top 10
- malloc/free calls during query
```

### Memory Profiling (for Phase 34)

**Tool**: Visual Studio Memory Profiler or Valgrind  
**Duration**: Full benchmark run  
**Output**:
- Heap snapshot (before/after)
- Allocation tree (where memory allocated)
- Leak detection (unreleased memory)

**Analysis**:
```
Expected patterns:
- ComponentPool allocations (1-10x per component type)
- Entity data structures (linear with entity count)
- No repeated allocations during execution

Red flags:
- Unbounded growth (leak)
- Allocations during query (should be pre-allocated)
```

---

## BASELINE ESTABLISHMENT PROCESS

### Step 1: Stabilize Build (Week 1)
- Compile Release build with O3 optimization
- Ensure deterministic execution (disable frequency scaling)
- Use consistent test hardware

### Step 2: Run Baseline Measurements (Week 2-3, Phase 34)
- Execute each benchmark 5x
- Record all 5 measurements
- Calculate mean, std dev, min, max

### Step 3: Establish Thresholds (Week 3)
- Mean becomes baseline
- Upper threshold = mean * 1.1 (10% regression)
- Warning threshold = mean * 1.05 (5% warning)

### Step 4: Document Results (Week 3)
- Store in `Performance/baselines.json`
- Create comparison report
- Include environment details

**Example baseline.json**:
```json
{
  "timestamp": "2026-04-15T14:30:00Z",
  "environment": {
    "cpu": "Intel Core i7-9700K @ 3.6GHz",
    "ram": "32GB DDR4",
    "os": "Windows 10 Build 19043",
    "build": "Release O3"
  },
  "metrics": {
    "query_1000_entities_ms": {
      "measurements": [5.1, 4.9, 5.2, 5.0, 4.8],
      "mean": 5.0,
      "std_dev": 0.15,
      "min": 4.8,
      "max": 5.2,
      "threshold_regression": 5.5,
      "threshold_warning": 5.25
    },
    "component_lookup_ns": {
      "measurements": [4.2, 4.1, 4.3, 4.0, 4.2],
      "mean": 4.16,
      "std_dev": 0.1,
      "min": 4.0,
      "max": 4.3,
      "threshold_regression": 4.58,
      "threshold_warning": 4.37
    }
  }
}
```

---

## PERFORMANCE TARGETS vs ARCHITECTURE CLAIM

### The 100x Claim

**From Architecture Document**:
> "Compile-time type checking (C++ compiler)  
> Zero-overhead abstraction (inline generated types)  
> No runtime type casting (no std::any)  
> CPU cache-friendly storage (SoA layout)  
> SIMD-optimizable loops (tight component arrays)  
> Bitset operations = CPU native instructions"  
> **Result: 100x faster than dynamic system**

### How We Validate

| Claim | Measurement | Threshold | Status |
|-------|-----------|-----------|--------|
| Bitset operations are fast | Query < 5ms | 100x baseline | ? |
| Zero-overhead | Component lookup < 5ns | 100x baseline | ? |
| No runtime casting | std::any_cast not in top 20 hotspots | None | ? |
| SoA cache-friendly | L1/L2 miss rate normal | < 2% misses | ? |
| SIMD-optimizable | Bitset queries vectorized | Check asm | ? |

**Pass Condition**: All claims validated (at least 80x improvement, preferably 100x+)

---

## WHAT IF TARGETS ARE NOT MET?

### Scenario 1: Only 50x Improvement (ACCEPTABLE)

**Action**:
- Update architecture document to reflect 50x reality
- Investigate remaining bottleneck (code generation issue or CPU limited)
- Continue to Phase 35 (not a blocker)
- Plan optimization for Phase 32 (Phase 41)

### Scenario 2: Only 10x Improvement (INVESTIGATION)

**Action**:
- STOP and investigate:
  - Is code generation producing correct enums?
  - Are bitset queries compiling to native instructions?
  - Is there unnecessary overhead in query iteration?
- Debug: CPU profiler + ASM review
- Fix and re-measure

### Scenario 3: No Improvement (BLOCKER)

**Action**:
- CRITICAL ISSUE
- Phase 32 code generation must be wrong
- Revert to manual components, debug diff
- Re-implement Phase 32

---

## CONTINUOUS PERFORMANCE MONITORING

### Phase 34+ Performance Dashboard

```
Real-time metrics (every commit):
┌─────────────────────────────────┐
│ Performance Dashboard            │
├─────────────────────────────────┤
│ Query Performance:  5.2ms  ✓     │ (baseline: 5.0ms)
│ Component Lookup:   4.1ns  ✓     │ (baseline: 4.16ns)
│ Entity Spawn:      98ms   ✓      │ (baseline: 100ms)
│ Memory per Entity: 95KB   ✓      │ (baseline: 100KB)
│ Graph Execution:   99ms   ✓      │ (baseline: 100ms)
│                                  │
│ Overall: 100% Targets Met ✓     │
└─────────────────────────────────┘
```

### Regression Alerts

```
If metric exceeds threshold:
1. Alert developer
2. Identify commit that caused regression
3. Block merge until fixed
4. Auto-revert if severe
```

---

## SUMMARY: PERFORMANCE VALIDATION STRATEGY

| Phase | Milestone | Measurements | Success Criteria |
|-------|-----------|--------------|-----------------|
| 32 | Code Gen Complete | Compilation time | Enum generates correctly |
| 33 | Factory Complete | Entity spawn time | < 100ms for 1000 entities |
| **34** | **Baselines Established** | **All 5 metrics** | **100x improvement validated** |
| 35+ | Continuous | Regression tests | All metrics within 5% baseline |
| Release | Production | Final validation | All targets met |

---

**END OF PERFORMANCE VALIDATION FRAMEWORK**
