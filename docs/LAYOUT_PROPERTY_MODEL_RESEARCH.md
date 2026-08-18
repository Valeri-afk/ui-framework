# Layout Property Model Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-18

This document examines how framework-managed properties could carry invalidation semantics without turning the C++ framework into a full dependency-property system.

## 1. Current implementation observation

Today a live `Node` layout mutation goes through `deferLayoutMutation()`, which applies the mutation through `NodeTree` and then queues the node for layout. This gives the framework ownership of scheduling, but it currently treats layout mutation broadly rather than classifying the exact phase affected by each property. fileciteturn124file0

The existing Node API includes properties such as position, size, min/max, padding, border, overflow, visibility and position mode. fileciteturn121file0

The current layout engine then performs a full recursive Measure/Arrange operation for a queued root. fileciteturn125file0

This is a useful correctness baseline, but it does not yet represent fine-grained property dependency semantics.

## 2. Four possible property models

### A. Hard-coded setter semantics

Each setter explicitly knows what to invalidate:

```cpp
setWidth(...)
    -> invalidateMeasure();

setAlignment(...)
    -> invalidateArrange();
```

**Advantages**

- very simple;
- zero property registry;
- excellent compile-time clarity;
- natural C++ implementation.

**Problems**

- semantics are distributed across many setters;
- relationship-specific metadata still needs parent-aware invalidation;
- adding new properties requires touching implementation code.

### B. Central static property metadata

Each framework-managed property has a small descriptor:

```text
Property
  affectsMeasure
  affectsArrange
  affectsRender
  affectsParentMeasure
  affectsParentArrange
```

Setters use the descriptor to request the appropriate invalidation.

**Advantages**

- central, auditable semantics;
- still relatively small;
- no generic string/object property storage required;
- possible to keep storage strongly typed and direct.

**Problems**

- some properties have context-dependent effects;
- relationship-specific properties need access to the active parent layout.

### C. Property change callbacks

Each property setter invokes a framework callback or policy object that determines its dependencies.

**Advantages**

- flexible;
- supports dynamic dependencies.

**Problems**

- more runtime machinery;
- harder to reason about than static metadata;
- can become a general dependency graph by accident.

### D. Full dependency-property system

A WPF-style property system can carry metadata, inheritance, coercion, value precedence, bindings and invalidation semantics. WPF's property metadata supports flags such as `AffectsMeasure`, `AffectsArrange`, `AffectsRender` and parent-affecting flags. citeturn497893search0turn497893search3

**Advantages**

- powerful and extensible;
- strong integration between properties and framework behavior.

**Problems**

- enormous architectural scope;
- runtime complexity;
- unnecessary features for the current framework;
- can become a framework-within-the-framework.

## 3. Current strongest direction

For a small C++ retained-mode framework, the strongest current hypothesis is:

> **typed framework-managed state + small static dependency metadata, without a general-purpose property registry.**

Conceptually:

```text
setWidth()
   ↓
store typed width value
   ↓
framework knows: affects Measure
   ↓
queue layout
```

This provides the important property-system benefit — automatic invalidation — without requiring CSS-like dynamic property lookup or a WPF-scale dependency framework.

## 4. Universal properties vs relationship properties

The earlier lifecycle research suggests two classes.

### Universal Node properties

These are meaningful independently of the parent:

```text
width
height
min/max
padding
border
margin (if introduced)
position
position mode
visibility
alignment
```

Their invalidation metadata can live directly with the Node state.

### Relationship-specific layout properties

These depend on the active parent policy:

```text
Grid row / column / span
Flex grow / shrink / basis / order
Dock side
```

Their mutation is better owned by the active parent policy, because the parent already knows the semantic dependency and invalidation target.

## 5. Why relationship properties should not pretend to be universal properties

A property such as `GridRow` has no intrinsic meaning when a Node is:

```text
unparented
inside Stack
inside Flex
inside Overlay
```

The active parent gives the property meaning.

Therefore forcing every Node to expose all possible parent-specific properties would create the exact "unneeded properties on components" problem that motivated the earlier architecture changes.

## 6. Parent-owned property semantics

For relationship properties:

```text
GridPolicy.setPlacement(child, placement)
```

can directly perform:

```text
store GridPlacement
invalidate Grid layout
```

The child never needs to understand that a Grid exists.

This also makes invalidation ownership explicit.

