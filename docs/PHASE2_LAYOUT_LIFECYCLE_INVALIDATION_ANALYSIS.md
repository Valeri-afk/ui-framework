# Phase 2 Layout Lifecycle and Invalidation Analysis

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document traces state changes through the proposed closed layout system and checks whether the framework can keep invalidation entirely internal.

## 1. Core lifecycle

The target lifecycle is:

```text
client/framework state mutation
        ↓
framework classifies affected state
        ↓
layout/render work is queued
        ↓
mutation scope is flushed
        ↓
layout pass
        ├── measure
        ├── resolve container geometry
        └── arrange
        ↓
render
```

The client does not call layout invalidation APIs directly.

## 2. Current source already has a deferred mutation boundary

`Node::deferLayoutMutation()` routes layout-affecting state changes through `NodeTree::enqueueNodeMutation()` and inserts the node into the layout queue after the mutation is applied. fileciteturn221file0

`NodeTree` also has a mutation scope and queue drain mechanism. fileciteturn223file0

This is a strong Phase 1 foundation because it prevents every setter from immediately running layout.

## 3. Current limitation: invalidation is coarse

Today the mutation helper essentially says:

```text
layout mutation
    → queue this node for layout
```

It does not yet distinguish:

```text
Measure dirty
Arrange dirty
Render dirty
```

or explicitly encode parent dependency.

This is acceptable as a correctness-first baseline but should not become the final semantics if we want efficient layout behavior.

## 4. Text content change

Example:

```text
Text.text = newValue
```

The conceptual state transition is:

```text
Text content changed
    ↓
Text measurement may have changed
    ↓
framework schedules layout work
    ↓
next pass measures Text under current proposal
    ↓
parent uses new desired size
```

Text does not call `markLayout()` and does not know its parent layout algorithm.

## 5. Font or wrapping change

The same semantic category applies to:

```text
font
font metrics
wrap policy
text shaping settings
```

These affect intrinsic measurement and therefore should be treated as measurement-affecting state.

Rendering-only changes such as text color need not force measurement.

The framework can therefore eventually distinguish:

```text
text content/font/wrap → Measure + Render
text color             → Render
```

without the text implementation scheduling either operation itself.

## 6. Node geometry property change

Examples:

```text
width
height
min/max
padding
position mode
```

These can affect the result of parent layout and/or the node's own measurement.

The framework should coalesce them into one layout transaction rather than executing a pass per setter.

Current deferred mutation already provides this basic coalescing boundary. fileciteturn221file0

## 7. Container configuration change

Example:

```text
Stack.gap changed
Stack.orientation changed
```

The owning container is the semantic layout unit.

The correct conceptual result is:

```text
container layout configuration changed
        ↓
container layout invalid
        ↓
framework schedules layout
```

The layout algorithm does not need a public invalidation API.

## 8. Child relationship change

When a child is added or removed:

```text
Panel child structure changed
       ↓
container desired geometry may change
       ↓
ancestor geometry may change
```

The current `NodeTree` already centralizes attach/remove operations and can therefore become the natural place to trigger the appropriate framework layout invalidation. fileciteturn223file0

This is another reason not to expose tree mutation responsibility to layout algorithms.

## 9. The key distinction: local dirty state vs propagated dependency

A property mutation should first create a local fact:

```text
Text measurement dirty
```

or:

```text
Container layout dirty
```

Then the framework determines how that affects ancestors.

This is preferable to every property carrying a full ancestor-propagation rule.

## 10. Parent dependency examples

### Vertical container

```text
Text height changed
    ↓
Column desired height changed
    ↓
Column parent may need remeasurement
```

### Fixed-height container

```text
Text height changed
    ↓
container height unchanged
    ↓
parent may not need Measure
```

The active layout semantics determine the propagation.

Therefore global property metadata should stay coarse:

```text
Text content affects measurement
```

rather than:

```text
Text content affects Measure of all ancestors
```

