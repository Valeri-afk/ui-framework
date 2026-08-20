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
Dropdown
```

## Implement now

The next immediately implementable controls are:

```text
Checkbox
RadioButton
Slider
```

### Checkbox

A generic binary state control (`checked` / `unchecked`). It can use existing Node, event and rendering infrastructure without introducing a new framework subsystem.

### RadioButton

A generic mutually-exclusive-choice control. The component owns its checked state and activation semantics. Group coordination remains explicit and should not be hidden in a global registry unless concrete reuse later proves that a reusable group abstraction is necessary.

### Slider

A generic scalar input control with `minimum`, `maximum`, `value` and `step`. Existing pointer input/capture infrastructure is sufficient for the basic interaction model.

## Analyze before implementation

### TextField / Input

**Deferred.** The current framework exposes keyboard events (`KeyDown` / `KeyUp`) but does not yet provide a text-input event, composition/IME handling, selection/caret model, or clipboard/editing contract. Implementing a key-only text field now would create an incomplete component and likely force framework changes later.

When text input infrastructure is designed, TextField should be reconsidered as a standard component.

### Image

**Deferred.** A proper image component requires a stable renderer/resource/texture ownership contract. Do not expose ad-hoc `SDL_Texture*` ownership through the component API.

## Defer to later framework subsystems

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

## Do not add as framework components

### Paper

Cancel as a framework component. It is primarily a visual surface/elevation styling pattern, not an independently semantic control.

### Label

Cancel as a dedicated framework component. Text presentation is already represented by `TextNode` and `TextPrimitive`.

### Card

Cancel as a dedicated framework component. A Card is a composition/style pattern that can be built from `PanelNode`, layout primitives and visual properties.

## Not currently required

```text
List
IconButton
```

These remain outside the current implementation set. They can be reconsidered when a concrete generic contract appears.

## Decision rules

1. A listed UI element is not automatically a framework component.
2. A component must provide a generic semantic contract broader than one application screen.
3. A component should remain `Node` unless structural child ownership/layout is central to its semantics.
4. Reuse existing framework input, layout, rendering and event infrastructure instead of recreating it inside a component.
5. Defer components whose correct implementation depends on an unresolved framework subsystem.
6. Cancel visual/style-only names that can be expressed by existing generic Nodes and properties without losing reusable behavior.
