# Development Roadmap

This document defines the planned development order of the framework.

The roadmap is intentionally high-level.

It defines:
- development phases;
- architectural scope of each phase;
- major dependencies between phases;
- stabilization criteria.

It does NOT define implementation details.

The roadmap describes the intended development direction, not necessarily the
current state of the source code.

Implementation decisions must be based on the current source code and discussed
before changes are made.

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
- Mutation during traversal and callbacks is safe and predictable.
- Node attach, detach, and reparent behavior is defined.
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

Establish a predictable input and event system on top of the stabilized
runtime and layout systems.

### Dependencies

- Phase 1
- Phase 2

### Exit criteria

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

Build reusable UI components on top of the stabilized runtime, layout, and
input systems.

### Dependencies

- Phase 1
- Phase 2
- Phase 3

### Notes

ControlNode must not be introduced merely for architectural symmetry.

Its introduction must be justified by responsibilities that cannot be
expressed cleanly by Node or existing component types.

Existing component code may be legacy, incomplete, or outside the active
development scope. Such code must not be treated as a stable architectural
contract without verification against the current source.

### Exit criteria

- Component responsibilities are clearly separated.
- Interactive behavior does not leak into unrelated nodes.
- ControlNode, if introduced, has a justified responsibility boundary.
- Basic components use existing runtime, layout, and input contracts without
  bypassing them.
- Legacy component code has either been updated, replaced, or explicitly
  excluded from the active component architecture.

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

### Notes

Phase 4 is not a strict dependency unless a particular navigation or modal
feature requires component-level behavior.

Modal functionality may therefore be stabilized independently of the complete
Component Model.

### Exit criteria

- Modal stack semantics are defined.
- Overlay ownership and lifecycle are clear.
- Modal focus behavior is predictable.
- Focus restoration is stable.
- Modal interaction respects hit-testing and event boundaries.
- Navigation does not introduce ownership inconsistencies.
- Modal lifecycle does not bypass NodeTree mutation and lifetime rules.

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

### Notes

Rendering may depend on Phase 5 for specific overlay or modal rendering
behavior, but Phase 5 is not a general prerequisite for the rendering
architecture.

RenderContext and a second backend are optional architectural directions.

They must not be introduced unless the current rendering architecture
demonstrates a concrete need for them.

SDL is currently the rendering backend of the framework. SDL-specific code
should remain outside the framework's core architectural contracts where
practical.

### Exit criteria

- Rendering responsibilities are clearly separated from runtime ownership.
- Clipping semantics are defined.
- Resource lifetime is defined.
- SDL-specific dependencies are isolated where practical.
- Rendering does not bypass NodeTree lifecycle or ownership rules.
- Additional rendering abstraction is introduced only where justified.

---

## Development Order

The intended dependency order is:

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

However, this is a dependency-oriented roadmap rather than a requirement to
complete every phase strictly in isolation.

A later phase may be partially investigated when necessary to validate an
earlier architectural decision.

Implementation work should remain focused on one primary architectural phase
at a time.

---

## Active Development Principle

The existence of a file or module does not imply that it is currently part of
the active refactoring scope.

Legacy, deprecated, experimental, or currently unused code must be verified
against the source before being treated as an architectural contract.

The current source code remains the source of truth for existing behavior.

This roadmap defines where the framework is intended to go, not what the
current source code is assumed to already implement.
