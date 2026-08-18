# PanelNode and Layout Responsibility Analysis

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document decomposes the current `StackPanelNode` implementation into state, semantic configuration, and algorithmic behavior to determine what should remain in framework-provided node types when layout orchestration becomes fully framework-owned.

## 1. Current StackPanelNode

The current type is:

```cpp
class StackPanelNode : public PanelNode
```

It currently owns exactly one meaningful semantic value:

```cpp
Orientation orientation_;
```

with setters/getters, plus the inherited children and the inherited layout hooks. fileciteturn206file0turn189file0

Its remaining implementation is algorithmic:

- determining whether orientation is vertical;
- constructing child measurement proposals;
- accumulating desired size;
- walking visible children;
- assigning final child sizes;
- advancing the placement cursor.

Those operations do not represent persistent component state. They are layout algorithm behavior. fileciteturn206file0

## 2. Split the current implementation into three categories

### A. Persistent container state

```text
orientation
```

This belongs naturally to a framework-provided container type or a framework-owned layout configuration object.

### B. Runtime structural state

Inherited from `PanelNode`:

```text
children
parent
ownership
visible-child traversal
attachment/detachment
```

This belongs to `PanelNode` and `NodeTree`, not the layout algorithm. fileciteturn187file0turn203file0

### C. Algorithm

The following are not node state:

```text
measure child with main-axis unbounded proposal
sum sizes
max cross-axis size
set cross-axis child size to content size
advance main-axis position
```

They belong to framework layout execution.

## 3. This gives a very clean prospective shape

Conceptually:

```text
PanelNode
    ├── children
    └── framework layout configuration

LayoutManager
    ├── stack algorithm
    ├── future flex algorithm
    ├── future grid algorithm
    └── geometry normalization
```

The important point is that `StackPanelNode` no longer needs to own the actual algorithm merely because it owns an orientation value.

## 4. `StackPanelNode` can be reduced substantially

A future framework-provided stack/one-dimensional container could conceptually contain only:

```text
orientation
```

plus whatever public container configuration is eventually justified:

```text
gap
main alignment
cross alignment
```

The actual computation would live in the framework layout subsystem.

This is a stronger fit for the closed-layout goal than the current model where every concrete panel is a layout implementation.

## 5. Why this is not a generic Strategy Pattern requirement

The framework does not need a public:

```cpp
ILayoutStrategy
```

for this architecture.

The `LayoutManager` can have framework-internal dispatch based on framework-defined container kinds or concrete container types.

Because the set of layout types is intentionally closed in the current design, a centralized framework-owned dispatch is acceptable and simpler than introducing a public virtual strategy interface.

This is fundamentally different from a client extension API.

## 6. Internal dispatch options

Two internal mechanisms remain plausible.

### A. Concrete type dispatch

```text
LayoutManager
    if StackPanelNode → stack algorithm
    if GridPanelNode  → grid algorithm
    if FlexPanelNode  → flex algorithm
```

This can be implemented through carefully bounded RTTI or another framework-owned capability mechanism.

### B. Explicit framework layout kind

```text
Node / PanelNode
    layoutKind = Stack / Flex / Grid / ...
```

The `LayoutManager` dispatches on the kind.

This avoids repeated RTTI but adds an explicit runtime discriminator and creates an invariant that the kind and node state must remain consistent.

No decision is made yet.

## 7. Why a `layoutKind` field is not automatically better

A discriminator can make the layout engine easy to dispatch, but it can also couple a single PanelNode object to every possible layout mode:

```text
PanelNode
    layoutKind
    orientation
    gap
    grid state
    flex state
    ...
```

That quickly starts recreating the God-object problem in a different form.

Therefore a discriminator is attractive only if layout configuration remains small and orthogonal.

## 8. Why concrete framework container types remain attractive

Separate framework types such as:

```text
StackPanelNode
FlexPanelNode
GridPanelNode
```

can keep only the state relevant to their layout semantics.

For example:

```text
StackPanelNode
    orientation
    gap
    alignment

GridPanelNode
    columns
    rows
    gaps

FlexPanelNode
    direction
    gap
    distribution
```

