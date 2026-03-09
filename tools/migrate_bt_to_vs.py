#!/usr/bin/env python3
r"""
migrate_bt_to_vs.py
-------------------
Migrate BehaviorTree JSON assets (schema v2 / blueprintType BehaviorTree)
to ATS VisualScript format (schema_version 4).

Usage:
    python3 tools/migrate_bt_to_vs.py <input.json> [output.json]

    # Migrate all v2 assets in-place:
    find Blueprints/AI -name "*.json" -exec python3 tools/migrate_bt_to_vs.py {} \;

The converter performs a structural mapping:
  - Selector   -> Selector node  (with Option0..N exec-out pins to children)
  - Sequence   -> VSSequence node (with ordered child chain)
  - Action     -> AtomicTask node (actionType -> taskType)
  - Condition  -> AtomicTask node (conditionType -> taskType, leaf evaluator)
  - Root       -> EntryPoint node
  - Repeater/Decorator -> Decorator node

Input/output parameters for SubGraph nodes are NOT auto-generated; they
must be added manually when wiring SubGraph calls.
"""

import json
import sys
import os
from typing import Dict, List, Any, Optional


# ---------------------------------------------------------------------------
# Node type mapping
# ---------------------------------------------------------------------------

_TYPE_MAP = {
    "Selector":  "Selector",
    "Sequence":  "VSSequence",
    "Parallel":  "Parallel",
    "Root":      "EntryPoint",
    "Action":    "AtomicTask",
    "Condition": "AtomicTask",  # BT Condition nodes are leaf evaluators → AtomicTask in VS
    "Decorator": "Decorator",
    "Repeater":  "Decorator",
    "AtomicTask": "AtomicTask",
}


def _map_node_type(type_v2: str) -> str:
    return _TYPE_MAP.get(type_v2, type_v2)


# ---------------------------------------------------------------------------
# Node conversion
# ---------------------------------------------------------------------------

def _convert_node(node_v2: Dict[str, Any]) -> Dict[str, Any]:
    """Convert a single BehaviorTree node to v4 VisualScript format."""
    node_v4: Dict[str, Any] = {}

    node_v4["id"]    = node_v2.get("id", 0)
    node_v4["type"]  = _map_node_type(node_v2.get("type", "AtomicTask"))
    node_v4["label"] = node_v2.get("name", "Unnamed")

    pos = node_v2.get("position", {})
    node_v4["position"] = {"x": pos.get("x", 0.0), "y": pos.get("y", 0.0)}

    # AtomicTask type mapping
    type_v2 = node_v2.get("type", "")
    if type_v2 == "Action" and "actionType" in node_v2:
        node_v4["taskType"] = node_v2["actionType"]
    elif type_v2 == "Condition" and "conditionType" in node_v2:
        node_v4["taskType"] = node_v2["conditionType"]
    elif type_v2 == "AtomicTask" and "atomicTaskId" in node_v2:
        node_v4["taskType"] = node_v2["atomicTaskId"]

    # Copy parameters (flat dict of primitives)
    params = node_v2.get("parameters", {})
    if params:
        node_v4["params"] = {k: v for k, v in params.items()}

    return node_v4


# ---------------------------------------------------------------------------
# Exec connection builder
# ---------------------------------------------------------------------------