## 7. Could the public API still be declarative?

Yes, but this is a separate decision from storage.

A public facade could eventually allow:

```text
child.layout().margin(...)
```

while a Grid-specific value is still internally stored and interpreted by Grid.

However, a CSS-like facade should only be added if it makes the C++ API genuinely clearer. The internal architecture should not be distorted to reproduce CSS syntax.

## 8. Dependency categories should be small

The research currently supports only a small set:

```text
Measure
Arrange
Render
```

Parent propagation is better treated as an effect of the active layout relationship than as a universal property flag where possible.

For example:

```text
Text content changed
    -> node measurement changed
    -> parent layout determines whether its Measure depends on child size
```

and:

```text
Grid row changed
    -> Grid policy directly knows it must recompute
```

This avoids needing every property descriptor to understand every possible ancestor.

## 9. Example property classification

Conceptually:

| Property | Local phase | Parent effect |
|---|---|---|
| Width | Measure | layout strategy decides propagation |
| Height | Measure | layout strategy decides propagation |
| Min/Max | Measure | layout strategy decides propagation |
| Padding | Measure | layout strategy decides propagation |
| Border | Measure | layout strategy decides propagation |
| Position | Arrange | usually none |
| PositionMode | Measure/Arrange | parent may be affected if flow participation changes |
| Alignment | Arrange | usually none |
| Visibility | Measure/Arrange/Render | parent flow may change |
| Background | Render | none |
| Text content | Measure/Render | parent may be affected by desired-size change |
| Grid placement | parent policy | parent policy |
| Flex item data | parent policy | parent policy |

This is a research classification, not a final API table.

## 10. The important distinction: property mutation vs layout execution

The property system should answer:

```text
"What dependency was invalidated?"
```

It should not answer:

```text
"How do I execute layout right now?"
```

Execution belongs to `LayoutManager` / `LayoutEngine`.

This prevents property setters from gaining hidden synchronous layout behavior.

## 11. Coalescing remains essential

Several property changes may occur in one client operation:

```text
setWidth()
setPadding()
setAlignment()
```

The framework should coalesce those into one queued layout operation rather than measure after each setter.

This fits the current deferred mutation model and avoids forcing client code to batch layout manually. fileciteturn124file0

## 12. Custom content properties

Text-specific state such as:

```text
text
font
wrap behavior
```

should be framework-visible if the framework is responsible for invalidation, but their measurement implementation remains content-specific.

The property semantics can say:

```text
text -> Measure + Render
font -> Measure + Render
```

without the layout engine knowing anything about TTF.

## 13. Custom layout properties

A custom layout strategy may have its own configuration:

```text
RadialLayout.radius
RadialLayout.startAngle
RadialLayout.spacing
```

Changing those values should invalidate the owning container layout directly.

This suggests custom strategy configuration should be owned by the strategy/container rather than becoming global Node properties.

## 14. Avoiding a generic property dictionary

A tempting design is:

```text
Node
  unordered_map<PropertyKey, Any>
```

The research does not currently justify it.

Reasons:

- layout is a hot path;
- universal properties are known at compile time;
- relationship properties have natural owners;
- generic storage adds runtime lookup/type-erasure costs;
- debugging becomes harder;
- it can recreate a hidden God Object.

A small typed C++ framework benefits from explicit state more than from maximal dynamic extensibility.

## 15. Current conclusion

The strongest current hypothesis is:

```text
Node
 └── explicit typed universal layout state
        + small static invalidation semantics

Panel / LayoutPolicy
 └── typed relationship-specific child metadata
        + direct policy-owned invalidation

LayoutEngine
 └── schedules and executes Measure/Arrange
```

This architecture gives the framework automatic invalidation without requiring a giant property system and without requiring the client to understand layout internals.

It also respects the C++ retained-mode constraint that data ownership should be explicit and predictable.

## 16. Remaining questions

1. Which current Node properties should actually be considered layout properties after the final Phase 2 redesign?
2. Should alignment remain universal Node state or be interpreted entirely by the parent policy?
3. Should margin be introduced as universal geometry state, or remain a property of the parent layout relationship?
4. How should custom content measurement state notify the framework without exposing invalidation APIs?
5. How much property metadata is needed before static classification becomes cumbersome?
6. Can the current deferred NodeTree mutation mechanism host this classification cleanly?

No implementation decision is made by this document.
