# Layout Dispatch Model Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document compares internal dispatch models for a closed layout engine after separating `Node` and `PanelNode` and removing client ownership of layout algorithms.

## 1. Current runtime

The current `LayoutManager` recursively calls virtual `Node::measure()` and `Node::arrange()`. `PanelNode` makes those functions pure virtual, so concrete panels currently provide the layout algorithm. fileciteturn201file0turn211file0

`StackPanelNode` shows that its persistent configuration is only `orientation`; the rest of its source is algorithmic measurement/arrangement behavior. fileciteturn210file0turn209file0

The target architecture is different:

```text
Client-facing component
    → configuration / behavior

Framework LayoutManager
    → all layout algorithms
```

## 2. Candidate A — Concrete container type dispatch

Conceptually:

```text
LayoutManager
    if node is StackPanelNode
        run stack algorithm
    else if node is FlexPanelNode
        run flex algorithm
    ...
```

### Advantages

- configuration stays close to the concrete framework component;
- no universal `PanelNode` layout property bag;
- no public strategy interface;
- natural for a closed set of framework-provided containers;
- adding a new built-in layout does not require storing unrelated state on existing panels.

### Costs

- `LayoutManager` knows every built-in panel type;
- dispatch requires RTTI or an equivalent type identity mechanism;
- the central manager grows as the framework gains built-in layout families.

### Important distinction

This is a **single dispatch at the container level**, not repeated capability discovery for every child.

The layout pass already has a concrete container. The engine only needs to select its algorithm once for that container.

This is materially different from:

```text
for each child:
    dynamic_cast<GridNode>
    dynamic_cast<FlexNode>
    dynamic_cast<...
```

which should be avoided.

## 3. Candidate B — Explicit layout kind discriminator

Conceptually:

```text
PanelNode
    layoutKind = Stack
```

and:

```text
LayoutManager
    switch(layoutKind)
```

### Advantages

- no RTTI required for layout dispatch;
- direct branch/switch;
- explicit runtime identity of layout semantics;
- easy to trace/debug.

### Costs

The discriminator must remain consistent with the component's configuration and allowed state.

A naive implementation can create:

```text
PanelNode
  layoutKind
  stack state
  flex state
  grid state
  absolute state
  ...
```

which recreates the universal layout-state problem.

A discriminator is therefore strongest when paired with **separate concrete framework container types or separate configuration records**, rather than as a giant mutable `PanelNode` union.

## 4. Candidate C — Framework-owned layout object/configuration

Conceptually:

```text
PanelNode
    └── LayoutObject
          ├── kind
          ├── configuration
          └── algorithm entry points
```

This is an internal policy/strategy model.

### Advantages

- algorithm and configuration can be isolated from Node;
- adding layout types does not require modifying one giant `LayoutManager` switch;
- algorithm-specific caches have a natural home;
- can potentially keep `LayoutManager` as orchestration and move layout policy into distinct implementation units.

### Costs

- another runtime object/lifetime to manage;
- additional indirection;
- more abstraction than current Phase 2 requirements justify;
- easy to accidentally expose the layout object as a public customization mechanism.

### Assessment

Architecturally valid, but not yet proven necessary for the minimal closed layout scope.

## 5. Candidate D — Compile-time/type trait dispatch

One could use C++ traits/helpers to associate a concrete panel type with its layout implementation.

This can reduce explicit RTTI in the algorithm code, but the runtime Node is still polymorphic, so the framework needs some way to determine the actual panel/layout family at runtime.

Without a compile-time closed traversal representation, traits alone do not eliminate runtime type identification.

### Assessment

Useful implementation technique later, not an architectural alternative by itself.

## 6. Current NodeTree implications

`NodeTree` already treats `PanelNode` as the structural child-owning capability and may use `dynamic_cast<PanelNode*>` when traversing arbitrary Nodes. fileciteturn203file0turn211file0

This means RTTI is already part of the runtime model for a legitimate reason: discovering whether a generic Node is a structural container.

There is therefore no strong architectural reason to prohibit all RTTI.

The stronger rule is:

> Do not use RTTI as a per-child layout capability discovery mechanism.

A container-level dispatch is much less problematic.

## 7. Central manager coupling

Concrete-type dispatch does couple `LayoutManager` to every built-in panel type.

