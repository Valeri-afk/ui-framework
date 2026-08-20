# Phase 5 — Source Audit

This document tracks source-level cleanup and architectural review. It is not a build-system specification.

## Removed

The following obsolete abstractions were removed from the current component architecture:

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

`GridNode` is not currently part of the retained layout infrastructure. The remaining layout foundation is intentionally smaller and should not keep orphan implementations without a corresponding public contract.

## Deferred, not obsolete

`components/modal.*` is retained only as a deprecated/inactive implementation reference until Phase 6. It is not the final Modal architecture.

`modalmanager` is retained because Phase 6 modality work will determine which parts remain valid.

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

Retain only layout nodes that currently have a complete, referenced contract and are used by the current architecture, such as `PanelNode` and `StackPanelNode`.

Do not preserve historical layout implementations merely because their algorithms may be useful in the future. A future layout primitive can be reintroduced from a documented requirement.

## Current source structure target

```text
core/
    framework runtime/infrastructure
    layout infrastructure
    internal rendering primitives

components/
    standard UI components

application/
    application-specific composition outside this framework
```

Every retained source file should have a current architectural role. Historical implementations should remain only when they are explicitly useful as references for an unresolved future phase.
