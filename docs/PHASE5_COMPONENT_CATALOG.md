# Phase 5 — Minimal Standard UI Component Catalog

## Purpose

Phase 5 builds only the minimum reusable UI catalog needed to validate the framework against a real application.

The chess application is a validation target, not a source of chess-specific framework components.

The framework provides:

1. generic base nodes and framework infrastructure;
2. a small set of standard UI components.

Application-specific components remain client code.

## Base framework layer

```text
Node
PanelNode
StackPanelNode
TextPrimitive / TextNode
```

The base layer owns reusable infrastructure such as layout, event dispatch, hit-testing, common state and framework-level properties.

## Standard UI components

### Implemented / actively validated

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
List
ListItem
```

### Deferred by phase dependency

```text
Modal / Dialog
Scroll / ScrollArea
```

These are standard UI concepts, but their final implementation must wait for the corresponding framework infrastructure. Modal depends on Phase 6 modality; Scroll must wait for the final framework-level scroll design.

### Not currently required

```text
IconButton
Checkbox
Switch
Select
Accordion / Section
```

They remain possible future components, not Phase 5 requirements. A later application may justify adding them.

## Application-level examples

These belong to the client application, not the framework:

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

The application may build these from standard framework components. For example:

```text
MoveList        → List + ListItem
PromotionDialog → Modal + application content
```

The framework does not own chess semantics.

## Promotion rule

A new component enters the framework only when all of the following are true:

- it represents a generic reusable UI concept;
- it has a clear framework-level contract;
- existing nodes/components cannot express the concept cleanly;
- the required low-level behavior belongs to an already available or explicitly planned framework subsystem;
- the component does not introduce unnecessary framework complexity.

The existence of a visually similar element in an application is not sufficient.

## Design reference

Use `docs/COMPONENT_DESIGN_GUIDE.md` for the detailed rules governing Node vs PanelNode, primitives, content, inheritance, state ownership, shared properties, composite components, validation and phase boundaries.
