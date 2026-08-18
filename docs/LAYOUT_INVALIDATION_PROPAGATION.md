# Layout Invalidation Propagation

> **Status:** research / no implementation decision
> **Date:** 2026-08-18

This document analyzes how layout invalidation propagates through a retained-mode tree and which changes require Measure, Arrange, or only Render work.

## 1. Why propagation matters

A local change can have effects at multiple levels:

```text
leaf content change
      ↓
leaf desired size changes
      ↓
parent allocation may change
      ↓
sibling geometry may change
      ↓
ancestor size may change
```

A correct invalidation system therefore cannot simply mark the changed node dirty and stop.

At the same time, invalidating the entire tree for every change is correct but unnecessarily expensive.

The architectural goal is:

> invalidate the smallest part of the tree that can be proven to depend on the changed result, without making normal client code responsible for dependency tracking.

## 2. Three levels of layout dirtiness

A useful conceptual distinction is:

```text
Measure dirty
    → this node's desired size may be different

Arrange dirty
    → this node's final position/size may be different

Render dirty
    → geometry may remain valid, but pixels/state changed
```

These states are related but not identical.

For example:

```text
alignment change
    → Arrange

background color change
    → Render

text change
    → Measure
       + potentially Arrange through ancestors
       + Render
```

## 3. Local property changes

### Width / height

A requested width/height can change the node's own measured size and therefore should begin at Measure.

Potential propagation:

```text
node Measure dirty
       ↓
parent may need Measure if it uses child desired size
```

### Padding / border

Padding/border can change the node's desired outer size and therefore can propagate upward through any parent whose own size depends on that child.

### Alignment

Alignment generally changes placement inside an already allocated rectangle, so it can often start at Arrange rather than Measure.

### Position

An explicit position change can generally begin at Arrange. If a position mode changes the child's participation in normal flow, it may also affect parent Measure.

## 4. Content changes

Text is the canonical case:

```text
text value changed
      ↓
content measurement may change
      ↓
node desired size changes
      ↓
parent layout may depend on desired size
```

Therefore content changes should conceptually start at Measure.

The framework then determines how far the resulting dependency propagates.

The client should not manually walk ancestors or call `markLayout()`.

## 5. Parent dependency classes

A parent can depend on a child in different ways.

### Parent depends on child desired size for its own Measure

Examples:

- vertical StackPanel whose height is the sum of child desired heights;
- horizontal StackPanel whose width depends on child desired widths;
- Grid with `Auto` tracks;
- a content-sized panel.

If the child's desired size changes, the parent may need Measure.

### Parent does not depend on child desired size for its own Measure

Examples may include:

- a fixed-size container;
- a container whose size is independently constrained;
- a Grid with fixed tracks where the child size cannot change track requirements.

The parent may not need another Measure, although Arrange may still be needed.

### Parent depends on child final size only for placement

Some containers know their own size and merely place children according to their final rectangles. In such cases, child changes may primarily require Arrange.

This distinction is central to minimizing invalidation.

## 6. WPF evidence

WPF explicitly models properties that affect the parent layout through `AffectsParentMeasure` and `AffectsParentArrange`, in addition to properties that affect the element's own Measure/Arrange/Render. citeturn697178search4turn697178search9

This confirms that upward invalidation is a known architectural problem, not a peculiarity of this framework.

WPF also defers invalidation rather than forcing synchronous layout work on every change. citeturn697178search2

## 7. Flutter evidence

Flutter exposes an even more explicit dependency model.

When a parent passes `parentUsesSize: true`, the parent's layout depends on the child's layout output. If the child later changes its layout information, the parent is notified. If the parent does not depend on child size, the child can change layout without automatically invalidating the parent. citeturn697178search3turn697178search7

This is a particularly useful insight:

> **whether an ancestor depends on a child's measured/layout result is itself part of the layout relationship.**

A framework can use this dependency knowledge to avoid always invalidating all ancestors.

## 8. Compose evidence

Compose separates measurement and placement into scopes and performs layout in constrained phases. Custom layouts can measure children and then place them, but measurement/placement are controlled by the framework's phase machinery. citeturn697178search5

This supports the broader principle that dependency propagation should remain inside the layout engine rather than becoming an arbitrary client responsibility.

## 9. StackPanel propagation

Consider a vertical StackPanel:

```text
StackPanel
 ├── A
 ├── B
 └── C
```

If B's desired height changes:

```text
B Measure
   ↓
StackPanel Measure
   ↓
StackPanel desired height may change
   ↓
Parent of StackPanel may need Measure
```

Then during Arrange:

```text
B final size/position
   ↓
C position may change
```

Therefore a child's content change can cause an upward Measure wave followed by a downward Arrange wave.

