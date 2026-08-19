# Development Roadmap

This document defines the planned development order of the framework.

The roadmap is intentionally high-level. It defines development phases,
architectural scope, major dependencies, and stabilization criteria.

Implementation decisions must be based on the current source code and
explicitly documented architectural decisions. The source code remains the
source of truth for current behavior.

---

## Current Development Status

### Current Phase

**Phase 2 — Layout (source-level complete)**

### Current branch

```text
main
```

### Previous completed phase

**Phase 1 — Runtime**

Phase 1 ownership, lifecycle, traversal and deferred mutation contracts are
accepted as the active runtime baseline.

### Phase 2 result

Phase 2 has completed its planned source-level layout migration. The framework
now owns the layout proposal → measurement → desired-size → final-geometry
pipeline, layout scheduling and invalidation.

The completed Phase 2 scope is:

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

A full Grid or full CSS/Flexbox model is not required for Phase 2. Existing
Grid code remains deferred and must not force the architecture to grow around
it.

### Validation policy

Compilation, runtime tests and full build validation are intentionally deferred
until the end of Phase 6 by project decision. During Phases 1–6, implementation
is validated through source inspection, architectural review and documented
numerical cases. This is intentional and must not be interpreted as an omitted
Phase 2 task.

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

**Completed at source level.**

### Architectural result

```text
Node
  → common runtime/component state and layout properties

PanelNode
  → structural child ownership

LayoutManager
  → closed layout orchestration and algorithms

Layout constraints subsystem
  → framework-owned interpretation of size/min/max

NodeTree
  → ownership, lifecycle, mutation and layout scheduling
```

The client does not need to know about:

```text
LayoutManager
layout queue
layout invalidation
internal measurement/arrangement lifecycle
constraint resolution
```

### Completed built-in layout scope

```text
Horizontal / Vertical one-dimensional flow
Text measurement with width-sensitive proposals
Size / min / max
Padding / border
Position / position mode
Alignment
Gap
Visibility / layout participation
Absolute-child separation from normal Linear flow
Framework-owned layout invalidation
```

### Explicit non-goals

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

### Constraint semantics

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

### Client contract

The client configures framework-owned components. Framework components may
provide internal content measurement, but layout scheduling, invalidation and
sibling/parent placement remain framework responsibilities.

### Legacy source policy

```text
src/components/*
```

is obsolete and must not be modified as part of the Phase 2 architecture. Its
old Label/Button implementations may be inspected only as historical evidence.

`GridNode` and `ControlNode` are deferred/legacy concerns unless a concrete
later-phase requirement requires revisiting them.

### Phase 2 exit criteria

The following source-level criteria are complete:

- effective measurement proposal semantics are stable;
- final size constraint semantics are stable;
- content-box / border-box semantics are stable;
- framework-owned one-dimensional layout is stable;
- width-dependent text measurement is integrated;
- compound content measurement follows the framework pipeline;
- alignment/gap semantics are stable;