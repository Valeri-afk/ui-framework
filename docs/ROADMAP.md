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

**Phase 5 — Component Model**

Phase 4 — Rendering / Backend is completed at source level on the Phase 4
baseline. Compilation, automated tests, runtime validation and full-build
validation remain intentionally deferred until the end of Phase 6.

### Current branch

```text
phase5-components
```

The current Phase 5 component architecture is consolidated in:

```text
docs/PHASE5_COMPONENT_ARCHITECTURE_CHECKPOINT.md
docs/COMPONENT_DESIGN_GUIDE.md
docs/PHASE5_COMPONENT_CATALOG.md
```

These are the current Phase 5 component references. Superseded architecture
notes must not become competing sources of truth.

### Previous completed phases

**Phase 1 — Runtime**

**Phase 2 — Layout**

**Phase 3 — Input / Events**

**Phase 4 — Rendering / Backend**

Phase 1 ownership, lifecycle, traversal and deferred mutation contracts are
accepted as the active runtime baseline.

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

The client does not need to know about LayoutManager, layout queues,
layout invalidation, internal measurement/arrangement lifecycle or constraint
resolution.

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

A full Grid or CSS/Flexbox model is not part of the current scope.

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

### Legacy source policy

Old layout/component implementations are historical material only and must
not be treated as active architecture.

### Validation policy

Compilation, runtime tests and full build validation remain intentionally
deferred until Phase 6.

---

## PHASE 3 — Input / Events

### Status

**Completed at source level.**

### Scope

- InputManager
- hit-test
- focus
- capture
- EventDispatcher
- keyboard

### Completed source-level scope

The current implementation includes mouse input, hover, click generation,
pointer capture, drag state, focus, keyboard targeting, NodeId-based event
propagation, tunneling/target/bubbling and mutation-safe dispatch behavior.

Modal-root filtering currently present in input routing is retained as Phase 6
preparation and does not make Modal a Phase 5 implementation.

### Explicit non-goals

- tab-order focus navigation;
- text input / IME;
- accelerator or key-chord subsystems;
- higher-level drag-and-drop transfer semantics;
- application-specific control semantics.

---

## PHASE 4 — Rendering / Backend

### Status

**Completed at source level.**

### Architectural result

```text
Layout final geometry
        ↓
actualPosition / actualSize
        ↓
render traversal
        ↓
renderer state isolation / clipping
        ↓
SDL3 backend
```

Top-level presentation ordering is coordinated through roots, overlays and
modal handling. The framework is SDL3-only.

Renderer-bound resources remain local to the node/component that owns them.
No generic resource manager was introduced. Animation remains a future
state-change mechanism rather than a separate mandatory subsystem.

### Explicit non-goals

- forcing a second backend;
- `RenderContext` without concrete need;
- global `zIndex` / stacking contexts;
- generic ResourceManager / AnimationManager;
- transform subsystem;
- scroll subsystem;
- persistent text-to-texture cache;
- full offscreen resource system;
- redesigning layout or input contracts for rendering convenience.

### Validation status

Phase 4 remains source-level complete. Compilation, automated tests, runtime
validation and full-build validation remain deferred to Phase 6.

---

## PHASE 5 — Component Model

### Status

**Current — component architecture checkpoint complete; minimal standard component layer implemented in the current branch.**

### Current architecture references

```text
docs/COMPONENT_DESIGN_GUIDE.md
docs/PHASE5_COMPONENT_ARCHITECTURE_CHECKPOINT.md
docs/PHASE5_COMPONENT_CATALOG.md
```

### Active standard component scope

```text
Button
ToggleButton
Menu / MenuItem
TabControl / TabItem
```

These are standard framework UI components with distinct generic contracts.

### Deferred components

```text
List
Scroll / ScrollArea
Modal
IconButton
```

`List` is deferred until it has a distinct generic contract beyond a thin
alias over an existing layout primitive.

`Scroll / ScrollArea` remains a separate framework architecture topic. Its
final design must account for content extent, viewport extent, offset/range,
clipping, coordinate conversion, layout integration and input/hit-test
integration before implementation.

`Modal` is deferred until Phase 6 modality infrastructure is complete. The
legacy Modal implementation is removed from the active component API; its
useful behavioral requirements are preserved in `PHASE6_MODALITY_REQUIREMENTS.md`.

`IconButton` is deferred until a stable graphics/icon primitive and resource
contract exists.

### Component architecture constraints

Components do not redefine:

```text
NodeTree ownership/lifecycle
layout orchestration
hit-test traversal
input/event dispatch
focus/capture
render traversal
clipping
framework-owned scrolling mechanics
```

Components own semantic state, presentation and intentional specialized
composition.

`PanelNode` is a structural/layout primitive, not a universal visual/content
base. A component becomes a `PanelNode` when child ownership/composition and
layout flow are actually part of its responsibility.

The framework does not use a universal arbitrary `content` model.

Shared base classes must emerge from concrete repeated semantics rather than
from hierarchy symmetry with another UI toolkit.

### Dependencies

- Phase 1
- Phase 2
- Phase 3
- Phase 4

### Validation policy

Phase 5 remains source-level development. Compilation, automated tests,
runtime validation and full-build validation remain deferred to Phase 6.

---

## PHASE 6 — Modal / Navigation / Validation

### Status

**Planned.**

### Scope

- ModalManager hardening and final integration;
- modal stacking semantics;
- focus restoration policy;
- keyboard navigation and focus traversal;
- navigation semantics;
- overlay integration;
- integration of the component layer with modal/navigation behavior;
- compilation;
- automated tests;
- runtime validation;
- full accumulated-phase validation.

### Goal

Establish higher-level interaction/navigation semantics and perform the full
technical validation intentionally deferred from Phases 1–5.

### Existing modal infrastructure

`ModalManager` already exists as Phase 6 preparation. Its current source is
not treated as the final Phase 6 contract until the modality architecture is
completed.

### Dependencies

- Phase 1
- Phase 2
- Phase 3
- Phase 4
- Phase 5

---

## Development Order

```text
Phase 1 — Runtime
        ↓
Phase 2 — Layout                    [source-level complete]
        ↓
Phase 3 — Input / Events            [source-level complete]
        ↓
Phase 4 — Rendering / Backend       [source-level complete]
        ↓
Phase 5 — Component Model           [current]
        ↓
Phase 6 — Modal / Navigation        [full build/test validation]
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
The roadmap defines intended development direction, not assumed existing
behavior.
