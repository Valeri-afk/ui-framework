# Phase 2 Property Responsibility Analysis

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document classifies the current layout-related state in `Node` and related types according to the proposed closed-layout architecture.

## 1. Current Node property inventory

The current `Node` exposes:

```text
position
position mode
size
min size
max size
padding
border
overflow
visibility
enabled
focusable
capturable
```

It also internally stores desired and actual geometry. fileciteturn212file0

The current common layout types include `PositionMode`, `LayoutValue`, `LayoutSizeValue`, `LayoutSize`, `Padding`, `Border`, `LayoutConstraints`, `Alignment`, and `Overflow`. fileciteturn213file0

The key question is not whether all of these fields can technically be on Node. It is whether each has meaning independently of parent layout and whether putting it there creates long-term coupling.

## 2. Strong candidates for common Node state

### Size

`width`/`height` are fundamental node geometry constraints. A node can request a fixed size regardless of its parent layout family.

**Classification:** Node-level.

### Min / Max size

These constrain the feasible geometry of an individual Node and apply across layout families.

**Classification:** Node-level.

### Padding

Padding describes the relationship between a node's outer box and its own content/children. It is meaningful for both leaf content and containers.

**Classification:** Node-level.

### Position mode

The distinction between normal layout participation and an absolute/overlay mode can be meaningful independently of the specific container algorithm, although the exact interpretation is performed by the parent/layout engine.

**Classification:** likely Node-level configuration, framework-interpreted.

### Position

The existence of an explicit position value belongs to the node. Whether the value is authoritative depends on `PositionMode` and the active layout system.

**Classification:** Node-level state.

## 3. Properties that need caution

### Border

Border contributes to the node's box geometry and rendering. The current layout manager already treats border as part of box/content conversion. fileciteturn201file0

**Classification:** likely Node-level.

However, border is also visual state. The framework should avoid letting border styling force layout ownership into Node beyond the geometric contribution it already has.

### Overflow

Overflow primarily affects clipping/rendering, but can also influence how developers perceive layout boundaries.

**Classification:** Node-level rendering/geometry state; not a layout algorithm property.

### Visibility

Visibility potentially affects layout participation as well as rendering. The framework should define one consistent semantic meaning.

**Classification:** Node-level state with framework-owned layout consequences.

### Enabled / Focusable / Capturable

These are runtime/input state, not layout state.

**Classification:** Node-level runtime state, but outside the layout subsystem.

## 4. Alignment is not necessarily purely Node-level

The current common `Alignment` enum contains:

```text
START
CENTER
END
SPACE_BETWEEN
STRETCH
```

This already mixes semantics that belong naturally to different scopes. `SPACE_BETWEEN` is clearly a container distribution concept, while `STRETCH` may apply to child allocation. fileciteturn213file0

Therefore it is risky to expose one universal `Alignment` property on every Node and assume it means the same thing in all layout contexts.

A cleaner future model may separate:

```text
container main-axis distribution
container cross-axis alignment
optional child alignment override
```

This is one of the strongest reasons not to simply copy the current `Alignment` enum into a CSS-like universal property bag.

## 5. Gap is container-owned

Gap belongs to the layout relationship among siblings. It does not describe the intrinsic state of an individual child.

**Classification:** framework container/layout state.

This is especially attractive for the minimal one-dimensional layout because it provides useful spacing without introducing the larger semantics of `margin`.

## 6. Margin should not yet be universal Node state

A margin value describes how a node participates in the parent layout. Its exact effect depends on the parent algorithm.

Unlike padding, which modifies a node's own content box, margin is fundamentally relationship-oriented.

**Classification:** defer; if introduced, likely parent/layout relationship semantics rather than universal Node geometry.

## 7. Text alignment

The existing `TextAlignment` enum is specifically text content semantics and should not be treated as general Node alignment.

```text
TextAlignment
    → content layout/rendering inside text
```

**Classification:** text/content component state.

This reinforces the separation between generic node layout and content-specific measurement/rendering.

## 8. Desired and actual geometry are outputs, not client properties

The current Node stores:

```text
desiredSize
actualSize
actualPosition
```

These should remain framework-owned outputs.

A client should not directly set them or treat them as normal configuration properties.

