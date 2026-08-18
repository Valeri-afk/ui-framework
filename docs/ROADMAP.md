# Development Roadmap

This document defines the planned development order of the framework.

The roadmap is intentionally high-level. It defines development phases, architectural scope, major dependencies, and stabilization criteria. It does not define implementation details.

Implementation decisions must be based on the current source code and discussed before changes are made. The source code remains the source of truth for current behavior.

---

## Current Development Status

### Current Phase

**Phase 2 — Layout**

### Status

**Phase 1 — Runtime is accepted as the active `main` source baseline.**

The Phase 1 ownership, lifecycle, traversal, and deferred-mutation contracts have been promoted into the active source tree. `phase1-worktree/` is retained only as a historical implementation snapshot and is not part of the active build.

Compilation and runtime verification remain a later project-level validation stage. This does not prevent architectural implementation work in the active phase.

### Previous Completed Phase

**Phase 1 — Runtime**

### Next Phase

**Phase 2 — Layout** is the primary active architectural scope.

Later phases may be analyzed when necessary to validate architectural decisions, but they should not be implemented prematurely.

---

## PHASE 1 — Runtime

### Status

**Completed / accepted.**

### Scope

- NodeTree
- UIManager
- Node
- PanelNode
- lifecycle
- traversal
- mutation

### Goal

Stabilize the runtime foundation of the framework.

### Exit Criteria

- Node ownership and lifetime rules are clear.
- NodeTree is the authoritative owner of live nodes.
- Node identity / NodeId invariants are stable.
- Traversal semantics are defined.
- Mutation during traversal and callbacks is safe and predictable.
- Node attach and framework-owned remove behavior are defined.
- Reparenting is explicitly deferred as a future capability.
- UIManager and NodeTree responsibilities are clearly separated.
- PanelNode follows the runtime contracts.
- No unresolved Phase 1 architectural issue blocks Phase 2.

Phase 1 exit criteria are architecturally satisfied. Build/runtime verification remains a later project-level validation stage.

---

## PHASE 2 — Layout

### Status

**Active.**

### Scope

- LayoutManager
- Measure
- Arrange
- StackPanel
- Grid
- alignment
- absolute positioning
- invalidation

### Goal

Establish a predictable layout system on top of the stabilized runtime.

### Dependencies

- Phase 1

### Exit Criteria

- Measure/Arrange contract is stable.
- Content-box / border-box semantics are defined.
- Parent-child layout constraints are predictable.
- Layout invalidation semantics are defined.
- StackPanel works correctly.
- Grid works correctly.
- Alignment and positioning semantics are defined.
- Absolute positioning does not violate runtime ownership rules.
- Layout does not bypass NodeTree lifecycle and mutation invariants.

---

## PHASE 3 — Input / Events

### Scope

- InputManager
- hit-test
- focus
- capture
- EventDispatcher
- keyboard

### Goal

Establish a predictable input and event system on top of the stabilized runtime and layout systems.

### Dependencies

- Phase 1
- Phase 2

### Exit Criteria

- Hit-testing semantics are defined.
- Input state ownership is clear.
- Focus semantics are stable.
- Pointer capture semantics are stable.
- Event propagation semantics are stable.
- Mutation during event dispatch is safe and predictable.
- Keyboard input integrates with focus.
- Input does not violate NodeTree lifecycle invariants.

---

## PHASE 4 — Component Model

### Scope

- ControlNode
- Button
- Toggle
- Text
- Image
- Scroll
- other components as required

### Goal

Build reusable UI components on top of the stabilized runtime, layout, and input systems.

### Dependencies

- Phase 1
- Phase 2
- Phase 3

### Notes

ControlNode must not be introduced merely for architectural symmetry. Its introduction must be justified by responsibilities that cannot be expressed cleanly by Node or existing component types.

Existing component code may be legacy, incomplete, or outside the active development scope. Such code must not be treated as a stable architectural contract without verification against the current source.

---

## PHASE 5 — Modal / Navigation

### Scope

- ModalManager
- focus restoration
- navigation
- overlays

### Goal

Establish higher-level interaction and navigation semantics.

### Dependencies

- Phase 1
- Phase 2
- Phase 3

Phase 4 is not a strict dependency unless a particular navigation or modal feature requires component-level behavior.

---

## PHASE 6 — Rendering / Backend

### Scope

- SDL rendering
- clipping
- resources
- optional RenderContext
- optional second backend

### Goal

Separate rendering concerns from the framework's core runtime where justified.

### Dependencies

- Phase 1
- Phase 2
- Phase 4 where component rendering requires it

RenderContext and a second backend are optional architectural directions and must not be introduced without a concrete need.

---

## Development Order

```text
Phase 1 — Runtime
        ↓
Phase 2 — Layout
        ↓
Phase 3 — Input / Events
        ↓
Phase 4 — Component Model
        ↓
Phase 5 — Modal / Navigation
        ↓
Phase 6 — Rendering / Backend
```

This is a dependency-oriented roadmap rather than a requirement to complete every phase strictly in isolation. Later phases may be investigated when necessary to validate an earlier architectural decision, but implementation remains focused on one primary phase at a time.

---

## Active Development Principle

The existence of a file or module does not imply that it is part of the active implementation scope.

Legacy, deprecated, experimental, or currently unused code must be verified against the source before being treated as an architectural contract.

The current source code remains the source of truth for existing behavior.

This roadmap defines where the framework is intended to go, not what the source is assumed to already implement.
