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

The current Phase 5 component work is consolidated in:

```text
docs/COMPONENT_DESIGN_GUIDE.md
docs/PHASE5_COMPONENT_ARCHITECTURE_CHECKPOINT.md
docs/PHASE5_COMPONENT_CATALOG.md
docs/PHASE5_COMPONENT_SET.md
docs/PHASE5_FINAL_CHECKPOINT.md
```

The Phase 6 handoff is captured in:

```text
docs/PHASE6_SCOPE_CANDIDATES.md
docs/PHASE6_MODALITY_REQUIREMENTS.md
docs/SCROLL_ARCHITECTURE.md
```

These documents are phase-specific sources of truth. `ARCHITECTURE.md` remains
the large architectural document and is not automatically rewritten during
Phase 5 work.

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

Top-level presentation ordering may later involve roots, overlays and modal
handling, but these remain framework-level concerns rather than standard
component responsibilities.

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

## PHASE 5 — Component Development

### Status

**Current — standard component set established; final validation and Phase 6 handoff remain.**

Phase 5 is intentionally a component-development phase. It should not absorb
unresolved framework subsystems into individual components.

### Active standard component set

```text
Button
ToggleButton
Menu / MenuItem
TabControl / TabItem
Checkbox
RadioButton
Slider
Dropdown
```

These are standard framework UI components with distinct generic contracts.

### Deferred components

```text
TextField / Input
Image
List
IconButton
Scroll / ScrollArea
Modal
```

`TextField / Input` is deferred until a proper text-input/editing path exists,
including future composition/IME, caret, selection and clipboard semantics.

`Image` is deferred until a framework resource/texture ownership contract
exists. The current `primitives` layer remains deliberately separate from
resource management.

`List` is deferred until it has a distinct generic contract beyond existing
panel/layout primitives.

`IconButton` is deferred until a stable graphics/icon/resource contract exists.

`Scroll / ScrollArea` remains a separate framework architecture topic. Its
final design must account for content extent, viewport extent, offset/range,
clipping, coordinate conversion, layout integration and input/hit-test
integration before implementation.

`Modal` is deferred until Phase 6 modality infrastructure is complete. The
legacy Modal implementation is removed from the active component API; useful
behavioral requirements are preserved in `PHASE6_MODALITY_REQUIREMENTS.md`.

### Cancelled standalone components

```text
Paper
Label
Card
```

These remain client-level composition/style concepts rather than independent
framework runtime components.

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
resource ownership infrastructure
text-input infrastructure
```

Components own semantic state, presentation and intentional specialized
composition.

`PanelNode` is a structural/layout primitive, not a universal visual/content
base. A component becomes a `PanelNode` when child ownership/composition and
layout flow are actually part of its responsibility.

The framework does not use a universal arbitrary `content` model.

Shared base classes must emerge from concrete repeated semantics rather than
from hierarchy symmetry with another UI toolkit.

### Phase 5 completion procedure

Before declaring Phase 5 complete:

1. Verify the active component set against `COMPONENT_DESIGN_GUIDE.md`.
2. Verify no removed legacy abstractions remain referenced by active source.
3. Verify every retained source file has a current architectural role.
4. Review the Phase 5 architecture/component documents.
5. Manually review `ARCHITECTURE.md`; do not rewrite it automatically.
6. Promote only justified framework-level requirements into the Phase 6 scope.

### Dependencies

- Phase 1
- Phase 2
- Phase 3
- Phase 4

### Validation policy

Phase 5 remains source-level component development. Compilation, automated tests,
runtime validation and full-build validation remain intentionally deferred until
Phase 6.

---

## PHASE 6 — Framework Infrastructure and Validation

### Status

**Planned. Final scope to be confirmed during the Phase 5 handoff.**

Phase 6 is not predefined as only a Modal or navigation phase. Its scope is
derived from real framework-level requirements discovered during component
development and the final architecture review.

### Current scope candidates

```text
Modality
Scrolling
Text input / editing
Image / resource management
Overlay / popup infrastructure, if concretely required
Compilation / automated tests / runtime validation / full build
```

See `PHASE6_SCOPE_CANDIDATES.md` for the rationale and current candidate
responsibility boundaries.

### Modality candidate

Potential responsibilities include:

```text
active modal registration
modal stack/order
exclusive hit-testing
input routing restrictions
focus/capture policy
Escape routing
background interaction blocking
focus restoration
optional scroll-lock policy
modal/overlay rendering order
```

Reference: `PHASE6_MODALITY_REQUIREMENTS.md`.

### Scrolling candidate

Potential responsibilities include:

```text
viewport bounds
content extent
scroll offset/range
coordinate conversion
clipping
wheel/gesture/drag input routing
hit-test through clipped/offset content
layout integration
nested scrolling policy
```

Reference: `SCROLL_ARCHITECTURE.md`.

### Text input candidate

Potential responsibilities include:

```text
text input events
composition / IME
caret
selection
editing commands
clipboard
focus/input lifecycle
```

### Image/resource candidate

Potential responsibilities include:

```text
resource ownership/lifetime
shared texture references
renderer/resource relationship
TextureHandle or equivalent
source rectangle
fit/crop/scale
opacity/tint/flip/rotation
```

This must remain separate from `primitives`.

### Overlay candidate

Overlay infrastructure should only be promoted into Phase 6 if a concrete
component requirement cannot be satisfied by the current NodeTree structure,
for example global popup ordering, escaping parent clipping, or outside-click
handling across unrelated subtrees.

### Goal

Build only the missing framework infrastructure justified by Phase 5
component requirements and perform the full validation intentionally deferred
from earlier phases.

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
Phase 5 — Component Development     [current]
        ↓
Phase 5 final document/architecture review
        ↓
Phase 6 scope confirmation
        ↓
Phase 6 — Framework Infrastructure + Validation
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
