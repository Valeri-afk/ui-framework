# Phase 2 Intrinsic Measurement Boundary

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document evaluates the minimum content-measurement boundary needed by the proposed closed layout engine, using the current source and the legacy `components` implementation only as historical evidence.

## 1. Current core and historical component models

The current core `Node` exposes protected virtual `measure()` and `arrange()` hooks, while `LayoutManager` drives recursive measurement/arrangement. fileciteturn212file0turn201file0

The legacy `Component` abstraction used essentially the same split, but its comments already expressed an important intended boundary: `measure()` returned content-box intrinsic size and the framework was responsible for padding, border, requested size, min/max and parent constraints. fileciteturn217file0

The legacy `Label` then implemented text measurement under constraints, including width-dependent wrapping, and returned a generic `LayoutSize`. fileciteturn215file0

This is useful evidence for the architectural boundary, but the legacy `components` hierarchy remains obsolete and should not be restored as-is.

## 2. Important distinction

There are two different operations:

```text
Content measurement
    proposal/constraints → desired content size

Container layout
    parent proposal + children → container size + child geometry
```

Text belongs to the first category.

Stack/Flex/Grid belong to the second.

A Button can combine both through framework-defined behavior without becoming a general layout strategy.

## 3. The legacy Component comments are architecturally valuable

The old `Component` explicitly described this boundary:

```text
Component.measure
    → content-box intrinsic size

Framework
    → padding
    → border
    → requested/preferred size
    → min/max
    → parent constraints
```

This is very close to the boundary independently reached through the current research.

The main thing to avoid is carrying over the old `Component::arrange()` contract to clients. The arrangement algorithm should remain framework-owned.

## 4. Text should be content measurement, not a layout container

The legacy Label demonstrates:

```text
available width
    ↓
wrapped text measurement
    ↓
width + height
```

It also stores text/font/wrap state and renderer text resources, while rendering remains text-specific. fileciteturn215file0

This is the correct semantic split for Text:

```text
Text state
    → framework-managed invalidation

Text measurement
    → content-specific implementation

Layout
    → framework-owned parent/container algorithm
```

## 5. What the measurement boundary should receive

The content measurement implementation needs only a framework-controlled proposal/constraint context.

At the conceptual level:

```text
proposal:
    width  = finite / unbounded / unspecified
    height = finite / unbounded / unspecified

result:
    desired content size
```

The current `MeasureContext` instead exposes `availableSize` plus a child-measure callback. fileciteturn213file0

That context is currently an internal layout mechanism and should not be treated as a normal client-facing API.

## 6. What the content measurement boundary should not receive

It should not need:

```text
parent Node*
NodeTree
LayoutManager
layout queue
invalidation manager
children ownership API
renderer traversal
```

The content measurement implementation should not be able to synchronously ask the parent to relayout.

This prevents measurement from becoming a hidden second layout engine.

## 7. Text width dependency

For wrapped text:

```text
parent proposes width = W
        ↓
text measures under W
        ↓
text returns desired height H
```

The framework can then use that result in the active container layout.

If W changes, the framework schedules another measurement. Text does not need to know why W changed.

This is the simplest concrete proof that a closed layout engine can coexist with content-specific measurement.

## 8. Button is a compound example

The legacy Button demonstrates a useful composition pattern: Button owns behavior/style and contains a Label-like content implementation, while its old `measure()` delegates to the content and its old `arrange()` centers that content. fileciteturn218file0

The new architecture should not copy the old implementation, but the conceptual split is useful:

```text
Button
    state / interaction / visual behavior
          ↓
    content measurement
          ↓
    framework computes Button geometry
```

Button therefore does not need to expose a client-owned layout algorithm.

## 9. Compound content does not require a separate public component tree contract

A Button may internally need text/icon composition, but this does not imply that every custom Button-derived client component must manage child layout manually.

The framework can provide its own internal composition mechanisms or specialized component behavior while still exposing only framework-defined layout semantics.

