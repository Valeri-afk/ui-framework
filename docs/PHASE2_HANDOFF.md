# Phase 2 Handoff / Current Context

> **Purpose:** canonical recovery document for continuing Phase 2 in a new chat.
> **Date:** 2026-08-19
> **Active branch:** `phase2-layout-migration`
> **Tests/build:** intentionally deferred until the end of Phase 6 by project decision.

## 1. Current project position

Phase 1 Runtime has been accepted as the active `main` baseline. Phase 2 Layout is the only active implementation phase.

The Phase 2 work is deliberately incremental. The objective is to move layout ownership from the Node inheritance hierarchy into the framework layout subsystem without rewriting the retained-mode runtime or NodeTree ownership model.

## 2. Historical architectural path

The project previously went through several layout models:

```text
1. One large Widget + separate layout engine
2. WPF-like separation of Node/component responsibilities
3. Single Node with client-defined layout algorithms
4. Measure/Arrange with client-owned algorithms + manual invalidation
5. Current migration toward framework-owned layout execution
```

The main reasons previous approaches were rejected were:

```text
- capability/base-class proliferation;
- framework-wide dependence on one universal Widget/Node;
- client knowledge of invalidation and layout contracts;
- custom measure/arrange code being easy to implement incorrectly;
- text measurement being awkward when treated as a separate client TextEngine service;
- attempting to make a CSS-style abstraction map directly onto C++ when the runtime semantics differ.
```

Legacy `src/components` is historical evidence only and must not be modified. In particular, old `Label`, `Button` and `Component` implementations are useful for understanding the desired content-measurement boundary but are not the implementation target.

## 3. Final architectural direction reached

The preferred target is:

> **Framework-owned retained-mode layout with a structural `Node` / `PanelNode` hierarchy and framework-provided layout components.**

The client configures components and properties. The framework owns layout execution and invalidation.

### `Node`

`Node` is the base runtime element.

It owns common:

```text
identity
parent/owner references
lifecycle state
common runtime state
enabled/visibility/focus-related state
size
min/max
padding
border
position / position mode
actual/desired geometry
events
render/update hooks
```

`Node` does not own children.

A generic client `Node` does not have to provide intrinsic measurement.

### `PanelNode`

`PanelNode` is the structural child-container base.

It owns:

```text
children
child attachment/removal
child traversal
structural container behavior
```

It should not require a client-defined layout algorithm in the final architecture.

### Framework components

Examples:

```text
Text
Button
Image
Stack/Linear panel
future Modal/dialog components
```

Client code may inherit from framework-provided components to customize behavior/state without implementing layout.

### `NodeTree`

`NodeTree` remains the runtime authority for:

```text
live-node ownership
identity registry
attach/detach
lifecycle
traversal
deferred mutation
layout scheduling
roots/overlays
```

It does not own layout algorithms.

### `LayoutManager`

`LayoutManager` becomes the framework-owned layout execution orchestrator:

```text
proposal generation
constraint resolution
content measurement dispatch
container layout
absolute placement when supported
arrangement
geometry commit
```

### `UIManager`

`UIManager` remains top-level subsystem orchestration:

```text
NodeTree
InputManager
ModalManager
LayoutManager
```

It does not own layout mathematics.

## 4. Client/framework contract

The final client contract should allow:

```text
create components
inherit from framework components
set framework properties
compose children through framework containers
implement component behavior/state
```

The client should not need to:

```text
implement measure()
implement arrange()
call markLayout()
call invalidateMeasure()
access LayoutManager
access NodeTree scheduling internals
operate a TextEngine service
implement container layout algorithms
```

There is intentionally no public `LayoutStrategy` / `CustomLayoutStrategy` in Phase 2.

A future narrow custom intrinsic-measurement extension (`proposal -> desired size`) may be considered only if a real use case requires it. Custom measurement remains conceptually distinct from custom layout.

