# Architecture Decision Records

## ADR-001: JSON as Single Source of Truth
**Date**: 2025-01
**Status**: Accepted
**Decision**: All game data stored as JSON. No hardcoded content in C++.
**Rationale**: Enables designers to modify game without recompile. Supports version control of game content.

## ADR-002: ECS Architecture
**Date**: 2025-01
**Status**: Accepted
**Decision**: Use Entity-Component-System pattern for game objects.
**Rationale**: Decouples data (components) from logic (systems). Enables flexible entity composition.

## ADR-003: ImGui for All Editors
**Date**: 2025-02
**Status**: Accepted
**Decision**: All editor UI built with ImGui + ImNodes.
**Rationale**: Native C++ integration, no web dependencies, fast iteration, immediate mode simplicity.

## ADR-004: IGraphRenderer Adapter Pattern
**Date**: 2025-06 (Phase 27)
**Status**: Accepted
**Decision**: All graph editors implement `IGraphRenderer` for TabManager integration.
**Rationale**: Uniform tab lifecycle (Open, Save, Close, IsDirty) across all editor types.

## ADR-005: Coordinate Transform (Phase 29 Fix)
**Date**: 2025-09 (Phase 29)
**Status**: Accepted
**Decision**: ScreenToCanvas = `(screen - canvasOrigin - offset) / zoom` (NOT multiplying offset by zoom).
**Rationale**: The formula `(screen - canvasOrigin - offset*zoom)/zoom` was a critical bug causing node selection offset at non-1.0 zoom.

## ADR-006: Condition Presets Embedded in Graph JSON (Phase 24)
**Date**: 2025-07 (Phase 24)
**Status**: Accepted
**Decision**: Condition presets stored inside graph JSON (v4 schema), not as external files.
**Rationale**: Graphs become self-contained. No external file dependency. Simpler sharing.

## ADR-007: C++14 Compliance
**Date**: 2025-03
**Status**: Accepted
**Decision**: All new code must be C++14 compliant (no C++17 features like structured bindings).
**Rationale**: Compatibility with older compilers and Visual Studio 2019 default settings.

## ADR-008: Centralized Minimap (Phase 37)
**Date**: 2026-01 (Phase 37)
**Status**: Accepted
**Decision**: All graph editors use `CanvasMinimapRenderer` for minimap, not individual implementations.
**Rationale**: Consistent UX, single bug fix location, DRY principle.
