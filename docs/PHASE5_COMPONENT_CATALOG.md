# Phase 5 — Minimal Standard UI Component Catalog

## Scope

Phase 5 does not attempt to implement a complete UI component library.

The chess application is used only as a validation target: it tells us whether the framework's generic UI capabilities are sufficient to build a real application. Chess-specific components remain application code.

The framework should contain only:

1. base framework nodes and layout primitives;
2. a small set of standard, reusable UI components.

## Base layer

```text
Node
PanelNode
StackPanelNode
TextPrimitive / TextNode
```

These are framework infrastructure and are not application-specific widgets.

## Standard UI components

### Implemented / currently validated

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
```

These components have an independent generic contract and have been reviewed against the component design guide.

### Deferred

```text
List
Scroll / ScrollArea
Modal
IconButton
```

`List` is deferred because the current implementation is only a semantic alias over `StackPanelNode` and does not yet provide sufficiently distinct generic behavior.

`Scroll / ScrollArea` is deferred while the framework-level scroll architecture is unresolved.

`Modal` is deferred until Phase 6 modality infrastructure exists. See `PHASE6_MODALITY_REQUIREMENTS.md`.

`IconButton` is deferred until the framework has a stable graphics/icon primitive and resource contract.

### Not currently required

```text
Checkbox
Switch
Select
Accordion / Section
```

These remain candidates for future framework extensions, but are not part of the current minimal catalog. They should only be added when a clear generic contract is established independently of an application-specific need.

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

A future framework `List` may be used to implement `MoveList`; a future framework `Modal` may be used to implement `PromotionDialog`. The framework does not own the chess semantics.

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

Use `COMPONENT_DESIGN_GUIDE.md` as the practical design and review checklist for every new component.