## 5. Layout scope chosen for Phase 2

The initial built-in layout is intentionally small: a one-dimensional Linear/Stack model.

Supported conceptual properties:

```text
orientation: Horizontal | Vertical
gap
main-axis distribution:
    START
    CENTER
    END
    SPACE_BETWEEN
cross-axis alignment:
    START
    CENTER
    END
    STRETCH
```

Current common Node geometry candidates:

```text
size
min/max
padding
border
position
position mode
visibility
```

`margin` is deferred.

Grid is deliberately excluded from the first Phase 2 implementation path even though a legacy/current `GridNode` exists in the repository. Its presence must not force a generic strategy/property model.

Full CSS Flexbox compatibility is deferred:

```text
flex-grow
flex-shrink
flex-basis
wrap
order
full intrinsic-size semantics
```

`ControlNode` remains deferred and must not be added merely for hierarchy symmetry.

## 6. Axis semantics

The old single `Alignment` enum was found to mix two different semantics.

The current direction uses:

```text
MainAxisAlignment
    START
    CENTER
    END
    SPACE_BETWEEN

CrossAxisAlignment
    START
    CENTER
    END
    STRETCH
```

This keeps container distribution and cross-axis placement conceptually separate.

## 7. Constraint model

A critical architectural distinction was established:

```text
measurement proposal != final size constraint
```

Current provisional semantics:

```text
Fixed size
    → constrains measurement proposal and final geometry

Max size
    → can narrow measurement proposal and final geometry

Min size
    → constrains final geometry

Auto
    → no explicit local size; intrinsic measurement / parent layout determines size
```

Padding/border translate between border-box and content-box.

This distinction is required for proposal-dependent content such as wrapped text.

Example:

```text
parent width = 500
maxWidth = 300
Text
```

The desired behavior is to measure Text using an effective width of 300, not measure at 500 and merely clamp the final width afterward.

A minimum does not automatically force a smaller intrinsic measurement proposal. For example, `minWidth=300` with a parent width of 500 may still measure at 500.

## 8. Measurement / layout pipeline

The target pipeline is:

```text
parent proposal
    ↓
resolve Node measurement proposal
    ↓
content measurement / child measurement
    ↓
desired content size
    ↓
box composition (padding/border)
    ↓
final size resolution
    ↓
container allocation
    ↓
arrangement
    ↓
actual geometry
```

For Text:

```text
effective width proposal
    ↓
text measurement
    ↓
wrapped intrinsic size
```

The text implementation does not know the parent layout algorithm and does not manually invalidate its ancestors.

## 9. Invalidation and lifecycle

The current Phase 1 deferred mutation system is intentionally preserved as the foundation.

Conceptually:

```text
property mutation
    ↓
framework mutation queue
    ↓
coalesced layout work
    ↓
LayoutManager pass
```

The internal system should eventually distinguish conceptually:

```text
Measure dirty
Arrange dirty
Render dirty
```

but the first implementation may conservatively recalculate a queued root/subtree. Fine-grained dependency graphs are deferred until real performance requirements justify them.

Mutations occurring during layout should be deferred rather than causing immediate recursive layout.

Geometry outputs such as `actualSize` and `actualPosition` are not themselves generic invalidation triggers.

## 10. Current source migration state

The migration has already started on `phase2-layout-migration`.

### New internal layout files

```text
include/ui_framework/core/linear_layout.hpp
src/core/linear_layout.cpp

include/ui_framework/core/layout_constraints.hpp
src/core/layout_constraints.cpp
```

### Current role of `StackPanelNode`

It has been reduced toward persistent configuration:

```text
orientation
gap
main alignment
cross alignment
```

Its old algorithm was extracted into `linear_layout.cpp`.

### Current role of `LayoutManager`

It already dispatches `StackPanelNode` through the internal Linear layout path while other nodes still use the legacy `Node::measure/arrange` bridge.