They are results of:

```text
state + constraints + parent layout
```

**Classification:** framework runtime state.

## 9. LayoutConstraints should remain an internal framework concept initially

The current `LayoutConstraints` structure provides min/max bounds and a clamp operation. fileciteturn213file0

This is useful internally for layout, but it does not follow that the client should manipulate `LayoutConstraints` directly.

The client should configure node properties such as size/min/max. The framework should derive the constraints passed during the layout pass.

**Classification:** framework-internal mechanics.

## 10. MeasureContext / ArrangeContext should not be public client configuration APIs

The current `MeasureContext` and `ArrangeContext` expose:

```text
availableSize
measureChild
contentPosition
contentSize
placeChild
```

These are useful internal orchestration primitives but are exactly the contract that historically forced component authors to understand layout internals. fileciteturn213file0

Under the closed-layout model, these should move toward framework-internal status rather than remain part of a client extensibility surface.

## 11. Content-specific state

Text properties such as:

```text
text
font
wrapping
text alignment
```

should belong to text/content components, not `Node`.

The framework can classify them as measurement-affecting state without making the layout engine know their implementation details.

## 12. Container-specific state

For the current minimal Phase 2 direction:

```text
orientation
main distribution
cross alignment
gap
```

should belong to framework-provided one-dimensional container types.

They should not be universal Node properties.

## 13. Positioning

The current `PositionMode` distinguishes `Layout` and `Absolute`. fileciteturn213file0

This is a useful universal primitive, but the framework still needs to define how absolute children interact with parent measurement and content box geometry.

The property itself can remain Node-level while its semantics remain framework-owned.

## 14. Proposed responsibility matrix

| State / concept | Node | Panel / container | Content component | LayoutManager |
|---|---:|---:|---:|---:|
| size | ✓ |  |  | interprets |
| min/max | ✓ |  |  | interprets |
| padding | ✓ |  |  | interprets |
| border | ✓ |  |  | interprets |
| position | ✓ |  |  | interprets |
| position mode | ✓ |  |  | interprets |
| visibility | ✓ |  |  | interprets |
| gap |  | ✓ |  | executes |
| orientation |  | ✓ |  | executes |
| main/cross alignment |  | ✓ |  | executes |
| Grid metadata |  | future specialized container |  | executes |
| Flex metadata |  | future specialized container |  | executes |
| text |  |  | ✓ | consumes measured size |
| font |  |  | ✓ | consumes measured size |
| text alignment |  |  | ✓ | consumes measured size/rendering |
| desired size | framework output | framework output |  | ✓ |
| actual rect | framework output | framework output |  | ✓ |
| constraints |  |  |  | ✓ |
| MeasureContext |  |  |  | ✓ |
| ArrangeContext |  |  |  | ✓ |

## 15. Important finding about the current `Alignment` enum

The presence of `SPACE_BETWEEN` inside the generic `Alignment` enum is a sign that the current type-level vocabulary has not yet fully separated:

```text
node state
container semantics
layout engine operations
```

This is not necessarily a bug in Phase 1. It is useful evidence for Phase 2 design: avoid adding more universal enums until their scope is clear.

## 16. Property invalidation implications

Once ownership is classified, invalidation can become simpler.

Examples:

```text
Node.width changed
    → framework measurement/layout invalidation

Node.padding changed
    → framework measurement/layout invalidation

Text.text changed
    → content measurement invalidation

Container.gap changed
    → owning container layout invalidation

Container.orientation changed
    → owning container layout invalidation
```

The important principle is that the client only mutates state. The framework translates state changes into layout work.

## 17. Current conclusion

The current Node already contains a mostly defensible set of universal geometry/runtime properties. The more important Phase 2 change is not to remove all layout state from Node, but to stop using Node inheritance as the location of layout algorithms.

The strongest current split is:

```text
Node
    common, parent-independent geometry/runtime state

PanelNode
    child ownership and structure

Framework container types
    layout-specific configuration

Content components
    content-specific measurement/rendering state

LayoutManager
    constraints, algorithms, invalidation execution, geometry outputs
```

This preserves the natural C++ inheritance model while closing the layout algorithm contract.

No implementation decision is made by this document.
