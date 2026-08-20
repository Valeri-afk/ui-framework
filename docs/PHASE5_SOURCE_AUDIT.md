# Phase 5 — Source Audit

This document tracks source-level cleanup and architectural review. It is not a build-system specification.

## Removed / obsolete

The following obsolete abstractions and orphan implementations no longer belong to the current source tree:

```text
components/component.hpp
components/paper.hpp
components/paper.cpp
components/label.hpp
components/label.cpp
components/flex_panel.hpp
components/flex_panel.cpp
core/controlnode.hpp
core/controlnode.cpp
src/components/flex_panel.cpp
src/components/label.cpp
src/core/controlnode.cpp
src/core/gridnode.cpp
include/ui_framework/components/modal.hpp
```

The legacy implementations are removed when their corresponding public contracts are gone. A historical algorithm is not a reason to keep an orphan `.cpp` or an incompatible legacy header in the active source tree.

The old Modal behavior remains documented in `PHASE6_MODALITY_REQUIREMENTS.md`; the new Modal component will be implemented only after Phase 6 modality infrastructure is complete.

## Deferred, not obsolete

`modalmanager.*` is retained as Phase 6 preparation. Its final responsibilities remain subject to the modality architecture.

`Scroll / ScrollArea` remains deferred until framework-level scroll ownership, clipping, coordinate conversion, hit-test integration and input routing are finalized.

## Retained foundation

### `core/text_primitive.*`

Retained as the internal text measurement/rendering primitive. It is not a Node and not a public service.

### `core/textnode.*`

Retained as the Node-level text component. It adapts the reusable text primitive to Node geometry, lifecycle, layout and rendering.

The distinction is intentional:

```text
TextPrimitive
    low-level reusable text implementation

TextNode
    NodeTree-facing visual component
```

### `core/primitives.*`

Retained as the low-level SDL drawing helper layer. It provides reusable stateless drawing operations and does not own component semantics.

Its role is documented in `PRIMITIVES_ROLE.md`.

### Layout infrastructure

Retain layout nodes and helpers that have a complete current contract and are referenced by the active architecture, including `PanelNode`, `StackPanelNode`, `LinearLayout`, and layout constraint machinery.

Do not preserve historical layout implementations merely because their algorithms may be useful in the future. A future layout primitive can be reintroduced from a documented requirement.

### Runtime/input/event infrastructure

The following remain framework infrastructure with separate responsibilities:

```text
UIManager
NodeTree
InputManager
EventDispatcher
EventHandlerStorage
LayoutManager
RenderingState
```

`UIManager` orchestrates frame/event flow. `NodeTree` owns structural registration, traversal and mutation safety. `InputManager` owns transient input state and SDL-to-framework input processing. `EventDispatcher` owns event propagation. `LayoutManager` owns measure/arrange processing. `RenderingState` owns renderer-state preservation. None should absorb component-specific semantics merely for convenience.

### Phase 6 preparation

Modal-related input/tree hooks remain only where they are useful as preparation for Phase 6 modality. They should not be interpreted as a requirement for the Phase 5 `Modal` component to own input routing.

## Current component source target

The active standard UI component layer currently contains:

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
```

Deferred standard components are intentionally not required in the active source tree:

```text
List
Scroll / ScrollArea
Modal
IconButton
```

`List` is deferred because the previous draft did not have a sufficiently distinct generic contract. `Modal` is deferred because its implementation depends on Phase 6 modality infrastructure.

## Source structure target

```text
core/
    framework runtime/infrastructure
    layout infrastructure
    input/event infrastructure
    internal rendering primitives

components/
    standard UI components

application/
    application-specific composition outside this framework
```

Every retained source file should have a current architectural role. Historical implementations should remain only when they are explicitly useful as references for an unresolved future phase.

## Audit conclusion

The current source foundation has no known remaining references to the removed `Widget` / `ControlNode` model or the removed active Modal header. The remaining legacy material is either removed or intentionally retained as Phase 6 preparation/documented history.
