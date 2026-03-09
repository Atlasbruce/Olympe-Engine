#!/usr/bin/env python3
"""
ATS Behavior Tree to Visual Script Migration Tool
Converts schema_version 2 BehaviorTree assets to schema_version 4 VisualScript

Usage:
    python Tools/migrate_bt_to_vs.py Blueprints/AI/idle.json
    python Tools/migrate_bt_to_vs.py Blueprints/AI/idle.json --no-backup
    python Tools/migrate_bt_to_vs.py Blueprints/AI/*.json --batch
    python Tools/migrate_bt_to_vs.py --batch Blueprints/AI/
"""

import json
import os
import sys
import shutil
import argparse

# ---------------------------------------------------------------------------
# Node type mapping: v2 BT type -> v4 VS type
# ---------------------------------------------------------------------------
BT_TO_VS_NODE_TYPE = {
    "Action":    "AtomicTask",
    "Condition": "Branch",
    "Sequence":  "VSSequence",
    "Selector":  "Branch",
    "Root":      "EntryPoint",
}

# ---------------------------------------------------------------------------
# Action type mapping: v2 actionType -> v4 AtomicTaskID
# ---------------------------------------------------------------------------
ACTION_TYPE_MAP = {
    "Idle":                          "Task_Wait",
    "MoveToGoal":                    "Task_MoveToLocation",
    "PatrolPickNextPoint":           "Task_PatrolPickPoint",
    "SetMoveGoalToPatrolPoint":      "Task_SetMoveGoal",
    "SetMoveGoalToTarget":           "Task_SetMoveGoal",
    "SetMoveGoalToLastKnownTargetPos": "Task_SetMoveGoalLastKnown",
    "AttackIfClose":                 "Task_Attack",
    "WaitRandomTime":                "Task_Wait",
    "ChooseRandomNavigablePoint":    "Task_ChooseRandomPoint",
    "RequestPathfinding":            "Task_RequestPathfinding",
    "FollowPath":                    "Task_FollowPath",
    "MoveForwardDistance":           "Task_MoveForward",
    "RotateDirectionRandomRange":    "Task_Rotate",
    "FindCover":                     "Task_FindCover",
    "HealSelf":                      "Task_HealSelf",
    "RotateToTarget":                "Task_RotateToTarget",
    "FireAtTarget":                  "Task_Attack",
    "MoveToTarget":                  "Task_MoveToLocation",
    "TakeCover":                     "Task_TakeCover",
    "MeleeAttack":                   "Task_Attack",
    "StrafeAroundTarget":            "Task_Strafe",
    "CallForBackup":                 "Task_CallForBackup",
    "ThrowGrenade":                  "Task_ThrowGrenade",
    "TakeDamageCover":               "Task_TakeCover",
    "UseMedKit":                     "Task_HealSelf",
    "FallBackToPosition":            "Task_MoveToLocation",
    "GroupTactics":                  "Task_GroupTactics",
    "BreachAndClear":                "Task_BreachAndClear",
    "FlankTarget":                   "Task_Flank",
    "SuppressFire":                  "Task_Attack",
}


# ---------------------------------------------------------------------------
# Validator: checks that the input JSON is a valid v2 BehaviorTree
# ---------------------------------------------------------------------------
def validate_v2(data):
    """Returns (ok, errors_list)."""
    errors = []
    if data.get("schema_version", 0) != 2:
        errors.append("schema_version is not 2 (got {})".format(
            data.get("schema_version", "missing")))
    if data.get("blueprintType", "") != "BehaviorTree":
        errors.append("blueprintType is not 'BehaviorTree' (got '{}')".format(
            data.get("blueprintType", "missing")))
    d = data.get("data", {})
    if not isinstance(d, dict):
        errors.append("'data' section is missing or not an object")
        return False, errors
    if "nodes" not in d or not isinstance(d["nodes"], list):
        errors.append("'data.nodes' is missing or not an array")
    if "rootNodeId" not in d:
        errors.append("'data.rootNodeId' is missing")
    return len(errors) == 0, errors


