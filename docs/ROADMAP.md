# Development Roadmap

This document defines the planned development order of the framework.

The roadmap is intentionally high-level. It defines development phases,
architectural scope, major dependencies, and stabilization criteria. Detailed
Phase 2 implementation context lives in `PHASE2_HANDOFF.md` and the Phase 2
architecture documents.

Implementation decisions must be based on the current source code and
explicitly documented architectural decisions. The source code remains the
source of truth for current behavior.

---

## Current Development Status

### Current Phase

**Phase 2 — Layout**

### Current branch

```text
phase2-layout-migration
```

### Previous completed phase

**Phase 1 — Runtime**

Phase 1 ownership, lifecycle, traversal and deferred mutation contracts are
accepted as the active runtime baseline.

### Current Phase 2 direction

Phase 2 is migrating toward a **framework-owned closed layout engine**.
Clients configure supported framework components and layout properties but do
not implement layout strategies, `measure/arrange`, or layout invalidation.

The first useful layout scope is deliberately small:

```text
Text measurement
container composition
one-dimensional flow
size / min / max
padding / border
position / position mode
alignment
gap
visibility / layout participation
automatic framework-owned invalidation
```

A full Grid or full CSS/Flexbox model is not required for the first Phase 2
implementation. Existing Grid code is deferred and must not force the
architecture to grow around it.

### Current implementation checkpoint

The latest implementation work has established an internal framework-owned
constraint subsystem:

```text
include/ui_framework/core/layout_constraints.hpp
src/core/layout_constraints.cpp
```

with the conceptual operations:

```text
resolveMeasurementProposal()
resolveFinalSize()
```

The Linear implementation has begun using the same constraint semantics, but
integration with the central `LayoutManager` is not yet complete.

The next implementation task is to make `LayoutManager` the single
orchestrator of the complete proposal → measurement → desired size → final
geometry pipeline, while removing duplicated constraint interpretation from
layout algorithms.

### Validation policy

Compilation and runtime tests are intentionally deferred until the end of
Phase 6 by project decision. During Phases 1–6, implementation is validated
through source inspection, architectural review and documented numerical
cases. This is intentional and must not be interpreted as an omitted test
step in the current phase.

---

## PHASE 1 — Runtime

### Status

**Completed / accepted architecturally.**

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

---

## PHASE 2 — Layout

### Status

**Active — migration in progress.**

### Architectural target

```text
Node
  → common runtime/component state

PanelNode
  → structural child ownership

LayoutManager
  → closed layout orchestration and algorithms

Layout constraints subsystem
  → framework-owned interpretation of size/min/max

NodeTree
  → ownership, lifecycle, mutation and layout scheduling
```

The client should not need to know about:

```text
LayoutManager
layout queue
layout invalidation
measure/arrange lifecycle
constraint resolution
```

### Initial built-in layout

The first layout family is a one-dimensional Linear/Stack-style layout:

```text
Horizontal
Vertical

Gap

Main axis:
    Start
    Center
    End
    SpaceBetween when required

Cross axis:
    Start
    Center
    End
    Stretch under explicitly defined measurement semantics
```

### Explicit non-goals for the first implementation

- full Grid system
- full CSS Flexbox compatibility
- flex wrapping
- flex grow/shrink/basis
- public `LayoutStrategy`
- public client-side `Measure/Arrange`
- CSS-style dynamic property system
- WPF dependency-property system
- universal Margin semantics
- ControlNode solely for hierarchy symmetry

These may be reconsidered only after a concrete requirement demonstrates the
need.

### Constraint semantics checkpoint

The current provisional rules are:

```text
Fixed size
    → constrains measurement proposal and final geometry

Max size
    → can narrow measurement proposal and constrains final geometry

Min size
    → constrains final geometry; does not automatically narrow intrinsic
      measurement proposal

Auto
    → no local explicit size; intrinsic measurement / parent layout decides

Padding/border
    → translate between outer box and content measurement
```

This distinction is required for width-dependent content such as wrapped
text.

### Client contract target

The historical problem being resolved is the client/framework layout contract.
The target is explicitly **not** to give clients a custom layout strategy
interface and then rely on clients to call invalidation correctly.

The client should instead configure framework-owned components. Framework
components may provide their own internal content measurement, but layout
scheduling and sibling/parent placement remain framework responsibilities.

### Legacy source policy

```text
src/components/*
```

is obsolete and must not be modified during Phase 2. Its old Label/Button
implementations may be inspected only as historical evidence.

`GridNode` and `ControlNode` are not required to define the first Phase 2
architecture. They remain deferred/legacy concerns unless a concrete source
integration issue requires revisiting them.

### Phase 2 exit criteria

- effective measurement proposal semantics are stable;
- final size constraint semantics are stable;
- content-box / border-box semantics are stable;
- framework-owned one-dimensional layout is stable;
- text measurement works with width-dependent wrapping;
- Button-like compound content can measure correctly;
- alignment/gap semantics are stable;
- nested containers work by the same framework pipeline;
- visibility participation is defined;
- absolute positioning semantics are defined if retained in the phase;
- invalidation remains framework-owned;
- clients do not implement layout algorithms;
- clients do not call layout invalidation APIs;
- runtime ownership/lifecycle invariants remain untouched;
- numerical acceptance cases are satisfied.

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

Establish a predictable input and event system on top of the stabilized runtime
and layout systems.

### Dependencies

- Phase 1
- Phase 2

---

## PHASE 4 — Component Model

### Scope

- ControlNode only if justified
- Button
- Toggle
- Text
- Image
- Scroll
- other components as required

### Goal

Build reusable UI components on top of the stabilized runtime, layout and
input systems.

Existing legacy component code must not be treated as the architecture.

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

RenderContext and a second backend remain optional architectural directions.

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

Later phases may be analyzed when necessary to validate an earlier
architectural decision, but implementation remains focused on one primary
phase at a time.

---

## Active Development Principle

The existence of a file or module does not imply that it is part of the active
implementation scope.

Legacy, deprecated, experimental, or currently unused code must be verified
against the source before being treated as an architectural contract.

The current source code remains the source of truth for existing behavior.
The roadmap defines the intended development direction, not assumed existing
behavior.