## 11. Simplest safe propagation for Phase 2

The framework can initially conservatively propagate layout invalidation upward to the appropriate layout root and recompute the subtree.

This gives:

```text
correctness first
```

without forcing the client to understand dependency rules.

Later, the layout engine can optimize by stopping propagation where the parent's geometry is independent of the child's desired size.

The public contract remains unchanged.

## 12. Why synchronous re-layout is dangerous

A content setter called during layout must not immediately start another global layout pass.

Pathological case:

```text
measure Text
    ↓
Text setter / side effect
    ↓
invalidate parent
    ↓
parent measure recursively
```

The mutation should instead be deferred and coalesced into a subsequent pass.

This follows naturally from the existing mutation queue model. fileciteturn223file0turn221file0

## 13. Render-only state

A state change such as:

```text
background color
text color
hover visual
```

should eventually avoid Measure/Arrange when it does not affect geometry.

However, Phase 2 does not need a fully optimized render-dirty tree immediately. The architecture should merely leave room for the separation.

## 14. Visibility

Visibility is special because it can affect:

```text
render
hit testing
layout participation
```

A concrete policy should define whether `visible=false` means:

```text
render hidden but still participates in layout
```

or:

```text
removed from layout participation
```

The current framework's `getVisibleChild()` already filters children from layout traversal, which indicates that visibility currently means non-participation in child layout. fileciteturn203file0

Therefore changing visibility is correctly treated as layout-affecting state.

## 15. Parent resize

A parent size change should not require the child to know anything about invalidation.

Conceptually:

```text
parent available width changes
        ↓
layout pass starts
        ↓
child receives new proposal
        ↓
Text / content remeasures
```

In many cases this is not a mutation of the Text node itself. It is simply a new measurement context generated by the parent.

This is an important distinction:

> **Not every new measurement requires an invalidation mutation on the child.**

The layout pass can legitimately ask the child a new question because its proposal changed.

## 16. Measurement cache implication

Because proposals can change, a cache must key at least on:

```text
child identity
proposal / constraints
content state version
```

A cache keyed only by child ID would be incorrect for wrapped text and other proposal-dependent content.

The cache, if introduced, must remain framework-internal.

## 17. Actual geometry changes

The framework should distinguish:

```text
state that requests a layout
```

from:

```text
actual geometry produced by the layout pass
```

A computed `actualSize` change should not recursively enqueue a new layout pass merely because the value changed.

Otherwise a correct layout pass could oscillate or repeatedly schedule itself.

Geometry is an output of layout, not an input mutation signal by default.

## 18. Layout as a transaction-like operation

The safest conceptual Phase 2 model is:

```text
flush deferred mutations
        ↓
begin layout pass
        ↓
measure
        ↓
arrange
        ↓
commit geometry
        ↓
end layout pass
        ↓
process resulting render work
```

If a state mutation occurs during measurement/arrangement, it is deferred to a later pass.

This makes re-entrancy behavior deterministic.

## 19. What the client sees

A client should see only:

```text
setText()
setWidth()
setPadding()
setGap()
setOrientation()
```

and the framework handles:

```text
invalidating
queuing
measuring
arranging
rendering
```

This is the practical meaning of closing the layout contract.

## 20. Current architecture verdict

The existing Phase 1 mutation queue is compatible with the closed Phase 2 model.

The major next architectural improvement is not a new public invalidation API. It is **internal dirty semantics**:

```text
Measure
Arrange
Render
```

plus framework-controlled dependency propagation.

The initial implementation can remain conservative and re-layout a queued subtree/root for correctness. Fine-grained invalidation can come later.

## 21. Remaining unresolved detail

Before coding the Phase 2 layout core, we still need to choose where framework-provided content measurement lives:

```text
inside concrete Node component implementation
through internal callback/ops
through an internal content object
or another framework-owned mechanism
```

But regardless of that choice, the invalidation contract itself should stay framework-owned.

No implementation decision is made by this document.
