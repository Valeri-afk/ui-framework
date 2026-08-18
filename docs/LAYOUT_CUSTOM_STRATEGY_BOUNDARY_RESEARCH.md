# Custom Layout Strategy Boundary Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document investigates whether a custom layout strategy can be exposed as a narrow, safe extension point while the framework retains ownership of measurement orchestration, invalidation, runtime, lifecycle and NodeTree ownership.

## 1. The desired property of the boundary

The extension should let an advanced user answer only this question:

> Given the container constraints and the participating children, what size should the container have and where should each child be placed?

It should not require the author to understand:

- NodeTree ownership;
- lifecycle;
- mutation queues;
- invalidation scheduling;
- destruction;
- renderer ownership;
- event dispatch;
- global layout execution.

This is the critical distinction from the historical open Measure/Arrange model.

## 2. What the strategy must be able to read

A layout strategy needs a controlled view of:

```text
container constraints / proposal
child collection
child measurement result
child layout metadata
container configuration
```

It should not need raw runtime ownership APIs.

A useful conceptual abstraction is:

```text
LayoutChild
    measure(proposal)
    desiredSize
    metadata
```

The framework decides what the proxy is allowed to expose.

## 3. What the strategy must be able to write

The strategy needs two outputs:

```text
container desired size
child final geometry
```

Geometry should be committed through a framework-controlled operation rather than by directly mutating `Node::actualPosition_` or `Node::actualSize_`.

Conceptually:

```text
strategy
    ↓
place(child, rect)
    ↓
framework validates/applies geometry
```

This keeps constraints, bookkeeping and future diagnostics in framework ownership.

## 4. What the strategy must not be able to do

The strategy should not be given APIs that can:

```text
insert child
remove child
destroy Node
change parent
start layout immediately
invalidate arbitrary ancestors
modify NodeTree directly
mount/unmount lifecycle
schedule renderer/input work
```

A custom layout author should not be able to accidentally break runtime invariants simply by writing the geometry algorithm.

This directly addresses the historical concern that a client component could violate framework contracts and cause apparently unrelated failures.

## 5. Measurement ownership

The framework should invoke content measurement.

A strategy may request child measurement through a controlled query, but the strategy should not own the content-measurement implementation.

Conceptually:

```text
LayoutStrategy
      |
      | measure(child, proposal)
      v
Framework Measurement Layer
      |
      v
Content / Text / Image / RichText
      |
      v
Size
```

This keeps content measurement and container allocation separate.

## 6. Invalidation ownership

A custom strategy should not call `markLayout()`.

There are two useful categories of change:

### Strategy configuration change

```text
Grid columns changed
Flex gap changed
Radial radius changed
```

The owning framework object already knows which layout strategy is affected and can schedule layout.

### Child/content change

```text
Text changed
Font changed
Image loaded
```

The framework-owned property/content state triggers invalidation. The active layout strategy determines whether parent measurement depends on the child's new result.

The custom strategy itself should not schedule the pass.

## 7. Can invalidation be inferred from strategy operations?

A tempting idea is to observe which child measurement results a strategy reads and automatically record dependencies.

Conceptually:

```text
strategy.measure()
    → queried child A
    → queried child B
```

then the framework records:

```text
container depends on A/B measurement
```

This could reduce declarative dependency metadata.

However, it has risks:

- dependencies may change between passes;
- conditional queries make invalidation behavior harder to reason about;
- hidden observation can complicate caching and debugging;
- framework behavior becomes more dynamic.

Therefore inference is interesting but should not be assumed necessary for the first architecture.

## 8. Explicit strategy dependency metadata

The opposite is for the strategy/container to state coarse dependency semantics such as:

```text
uses child desired size for container Measure
uses child final size for placement
```

This is easier to reason about but expands the extension contract.

For a small framework, a coarse built-in strategy-level dependency model may be more practical than a general dynamic dependency tracker.

## 9. Strategy configuration changes

A strategy may contain state that affects layout:

```text
Stack:
    orientation
    gap

Grid:
    rows
    columns
    gaps

Flex:
    direction
    gap
    distribution
```

These values should be mutated through framework-controlled setters/configuration objects so that the owning container can schedule layout automatically.

The strategy should not need to discover an invalidation manager or queue itself.

## 10. Strategy lifetime

A layout strategy must not own runtime Nodes.

The preferred conceptual ownership is:

```text
PanelNode
    owns LayoutStrategy

NodeTree
    owns runtime Nodes
```

Strategy destruction therefore happens as part of the owning container/framework object lifecycle rather than independently of the Node tree.

This avoids creating a second runtime ownership graph.

