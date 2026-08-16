# Development Roadmap

This document defines the planned development order of the framework.

The roadmap is intentionally high-level.

It defines:
- development phases;
- architectural scope of each phase;
- major dependencies between phases;
- stabilization criteria.

It does NOT define implementation details.

Implementation decisions must be based on the current source code and discussed before changes are made.

---

## PHASE 1 — Runtime

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

### Dependencies

None.

### Exit criteria

- Node ownership and lifetime rules are clear.
- NodeTree is the authoritative owner of live nodes.
- Node identity / NodeId invariants are stable.
- Traversal semantics are defined.
- Mutation during traversal/callbacks is safe and predictable.
- Node attach/detach/reparent behavior is defined.
- UIManager and NodeTree responsibilities are clearly separated.
- PanelNode follows the runtime contracts.
- No unresolved runtime issue blocks later phases.

---

## PHASE 2 — Layout

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

### Exit criteria

- Measure/Arrange contract is stable.
- Content-box / border-box semantics are defined.
- Parent-child layout constraints are predictable.
- Layout invalidation is defined.
- StackPanel works correctly.
- Grid works correctly.
- Alignment and positioning semantics are defined.
- Absolute positioning does not violate runtime ownership rules.

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

### Exit criteria

- Hit-testing semantics are defined.
- Input state ownership is clear.
- Focus semantics are stable.
- Pointer capture semantics are stable.
- Event propagation semantics are stable.
- Mutation during event dispatch is safe.
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

ControlNode must not be introduced merely for architectural symmetry.

Its introduction must be justified by responsibilities that cannot be expressed cleanly by Node or existing component types.

### Exit criteria

- Component responsibilities are clearly separated.
- Interactive behavior does not leak into unrelated nodes.
- ControlNode, if introduced, has a justified responsibility boundary.
- Basic components use existing runtime/layout/input contracts without bypassing them.

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
- relevant parts of Phase 4

### Exit criteria

- Modal stack semantics are defined.
- Overlay ownership and lifecycle are clear.
- Modal focus behavior is predictable.
- Focus restoration is stable.
- Modal interaction respects hit-testing and event boundaries.
- Navigation does not introduce ownership inconsistencies.

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
- Phase 4
- Phase 5 where rendering depends on overlays/modal behavior

### Notes

RenderContext and a second backend are optional architectural directions.

They must not be introduced unless the current rendering architecture demonstrates a concrete need for them.

### Exit criteria

- Rendering responsibilities are clearly separated from runtime ownership.
- Clipping semantics are defined.
- Resource lifetime is defined.
- SDL-specific dependencies are isolated where practical.
- Additional rendering abstraction is introduced only where justified.
