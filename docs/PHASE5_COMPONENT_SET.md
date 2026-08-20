# Phase 5 — Standard Component Set

This document records the final standard component set established during Phase 5. Phase 5 is complete as a component-development phase; current framework subsystem work is Phase 6.

## Implemented standard components

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
Checkbox
RadioButton
Slider
Dropdown
```

These components have distinct generic contracts and are active framework components.

The `components` layer is active and supported. It is **not deprecated**.

## Deferred components

### TextField / Input

Deferred. The framework still needs a proper text-input/editing contract beyond `KeyDown` / `KeyUp`, including text input, composition/IME, caret/selection and editing/clipboard behavior.

### Image

Deferred. A proper image component requires a stable renderer/resource/texture ownership contract. Do not expose ad-hoc `SDL_Texture*` ownership through the component API.

### List

Deferred because its current design does not provide a sufficiently distinct generic contract over existing panel/layout primitives.

### IconButton

Deferred until a stable graphics/icon primitive and resource contract exists.

## Framework-level behavior, not standalone components

### Scroll

Scrolling is now implemented as framework-level behavior through `ScrollManager` and its `UIManager`/NodeTree integration.

The current implementation covers:

```text
viewport/content extent
scroll offset/range
clamping
wheel routing
nested scroll chaining
coordinate transformation
layout-derived extent calculation
```

The behavior is source-level only until full build/runtime validation. A standalone `Scroll` / `ScrollArea` component is not currently required.

### Modal

Modality is implemented through `ModalManager` as framework infrastructure.

The current service owns the framework-level modal behavior; the old Modal component is deprecated/inactive and is retained only as historical reference. A standalone public Modal component is not currently required.

## Do not add as framework components

### Paper

Cancel as a framework component. It is primarily a visual surface/elevation styling pattern.

### Label

Cancel as a dedicated framework component. Text presentation is already represented by `TextNode` / `TextPrimitive`.

### Card

Cancel as a dedicated framework component. A Card is a composition/style pattern that can be built from `PanelNode`, layout primitives and visual properties.

## Component architecture rule

1. A listed UI element is not automatically a framework component.
2. A component must provide a generic semantic contract broader than one application screen.
3. A component should remain `Node` unless structural child ownership/layout is central to its semantics.
4. Reuse existing framework input, layout, rendering and event infrastructure instead of recreating it inside a component.
5. Defer components whose correct implementation depends on unresolved framework infrastructure.
6. Cancel visual/style-only concepts that can be expressed by existing generic Nodes and properties.
7. Keep framework-level behavior in framework services/subsystems when it crosses component boundaries.
