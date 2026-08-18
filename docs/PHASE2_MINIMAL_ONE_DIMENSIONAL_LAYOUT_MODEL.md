# Phase 2 Minimal One-Dimensional Layout Model

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document tests whether the current Phase 2 requirements can be expressed by one framework-owned one-dimensional layout algorithm rather than separate Stack/Row/Column systems or a full Flexbox implementation.

## 1. Candidate model

The minimal container has:

```text
orientation: horizontal | vertical
gap
main-axis distribution
a cross-axis alignment policy
```

The framework supplies constraints and child measurement. The algorithm computes the container size and final child rectangles.

## 2. Measurement model

For a vertical container:

```text
proposal width  = bounded by parent when available
proposal height = unbounded or parent-constrained according to container context

measure children
sum main-axis sizes + gaps
max cross-axis size
```

For horizontal layout, the axes are exchanged.

This is essentially the behavior already demonstrated by the current StackPanel implementation, whose vertical path measures children with an unbounded main axis and accumulates their sizes. fileciteturn126file0

## 3. Container desired size

After child measurement:

```text
main = sum(child main sizes) + gap * (count - 1)
cross = max(child cross sizes)
```

Then the framework's normal Node rules apply:

```text
padding/border
requested size
min/max
parent constraints
```

This keeps container algorithm and common Node geometry semantics separate.

## 4. Child final allocation

The container has a final content-box allocation.

The algorithm computes the remaining main-axis free space:

```text
freeSpace = availableMain - requiredMain
```

Then applies a selected policy.

Initial policies can remain small:

```text
start
center
end
```

`space-between` may be added because it is already represented in current alignment vocabulary, but it should be explicitly treated as a container distribution rule rather than a universal Node alignment property. fileciteturn213file0

## 5. Cross-axis allocation

Initial cross-axis policies can be:

```text
start
center
end
stretch
```

The child can retain its measured cross-axis size except when the policy is `stretch`.

A per-child override is not necessary until an actual UI scenario requires it.

## 6. Why `grow/shrink` can be deferred

The one-dimensional algorithm can already support:

```text
fixed-size children
content-sized children
alignment
fixed gap
```

without flex grow/shrink.

Grow/shrink becomes important when the framework must intentionally redistribute remaining or deficient space among children.

That introduces additional questions around basis, remeasurement and text wrapping.

Because current Phase 2 requirements do not require it, omitting grow/shrink keeps the first algorithm deterministic and simpler.

## 7. Why `wrap` can be deferred

Wrapping changes the problem from one-dimensional sequencing into line formation:

```text
measure children
→ decide line breaks
→ determine line sizes
→ distribute cross-axis space
→ place lines
```

This introduces more measurement/allocation interactions and is not needed for the current minimal UI scope.

## 8. Text inside the one-dimensional container

The most important case remains:

```text
Vertical container
    ↓
Text
```

The parent provides a bounded cross-axis proposal (typically width).

Text computes its wrapped height.

The container sums that height into its main-axis desired size.

This requires no Grid and no general intrinsic query engine.

## 9. Button inside the one-dimensional container

A framework Button can participate as a leaf/content-bearing component:

```text
measure Button content
    ↓
Button desired size
```

The container sees only the generic size result.

The container does not need to know that Button uses Text internally.

## 10. Fixed child vs content-sized child

The algorithm should distinguish the final proposal from the content's desired size.

For example:

```text
child width = fixed 100
```

should give the child a finite width proposal before content measurement.

For:

```text
child width = automatic / fill according to container policy
```

the container may use the content's desired size or its own available cross-axis size according to a defined policy.

These semantics must be specified explicitly rather than inferred from CSS names.

## 11. Position mode / absolute child

Absolute positioning is not naturally part of the same flow algorithm.

The minimal model can therefore define:

```text
normal-flow children
absolute/overlay children
```

The flow pass handles only normal children.

A second framework positioning step handles absolute children.

This is still framework-owned and does not require a custom layout API.

The current Node already has `PositionMode`, making this a possible continuation of existing semantics. fileciteturn213file0

## 12. Gap vs margin

The minimal algorithm should prefer:

```text
gap = spacing between flow items
```

and leave `margin` undefined for now.

This avoids relationship semantics that are not necessary for current requirements.

## 13. Container configuration ownership

Configuration belongs to concrete framework containers, not to every Node.

Conceptually:

```text
StackPanel
    orientation
    gap
    alignment

Future FlexPanel
    direction
    gap
    distribution
```

`PanelNode` remains structural.

## 14. Could Stack and Flex be one algorithm?

At the minimal feature set, yes.

A one-dimensional layout algorithm can represent:

```text
Vertical stack
Horizontal row
```

by changing orientation.

It can later be extended with:

```text
free-space distribution
```

without changing its conceptual pipeline.

Therefore the framework may not need separate internal algorithm machinery for Stack vs Row/Column at Phase 2.

A future full Flex implementation can still become a richer algorithm later if grow/shrink/wrap are required.

## 15. Why not call the initial system "Flexbox"

The initial semantics are intentionally smaller than CSS Flexbox.

Calling it Flexbox would create expectations around:

- flex-basis;
- grow/shrink;
- wrapping;
- order;
- min-content/max-content interactions;
- exact CSS alignment behavior.

The framework should define its own minimal semantics instead of promising a standard it does not implement.

## 16. Node geometry remains framework-wide

The layout algorithm should consume existing Node-level state such as:

```text
size
min/max
padding
border
position mode
visibility
```

and produce:

```text
desiredSize
actualSize
actualPosition
```

The current Node already stores these concepts and the LayoutManager already handles much of their normalization. fileciteturn221file0turn201file0

## 17. Invalidation implications

Container configuration changes:

```text
gap
orientation
alignment
```

invalidate the owning container's layout.

Text content changes invalidate content measurement.

Parent proposal changes naturally cause a new measurement query without requiring a child mutation.

The existing deferred NodeTree mutation mechanism is compatible with this model. fileciteturn221file0turn223file0

## 18. Complexity boundary

The minimal algorithm remains roughly linear in participating children for ordinary flow:

```text
measure each child once
calculate aggregate size
place each child once
```

This is a valuable property for Phase 2.

Features that would break the simple model should be treated as deliberate extensions:

```text
grow/shrink with width-dependent text
wrap
multi-line distribution
full intrinsic queries
```

## 19. What this model can already express

The current model can represent:

```text
Vertical forms
Horizontal toolbars
Headers / footers
Button content
Simple dialog content
Navigation rows
Lists with fixed/content-sized children
Spacing through gap
Basic alignment
Padding/border-based components
```

This covers the currently stated Phase 2 requirements far more directly than Grid.

## 20. Current conclusion

The research supports a strong and deliberately small Phase 2 layout core:

```text
Node
 └── common geometry/runtime state

PanelNode
 └── child structure

Framework one-dimensional container
 ├── horizontal / vertical
 ├── gap
 ├── main distribution
 └── cross alignment

Framework content components
 └── intrinsic measurement

LayoutManager
 └── closed measurement + arrangement + invalidation execution
```

No Grid is required for this foundation.

No public custom layout is required.

No full CSS Flexbox implementation is required.

The remaining architecture question is how concrete framework components expose their intrinsic measurement to the closed LayoutManager, which remains an internal implementation boundary rather than a client contract.

No implementation decision is made by this document.
