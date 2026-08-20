# Phase 5 — Component Architecture Checkpoint

## Purpose

This checkpoint records the current component architecture that should guide further framework component development. It is intentionally separate from the large architecture document.

## Framework layers

```text
Framework infrastructure
    ↓
Base visual/layout nodes and low-level behavior
    ↓
Standard UI components
    ↓
Application-specific components
```

The framework is not a complete widget library. It provides a minimal set of generic UI concepts and the infrastructure required to implement them.

An application such as the chess application is a validation target, not a source of application-specific framework components.

## Base types

### `Node`

`Node` is the default base for visual components.

A component should inherit from `Node` unless it actually needs structural child ownership and child layout.

`Node` may contain framework-wide properties and use internal visual primitives without becoming a `PanelNode`.

### `PanelNode`

`PanelNode` is the structural base for components that own children.

The existence of text, icons/images, borders, or multiple visual primitives does not by itself require `PanelNode`.

### `StackPanelNode`

`StackPanelNode` provides reusable child layout flow. Composite components should reuse it when the required flow matches its semantics instead of duplicating measurement/arrangement code.

## Component responsibilities

A standard visual component normally owns:

```text
component-specific semantic state
component-specific visual properties
presentation of that state
semantic actions/callbacks
coordination of intentionally owned specialized children
```

The component should not normally own:

```text
NodeTree traversal
layout engine implementation
hit-test implementation
global event dispatch
framework input routing
modality routing
other generic low-level infrastructure
```

A useful boundary test is: if a component cannot reasonably implement the property or behavior with the tools exposed by its base/framework APIs, that behavior is a strong candidate for framework infrastructure.

## Visual primitives

A visual primitive does not have to be a `Node`.

`TextPrimitive` is an internal framework primitive that centralizes text measurement and rendering. This allows text-bearing components to reuse correct text behavior without creating a client-facing text service or forcing every component into a child-node content model.

A primitive should become a child component only when independent structure, lifecycle, hit-testing, or semantic behavior makes a `Node` useful.

## Content and children

The framework does not use a universal `content` model.

Not every component can contain every other component. A specialized component may define an explicit child contract:

```text
Menu       → MenuItem
TabControl → TabItem
```

A specialized component may itself be used as a child of a suitable `PanelNode`. Child capability and accepted-content semantics are separate questions.

The fact that a primitive can be placed in a panel does not mean that every component should expose arbitrary children.

## State ownership

A composite component may coordinate state of its specialized children when that relationship is intrinsic to the component.

Examples:

```text
Menu       → active/selected MenuItem
TabControl → selected/active TabItem
```

This does not justify generic `Node::selected`, `Node::active`, or `SelectableNode` abstractions yet. Similar names do not establish a shared semantic contract.

## Inheritance

Inheritance is used when a specialized component genuinely extends the semantic and visual contract of its base.

Current example:

```text
ToggleButton : Button
```

`ToggleButton` reuses Button interaction, activation, layout, text handling, press animation and presentation pipeline, while adding persistent selected state.

Do not create a generic `ButtonBase` until multiple concrete components prove the abstraction independently useful.

## Layout rule

A component should not become a `PanelNode` merely because it contains more than one visual primitive.

The deciding question is:

> Does this component require structural child layout/ownership that is part of its semantics?

If no, prefer `Node`.

If yes, use `PanelNode` or an existing specialized panel such as `StackPanelNode`.

## Standard component catalog at this checkpoint

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
```

These have a distinct generic contract and have been actively validated.

### Deferred standard components

```text
List
Scroll / ScrollArea
Modal
IconButton
```

`List` remains deferred because the current implementation is only a thin semantic alias over `StackPanelNode` and does not yet provide sufficiently distinct generic behavior.

`Scroll / ScrollArea` remains deferred until the framework-level scroll architecture is settled.

`Modal` remains deferred until Phase 6 modality infrastructure is complete. The current/future Modal component is therefore considered **deprecated/inactive for implementation purposes until Phase 6**, while the `components` layer itself is not deprecated.

`IconButton` remains deferred until a stable graphics/icon primitive and resource contract exists.

## Framework component versus application component

The framework should provide generic components such as:

```text
Button
Menu
List
Dialog
Scroll
```

The application should compose these into application-specific concepts such as:

```text
ChessBoard
MoveList
PlayerCard
GameClock
PromotionDialog
AnalysisPanel
```

The chess application is used to test whether the generic framework set is sufficient. It does not determine the framework API directly.

## Component creation checklist

Before adding a new standard component:

1. Is it a generic UI concept?
2. Does it add a distinct contract rather than rename an existing node?
3. Can its responsibilities be expressed using current framework infrastructure?
4. Does it actually require child ownership/layout?
5. Which state belongs to the component?
6. Which state belongs to specialized children?
7. Can an existing standard component be reused instead?
8. Is inheritance genuinely sharing a stable contract?
9. Does the component avoid duplicating framework low-level behavior?
10. Does it keep the framework minimal?

## Phase 5 boundary

Phase 5 should establish component architecture and implement the standard components whose contracts do not depend on later framework subsystems.

Components that depend on unresolved framework systems should have their requirements documented and implementation deferred rather than forcing an incomplete architecture into Phase 5.
