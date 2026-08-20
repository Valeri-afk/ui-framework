# Phase 5 — Minimal Standard UI Component Catalog

## Scope

Phase 5 does not attempt to implement a complete UI component library.

The chess application is used only as a validation target: it tells us whether the framework's generic UI capabilities are sufficient to build a real application. Chess-specific components remain client code.

The `components` layer is an active, supported framework layer. It is **not deprecated**.

## Base layer

```text
Node
PanelNode
StackPanelNode
TextPrimitive / TextNode
```

## Standard UI components

### Implemented / currently validated

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

These components have a distinct generic contract and are intended to be reusable independently of the chess application.

`Dropdown` is a composite of `Button` and `Menu/MenuItem`. Its Phase 5 implementation keeps the menu as a child and uses the existing absolute-position layout capability. It does not introduce global overlay or modality behavior.

### Analyze before implementation

```text
TextField / Input
Image
```

`TextField / Input` requires a proper text-input contract, including text editing and potentially composition/IME handling. Do not create a partial key-only text field.

`Image` requires a stable framework resource/texture ownership contract. Do not expose ad-hoc `SDL_Texture*` ownership from the component.

### Deferred to later framework phases

```text
Scroll / ScrollArea
Modal
```

`Scroll / ScrollArea` is deferred while viewport/content bounds, offset/range, clipping, coordinate conversion, hit-test integration and input routing remain unresolved. See `SCROLL_ARCHITECTURE.md`.

`Modal` is deferred until Phase 6 modality infrastructure exists. The old implementation is legacy reference material only; the final Modal component is not part of the active Phase 5 API. See `PHASE6_MODALITY_REQUIREMENTS.md`.

### Cancelled as standalone framework components

```text
Paper
Label
Card
```

`Paper` is a visual surface/elevation styling pattern, not an independent semantic control.

`Label` duplicates the generic text role already provided by `TextNode` / `TextPrimitive` without adding a sufficiently distinct contract.

`Card` is a composition/style pattern that can be built from `PanelNode`, layout primitives and visual properties. It should remain client-level unless a future generic contract proves otherwise.

### Not currently required

```text
List
IconButton
```

`List` remains deferred because its current design does not yet provide a sufficiently distinct generic contract over existing panel/layout primitives.

`IconButton` remains deferred until a stable graphics/icon primitive and resource contract exists.

## Explicitly application-level

The following are examples of application components, not framework components:

```text
ChessBoard
ChessSquare
GameClock
PlayerCard
MoveList
PromotionDialog
AnalysisPanel
GameScreen
```

The framework provides generic building blocks; the application owns chess semantics.

## Promotion rule

A new component should enter the framework only when all of the following are reasonably true:

```text
standard reusable UI concept
        +
independent generic contract
        +
not merely a renamed existing Node/layout primitive
        +
implementation fits the framework/component responsibility boundary
```

Do not add a framework component merely because an application contains a visually similar element.

## Design guide

Use `COMPONENT_DESIGN_GUIDE.md` and `PHASE5_COMPONENT_ARCHITECTURE_CHECKPOINT.md` as the practical design and review references for every new component.