However, the Phase 2 layout set is deliberately small:

```text
one-dimensional layout
optional absolute/overlay
Text/content measurement
```

There is currently no requirement for dozens of independent layout families.

For such a closed world, central ownership can be an advantage because the layout rules remain auditable in one framework subsystem.

The central manager should still be internally split into implementation functions/files rather than becoming one enormous source file.

## 8. Why a public Strategy interface is not justified

A public `LayoutStrategy` would solve the dispatch extensibility problem, but it would also create a long-lived public contract around:

```text
child measurement
proposal semantics
placement
caching
invalidation
reentrancy
error handling
```

The current requirements do not establish that this freedom is necessary.

Therefore the framework can remain internally strategy-like while keeping dispatch closed.

## 9. Configuration ownership

The best dispatch mechanism depends partly on where container configuration lives.

A strong candidate is:

```text
StackPanelNode
    orientation
    gap
    alignment

FlexPanelNode
    direction
    gap
    distribution
```

with the layout algorithm selected from the concrete framework type.

This keeps unrelated state out of `PanelNode`.

## 10. What about user-derived PanelNode?

This dispatch research reinforces the previous client inheritance analysis.

If a user can derive directly from `PanelNode`, the framework needs semantics for the layout of that derived type.

The cleanest closed-world choices remain:

1. `PanelNode` has a single default framework layout;
2. user-defined containers must derive from a framework-provided concrete panel family;
3. `PanelNode` can choose only among a small fixed set of framework layouts.

A custom arbitrary algorithm is not part of the current public contract.

The dispatch mechanism alone does not decide which of these client semantics is best.

## 11. Performance

The cost hierarchy is important:

```text
container-level dispatch
    << recursive child measurement / traversal
```

A single RTTI check or layout-kind branch per container is unlikely to be the dominant cost compared with the recursive work already performed by `LayoutManager`.

A generic property dictionary or per-child capability probing would have a much less attractive hot-path profile.

Performance should eventually be measured, but architecture should not be distorted to avoid a single controlled branch.

## 12. Debuggability

A closed dispatch model has an advantage for diagnostics.

The framework can log/trust:

```text
PanelNode type
layout family
configuration
```

and reproduce layout behavior deterministically.

A user-defined strategy ecosystem would require stronger runtime diagnostics around arbitrary algorithms.

## 13. Recommended current direction

For the reduced Phase 2 scope, the research currently favors:

```text
Concrete framework panel types
        ↓
framework-internal dispatch
        ↓
LayoutManager algorithms
```

The dispatch mechanism itself can initially be concrete-type based if that minimizes data-model complexity. A small internal layout-kind mechanism can be introduced later if profiling or maintainability shows RTTI dispatch is undesirable.

The research does **not** currently justify a separate runtime `LayoutObject` hierarchy merely for dispatch.

## 14. Proposed internal conceptual shape

```text
Node
  └── PanelNode
        ├── StackPanelNode
        ├── FlexPanelNode
        └── future framework panels

LayoutManager
  ├── measure/arrange common mechanics
  ├── stack layout implementation
  ├── flex layout implementation (when needed)
  └── content measurement integration
```

The `PanelNode` hierarchy describes **which built-in semantics a component has**. It does not expose the implementation hooks to clients.

## 15. Important migration insight

The current implementation can be migrated incrementally.

Step 1:

```text
keep Node::measure/arrange internally
```

Step 2:

```text
move Stack algorithm into LayoutManager-side implementation
```

Step 3:

```text
make StackPanelNode expose only configuration
```

Step 4:

```text
remove pure virtual layout hooks from PanelNode when no remaining built-in path needs them
```

This is safer than rewriting the entire layout runtime at once.

## 16. Current conclusion

The closed-layout requirement does not force a sophisticated Strategy Pattern.

For the current framework, a simpler model is more natural:

> **Node/Panel hierarchy describes framework component capabilities; LayoutManager owns and executes all supported layout algorithms.**

Concrete framework containers can carry only the configuration relevant to their layout family. A controlled container-level runtime dispatch is acceptable and does not recreate the original dynamic-cast problem.

The next architectural question is therefore not "which strategy pattern?" but "which concrete framework container families are actually necessary for Phase 2?"
