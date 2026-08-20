# Phase 5 — Menu Implementation Checkpoint

## 1. Initial structure

```text
Menu : PanelNode
    ├── MenuItem : Node
    ├── MenuItem : Node
    └── ...
```

`MenuList` is **not** introduced in the first implementation slice.

Material UI provides `MenuList` as an optional reusable container primarily for menu focus/keyboard behavior, while `Menu` remains the temporary surface/container and `MenuItem` the item. citeturn849724search7turn849724search0turn849724search2

Our retained-mode runtime should introduce an equivalent `MenuList` only if concrete reuse outside `Menu` demonstrates that the focus/navigation subsystem is independently useful.

## 2. Menu responsibilities

`Menu` owns menu-level semantic coordination:

```text
open/close semantics
active/highlighted item policy
selection policy where applicable
item navigation policy
item activation coordination
```

It also owns structural child composition through `PanelNode`.

It does **not** reimplement framework input, hit-testing, event dispatch or focus infrastructure.

## 3. MenuItem responsibilities

`MenuItem : Node` owns its own presentation and item-level semantic state.

Candidate state:

```text
selected
highlighted / active presentation state
```

The exact separation between parent-owned active state and item-owned derived presentation state should follow the concrete implementation.

`MenuItem` may use internal `TextPrimitive` and future icon primitives without becoming a `PanelNode`.

## 4. Parent-child coordination

`Menu` may intentionally change the semantic state of its `MenuItem`s because the relationship is explicitly defined by the component design.

This is a component-specific contract, not a generic framework rule that all parents may arbitrarily control all children.

The framework should not add generic `Node.selected`, `Node.highlighted`, or similar properties merely to support Menu.

## 5. MenuList future boundary

A future `MenuList : PanelNode` becomes justified if concrete requirements establish reusable behavior such as:

```text
keyboard item navigation
roving focus
focus wrapping policy
disabled-item navigation rules
menu-specific focus initialization
```

that should be available independently from temporary popup/surface semantics.

Until then, those responsibilities may live directly in `Menu`.
