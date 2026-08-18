# Text-Driven Layout Pipeline Analysis

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document stress-tests the proposed closed Phase 2 layout core against the most important dependency in the system: text height depending on the width supplied by the parent.

## 1. The core scenario

Consider:

```text
Root / Container
    width = 500
    ↓
Text
    width determined by parent
    height determined by wrapping
```

The intended dependency is:

```text
container available width
        ↓
text measurement proposal
        ↓
text desired height
        ↓
container desired size
        ↓
parent geometry
        ↓
arrangement
```

This is directional rather than circular as long as the text receives its width from an ancestor proposal rather than asking the ancestor for its final size during measurement.

## 2. The current framework already has the basic shape

The current `LayoutManager` passes an available size into recursive measurement, converts between border/content sizes, and lets a Node invoke `measureChild()` for its visible children. fileciteturn132file0

The current `Node` exposes virtual `measure()` and `arrange()` hooks, which are called by the recursive layout manager. fileciteturn130file0turn132file0

Therefore the framework is already structurally close to:

```text
parent proposal
    ↓
child measurement
    ↓
desired size
    ↓
parent arrangement
```

The architectural question is about ownership and public contract, not whether the pipeline is conceptually possible.

## 3. Where the width comes from

The width of a wrapped text node can come from several places:

### Fixed node width

```text
Text width = 300
```

The measurement proposal can directly use 300.

### Parent available width

```text
Container width = 500
Text width = fill available width
```

The container supplies the relevant finite width during child measurement.

### Width constrained by min/max

The framework normalizes the available/desired size through its min/max rules.

### Unbounded width

An unconstrained or intrinsic-like measurement may produce a natural width, but this should only be requested by a layout algorithm that actually needs it.

## 4. The key invariant

The layout engine should maintain:

> **A measurement operation receives a proposal/constraint context; it does not synchronously ask its parent for a newly computed layout result.**

This prevents the dangerous cycle:

```text
parent measure
    ↓
child measure
    ↓
child asks parent to layout
    ↓
parent measure again
    ↓
...
```

Framework-owned deferred invalidation is therefore part of layout correctness, not merely an optimization.

## 5. One layout pass

For the minimal Phase 2 case:

```text
start pass
    ↓
measure root/container
    ↓
measure Text with current width proposal
    ↓
Text returns desired height
    ↓
container computes desired size
    ↓
arrange container
    ↓
place Text
    ↓
commit geometry
    ↓
end pass
```

No synchronous second global layout pass is required when the width proposal is already determined before the text measurement.

This is the normal case for:

- Text inside a fixed-width container;
- Text inside a horizontal/vertical container with a known cross-axis constraint;
- Button content measured under its available content width.

## 6. Parent resize

Now change:

```text
container width: 500 → 300
```

The framework should do:

```text
container geometry/state changes
        ↓
framework invalidation
        ↓
new layout pass
        ↓
Text receives width = 300
        ↓
Text computes new wrapped height
        ↓
container gets new desired size
        ↓
arrange
```

The text component does not need to know that a parent was resized.

It simply receives a different proposal on the next measurement.

## 7. Text content change

Change:

```text
Text("short")
→
Text("much longer content ...")
```

The content state's change is measurement-affecting.

Conceptually:

```text
text property changes
        ↓
framework marks measurement/layout work
        ↓
coalesced queue
        ↓
next pass remeasures Text
        ↓
parent arrangement uses new desired size
```

The text measurement implementation does not schedule the pass itself.

## 8. Parent dependency after Text measurement

Suppose Text changes from height 20 to height 80.

A vertical container may depend on that desired height:

```text
Text desired height changed
        ↓
Column desired height changed
        ↓
Column's parent may also depend on Column size
```

The propagation is therefore:

```text
local content dependency
        ↓
active parent layout dependency
        ↓
possibly further ancestors
```

This is why invalidation semantics belong in the framework/layout relationship rather than in Text.

## 9. Important distinction: measurement result vs final allocation

The text's measured size is not necessarily its final size.

For example:

```text
Text desired = 450 × 100
parent allocates = 300 × 100
```

