# Layout Algorithm Feasibility Analysis

> **Status:** research / no implementation decision
> **Date:** 2026-08-18

This document stress-tests the current layout hypothesis against concrete algorithms rather than assuming that one generic protocol is equally suitable for Stack, Grid and Flex.

## 1. Test protocol

The candidate internal pipeline is:

```text
parent proposal / constraints
        ↓
child content measurement
        ↓
container resolves its own size
        ↓
container allocates child rectangles
        ↓
framework applies final geometry
```

The questions are:

- Can each layout be expressed naturally?
- How many child measurements are required?
- Does the container need child measurement before it knows the proposal for that child?
- Can the algorithm remain deterministic without circular queries?
- Where does invalidation belong?
- Does the algorithm require richer intrinsic queries or multi-pass measurement?

---

## 2. Text as a leaf

For a text leaf:

```text
proposal width = finite
proposal height = unbounded
        ↓
text measurement
        ↓
Size(width, requiredHeight)
```

This is straightforward.

The historical `Label` already demonstrated essentially this relationship: it measured wrapped text using the available width and returned a generic `LayoutSize`. fileciteturn120file0

Conclusion:

> proposal-driven content measurement is a natural fit for text.

---

## 3. Stack / Column

A vertical Stack can be modeled as:

```text
parent gives bounded width
height may be unbounded during Measure

for each child:
    measure child with
        width = parent's available width
        height = unbounded

sum child heights
max child width
```

The current `StackPanelNode` already follows this general structure: it passes an effectively unbounded main-axis proposal and accumulates the children along the main axis. fileciteturn126file0

### Feasibility

**Very high.**

No advanced intrinsic query system is required for the first useful version.

### Limitation of current implementation

The current implementation fills the cross-axis size during Arrange and does not yet implement the full alignment/gap semantics discussed earlier. fileciteturn126file0

That is a container policy gap, not a failure of the underlying proposal/measurement model.

---

## 4. Stack with text

Consider:

```text
Vertical Stack
  ├── Text A
  ├── Button
  └── Text B
```

The pipeline is straightforward:

```text
Stack proposal
   ↓
Text A proposal
   ↓
Text A size

Button content measurement
   ↓
Button size

Text B proposal
   ↓
Text B size

sum heights
   ↓
Stack desired size
```

A text change automatically invalidating Measure can cause the Stack to be remeasured. No part of this requires the Stack to know about TTF/text internals.

### Conclusion

This strongly supports a framework-owned layout engine plus generic content measurement.

---

## 5. Grid with fixed columns and Auto rows

Consider:

```text
columns: 300px
rows: Auto

child: Text
```

The algorithm can determine the child's width first:

```text
fixed column = 300
        ↓
Text proposal width = 300
        ↓
Text desired height
        ↓
Auto row height
```

This is deterministic and requires only ordinary proposal-driven measurement.

### Conclusion

A compact Grid can support a useful subset of two-dimensional layout without intrinsic-query machinery.

---

## 6. Grid with Auto columns + wrapped Text

Now consider:

```text
columns: Auto, Auto
rows: Auto

child Text spans one Auto column
```

The desired width of the text may depend on its own intrinsic content, while its desired height depends on the width selected for the column.

This creates a dependency:

```text
column width
   ↓
text wrapped height
   ↓
row height
```

If the column width itself is chosen from the text's intrinsic width:

```text
text intrinsic width
   ↓
column width
   ↓
text wrapping
   ↓
text height
```

This is not necessarily an infinite loop, but it means the Grid algorithm must define an ordering for intrinsic width vs constrained height.

### Conclusion

A fully general Grid is more than a simple two-pass recursive traversal.

A Phase 2 Grid should therefore deliberately choose a restricted sizing model before attempting CSS-level behavior.

---

## 7. Grid with spanning

Suppose:

```text
columns: Auto, 1fr
child spans both columns
```

The child's desired width can contribute to multiple tracks simultaneously.

This can be resolved with a specialized track-sizing algorithm, but the algorithm is no longer simply:

```text
measure child
→ assign one track
```

Spanning creates parent-level allocation dependencies.

### Conclusion

Spanning is still reasonable for Phase 2, but it must be treated as part of the Grid algorithm itself rather than as generic child behavior.

---

## 8. Flex / Row with intrinsic content

A simple horizontal Flex-like container can start from:

```text
child intrinsic/basis sizes
        ↓
sum basis sizes
        ↓
available width - basis
        ↓
grow/shrink distribution
        ↓
final child widths
```

For non-wrapping Flex, this can potentially be implemented without repeatedly measuring each child after the final allocation if the child content can tolerate the difference between its measured basis and its final width.

However, text wrapping exposes a problem:

