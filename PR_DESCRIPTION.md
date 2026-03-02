PR title

Refactor: centralize node graph renderer and command adapters; expose created node ID; auto-select/center on create

Branch

feature/nodegraph-shared-migration

Summary

- Centralize rendering, serialization and command execution for node graphs so both BehaviorTree runtime/editor and the Blueprint NodeGraph panel reuse the same logic.
- Add adapters to execute commands through each editor's CommandStack while preserving undo/redo.
- Expose created node ID from CreateNodeCommand so callers can select and center the view after node creation.
- Auto-select and center the editor view on newly created nodes in the Blueprint NodeGraph panel.
- Add a unit test validating CreateNodeCommand writes the created node ID.
- Add migration notes in docs.

Key changes

- New shared utilities:
  - `Source/NodeGraphShared/Renderer.h` — generic `RenderNodeVisual`, `RenderBTNode`, `RenderBTNodeConnections`.
  - `Source/NodeGraphShared/CommandAdapter.h` — adapter for BehaviorTree editor command stack (Add/Delete/Move/Link).
  - `Source/NodeGraphShared/BlueprintAdapter.h` — adapter for Blueprint editor command stack (Create/Delete/Move/Link/Duplicate), returns created node id.
  - `Source/NodeGraphShared/Serializer.h` — centralized serializer used by the BT editor.

- Editor updates:
  - `Source/AI/BehaviorTreeDebugWindow.cpp` — delegate rendering to shared renderer; use `CommandAdapter` for create/delete/move/connect to preserve undo/redo; autosave preserved.
  - `Source/BlueprintEditor/NodeGraphPanel.cpp` — create/connect/duplicate via `BlueprintAdapter`; automatically select and center created node.

- Command system:
  - `Source/BlueprintEditor/BPCommandSystem.h/.cpp` — `CreateNodeCommand` accepts `int* outCreatedId` to return created node id after execution.

- Tests & docs:
  - `Tests/Editor/ClipboardCopyPasteTest.cpp` — appended `TestJ_CreateNodeCommand` to validate created id behavior.
  - `Docs/Developer/NodeGraphShared_Migration.md` — migration notes and next steps.

Files changed/added (high level)

- Added:
  - `Source/NodeGraphShared/Renderer.h`
  - `Source/NodeGraphShared/CommandAdapter.h`
  - `Source/NodeGraphShared/BlueprintAdapter.h`
  - `Source/NodeGraphShared/Serializer.h`
  - `Docs/Developer/NodeGraphShared_Migration.md`

- Modified:
  - `Source/AI/BehaviorTreeDebugWindow.cpp`
  - `Source/BlueprintEditor/NodeGraphPanel.cpp`
  - `Source/BlueprintEditor/BPCommandSystem.h`
  - `Source/BlueprintEditor/BPCommandSystem.cpp`
  - `Tests/Editor/ClipboardCopyPasteTest.cpp`

Testing checklist (manual)

- Build: compilation success for desktop targets.
- BehaviorTree editor (editor mode):
  - Create node -> node appears, selected, view recenters.
  - Move node -> undo & redo revert/restore correctly.
  - Delete node -> undo restores the node.
  - Duplicate node -> creates duplicate; undo removes it.
  - Connect/disconnect -> undo/redo works.
  - Autosave produces valid JSON.

- Blueprint NodeGraph panel:
  - Create node from context menu -> node selected & centered.
  - Duplicate node -> works and supports undo/redo.
  - Create links via drag -> commands executed by `BlueprintAdapter`.

- Tests:
  - `Tests/Editor/ClipboardCopyPasteTest` should pass (includes `TestJ_CreateNodeCommand`).

Commands to push + open PR (local)

1) Create branch and commit

    git checkout -b feature/nodegraph-shared-migration
    git add .
    git commit -m "Refactor: centralize node graph renderer & adapters; expose created node id; auto-select/center on create"

2) Push branch

    git push -u origin feature/nodegraph-shared-migration

3) Open PR (GitHub CLI)

    gh pr create --title "Refactor: centralize node graph renderer and adapters; expose created node ID; auto-select/center on create" --body "<PASTE PR DESCRIPTION FROM THIS FILE>" --base main --head feature/nodegraph-shared-migration

If `gh` is unavailable, open the PR via GitHub web UI selecting this branch and `main` as base.

Suggested reviewers

- UI/Editor owner
- AI/BT systems lead
- Owner of `BlueprintEditor` command system

Labels suggested

- `area/editor`, `refactor`, `needs-review`

Rollback plan

- Revert the merge commit on `main` if needed:

    git checkout main
    git revert <merge-commit-hash>

Notes / follow-ups

- Consider exposing a generic command result API on `CommandStack` for non-pointer returns.
- Consolidate more rendering in `NodeGraphPanel` to fully use `RenderNodeVisual` (keep comment node logic as currently implemented).
- Add headless unit tests for adapters if a test harness exists.