The exact composition mechanism remains open.

## 10. Measuring content vs applying Node geometry

The current layout manager already performs box conversion and writes desired/actual geometry around the Node measurement hooks. fileciteturn201file0

This is the right place to keep:

```text
requested size
padding/border
min/max
parent constraints
final geometry
```

The content measurement implementation should return only the content result.

## 11. Important implication for client inheritance

A client-derived `Node` must not be forced to implement a measurement algorithm just because it is a custom component.

Otherwise the closed-layout goal fails:

```text
class MyButton : public Button
    → should not need measure()

class MyTextualControl : public Node
    → should not need measure() unless framework explicitly exposes a content-measurement extension
```

Therefore custom intrinsic measurement is itself a separate API question from component inheritance.

## 12. Two possible future content extension models

### Model A — framework components only

Only framework-provided Text/Image/etc. have intrinsic measurement implementations.

This gives the strictest closed system.

### Model B — narrow custom content-measurement extension

An advanced user may supply a content measurement implementation with only:

```text
proposal → Size
```

while the framework still owns layout, invalidation and geometry.

This is much narrower than custom layout.

The current research does not yet establish whether Model B is necessary in Phase 2.

## 13. Why Model B is materially safer than custom layout

A custom measurement provider cannot decide:

```text
where siblings go
what parent size is
when layout runs
which ancestor becomes dirty
how children are owned
```

It only answers:

```text
How large would this content like to be under this proposal?
```

This is a much smaller contract.

## 14. TextEngine ownership

The legacy Label shows that text measurement and renderer resources can be implemented together at the content component level. fileciteturn215file0

However, the new architecture should avoid exposing `TTF_TextEngine` as a client-facing layout service.

The text backend can be an internal implementation detail of the framework's Text component/content measurement layer.

This directly addresses the historical discomfort of having developers manually operate a separate TextEngine service.

## 15. Invalidation boundary

Text state such as:

```text
text
font
wrap policy
```

is measurement-affecting state.

The framework should observe/change this state through its normal property/content mutation path and schedule layout automatically.

The measurement implementation should never require:

```text
markLayout()
invalidateMeasure()
```

## 16. Measurement cache

A per-pass cache may eventually be useful:

```text
(content identity, proposal, relevant content state version)
    → Size
```

But caching should be framework-owned and invisible to client code.

Phase 2 can begin without an exposed cache abstraction if the initial layout workload is small.

## 17. Current strongest boundary

The current research supports the following conceptual model:

```text
Node / framework component
      │
      └── optional content measurement capability
                 │
                 │ proposal
                 ▼
          content implementation
                 │
                 ▼
              Size
                 │
                 ▼
         LayoutManager / parent
                 │
                 ▼
          final geometry
```

The critical point is that the capability is not the same as a custom layout algorithm.

## 18. Does this require a new base class?

Not necessarily.

The research does not currently justify introducing a public:

```cpp
ContentMeasurableNode
```

base class solely to formalize the idea.

That could recreate the same C++ inheritance explosion we have been trying to avoid.

The framework can keep the capability internal until a concrete set of content types demonstrates that a separate abstraction is necessary.

## 19. Current verdict

The historical `Component::measure()` and `Label::measure()` provide useful evidence that the framework can cleanly separate:

```text
content-specific measurement
```

from:

```text
framework-owned layout orchestration
```

The current research does not justify reviving the old `components` architecture or exposing `MeasureContext` to clients.

The strongest Phase 2 direction remains:

```text
Node
PanelNode
Framework-provided content components
Framework-provided containers
Framework-owned LayoutManager
Framework-owned invalidation
```

with content measurement as a narrow internal capability.

## 20. Remaining design question

The final unresolved question before implementation is:

> **How should framework-provided components such as Text and Button expose their measurement behavior to the closed LayoutManager without requiring every custom Node subclass to implement `measure()`?**

That is now a much narrower question than the original "how should the entire layout engine work?" problem.

No implementation decision is made by this document.
