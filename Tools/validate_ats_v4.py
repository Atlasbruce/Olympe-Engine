#!/usr/bin/env python3
"""Validate ATS v4 JSON files for schema compliance.

Usage:
    python3 Tools/validate_ats_v4.py

Run from the repository root.  The script scans Blueprints/AI/ recursively for
JSON files, validates every file that declares schema_version == 4, and reports
any compliance issues.

Exit code 0 means all v4 files are valid; non-zero means at least one error was
found.
"""

import json
import sys
from pathlib import Path


def validate_v4_file(path):
    """Return a list of error strings for the given file path.

    Returns an empty list for non-v4 files (they are silently skipped).
    """
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
    except json.JSONDecodeError as exc:
        return [f"JSON parse error: {exc}"]
    except OSError as exc:
        return [f"Cannot open file: {exc}"]

    # Only validate v4 files
    if data.get("schema_version") != 4:
        return []

    errors = []

    # Required root-level fields for schema v4 VisualScript
    required_fields = [
        "schema_version",
        "type",
        "graphType",
        "name",
        "blackboard",
        "nodes",
        "execConnections",
        "dataConnections",
    ]
    for field in required_fields:
        if field not in data:
            errors.append(f"Missing required field: '{field}'")

    # 'type' and 'graphType' must match
    if "type" in data and "graphType" in data:
        if data["type"] != data["graphType"]:
            errors.append(
                f"'type' ({data['type']!r}) != 'graphType' ({data['graphType']!r})"
            )

    # Node-level checks
    for node in data.get("nodes", []):
        node_label = node.get("name") or node.get("id", "?")

        # Legacy field names that should have been migrated
        if "nodeID" in node:
            errors.append(
                f"Node '{node_label}': uses legacy field 'nodeID' (should be 'id')"
            )
        if "nodeType" in node:
            errors.append(
                f"Node '{node_label}': uses legacy field 'nodeType' (should be 'type')"
            )

        # Every node must have an 'id' and a 'type'
        if "id" not in node and "nodeID" not in node:
            errors.append(f"Node '{node_label}': missing 'id' field")
        if "type" not in node and "nodeType" not in node:
            errors.append(f"Node '{node_label}': missing 'type' field")

    # Legacy top-level fields
    legacy_fields = {
        "localBlackboard": "blackboard",
        "ExecConnections": "execConnections",
        "DataConnections": "dataConnections",
    }
    for old_field, new_field in legacy_fields.items():
        if old_field in data:
            errors.append(
                f"Legacy field '{old_field}' found (should be '{new_field}')"
            )

    return errors


def main():
    root = Path("Blueprints/AI")
    if not root.exists():
        print(
            f"ERROR: Directory '{root}' not found. Run from the repository root.",
            file=sys.stderr,
        )
        return 1

    total_v4 = 0
    total_errors = 0

    for json_file in sorted(root.rglob("*.json")):
        errors = validate_v4_file(json_file)

        # Skip non-v4 files (validate_v4_file returns [] for them)
        try:
            with open(json_file, encoding="utf-8") as f:
                data = json.load(f)
            if data.get("schema_version") != 4:
                continue
        except (json.JSONDecodeError, OSError):
            # validate_v4_file already captured this error
            pass

        total_v4 += 1

        if errors:
            print(f"\u274c {json_file}:")
            for err in errors:
                print(f"  - {err}")
            total_errors += len(errors)
        else:
            print(f"\u2705 {json_file}")

    print()
    if total_v4 == 0:
        print("No v4 files found.")
        return 0

    print(
        f"Validated {total_v4} v4 file(s): "
        f"{'0 errors' if total_errors == 0 else str(total_errors) + ' error(s)'}"
    )
    return 0 if total_errors == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