The framework can then inspect the concrete type once at the container level and execute the corresponding internal algorithm.

This is very different from requiring every child to be dynamically classified.

## 9. Potential inheritance boundary

A promising hierarchy remains:

```text
Node
  ├── Button
  ├── Text
  ├── Image
  └── PanelNode
        ├── StackPanelNode
        ├── FlexPanelNode
        └── ... framework containers
```

The user can inherit from `Node` or `PanelNode` for component composition, but does not override layout hooks.

The framework-provided concrete container already has a known layout semantics.

## 10. What about user-derived PanelNode?

This is the most important client API question.

If a user writes:

```cpp
class InventoryPanel : public PanelNode
{
    // custom component behavior
};
```

there are two possible semantics.

### Semantics A — inherits the base panel's default layout

The framework supplies a default one-dimensional layout to `PanelNode`.

This is highly convenient but requires a clear default.

### Semantics B — user-derived PanelNode does not get a new layout

It must opt into one of the framework-provided container types or use composition with a framework panel internally.

This protects the closed layout model but makes custom container components less direct.

### Semantics C — PanelNode can configure one of the framework layouts

For example:

```text
PanelNode
    layout = Vertical / Horizontal
```

but the user cannot introduce a new algorithm.

This may provide the best trade-off if the configuration surface remains small.

No decision is made yet.

## 11. Existing `StackPanelNode` suggests C may be natural

The only state currently specific to `StackPanelNode` is orientation. fileciteturn206file0

This suggests that a closed framework container could expose a small layout configuration rather than requiring a separate algorithm implementation per node.

However, before changing the hierarchy we should test the idea against future Grid/Flex state because those may not fit a single generic `PanelNode` state object without reintroducing the God-object issue.

## 12. Node / PanelNode responsibilities

The research now supports a strong responsibility split:

### Node

- runtime identity;
- parent/owner references;
- common state;
- events;
- lifecycle;
- common geometry properties;
- leaf behavior.

### PanelNode

- child ownership;
- child structural operations;
- child traversal;
- container participation in NodeTree.

### Concrete framework container

- persistent layout configuration;
- no direct ownership of layout execution.

### LayoutManager

- measurement orchestration;
- layout algorithm execution;
- proposal generation;
- geometry placement;
- invalidation execution.

## 13. Current `Node::measure()` / `arrange()` implication

Because the current StackPanel algorithm is almost entirely algorithmic and because `LayoutManager` already owns the recursive orchestration, the strongest migration direction is now:

```text
Current:
LayoutManager → Node::measure/arrange → StackPanelNode algorithm

Potential target:
LayoutManager → framework layout semantics → Stack algorithm
```

The intermediate step can preserve the current hooks while moving algorithm ownership inward. No immediate source change is implied by this document.

## 14. Why this supports a closed layout engine

If the only persistent layout-specific state exposed by framework containers is configuration such as:

```text
orientation
gap
alignment
```

then normal client code can configure layout without implementing it.

The resulting relationship is:

```text
Client
  └── chooses/configures built-in layout

Framework
  └── executes built-in layout
```

This is exactly the desired direction: declarative use without client-owned layout algorithms.

## 15. Current architectural risk

The main risk is not `PanelNode` itself. It is making `PanelNode` too generic.

If we put all future layout configuration into it:

```text
orientation
gap
alignment
grid tracks
flex basis
grid row/column
absolute offsets
...
```

then we simply recreate a layout God object.

Therefore the cleanest boundary remains:

```text
Node
PanelNode
concrete framework layout containers
```

rather than one universal configurable PanelNode.

## 16. Current verdict

The current `StackPanelNode` source strongly supports the following conclusion:

> The orientation is component state; the measure/arrange code is framework layout algorithm.

This is evidence that a closed layout architecture can preserve the useful `Node` → `PanelNode` inheritance boundary while moving algorithmic responsibility out of concrete client-visible nodes.

The next design question is not whether `PanelNode` should exist. It should.

The next question is how framework-provided container types should expose their **configuration** without exposing their **algorithm implementation**.

No implementation decision is made by this document.