This is a deliberate migration seam, not final architecture.

### Current `types.hpp`

It now contains separate axis alignment types:

```text
MainAxisAlignment
CrossAxisAlignment
```

and retains existing value/size/constraint/context types.

## 11. Important current implementation status

The implementation is **not yet complete Phase 2**.

The next architectural/code step is to connect the common `layout_constraints` subsystem to the central `LayoutManager` pipeline so that constraint semantics are not duplicated inside specific layout algorithms.

After that, the next major target is framework-owned Text/intrinsic measurement.

The old client-side `measure/arrange` virtual hooks remain temporarily for migration and must not yet be removed.

## 12. Known static issues to resolve before final Phase 2 cleanup

The current migration is intentionally incomplete. Before removing legacy hooks, the following must be resolved:

```text
- single ownership of constraint resolution;
- central LayoutManager proposal pipeline;
- Text/content measurement integration;
- final stretch semantics with min/max;
- normal-vs-absolute positioning semantics;
- internal measurement dispatch choice;
- invalidation propagation details;
- removal of concrete StackPanel measure/arrange overrides;
- removal of client-facing layout obligations from PanelNode.
```

## 13. Build/test policy

By project decision, **build and tests are intentionally not run before the end of Phase 6**.

Therefore:

```text
No build result before Phase 6
No runtime test result before Phase 6
```

This is intentional and must not be treated as an accidental missing verification step during Phase 2 development.

Until Phase 6, validation is performed through:

```text
source inspection
architectural consistency checks
static reasoning
numerical layout cases
ownership/lifecycle reasoning
documentation checkpoints
```

## 14. Large-file editing policy

Large files such as:

```text
src/core/layoutmanager.cpp
src/core/nodetree.cpp
src/core/nodetree.hpp
src/core/node.cpp
```

should be changed incrementally and surgically.

The assistant should first attempt edits itself. The user can manually patch a large file only when the available editing mechanism makes a precise safe change impractical.

Small files may be edited directly.

## 15. Files explicitly protected from Phase 2 changes

Do not modify:

```text
src/components/*
```

They are legacy/historical reference material only.

`inputmanager.cpp` should also remain untouched unless an actual Phase 2 dependency is demonstrated.

## 16. Authoritative documentation map

Use this document first when restoring context in a new chat.

Then consult:

```text
PHASE2_ARCHITECTURE_SPECIFICATION.md
    → target architecture

PHASE2_SOURCE_LEVEL_MAPPING.md
    → source-to-architecture mapping

PHASE2_IMPLEMENTATION_MIGRATION_PLAN.md
    → implementation order

PHASE2_CONSTRAINT_SEMANTICS.md
    → Auto/fixed/min/max semantics

PHASE2_NUMERICAL_LAYOUT_CASES.md
    → numerical validation cases

PHASE2_LAYOUT_LIFECYCLE_INVALIDATION_ANALYSIS.md
    → lifecycle/invalidation reasoning

INTRINSIC_MEASUREMENT_DISPATCH_ANALYSIS.md
    → measurement dispatch alternatives

PHASE2_MEASUREMENT_DISPATCH_RECOMMENDATION.md
    → current preferred internal measurement boundary
```

Earlier layout research documents remain valuable historical evidence. They should not be interpreted as stronger than this current checkpoint when the documents disagree.

## 17. Immediate continuation point

The exact continuation point is:

```text
Current branch:
    phase2-layout-migration

Current architecture:
    Node + PanelNode structure
    framework-owned Linear algorithm
    internal constraint subsystem
    LayoutManager still partly transitional

Next step:
    centralize constraint proposal/final-size resolution inside LayoutManager
    without removing the legacy hooks yet

Then:
    integrate framework-owned Text/intrinsic measurement

Then:
    finish Linear semantics and invalidate/arrange ownership

Then:
    remove the old client layout contract
```

This document is the primary context recovery point for the next chat.
