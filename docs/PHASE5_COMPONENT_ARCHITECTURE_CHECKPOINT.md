# Phase 5 — Component Architecture Checkpoint

This document is the practical checkpoint for further component development. It complements the consolidated architecture document without replacing it.

## Framework layers

```text
Framework infrastructure
    ↓
Base nodes / layout / rendering primitives
    ↓
Standard UI components
    ↓
Application-specific composition
```

The framework is intentionally minimal. A chess application is a validation target, not a source of chess-specific framework components.

## Base types

### `Node`

Default base for visual components. Use it unless the component genuinely requires structural child ownership and child layout.

### `PanelNode`

Structural/layout base for components that own children and manage their geometry/composition.

Text, icons, images, borders, or multiple visual primitives do not by themselves justify `PanelNode`.

### `StackPanelNode`

Reusable child-flow layout primitive. Composite components should reuse it when its layout policy matches their semantics instead of reimplementing measurement/arrangement.

## Component responsibility

Components own:

```text
component-specific semantic state
component-specific visual properties
presentation
semantic actions/callbacks
coordination of intentionally owned specialized children
```

Framework infrastructure owns coordinated generic mechanisms such as:

```text
NodeTree lifecycle/traversal
layout/geometry processing
hit-testing
input/event dispatch
focus/capture
render traversal
clipping
mutation/update scheduling
modality
scroll mechanics when implemented
```

Boundary rule: if a component cannot reasonably implement a behavior with the tools exposed by its framework APIs, that behavior is a strong infrastructure candidate.

## Primitives

A visual primitive does not have to be a Node.

`TextPrimitive` is an internal reusable text measurement/rendering implementation. `TextNode` adapts that implementation to the NodeTree.

```text
TextPrimitive
    ↓
TextNode / text-bearing components
```

A primitive becomes a Node/component only when independent lifecycle, layout participation, hit-testing, or semantic behavior makes that useful.

## Children and content

There is no universal `content` model.

Specialized components may define explicit semantic child contracts:

```text
Menu       → MenuItem
TabControl → TabItem
```

Structural child ownership and semantic content are separate concepts.

## State ownership

Composite components may coordinate their intentionally specialized children:

```text
Menu       → active/selected MenuItem
TabControl → selected/active TabItem
```

This does not justify generic `Node::selected`, `Node::active`, or `SelectableNode` abstractions. Similar property names are not sufficient evidence of a shared semantic contract.

## Inheritance

Use inheritance only when the specialized component genuinely extends a stable parent contract.

Current justified example:

```text
ToggleButton : Button
```

Do not introduce `ButtonBase`, `ActionNode`, `SelectableNode`, or similar abstractions until concrete components prove a stable reusable contract.

## Layout rule

Ask:

> Does this component require structural child ownership/layout as part of its semantics?

If no → `Node`.

If yes → `PanelNode` or an existing specialized panel such as `StackPanelNode`.

## Current standard components

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
```

These have a distinct generic contract and are the current Phase 5 component set.

## Deferred components

```text
List
Scroll / ScrollArea
Modal
IconButton
```

`List` is deferred because a thin `StackPanelNode` alias does not justify a separate abstraction.

`Scroll / ScrollArea` is deferred until the framework-level scroll contract is finalized.

`Modal` is deferred until Phase 6 modality infrastructure is complete. The current Modal implementation is deprecated/inactive and retained only as a reference. The `components` layer itself is **not deprecated**.

`IconButton` is deferred until a stable graphics/icon primitive and resource contract exists.

## Application boundary

Framework:

```text
Button
Menu
List
Dialog
Scroll
```

Application:

```text
ChessBoard
MoveList
PlayerCard
GameClock
PromotionDialog
AnalysisPanel
```

The application validates framework sufficiency; it does not define framework-specific domain components.

## New component checklist

Before implementation:

1. Is this a generic UI concept?
2. Does it have an independent contract?
3. Is it more than a renamed existing node/layout primitive?
4. Does it actually require structural children?
5. Which state belongs to the component?
6. Which state belongs to specialized children?
7. Can an existing component be reused?
8. Is inheritance sharing a stable contract?
9. Is framework infrastructure being duplicated?
10. Does the abstraction preserve the minimal framework goal?

## Phase boundary

If a component depends on an unfinished framework subsystem, document its requirements and defer implementation rather than forcing an incomplete architecture into the current phase.