This is not an argument for client-side invalidation. It is an argument for making the dependency relationship a property of the framework's layout engine.

## 10. Grid propagation

Grid is more subtle.

### Fixed tracks

If a child lives in a fixed-size row/column, changing its desired size may not change the track size.

Potentially:

```text
child Measure
   ↓
Grid Arrange
```

without remeasuring Grid itself.

### Auto tracks

If the child contributes to an Auto track:

```text
child Measure
   ↓
Auto track size may change
   ↓
Grid Measure
   ↓
Grid Arrange
```

### Spanning child

A child spanning multiple Auto/Fr tracks may create cross-track dependencies and may make invalidation more expensive.

This is one reason not to implement full CSS Grid semantics prematurely.

## 11. Flex propagation

Flex-like layouts also depend on the algorithm.

If flex grow/shrink and intrinsic sizes are involved, a child's measured size can affect free-space distribution among siblings.

Therefore:

```text
child measurement change
   ↓
Flex measure/resolve
   ↓
all sibling positions/sizes may change
```

Again, the important lesson is that the **container knows the dependency relationship**.

## 12. Relationship-specific invalidation

This strengthens the earlier conclusion about parent-owned layout metadata.

For example:

```text
GridPlacement.row changed
```

The framework knows:

```text
Grid interprets GridPlacement
```

so it can invalidate the Grid layout appropriately.

Likewise:

```text
FlexItem.grow changed
```

can invalidate the active Flex layout.

The child does not need to know which ancestor understands the property.

## 13. Invalidation wave model

A useful conceptual model is:

```text
Property change
      ↓
classify dependency
      ↓
mark local Measure/Arrange/Render state
      ↓
propagate upward only while ancestor measurement depends on child result
      ↓
coalesce queued work
      ↓
run layout once
      ↓
propagate geometry downward
```

This is more precise than simply saying "walk to root and mark layout".

## 14. Can Phase 2 use root-level invalidation initially?

Yes, for correctness.

A root-level queue is easy to reason about and is already compatible with the current framework.

But the research now shows that root-level invalidation is an implementation simplification rather than the conceptual layout dependency model.

A future optimization can introduce dependency-aware dirty propagation without changing the public client contract.

Therefore Phase 2 can keep the existing root queue initially while designing the internal concepts so that more precise propagation remains possible later.

## 15. Avoiding a global dependency graph

A tempting design would be:

```text
Property
   ↓
Global dependency graph
   ↓
all affected nodes
```

This is likely too complex for the current framework.

The tree already encodes most layout dependencies:

```text
parent
  ↓
children
```

and the active layout strategy can determine whether it depends on child desired size.

A local upward walk plus policy-specific dependency information is likely simpler than a general-purpose dependency graph.

## 16. Potential internal dependency categories

A framework may eventually distinguish:

```text
NeedsMeasure
NeedsArrange
NeedsRender
```

and parent dependency such as:

```text
UsesChildDesiredSize
UsesChildGeometry
UsesChildIntrinsicData
```

The exact representation is intentionally left open.

## 17. Important consequence for custom content measurement

A custom content measurement implementation should not have to know whether its parent is Stack, Grid or Flex.

The framework asks it for a size.

The active parent strategy decides whether that size participates in:

- its own Measure;
- its Arrange;
- sibling distribution;
- ancestor propagation.

This keeps content and container responsibilities separated.

## 18. Important consequence for custom container layout

A custom container strategy may need to declare or implicitly communicate what child outputs it depends on.

However, the client author should ideally not manage ancestor invalidation directly.

A framework-owned custom-layout scope can provide the dependency semantics, while the extension only defines geometry.

This is a key area for later design if custom layout remains part of the final architecture.

## 19. Current conclusion

The strongest research result is:

> **Invalidation propagation is a property of layout dependencies, not a responsibility of content/component authors.**

The framework should know:

```text
which state affects Measure
which state affects Arrange
which state affects Render
which parent relationship depends on which child outputs
```

The client should mutate state and configuration through framework APIs.

The current root-level invalidation queue can remain the Phase 2 implementation baseline for correctness, while the architecture keeps the concept of dependency-aware propagation available for later optimization.

## 20. Open questions

1. Which built-in containers actually require child desired-size dependency for Measure?
2. Can Stack/Grid/Flex expose this dependency through their layout strategy rather than through individual child types?
3. Can content measurement changes be propagated without consulting the concrete content type?
4. Can relationship metadata mutations invalidate the parent directly without global property lookup?
5. Should the custom-layout extension explicitly declare dependencies, or can the framework infer them from the measurement/placement operations used during a pass?
6. Can the current `NodeTree` root-level queue later support dependency-aware dirty bits without a second runtime ownership system?

No implementation decision is made by this document.
