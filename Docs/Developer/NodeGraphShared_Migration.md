NodeGraphShared Migration Notes

Overview

This document summarizes the migration work performed to centralize node graph rendering, command execution, and serialization between the Behavior Tree editor (runtime/editor) and the standalone Blueprint Node Graph panel.

Goals

- Avoid duplicated rendering and serialization logic.
- Reuse the same undo/redo command infrastructure where possible.
- Provide adapters so both editors can call into shared code without tightly coupling their internal command stacks.

What was added

- `Source/NodeGraphShared/Serializer.h`
  - Inline serializer for `BehaviorTreeAsset` to produce the JSON schema used by saves/autosaves.

- `Source/NodeGraphShared/Renderer.h`
  - `RenderNodeVisual(...)` — generic node visual renderer (title, pins, body) that accepts explicit position/size and is editor-agnostic.
  - `RenderBTNode(...)` — adapter for rendering `BTNode` instances using layout data.
  - `RenderBTNodeConnections(...)` — shared bezier link drawing for BT nodes.

- `Source/NodeGraphShared/CommandAdapter.h`
  - Adapter that wraps the `BTCommandStack` used by the BehaviorTree editor and executes `AddNodeCommand`, `DeleteNodeCommand`, `ConnectNodesCommand`, `DisconnectNodesCommand`, `MoveNodeCommand` through it. Returns created node id when available.

- `Source/NodeGraphShared/BlueprintAdapter.h`
  - Adapter for the `Blueprint::CommandStack` (standalone node graph editor). Provides `CreateNode`, `DeleteNode`, `MoveNode`, `ConnectNodes`, `DisconnectNodes`, `DuplicateNode` helpers that execute Blueprint command classes.

Changes made

- `BehaviorTreeDebugWindow.cpp`
  - Delegates node rendering to `NodeGraphShared::RenderBTNode`.
  - Uses `NodeGraphShared::CommandAdapter` for create/delete/connect/disconnect/move actions in editor mode so undo/redo works.
  - Delegates serialization to `NodeGraphShared::SerializeBehaviorTreeToJson`.

- `BlueprintEditor/NodeGraphPanel.cpp`
  - Uses `NodeGraphShared::BlueprintAdapter` to create/connect/duplicate nodes through the Blueprint command stack, centralizing command execution.
  - Preserves comment-node editing and clipboard/paste flows.

Notes on design choices

- Incremental migration: The adapters allow shared code usage while keeping each editor's native command stack intact. This reduces risk and keeps undo/redo consistent with each editor's user expectations.

- ImNodes link creation/registration logic remains in each panel where interactive events (IsLinkCreated / IsLinkDestroyed) are processed. The adapters are used to execute the commands created as a result of those events.

Testing

- The repository builds successfully after the changes.
- Visual/interaction tests (create, connect, move, delete, undo/redo) should be performed manually in both the Behavior Tree editor and the Blueprint Node Graph panel to validate parity.

Follow-ups

- Further consolidate rendering in `NodeGraphPanel` to call `RenderNodeVisual` where possible (non-comment nodes) to remove duplicate rendering behavior.
- Consider exposing created node ids from Blueprint `CreateNodeCommand` so callers can immediately get the created node id without querying the graph.
- Add unit tests for command adapter behaviors if a headless test harness exists.