```text
Text basis width
   ↓
Text basis height

Flex shrinks Text width
   ↓
Text height may change
```

If the container's cross-axis size depends on final text height, a remeasurement can become necessary.

### Conclusion

Flex is significantly more sensitive to the choice of measurement protocol than Stack.

---

## 9. Flex wrapping

Flex wrap adds another dependency:

```text
child basis sizes
    ↓
line breaking
    ↓
line width allocation
    ↓
child final width
    ↓
text height / cross-size
```

A fully general wrapping Flex layout therefore pushes toward multiple measurement stages or richer intrinsic queries.

### Conclusion

A full Flexbox implementation should not automatically be part of the first Phase 2 implementation simply because a historical FlexPanel existed.

The historical `flex_panel.cpp` is evidence that grow/shrink/wrap/gap/alignment were previously useful, but those capabilities carry algorithmic complexity that the current framework's minimal measurement model does not automatically solve. fileciteturn117file0

---

## 10. Overlay / absolute positioning

Overlay-like layout is much simpler:

```text
parent proposal
   ↓
children measure
   ↓
parent takes its own constrained size
   ↓
children receive independent final rectangles
```

Absolute positioning may not contribute to parent flow measurement at all.

This is a strong candidate for an early built-in policy because it does not require complicated sibling dependency resolution.

---

## 11. The important distinction: measure result vs final allocated size

A layout algorithm may receive:

```text
child desired size = 500x100
```

and allocate:

```text
child final size = 300x100
```

This is normal in a constrained parent.

But if the child's content changes its behavior based on final width, such as wrapping Text, then the layout system may need a second measurement under the final width to determine final height.

This is the fundamental reason that a completely single-measure-only protocol cannot express every advanced layout algorithm.

The framework therefore has to choose one of:

```text
A. restrict algorithms so one measurement is enough;
B. allow controlled remeasurement;
C. add explicit intrinsic queries;
D. use algorithm-specific multi-pass behavior.
```

Different mature frameworks choose different points in this space. Flutter, Compose and SwiftUI provide different combinations of constraints, intrinsic queries, repeated proposals and algorithm-specific rules. citeturn260230search0turn260230search2turn260230search6

---

## 12. What can be expressed with a simple first version

A conservative first engine can support very well:

```text
Leaf intrinsic/content measurement
Fixed sizes
Min/max constraints
Vertical/Horizontal Stack
Simple Grid
Simple absolute/overlay positioning
Basic alignment
```

These can all use a predictable measure → arrange pipeline.

The engine does not need to solve every CSS/Flexbox intrinsic sizing problem to be useful.

---

## 13. What should probably be deferred

The following should require concrete application justification before implementation:

- full Flexbox grow/shrink + wrapping + intrinsic interaction;
- CSS-level Grid intrinsic track sizing;
- arbitrary cross-axis dependencies;
- multiple remeasure loops until convergence;
- general-purpose intrinsic min/max query APIs;
- global dependency graphs for arbitrary layout equations.

These features can be added after the core measurement boundary is stable.

---

## 14. Current source implications

The existing framework already has several useful pieces:

- `MeasureContext::availableSize` and `measureChild`; fileciteturn123file0
- recursive `measureRecursive()` and `arrangeRecursive()`; fileciteturn125file0
- min/max normalization and box conversion; fileciteturn125file0
- Stack's main-axis unbounded measurement; fileciteturn126file0
- framework-owned deferred layout mutation and queueing; fileciteturn124file0
- historical text measurement under available width. fileciteturn120file0

Therefore the remaining problem is not discovering whether Measure/Arrange can work at all. The problem is deciding which parts should remain as internal framework mechanisms and which should become explicit, narrow extension points.

## 15. Current architectural test

A candidate Phase 2 architecture should pass all of these without client-side invalidation:

```text
Text inside Stack
Text inside Grid fixed column
Text inside Grid Auto row
Button whose size depends on Text
Image with intrinsic size
Absolute child
Simple Flex row
Custom RichText measurement
Custom container placement
```

It should remain deterministic and should not require the layout engine to know content-specific implementation details.

## 16. Current conclusion

The hypothesis remains viable, but the feasibility test reveals an important boundary:

> **The generic proposal → measurement → placement model is enough for a strong core UI layout system, but advanced Grid/Flex intrinsic behavior may require algorithm-specific additional measurement semantics.**

Therefore the framework should not design the core around the assumption that every future layout can be expressed by exactly one Measure call and exactly one Arrange call.

At the same time, it should also not introduce a general multi-pass intrinsic engine before a real layout requires one.

The safest direction remains:

```text
simple deterministic core
+
framework-owned invalidation/scheduling
+
content-specific measurement
+
algorithm-specific complexity only where justified
```

No implementation decision is made by this document.
