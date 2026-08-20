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

### Implemented / actively validated

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
```

### Strong candidates for the minimal standard set

```text
List
ListItem
Dialog / Modal
Scroll / ScrollArea
```

These should be added only after their contracts are checked against the existing framework infrastructure and legacy implementations.

### Deferred

```text
IconButton
Checkbox
Switch
Select
Accordion / Section
```

These are not part of the mandatory catalog yet.

A control should be promoted into the framework when there is a clear generic contract and more than a superficial visual reason to have it as a separate component. Existing components should be preferred for simple variants where possible.

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

A framework `List` may be used to implement `MoveList`; a framework `Dialog` may be used to implement `PromotionDialog`. The framework does not own the chess semantics.

## Design rule

```text
Application requirement
        ↓
Does the framework already provide the infrastructure?
        ↓
yes → compose a client component
no  → extend generic framework infrastructure/component
```

Do not add a framework component merely because an application contains a visually similar element.