Whether the new width of 300 requires a remeasurement depends on the semantics of the layout algorithm and on whether text height is width-sensitive.

For the minimal Phase 2 one-dimensional layout, the framework should prefer to choose child proposals that already represent the relevant width before measurement wherever possible. This minimizes post-allocation remeasurement.

## 10. Why this favors a conservative one-dimensional layout first

A simple vertical/horizontal container can establish a predictable cross-axis proposal:

```text
Vertical container
    width = known/bounded
    height = unconstrained or bounded

Text child
    receives bounded width
    measures height
```

This lets text remain a pure content measurement problem.

More advanced Flex/Grid behaviors can create cases where final width depends on child intrinsic size and therefore require additional measurement stages. Those should not define the first architecture.

## 11. Button case

A Button with Text can be treated as a compound content node:

```text
Button
    ↓
measure Text/content
    ↓
add padding/border
    ↓
Button desired size
```

When the Button's parent changes its available width, the Button passes the new content proposal to its content measurement implementation.

This keeps the same pipeline:

```text
Parent proposal
    ↓
Button content measurement
    ↓
Button desired size
    ↓
Parent allocation
```

## 12. RichText case

A future RichText component can use the identical contract:

```text
proposal
    ↓
RichText engine
    ↓
desired size
```

The framework does not need to know whether the implementation uses:

- SDL_ttf;
- HarfBuzz;
- another shaping engine;
- embedded runs;
- images;
- links;
- custom inline objects.

This is the strongest evidence so far that the historical TextEngine service does not need to return as a public framework concept.

## 13. Measurement context should remain framework-controlled

The current `MeasureContext` already provides `availableSize` and a `measureChild` callback. fileciteturn123file0

A future design can keep the idea that the framework supplies the measurement context while tightening what the client-facing component is allowed to do with it.

In particular, measurement code should not be given APIs for:

- invalidating ancestors;
- mutating the NodeTree;
- triggering immediate layout;
- changing lifecycle state.

## 14. Current limitation worth resolving

The current `LayoutManager` uses a floating-point maximum as an infinity sentinel and normalizes constraints around it. fileciteturn132file0

That is workable, but the research suggests that a future internal constraint model should distinguish semantic unboundedness from an ordinary very-large finite number.

This does not require immediate API expansion. It is simply an internal semantic concern worth preserving before more complex layouts are introduced.

## 15. Current layout queue is coarse but usable as a baseline

The current `LayoutManager::processLayoutQueue()` processes queued roots and recursively runs measurement and arrangement. fileciteturn132file0

This is sufficient for a correctness-first Phase 2 architecture.

A finer-grained dirty tree can be introduced later if measurements show that root-level relayout is too expensive.

The public contract does not need to change to support that optimization.

## 16. The main architectural conclusion

The text-width dependency does not force a client-visible custom layout API.

The framework can keep:

```text
closed container layout
+
content-specific measurement
+
framework-owned invalidation
+
internal Measure/Arrange
```

The critical requirement is that the measurement proposal comes from the framework/layout algorithm and that content measurement cannot synchronously re-enter the parent layout.

## 17. What remains unresolved

The current text pipeline leaves only a few architectural questions before implementation:

1. How should proposal/constraint semantics be represented internally in C++?
2. Which component types actually need a content measurement capability in Phase 2?
3. Should a compound component such as Button measure its content directly or use a generic child mechanism?
4. How should absolute-positioned children interact with normal flow measurement?
5. Do we need a second measurement after final allocation for any Phase 2 layout case, or can the first implementation avoid such cases by construction?
6. How much of the current `Node::measure()/arrange()` API can be retained internally while removing its status as a client extension contract?

## 18. Current verdict

The most dangerous historical problem — text measurement entangled with the layout engine — appears solvable without reopening the layout system to client-defined algorithms.

The minimum viable architecture can remain:

```text
Text/content measurement
        ↓
framework-controlled proposal
        ↓
one-dimensional container layout
        ↓
framework-owned geometry
        ↓
framework-owned invalidation
```

No implementation decision is made by this document.
