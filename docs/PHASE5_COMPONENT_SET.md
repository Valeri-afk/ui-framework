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

These components have an active Phase 5 implementation. Their contracts should continue to be checked against `COMPONENT_DESIGN_GUIDE.md`.

## Implement now

### Checkbox

A generic binary state control:

```text
checked / unchecked
```

It is a `Node` component. Its state and presentation are local; it uses existing event and rendering infrastructure.

### RadioButton

A generic mutually-exclusive-choice control. The component owns its checked state and activation semantics. Group coordination remains explicit and is not hidden in a global registry.

### Slider

A generic scalar input control:

```text
minimum
maximum
value
step
```

Pointer dragging uses existing input/capture infrastructure. The component maps pointer position to its value and renders its track/thumb.

### Dropdown

A composite standard control built from the existing `Button` and `Menu/MenuItem` concepts. Its open state belongs to the Dropdown; item selection updates the trigger and closes the menu.

The current Phase 5 implementation deliberately does **not** introduce modal or global overlay behavior. Its menu remains a child of the Dropdown and uses the existing absolute-position layout capability.

This is sufficient as a generic component contract for now. If later applications require menus to escape parent clipping or participate in global overlay ordering, that belongs to the future overlay/modality architecture rather than a Dropdown-specific workaround.

## Analyze before implementation

### TextField / Input

Standard UI component, but deferred until text input infrastructure is explicitly designed. Keyboard key events alone are not sufficient for a correct text-entry API; text editing, input composition/IME, selection/caret behavior and clipboard semantics may require framework support.

### Image

Keep as a standard visual component candidate, but defer implementation until the framework's renderer-bound image/resource contract is explicitly established. The framework should not smuggle an ad-hoc `SDL_Texture*` ownership model into a public component API.

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

Deferred until Phase 6 modality infrastructure is complete. The old Modal implementation is legacy reference material only. Requirements are recorded in `PHASE6_MODALITY_REQUIREMENTS.md`.

## Do not add as framework components

### Paper

Cancel as a framework component. `Paper` is primarily a visual surface/elevation styling concept, not an independently semantic UI control.

### Label

Cancel as a dedicated framework component. Text presentation is already represented by `TextNode` and `TextPrimitive`.

### Card

Cancel as a dedicated framework component. A Card is a composition/style pattern rather than a fundamentally distinct runtime or interaction model. It should be built by the client from `PanelNode`/layout primitives and standard visual properties.

## Decision rules

1. A listed UI element is not automatically a framework component.
2. A component must provide a generic semantic contract broader than one application screen.
3. A component should remain `Node` unless structural child ownership/layout is central to its semantics.
4. Reuse existing framework input, layout, rendering and event infrastructure instead of recreating it inside a component.
5. Defer components whose correct implementation depends on an unresolved framework subsystem.
6. Cancel visual/style-only names that can be expressed through existing generic Nodes and properties without losing reusable behavior.
