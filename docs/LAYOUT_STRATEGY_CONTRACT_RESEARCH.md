# Layout Strategy Contract Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-18

This document analyzes the smallest useful contract for a built-in or advanced custom container layout strategy.

## 1. The question

If the framework owns layout orchestration and invalidation, what should a layout strategy actually be allowed to do?

A naive contract would simply expose:

```text
measure()
arrange()
```

But that leaves unanswered questions:

- how often may a child be measured?
- may a child be measured with multiple proposals?
- how does the strategy declare that it depends on child size?
- who owns the resulting geometry?
- can a strategy trigger layout or mutate the tree?
- how are loops and invalidation handled?
- can the framework optimize layout around the strategy?

## 2. Mature systems expose different contracts

### WPF

A custom Panel receives `MeasureOverride` and `ArrangeOverride`. The panel measures children during the measure pass and arranges them during the arrange pass. The normal framework lifecycle remains around those methods. citeturn260230search5

### Flutter

A `RenderBox` receives constraints and implements its layout algorithm. A parent explicitly indicates whether it will use a child's resulting size (`parentUsesSize`). Flutter therefore carries dependency information as part of the child layout call. Tight constraints can also enable optimizations. citeturn260230search0turn260230search9

### Jetpack Compose

Custom layout code measures children and then places them. Compose prohibits measuring the same child more than once in a normal layout pass and uses scopes so measurement and placement happen only in their legal phases. citeturn260230search2

### SwiftUI

A custom `Layout` has a sizing phase (`sizeThatFits`) and placement phase (`placeSubviews`). The layout receives subview proxies, which can ask for child sizes under proposals, retrieve layout values, and place children. SwiftUI may call `sizeThatFits` more than once with different proposals to discover the container's flexibility. citeturn260230search1turn260230search6

These systems demonstrate that there is no single universal custom-layout protocol. The important questions are the same, but the answers differ.

## 3. Minimum conceptual inputs

A container strategy needs at least:

### Container proposal / constraints

The space within which the container must determine its own size.

### Child access

The strategy needs a collection of children or child proxies.

### Child measurement

The strategy needs a way to ask a child for a size under an allowed proposal/constraint.

### Child metadata

The strategy may need container-specific values such as:

```text
Grid row/column/span
Flex grow/shrink/order
```

### Final placement

The strategy must be able to assign each child a final rectangle/proposal.

These are the core geometric inputs/outputs. Everything else should stay outside the strategy contract unless a concrete feature requires it.

## 4. What should NOT be in the strategy contract

A layout strategy should not need direct access to:

- NodeTree ownership operations;
- `unique_ptr` ownership;
- lifecycle mount/unmount;
- destruction;
- mutation queue;
- global layout queue;
- invalidation roots;
- renderer state;
- input state.

This is the key difference between a safe strategy extension and the previous open Measure/Arrange component model.

## 5. Measurement count is an architectural choice

There are at least three models.

### Single measurement per child

Compose deliberately enforces a one-measure-per-child rule in normal layout. citeturn260230search2

Advantages:

- predictable complexity;
- simpler invalidation;
- no hidden repeated work;
- easier reasoning about layout dependencies.

Cost:

- some algorithms are harder to express without intrinsic/precomputed information.

### Multiple proposals allowed

SwiftUI allows a custom layout to query subviews under different proposals and may call the container's size method more than once to evaluate flexibility. citeturn260230search6

Advantages:

- expressive intrinsic/flexible layouts;
- easy to ask "what if width were X?".

Cost:

- potentially more measurement work;
- caching becomes more important;
- dependency semantics are more complicated.

### Memoized arbitrary measurement

A framework may allow multiple measurements but cache `(child, proposal) -> size` within a pass.

This can support flexible algorithms while bounding repeated measurement, but introduces cache lifetime and invalidation complexity.

## 6. Preferred direction for this framework

Because the framework is small and correctness-first, the safest initial direction is likely:

```text
one logical measurement result per child/proposal per pass
+
framework-owned caching if a strategy needs repeated equivalent queries
```

This does not require the exact Compose restriction, but it establishes a predictable upper bound.

A future richer intrinsic-query system can be added if a real layout requires it.

## 7. Child measurement as a query

The custom strategy should ideally see a child as a proxy/value rather than as a raw mutable runtime object.

Conceptually:

```text
ChildProxy
    measure(proposal)
    desiredSize
    layout metadata
    place(...)
```

The proxy does not expose:

```text
NodeTree
lifecycle
ownership
mutation
```

SwiftUI's `LayoutSubview` is a strong example of this type of boundary. citeturn260230search1

The C++ implementation does not need to copy SwiftUI's proxy design, but the architectural restriction is valuable.

## 8. Placement should be framework-mediated