def _build_exec_connections(nodes_v2: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    """
    Build execConnections from the children arrays in the v2 format.

    Connection semantics by node type:
      Selector   : fromPin=OptionN    (tries each child in turn)
      Sequence   : fromPin=Start (first child), Completed->Next (chain)
      Root       : fromPin=Out (single child)
      Condition  : fromPin=Out (single child, then-branch)
      Other      : fromPin=Out if one child
    """
    connections: List[Dict[str, Any]] = []

    id_to_node: Dict[int, Dict[str, Any]] = {n.get("id", 0): n for n in nodes_v2}

    for node in nodes_v2:
        node_id  = node.get("id", 0)
        type_v2  = node.get("type", "")
        children = node.get("children", [])

        if not children:
            continue

        if type_v2 == "Selector":
            for i, child_id in enumerate(children):
                connections.append({
                    "fromNode": node_id,
                    "fromPin":  "Option{}".format(i),
                    "toNode":   child_id,
                    "toPin":    "In",
                })

        elif type_v2 in ("Sequence", "Root"):
            # Chain: node->Start->child[0], child[i-1]->Completed->child[i]
            if children:
                connections.append({
                    "fromNode": node_id,
                    "fromPin":  "Start" if type_v2 == "Sequence" else "Out",
                    "toNode":   children[0],
                    "toPin":    "In",
                })
                for i in range(1, len(children)):
                    connections.append({
                        "fromNode": children[i - 1],
                        "fromPin":  "Completed",
                        "toNode":   children[i],
                        "toPin":    "In",
                    })

        elif type_v2 == "Condition":
            # Condition maps to AtomicTask: single child on "Out" pin
            if len(children) == 1:
                connections.append({
                    "fromNode": node_id,
                    "fromPin":  "Out",
                    "toNode":   children[0],
                    "toPin":    "In",
                })

        else:
            # Generic: single Out connection
            if len(children) == 1:
                connections.append({
                    "fromNode": node_id,
                    "fromPin":  "Out",
                    "toNode":   children[0],
                    "toPin":    "In",
                })

    return connections


# ---------------------------------------------------------------------------
# Full migration
# ---------------------------------------------------------------------------

def migrate_v2_to_v4(data_v2: Dict[str, Any]) -> Optional[Dict[str, Any]]:
    """
    Convert a BehaviorTree (schema v2) JSON object to VisualScript v4.

    Returns the migrated dict, or None if already v4 or unrecognised format.
    """
    schema_version = data_v2.get("schema_version", 0)
    blueprint_type = data_v2.get("blueprintType", "")

    # Already v4 — skip
    if schema_version == 4:
        print("  Already schema_version 4, skipping.")
        return None

    # Must be BehaviorTree-style (blueprintType or schema_version <= 2)
    if schema_version not in (0, 1, 2) and blueprint_type != "BehaviorTree":
        print("  Unknown schema version {}; skipping.".format(schema_version))
        return None

    data_section: Dict[str, Any] = data_v2.get("data", {})
    nodes_v2: List[Dict[str, Any]] = data_section.get("nodes", [])
    root_id: int = data_section.get("rootNodeId", 1)
    graph_name: str = data_v2.get("name", os.path.splitext(
        os.path.basename(sys.argv[1]))[0])

    data_v4: Dict[str, Any] = {
        "schema_version":  4,
        "id":              graph_name.lower().replace(" ", "-"),
        "name":            graph_name,
        "graphType":       "VisualScript",
        "entryPointId":    root_id,
        "blackboard":      [],
        "nodes":           [],
        "execConnections": [],
        "dataConnections": [],
    }

    # Copy description if present
    if "description" in data_v2:
        data_v4["description"] = data_v2["description"]

    # Convert nodes
    for node_v2 in nodes_v2:
        data_v4["nodes"].append(_convert_node(node_v2))

    # Build exec connections from children arrays
    data_v4["execConnections"] = _build_exec_connections(nodes_v2)

    # Copy metadata block if present
    if "metadata" in data_v2:
        data_v4["metadata"] = data_v2["metadata"]

    return data_v4


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    if len(sys.argv) < 2:
        print("Usage: python3 migrate_bt_to_vs.py <input.json> [output.json]")
        print()
        print("  If output.json is omitted, the file is migrated in-place.")
        sys.exit(1)

    input_path  = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else input_path

    if not os.path.isfile(input_path):
        print("ERROR: File not found: {}".format(input_path))
        sys.exit(1)

    with open(input_path, "r", encoding="utf-8") as fh:
        try:
            data_v2 = json.load(fh)
        except json.JSONDecodeError as exc:
            print("ERROR: Failed to parse {}: {}".format(input_path, exc))
            sys.exit(1)

    print("Migrating: {}".format(input_path))
    data_v4 = migrate_v2_to_v4(data_v2)

    if data_v4 is None:
        print("  Skipped (no migration needed).")
        return

    with open(output_path, "w", encoding="utf-8") as fh:
        json.dump(data_v4, fh, indent=2)
        fh.write("\n")

    node_count = len(data_v4.get("nodes", []))
    conn_count = len(data_v4.get("execConnections", []))
    print("  Done. {} nodes, {} exec connections -> {}".format(
        node_count, conn_count, output_path))


if __name__ == "__main__":
    main()
