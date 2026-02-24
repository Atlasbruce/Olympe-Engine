# Atomic Task System – Async Pathfinding

## Overview

`PathfindingManager` provides a thread-safe singleton API for submitting
asynchronous pathfinding requests from within the Atomic Task System.  Each
request runs on a detached `std::thread`, computes a straight-line path string,
and signals completion.  Callers poll `IsComplete()` across frames and retrieve
the result with `GetPathString()`.

---

## PathfindingManager API

Header: `Source/TaskSystem/Pathfinding/PathfindingManager.h`

```cpp
PathfindingManager& PathfindingManager::Get();

RequestID Request(const ::Vector& start,
                  const ::Vector& target,
                  float           delaySeconds = 0.0f);

bool        IsComplete   (RequestID id);
std::string GetPathString(RequestID id);
void        Cancel       (RequestID id);
```

| Method | Description |
|--------|-------------|
| `Get()` | Returns the singleton instance. |
| `Request(start, target, delay)` | Submits an async request.  Returns a `RequestID`.  `delay` simulates computation time (seconds). |
| `IsComplete(id)` | Returns `true` once the worker thread has finished. |
| `GetPathString(id)` | Returns the path string `"(sx,sy,sz)->(tx,ty,tz)"`.  Empty if not complete. |
| `Cancel(id)` | Marks the request cancelled and removes the entry.  The worker thread discards its result if it hasn't written yet. |

### Thread-safety

- `m_requests` is guarded by `m_mutex` on every access.
- `m_nextID` is `std::atomic<uint64_t>` – ID allocation is lock-free.

---

## Task_RequestPathfinding

Header: `Source/TaskSystem/AtomicTasks/AI/Task_RequestPathfinding.h`

### Parameters

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `Target` | Vector | Yes | Destination position. |
| `AsyncDelay` | Float | No | Simulated delay in seconds (default `0.0`). |

### LocalBlackboard

| Key | Type | Direction | Description |
|-----|------|-----------|-------------|
| `Position` | Vector | Read | Current entity position (start of path). |
| `Path` | String | Write | Resulting path string on success. |

### Lifecycle

```
Tick 1 (m_hasRequest == false):
  Read BB["Position"] → start
  Read params["Target"] → target
  PathfindingManager::Get().Request(start, target, delay) → m_requestID
  m_hasRequest = true
  return Running

Ticks 2..N (m_hasRequest == true):
  PathfindingManager::Get().IsComplete(m_requestID)?
    No  → return Running
    Yes → GetPathString(m_requestID) → path
          Cancel(m_requestID)
          BB["Path"] = TaskValue(path)
          return Success

Abort():
  Cancel(m_requestID)
  m_hasRequest = false
```

---

## Example Graph JSON (v3)

```json
{
  "schemaVersion": 3,
  "type": "TaskGraph",
  "localVariables": [
    { "name": "Position", "type": "Vector",  "default": [0,0,0] },
    { "name": "Path",     "type": "String",  "default": "" }
  ],
  "rootNodeId": 0,
  "nodes": [
    {
      "id": 0,
      "name": "RequestPath",
      "atomicTaskId": "Task_RequestPathfinding",
      "params": {
        "Target":     { "type": "Literal", "value": [10, 5, 0] },
        "AsyncDelay": { "type": "Literal", "value": 0.5 }
      },
      "nextOnSuccess": -1,
      "nextOnFailure": -1
    }
  ]
}
```

---

## Build & Test

```bash
mkdir build && cd build
cmake -DBUILD_TESTS=ON -DBUILD_RUNTIME_ENGINE=OFF \
      -DBUILD_BLUEPRINT_EDITOR_STANDALONE=OFF ..
cmake --build .
ctest --output-on-failure
```

Use `AsyncDelay=0.0` in tests for deterministic, near-instant completion.