## 11. Reentrancy safety

A strategy should be treated as running inside a framework-controlled layout phase.

During this phase, the framework should prevent or defer operations that would make the layout pass recursively re-enter itself.

For example:

```text
measure strategy
    ↓
strategy changes layout property
    ↓
property mutation is deferred
    ↓
current pass completes
    ↓
next scheduled pass sees the change
```

This is materially safer than exposing an immediate `invalidateLayout()` call.

## 12. Error containment

A custom strategy can still contain logical errors:

- overlapping children unexpectedly;
- invalid positions;
- negative sizes;
- failing to place all children;
- returning nonsensical desired sizes.

The framework should therefore normalize or validate geometry at the boundary.

Possible protections include:

```text
finite coordinate validation
non-negative size validation
min/max clamping
ignored placement for removed/invisible child
safe fallback for invalid container size
```

The exact safeguards remain implementation work.

## 13. Measurement count and caching

A custom strategy should not be able to generate unbounded measurement work accidentally.

The framework could provide a per-pass measurement cache:

```text
(child, proposal, content-state-version)
      → Size
```

This is an internal optimization and should not be part of the client contract.

Alternatively, a simpler first version can enforce a bounded measurement protocol without exposing caching to the strategy.

## 14. Custom content measurement should not require a custom container

A RichText implementation may need special measurement but can still be placed in built-in containers:

```text
RichText
    measurement = custom
    container = Grid / Stack / Flex
```

Therefore the framework should not make the content author implement a custom layout strategy simply because measurement is specialized.

This is one of the strongest reasons to keep the two extension boundaries separate.

## 15. Custom container does not imply custom content measurement

A custom container such as:

```text
ChessBoard
RadialMenu
Timeline
Graph
```

can use ordinary Text/Image/Button children.

The custom container strategy only needs to ask the framework for child sizes.

This prevents a custom-container author from inheriting text/image/layout implementation responsibilities that do not belong to the container.

## 16. Public API vs internal contract

The framework can expose a narrow advanced API while keeping built-in layout completely opaque.

Normal users may see:

```text
Stack
Grid
Flex
Overlay
```

without ever seeing `LayoutStrategy`.

Advanced users may opt into:

```text
CustomLayoutStrategy
```

only when necessary.

This supports the intended "closed by default" model while preserving an escape hatch while framework coverage is incomplete.

## 17. Comparison with historical current API

The current Node exposes both:

```cpp
virtual LayoutSize measure(MeasureContext &ctx);
virtual void arrange(ArrangeContext &ctx);
```

as protected virtual methods. fileciteturn130file0

The current LayoutManager directly calls `node.measure(ctx)` and `node.arrange(ctx)` during recursive layout. fileciteturn125file0

This means today's runtime still couples Node identity to layout algorithm dispatch.

The research does not yet prove that this code must be removed, but it identifies the architectural seam clearly:

```text
Node runtime identity
        currently also owns
Node layout algorithm hooks
```

A future architecture may separate these responsibilities while retaining Measure/Arrange internally.

## 18. The smallest plausible advanced contract

The research currently suggests the following conceptual boundary:

```text
CustomLayoutStrategy

Input:
    container proposal
    child proxies
    child measurement queries/results
    relationship metadata
    strategy configuration

Output:
    desired container size
    child placement rectangles

Forbidden responsibilities:
    runtime ownership
    lifecycle
    invalidation scheduling
    NodeTree mutation
    renderer/input state
```

The exact C++ signatures remain deliberately unspecified.

## 19. Does a custom strategy actually need Arrange?

Possibly not as a public concept.

A strategy could conceptually be viewed as one geometry computation:

```text
input tree + proposal
    ↓
output desired size + placements
```

The framework can internally split this into Measure and Arrange for correctness and caching.

This is an important possibility because it would reduce the extension contract even further.

The user does not need to think in two passes if the framework can present a simpler geometry policy abstraction while internally using two phases.

However, some layouts inherently need the final allocated container size before placement, so the framework still needs an internal separation even if the custom strategy interface hides it.

## 20. Current conclusion

The research supports a strong architectural principle:

> **A custom layout extension should be a geometry policy, not a runtime integration contract.**

The framework should own:

```text
when the policy runs
what state it can observe
how children are measured
how geometry is applied
how invalidation is scheduled
how lifecycle and ownership are protected
```

The policy should own only:

```text
how space is allocated among children
```

This appears capable of preserving custom extensibility without reproducing the historical problem in which client component authors had to understand `markLayout`, NodeTree and lifecycle internals.

No implementation decision is made by this document.