# ---------------------------------------------------------------------------
# Validator: checks that the output JSON is a valid v4 VisualScript
# ---------------------------------------------------------------------------
def validate_v4(data):
    """Returns (ok, errors_list)."""
    errors = []
    if data.get("schema_version", 0) != 4:
        errors.append("schema_version is not 4")
    if data.get("graphType", "") != "VisualScript":
        errors.append("graphType is not 'VisualScript'")
    if not isinstance(data.get("nodes", None), list):
        errors.append("'nodes' is missing or not an array")
    if not isinstance(data.get("execConnections", None), list):
        errors.append("'execConnections' is missing or not an array")
    # Check that there is at least one EntryPoint node
    has_entry = any(n.get("type") == "EntryPoint"
                    for n in data.get("nodes", []))
    if not has_entry:
        errors.append("No EntryPoint node found in 'nodes'")
    return len(errors) == 0, errors


# ---------------------------------------------------------------------------
# Converter: migrates a v2 BehaviorTree JSON to v4 VisualScript
# ---------------------------------------------------------------------------
def convert_v2_to_v4(data):
    """
    Converts a parsed v2 BehaviorTree dict to a v4 VisualScript dict.
    Returns the converted dict.
    """
    d = data["data"]
    v2_nodes = d["nodes"]
    root_id = d["rootNodeId"]

    # Build a lookup map: id -> node
    node_map = {n["id"]: n for n in v2_nodes}

    # Find max ID to allocate new IDs safely
    max_id = max((n["id"] for n in v2_nodes), default=0)

    # Track IDs used by newly created intermediate nodes
    next_new_id = [max_id + 100]  # use a list for mutability in nested function

    def alloc_id():
        nid = next_new_id[0]
        next_new_id[0] += 1
        return nid

    # EntryPoint node gets id = max_id + 1
    entry_id = max_id + 1

    # -----------------------------------------------------------------------
    # Convert v2 nodes → v4 nodes
    # All existing IDs are preserved; intermediate nodes for Selector
    # nesting get new IDs allocated via alloc_id().
    # -----------------------------------------------------------------------

    v4_nodes = []
    exec_connections = []
    # Extra intermediate nodes created for Selector → Branch nesting
    extra_nodes = []

    # Add EntryPoint node
    v4_nodes.append({
        "id": entry_id,
        "type": "EntryPoint",
        "name": "Start",
        "position": {"x": 0, "y": 200}
    })
    # EntryPoint → root
    exec_connections.append({
        "fromNode": entry_id,
        "fromPin": "Out",
        "toNode": root_id,
        "toPin": "In"
    })

    def convert_node(n):
        """Returns a v4 node dict for the given v2 node dict."""
        bt_type = n.get("type", "Action")
        node_id = n["id"]
        node_name = n.get("name", "Node_{}".format(node_id))
        pos = n.get("position", {"x": 0, "y": 0})
        params = n.get("parameters", {})

        v4_type = BT_TO_VS_NODE_TYPE.get(bt_type, "AtomicTask")

        result = {
            "id": node_id,
            "type": v4_type,
            "name": node_name,
            "position": pos,
        }

        if bt_type == "Action":
            action_type = n.get("actionType", "")
            task_id = ACTION_TYPE_MAP.get(action_type, "Task_" + action_type if action_type else "Task_Unknown")
            result["taskType"] = task_id
            if params:
                result["params"] = params

        elif bt_type == "Condition":
            cond_type = n.get("conditionType", "")
            result["conditionKey"] = cond_type
            if params:
                result["params"] = params

        elif bt_type == "Sequence":
            children = n.get("children", [])
            result["children"] = children

        elif bt_type == "Selector":
            # Selector becomes a Branch; children wired via Then/Else
            # The actual children wiring is done below in build_selector_connections
            pass

        return result

    def build_selector_connections(selector_id, children):
        """
        For a Selector with N children, create a chain of Branch nodes.
        selector_id: the ID of the original Selector node (now Branch)
        children: list of child IDs

        Structure:
          Branch(selector_id): Then → child[0], Else → intermediate_branch1
          intermediate_branch1: Then → child[1], Else → intermediate_branch2
          ...
          intermediate_branchN-2: Then → child[N-2], Else → child[N-1]

        For 1 child: just connect Then → child[0]
        For 0 children: nothing
        """
        if len(children) == 0:
            return
        if len(children) == 1:
            exec_connections.append({
                "fromNode": selector_id,
                "fromPin": "Then",
                "toNode": children[0],
                "toPin": "In"
            })
            return
        if len(children) == 2:
            exec_connections.append({
                "fromNode": selector_id,
                "fromPin": "Then",
                "toNode": children[0],
                "toPin": "In"
            })
            exec_connections.append({
                "fromNode": selector_id,
                "fromPin": "Else",
                "toNode": children[1],
                "toPin": "In"
            })
            return

        # N > 2: create intermediate Branch nodes
        current_branch = selector_id
        for i in range(len(children) - 2):
            # Then → child[i]
            exec_connections.append({
                "fromNode": current_branch,
                "fromPin": "Then",
                "toNode": children[i],
                "toPin": "In"
            })
            # Else → next intermediate branch
            next_branch_id = alloc_id()
            exec_connections.append({
                "fromNode": current_branch,
                "fromPin": "Else",
                "toNode": next_branch_id,
                "toPin": "In"
            })
            # Create the intermediate branch node
            v4_nodes.append({
                "id": next_branch_id,
                "type": "Branch",
                "name": "Selector_Branch_{}".format(next_branch_id),
                "position": {"x": 0, "y": 0}
            })
            current_branch = next_branch_id

        # Last intermediate: Then → child[N-2], Else → child[N-1]
        exec_connections.append({
            "fromNode": current_branch,
            "fromPin": "Then",
            "toNode": children[-2],
            "toPin": "In"
        })
        exec_connections.append({
            "fromNode": current_branch,
            "fromPin": "Else",
            "toNode": children[-1],
            "toPin": "In"
        })

    def build_sequence_connections(seq_id, children):
        """
        For VSSequence, the executor uses ChildrenIDs internally.
        We also add explicit exec connections for completeness:
          After all children done, VSSequence follows Out.
        No between-child connections needed (handled by ChildrenIDs).
        """
        # Nothing extra needed - ChildrenIDs in the node handles execution order.
        pass

    def build_action_connections(action_id, children):
        """
        Actions in BT normally have no children.
        In npc_ai.json, Action nodes have children (unusual).
        We treat them as AtomicTask → chain children via Out → In.
        """
        if not children:
            return
        # Connect action → first child
        exec_connections.append({
            "fromNode": action_id,
            "fromPin": "Out",
            "toNode": children[0],
            "toPin": "In"
        })
        # Chain remaining children
        for i in range(1, len(children)):
            exec_connections.append({
                "fromNode": children[i-1],
                "fromPin": "Out",
                "toNode": children[i],
                "toPin": "In"
            })

    # Convert all v2 nodes
    for n in v2_nodes:
        v4_node = convert_node(n)
        v4_nodes.append(v4_node)

        bt_type = n.get("type", "Action")
        children = n.get("children", [])

        if bt_type == "Selector":
            build_selector_connections(n["id"], children)
        elif bt_type == "Sequence":
            build_sequence_connections(n["id"], children)
        elif bt_type == "Action":
            build_action_connections(n["id"], children)
        elif bt_type == "Condition":
            # Condition in BT has children (the subtree to execute if true)
            # In v4 it becomes Branch: Then → children
            if len(children) == 1:
                exec_connections.append({
                    "fromNode": n["id"],
                    "fromPin": "Then",
                    "toNode": children[0],
                    "toPin": "In"
                })
            elif len(children) > 1:
                exec_connections.append({
                    "fromNode": n["id"],
                    "fromPin": "Then",
                    "toNode": children[0],
                    "toPin": "In"
                })

    # Extract blackboard from node parameters (optional, best-effort)
    blackboard = extract_blackboard(v2_nodes)

    # Build the v4 result
    v4 = {
        "schema_version": 4,
        "graphType": "VisualScript",
        "name": data.get("name", "UnnamedGraph"),
        "description": data.get("description", "Migrated from schema_version 2 BehaviorTree"),
        "blackboard": blackboard,
        "nodes": v4_nodes,
        "execConnections": exec_connections,
        "dataConnections": []
    }

    # Preserve editor state if present
    if "editorState" in data:
        v4["editorState"] = data["editorState"]

    return v4