A strategy should conceptually produce child geometry through the framework, rather than directly writing `Node::actualPosition` / `actualSize`.

For example:

```text
strategy
   ↓
place(child, rect)
   ↓
framework applies geometry
```

This gives the framework a single place to enforce:

- min/max constraints;
- box conversion;
- coordinate validity;
- final geometry bookkeeping;
- future layout debugging/tracing.

It also prevents the strategy from silently bypassing layout invariants.

## 9. Strategy-owned state

A strategy may reasonably own algorithm configuration and caches:

```text
Grid:
    rows
    columns
    gaps

Flex:
    direction
    gap
    distribution

optional cache:
    measured child information
```

But strategy state should not own runtime Nodes.

The child tree remains owned by NodeTree/PanelNode under the accepted Phase 1 ownership model.

## 10. Dependency declaration

A strategy can depend on child outputs in different ways.

For a StackPanel, child desired size affects the parent's size.

For a fixed-size overlay, child desired size may not affect the parent's size.

Flutter makes this dependency explicit through `parentUsesSize` when a parent says it will use the child's resulting size. citeturn260230search0turn260230search9

A future ui-framework strategy API could either:

```text
A. declare the dependency explicitly;
```

or:

```text
B. let the framework infer it from the measurement operations performed during the pass.
```

Inference is attractive for client simplicity but must be proven safe and understandable.

Explicit declaration is simpler to reason about but increases the custom-strategy contract.

No decision is made yet.

## 11. Proposal / constraint model

The strategy needs a representation of the space proposed by its parent.

Flutter uses four min/max numbers (`BoxConstraints`). citeturn260230search3

SwiftUI uses width/height proposals where unspecified and infinite values have different meanings. citeturn260230search6turn260230search7

The current framework already uses `MeasureContext::availableSize` and min/max node constraints. A future architecture should avoid introducing two unrelated constraint models unless there is a concrete need.

A strong candidate is to keep one internal constraint representation and provide different convenience semantics where necessary.

## 12. Cycles and re-entrancy

A layout strategy must not be able to cause uncontrolled recursive layout through its measurement queries.

Potential pathological case:

```text
Parent measures child
    ↓
child triggers parent invalidation synchronously
    ↓
parent re-enters measure
    ↓
cycle
```

Framework-owned invalidation and deferred scheduling are important precisely because the strategy should not be able to trigger a new layout pass synchronously from within a measurement query.

This is another reason not to expose `markLayout()` or equivalent scheduling APIs to the strategy.

## 13. Custom content vs custom container

The strategy contract should not be used for custom leaf content measurement unless the framework has no better internal measurement boundary.

The two extension cases remain:

```text
Custom content measurement
    proposal -> Size

Custom container layout
    proposal + child proxies -> size + child placement
```

The custom container may use child measurement, but the child does not become a layout strategy merely because it has special intrinsic sizing.

## 14. Performance considerations

Layout passes may visit many Nodes. Therefore:

- generic string-based property lookups should not be required in the hot path;
- strategy dispatch should be direct once the active layout container is known;
- child metadata lookup should be predictable;
- repeated measurement should be bounded or cached;
- placement should avoid unnecessary allocations;
- invalidation should be coalesced.

This argues against a heavy universal runtime property dictionary as the core hot-path representation.

## 15. Current strongest conceptual contract

The smallest safe strategy contract currently suggested by the research is approximately:

```text
Container proposal / constraints
        ↓
children as framework-controlled proxies
        ↓
measure child under controlled proposal
        ↓
strategy computes own desired size
        ↓
framework provides final container geometry
        ↓
strategy places children through framework-mediated placement
```

The strategy does **not** own:

```text
invalidation
scheduling
ownership
lifecycle
mutation
```

## 16. Could the custom contract be removed entirely?

Yes, if the framework's built-in layout system eventually covers every layout needed by the target applications.

But during early framework development, an opt-in extension may still be valuable for uncovered cases such as RichText containers, radial layouts or domain-specific boards.

The important point is:

> custom layout can be an optional advanced mechanism rather than the default architecture for ordinary components.

## 17. Current conclusion

The research now suggests that the final architecture does not need to expose the current `Node::measure()` / `Node::arrange()` virtual API directly to every custom Node.

A stronger boundary is emerging:

```text
Framework
    ├── owns layout orchestration
    ├── owns invalidation
    ├── owns constraints
    └── owns geometry application

Content
    └── can supply intrinsic measurement when necessary

Container Layout Strategy
    ├── measures children through controlled proxies
    ├── computes container size
    └── places children through framework APIs
```

Whether this should be implemented as virtual strategy objects, concrete built-in managers, policies, or another C++ mechanism remains open.

No source implementation should be committed until the ownership and API boundaries above are reviewed against the existing framework types.
