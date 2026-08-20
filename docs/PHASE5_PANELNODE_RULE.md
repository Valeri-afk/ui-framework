# Phase 5 — PanelNode Rule

## Status

Working architectural decision for Phase 5. This clarifies the role of `PanelNode`; it does not create a new component hierarchy.

## Core rule

`PanelNode` should be understood as a **structural/layout primitive**, not as a generic base class for anything that displays multiple visual elements or content.

```text
Node
    = runtime/component object

PanelNode
    = Node + owned child structure + child/layout flow semantics
```

The mere existence of:

```text
text
icon
image
badge
multiple visual primitives
```

inside a component does **not** imply that the component should inherit from `PanelNode`.

## When PanelNode is justified

A component should use `PanelNode` when its responsibilities naturally include structural child composition such as:

- owning multiple framework `Node` children;
- arranging those children through a layout flow;
- managing child geometry;
- defining child-level composition semantics;
- exposing child composition as part of the component's design.

Examples that may naturally be `PanelNode`-based:

```text
ButtonGroup
Menu
TabControl
ListBox
Accordion
Dialog
```

The final type of each concrete component still follows its actual implementation needs.

## What does NOT justify PanelNode

The following alone are insufficient:

```text
Button has text
Button has icon
Button has text + icon
component renders several primitives
component has complex visual presentation
```

For example, a `Button` may remain:

```text
Button : Node
```

while internally representing text and icon through semantic state or shared framework rendering machinery.

If a Button later requires real child Nodes with a meaningful internal layout flow, it may instead become:

```text
Button : PanelNode
```

This decision is made from the composition/layout requirement, not from the presence of content.

## Content vs structural composition

```text
visual/content primitive
    ≠ automatically a structural child

structural child
    ≠ automatically public semantic content
```

A `TextNode` can be a child of a `PanelNode` without forcing every component that displays text to become a `PanelNode`.

Likewise, a specialized component such as `MenuItem` can structurally be a child Node while its meaningful semantic composition is defined by `Menu`.

## Relation to the general content model

The framework intentionally does not adopt a universal:

```text
"everything is content of everything"
```

model.

Components decide which Nodes they intentionally compose and how those children participate in their semantics. `PanelNode` only supplies the generic structural ownership/layout capability needed for such composition.

## Current Button implication

Current working hypothesis:

```text
Button
    → likely Node initially
```

A Button may support text, icon, or text+icon without automatically becoming a `PanelNode`.

`IconButton`, `ToggleButton`, or other specialized action components should be classified by semantic/interaction differences, not by whether their content contains one or multiple visual primitives.

## Decision rationale

This keeps the hierarchy minimal:

```text
Node
    ↓
PanelNode only when structural composition/layout is actually required
```

It avoids making `PanelNode` an artificial common superclass for all visually rich components and preserves its role as a genuine framework composition primitive.

No compilation/tests/runtime validation before Phase 6.