# ---------------------------------------------------------------------------
# Blackboard extractor: finds unique BB keys from node parameters
# ---------------------------------------------------------------------------
def extract_blackboard(v2_nodes):
    """Best-effort extraction of blackboard variables from node parameters."""
    bb_keys = {}  # key -> type hint

    for n in v2_nodes:
        params = n.get("parameters", {})
        bt_type = n.get("type", "")

        if bt_type == "Condition":
            cond_type = n.get("conditionType", "")
            if cond_type == "CheckBlackboardValue":
                key = params.get("key", "")
                if key and key not in bb_keys:
                    val = params.get("value", "")
                    # Guess type from value
                    try:
                        int(val)
                        bb_keys[key] = "Int"
                    except (ValueError, TypeError):
                        bb_keys[key] = "String"

    result = []
    for key, typ in sorted(bb_keys.items()):
        result.append({
            "key": key,
            "type": typ,
            "value": 0 if typ == "Int" else "",
            "global": False
        })
    return result


# ---------------------------------------------------------------------------
# File migration: single file
# ---------------------------------------------------------------------------
def migrate_file(input_path, backup=True, verbose=True):
    """
    Migrates a single v2 JSON file to v4 in-place.
    Returns True on success, False on failure.
    """
    if verbose:
        print("Migrating: {}".format(input_path))

    # Load JSON
    try:
        with open(input_path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except Exception as e:
        print("  ERROR: Failed to read '{}': {}".format(input_path, e))
        return False

    # Check if already v4
    schema = data.get("schema_version", 0)
    if schema >= 4:
        if verbose:
            print("  SKIP: already schema_version {} (>=4)".format(schema))
        return True

    # Validate v2 input
    ok, errors = validate_v2(data)
    if not ok:
        print("  ERROR: Invalid v2 structure:")
        for e in errors:
            print("    - {}".format(e))
        return False

    # Backup original
    if backup:
        bak_path = input_path + ".bak"
        try:
            shutil.copy2(input_path, bak_path)
            if verbose:
                print("  Backup: {}".format(bak_path))
        except Exception as e:
            print("  WARNING: Failed to create backup: {}".format(e))

    # Convert
    try:
        v4_data = convert_v2_to_v4(data)
    except Exception as e:
        print("  ERROR: Conversion failed: {}".format(e))
        import traceback
        traceback.print_exc()
        return False

    # Validate output
    ok, errors = validate_v4(v4_data)
    if not ok:
        print("  ERROR: v4 validation failed:")
        for e in errors:
            print("    - {}".format(e))
        return False

    # Write output
    try:
        with open(input_path, "w", encoding="utf-8") as f:
            json.dump(v4_data, f, indent=2, ensure_ascii=False)
    except Exception as e:
        print("  ERROR: Failed to write output: {}".format(e))
        return False

    if verbose:
        print("  OK: {} nodes, {} connections".format(
            len(v4_data["nodes"]),
            len(v4_data["execConnections"])))
    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="ATS Behavior Tree to Visual Script Migration Tool")
    parser.add_argument(
        "paths",
        nargs="+",
        help="JSON file(s) or directory (with --batch) to migrate")
    parser.add_argument(
        "--batch",
        action="store_true",
        help="Process all .json files in the given directories")
    parser.add_argument(
        "--no-backup",
        action="store_true",
        help="Do not create .bak backups of original files")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate and show what would be converted, but do not write")
    args = parser.parse_args()

    files = []
    if args.batch:
        for p in args.paths:
            if os.path.isdir(p):
                for fname in os.listdir(p):
                    if fname.endswith(".json"):
                        files.append(os.path.join(p, fname))
            elif os.path.isfile(p):
                files.append(p)
            else:
                # Glob-like: treat as pattern prefix
                import glob as globmod
                files.extend(globmod.glob(p))
    else:
        files = list(args.paths)

    if not files:
        print("No files to process.")
        return 0

    success = 0
    skipped = 0
    failed = 0

    for fpath in sorted(files):
        if not os.path.isfile(fpath):
            print("SKIP (not a file): {}".format(fpath))
            skipped += 1
            continue

        # Dry run: just validate
        if args.dry_run:
            try:
                with open(fpath, "r", encoding="utf-8") as f:
                    data = json.load(f)
                schema = data.get("schema_version", 0)
                if schema >= 4:
                    print("SKIP (v{}): {}".format(schema, fpath))
                    skipped += 1
                    continue
                ok, errors = validate_v2(data)
                if ok:
                    print("WOULD MIGRATE: {}".format(fpath))
                    success += 1
                else:
                    print("INVALID v2: {}".format(fpath))
                    for e in errors:
                        print("  - {}".format(e))
                    failed += 1
            except Exception as e:
                print("ERROR reading {}: {}".format(fpath, e))
                failed += 1
            continue

        # Actual migration
        result = migrate_file(fpath, backup=not args.no_backup)
        if result:
            success += 1
        else:
            failed += 1

    print("")
    print("Summary: {} migrated, {} skipped, {} failed".format(
        success, skipped, failed))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
