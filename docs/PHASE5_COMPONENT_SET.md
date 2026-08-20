# Phase 5 — Standard Component Set

This is the current working inventory of standard UI components considered for the framework. It intentionally covers the small generic set needed to support the target application class without turning the framework into a full widget toolkit.

## Already implemented

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

These are the active standard Phase 5 components.

## Deferred components

### TextField / Input

Deferred. The current framework exposes keyboard events (`KeyDown` / `KeyUp`) but does not yet provide a proper text-input event, composition/IME handling, selection/caret model, or clipboard/editing contract. A key-only text field would be incomplete.

### Image

Deferred. A proper image component requires a stable renderer/resource/texture ownership contract. Do not expose ad-hoc `SDL_Texture*` ownership through the component API.

### Scroll

Deferred until scroll mechanics are designed at framework level:

```text
viewport
content extent
offset/range
coordinate conversion
clipping
hit-test integration
input routing
layout integration
```

### Modal

Deferred until Phase 6 modality infrastructure is complete. The legacy implementation has been removed from the active source tree; its behavior is retained only as historical reference and in `PHASE6_MODALITY_REQUIREMENTS.md`.

### List

Deferred because its current design does not yet provide a sufficiently distinct generic contract over existing panel/layout primitives.

### IconButton

Deferred until a stable graphics/icon primitive and resource contract exists.

## Do not add as framework components

### Paper

Cancel as a framework component. It is primarily a visual surface/elevation styling pattern, not an independently semantic control.

### Label

Cancel as a dedicated framework component. Text presentation is already represented by `TextNode` and `TextPrimitive`.

### Card

Cancel as a dedicated framework component. A Card is a composition/style pattern that can be built from `PanelNode`, layout primitives and visual properties.

## Component architecture rule

1. A listed UI element is not automatically a framework component.
2. A component must provide a generic semantic contract broader than one application screen.
3. A component should remain `Node` unless structural child ownership/layout is central to its semantics.
4. Reuse existing framework input, layout, rendering and event infrastructure instead of recreating it inside a component.
5. Defer components whose correct implementation depends on an unresolved framework subsystem.
6. Cancel visual/style-only concepts that can be expressed by existing generic Nodes and properties without losing reusable behavior.
7. The `components` layer is active and supported; it is not deprecated.
