# Phase 5 — Source Audit

This document tracks source-level cleanup and architectural review. It is not a build-system specification.

## Removed

The following obsolete abstractions have been removed from the current architecture:

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
src/core/gridnode.cpp
```

The legacy implementations are removed when their corresponding public contracts are gone. A historical algorithm is not a reason to keep an orphan `.cpp` in the active source tree.

## Deferred, not obsolete

`components/modal.*` is retained only as a deprecated/inactive implementation reference until Phase 6. It is not the final Modal architecture.

`modalmanager.*` is retained because Phase 6 modality work will determine which responsibilities remain valid.

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

Retain only layout nodes and helpers that have a complete current contract and are referenced by the active architecture, including `PanelNode`, `StackPanelNode`, `LinearLayout`, and the layout constraint machinery.

Do not preserve historical layout implementations merely because their algorithms may be useful in the future. A future layout primitive can be reintroduced from a documented requirement.

## Input and event infrastructure

`InputManager`, `EventDispatcher`, `EventHandlerStorage`, `NodeTree`, and the event types remain framework infrastructure. Their responsibilities must not migrate into individual standard components merely to simplify a component implementation.

The unresolved Phase 6 modality behavior should be implemented at this infrastructure boundary rather than inside `Modal`.

## Rendering infrastructure

`RenderingState` remains separate from drawing primitives. Rendering state coordinates the current rendering context; `primitives` performs low-level drawing operations.

Standard components may use both through the existing framework APIs, but should not duplicate their responsibilities.

## Current source structure target

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

## Remaining audit targets

The next source-level review should focus on:

```text
rendering_state
ui_manager
inputmanager
nodetree
layoutmanager / linear_layout
modalmanager
```

The question is not whether these files are old, but whether each responsibility belongs to the current framework architecture and whether any responsibilities are duplicated.
