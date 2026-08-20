# Phase 5 — Component Pattern Review

## Reviewed components

```text
Button      : Node
ToggleButton: Button
Menu        : StackPanelNode
MenuItem    : Node
TabControl  : StackPanelNode
TabItem     : Node
```

## Patterns that are already justified

### 1. Framework-owned primitives

`TextPrimitive` is a valid internal framework primitive. Button, MenuItem and TabItem can use the same text measurement/rendering implementation without exposing a client-facing text service.

This is infrastructure reuse, not a component inheritance hierarchy.

### 2. Structural components use existing layout nodes

`Menu` uses `StackPanelNode(Vertical)` and `TabControl` uses `StackPanelNode(Horizontal)`.

Composite components should reuse framework layout behavior instead of reimplementing child measurement or arrangement.

### 3. Specialized items remain Node unless child layout is required

`MenuItem` and `TabItem` are currently `Node`, not `PanelNode`.

The presence of text or other visual primitives does not by itself require child layout.

### 4. Parent components coordinate subsystem state

`Menu` coordinates `active/selected` state of its `MenuItem` children.

`TabControl` coordinates selection and active state of its `TabItem` children.

This is component-specific ownership expressed by the component API. It is not a generic rule that all parents control all children.

### 5. Specialized Button inheritance is currently useful

`ToggleButton : Button` reuses Button interaction, activation and rendering pipeline while adding persistent `selected` state.

The current evidence does not justify introducing a separate `ButtonBase` abstraction yet.

## Patterns explicitly not promoted to generic abstractions yet

### Generic selectable node

`MenuItem.selected` and `TabItem.active` are similar but not identical semantic contracts. Do not introduce `Node::selected`, `Node::active`, or a generic `SelectableNode` yet.

### Generic parent-child coordinator

`Menu` and `TabControl` both coordinate children, but their policies differ. Do not create a framework-level parent-control mechanism merely because the pattern repeats twice.

### Generic ContentNode / universal content API

The current retained-mode model intentionally does not treat every Node as arbitrary content of every other Node. Do not introduce universal content composition during Phase 5.

### ButtonBase

Do not extract Button interaction into a new public or framework-level base until another concrete component proves that the shared abstraction is independently reusable.

## Current architectural evidence

The most stable boundary observed so far is:

```text
Framework infrastructure
    ├── Node state and common properties
    ├── PanelNode child ownership
    ├── StackPanelNode / layout flow
    ├── hit-test and event dispatch
    ├── TextPrimitive and other proven internal primitives
    └── layout measurement/arrangement

Components
    ├── semantic state
    ├── visual properties
    ├── component-specific presentation
    └── coordination of intentionally owned specialized children
```

This boundary should remain provisional and continue to be tested against more component families.
