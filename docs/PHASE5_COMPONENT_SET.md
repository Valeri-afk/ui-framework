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
```

These components have an established generic contract and are retained as active Phase 5 components.

## Implement now

### Checkbox

A generic binary state control:

```text
checked / unchecked
```

It is a `Node` component. Its state and presentation are local; it does not require a new framework subsystem.

### RadioButton

A generic mutually-exclusive-choice control candidate. The base component owns its checked state and activation semantics. Group coordination should remain explicit and should not be hidden in a global registry.

For the current minimal framework, the component does not introduce a universal `RadioGroup` abstraction unless concrete reuse proves that group coordination requires one.

### Slider

A generic continuous/discrete scalar input control:

```text
minimum
maximum
value
step
```

Pointer dragging uses existing input/capture infrastructure. The component maps pointer position to its value and renders its own track/thumb.

## Analyze before implementation

### Dropdown

Keep as a standard UI candidate, but first define the contract relative to the existing `Menu/MenuItem` and overlay system. A dropdown should not become a hidden modal implementation.

The likely generic composition is:

```text
Dropdown
    closed → selected-value presentation
    open   → Menu/MenuItem presentation
```

The final popup placement and ownership rules should be defined before implementation.

### TextField / Input

Standard UI component, but deferred until text input infrastructure is explicitly designed. Keyboard key events alone are not sufficient for a correct text-entry API; text editing, input composition/IME, selection/caret behavior and clipboard semantics may eventually require framework support.

Do not force a partial text-entry implementation into Phase 5 merely to create the class.

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

Cancel as a framework component. `Paper` is primarily a visual surface/elevation styling concept, not an independently semantic UI control. The generic framework should provide the underlying Node box/background/border/rendering mechanisms instead of a Paper abstraction.

### Label

Cancel as a dedicated framework component. Text presentation is already represented by `TextNode` and `TextPrimitive`. A separate `Label` abstraction would duplicate a generic text node without adding a sufficiently distinct contract.

### Card

Cancel as a dedicated framework component. A Card is a composition/style pattern rather than a fundamentally distinct runtime or interaction model. It should be built by the client from `PanelNode`/layout primitives and standard visual properties.

This does not prevent the framework from later adding a genuinely reusable surface/container primitive if concrete generic behavior appears that cannot be expressed by existing infrastructure.

## Decision rules

1. A listed UI element is not automatically a framework component.
2. A component must provide a generic semantic contract that is broader than one application screen.
3. A component should remain `Node` unless structural child ownership/layout is central to its semantics.
4. Reuse existing framework input, layout, rendering and event infrastructure instead of recreating it inside a component.
5. Defer components whose correct implementation depends on an unresolved framework subsystem.
6. Cancel visual/style-only names that can be expressed through existing generic Nodes and properties without losing reusable behavior.
