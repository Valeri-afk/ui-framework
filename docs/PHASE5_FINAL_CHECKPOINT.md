# Phase 5 — Final Checkpoint

Phase 5 is the component-development phase. Its purpose is to establish a small, reusable standard UI layer without expanding into unresolved framework subsystems.

## Active component set

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

These components have distinct generic contracts and may be used independently of any application-specific semantics.

## Deferred components

```text
TextField / Input
Image
List
IconButton
Scroll / ScrollArea
Modal
```

Reasons:

- `TextField / Input` requires text-input/editing infrastructure beyond key events, including a proper text-input event path and eventually composition/IME, caret, selection and clipboard semantics.
- `Image` requires a resource/texture ownership and lifetime contract. The current framework has drawing primitives and renderer-state helpers, but intentionally does not expose an ad-hoc texture ownership API from components.
- `List` currently has no sufficiently distinct generic contract over existing panel/layout infrastructure.
- `IconButton` depends on a stable graphics/icon resource contract.
- `Scroll / ScrollArea` depends on framework-level viewport, extent, offset, clipping, coordinate conversion, hit-test, input-routing and layout decisions.
- `Modal` depends on Phase 6 modality infrastructure and is not part of the active Phase 5 API.

## Cancelled standalone framework components

```text
Paper
Label
Card
```

They are not required as independent runtime concepts:

- `Paper` is a surface/style pattern.
- `Label` is covered by `TextNode` / `TextPrimitive`.
- `Card` is a composition/style pattern that can be built from panels, layout and visual properties.

## Component architecture established in Phase 5

```text
Node
 ├── leaf visual/interactive components
 └── PanelNode
      └── StackPanelNode
           └── composite standard components when the child structure is semantic
```

Components own semantic state and presentation. Framework subsystems own generic traversal, input routing, event propagation, layout processing, renderer state and resource infrastructure.

A component must not reimplement framework-level behavior merely to simplify its own implementation.

## Presentation rule

Simple geometric presentation is drawn through `primitives`.

Complex texture-based presentation should use a future `Image`/resource layer rather than forcing component-specific SDL texture ownership into Phase 5.

A component may eventually combine both approaches.

## Phase 5 source boundary

Legacy `Widget` / `ControlNode` component infrastructure has been removed from the active source tree.

`components/` is an active framework layer and is not deprecated.

The old Modal implementation is removed from the active source tree and retained only through its documented requirements/history for Phase 6.

## Phase 5 completion procedure

Before declaring Phase 5 complete:

1. Verify the active component set against `COMPONENT_DESIGN_GUIDE.md`.
2. Verify no removed legacy abstractions remain referenced by active source.
3. Verify every retained source file has a current architectural role.
4. Review the architecture-related Phase 5 documents.
5. Review `ARCHITECTURE.md` manually; do not rewrite it automatically.
6. Extract only the genuinely new framework-level requirements into the Phase 6 scope.

## Phase 6 candidates discovered during Phase 5

The final Phase 6 scope must be decided after the Phase 5 document review. Current evidence suggests these candidate subsystem areas:

```text
Modality
Scrolling
Text input / editing
Image/resource management
Overlay/popup infrastructure, if Dropdown requirements prove global overlays are necessary
```

This list is a candidate set, not a commitment that every item belongs in Phase 6.
