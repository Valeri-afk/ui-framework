# Layout Relationship Ownership Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document compares three ownership models for parent-specific layout metadata in a retained-mode C++ UI framework:

1. metadata stored on the child;
2. metadata owned by the parent/layout policy;
3. a separate relationship object between parent and child.

The goal is not to reproduce CSS or WPF, but to determine which model is natural for the existing `NodeTree` ownership and lifecycle model.

## 1. The semantic question

Properties such as:

```text
Grid row / column / span
Flex grow / shrink / order
Dock side
Canvas offset
```

are not intrinsic characteristics of a child component. Their meaning is supplied by the parent layout that interprets them.

WPF explicitly uses attached properties for this relationship: a `Grid` or `DockPanel` defines values that child elements may provide, and the parent reads those values during layout. Microsoft documentation explicitly describes attached properties as a way to avoid bloating common base classes with properties used by only a few layout containers. citeturn574267search1turn574267search2

SwiftUI exposes a related concept through `LayoutValueKey`, where a view carries layout-specific values and the active layout reads them through `LayoutSubview`. citeturn574267search0turn574267search5

These systems prove the relationship problem is real, but do not require this C++ framework to reproduce their property systems.

## 2. Model A — Child-owned metadata

Conceptually:

```text
Node
 ├── universal state
 └── layoutData
       ├── GridData
       ├── FlexData
       └── ...
```

### Strengths

- child-oriented API is easy to imagine;
- metadata is immediately available from the child;
- reusing a child across parents can preserve values if desired;
- no parent-side map lookup is required.

### Weaknesses

The metadata describes relationships that may no longer exist:

```text
Button
  GridRow = 2
  GridColumn = 3

Button moved to Flex
```

The Grid values are now stale or irrelevant.

The child also accumulates layout metadata for layouts it may never use. This recreates a smaller version of the "irrelevant properties on every component" problem.

A generic type-erased `layoutData` store additionally introduces runtime type machinery that may become a hidden property system.

### Ownership assessment

Semantically weak for parent-specific metadata. It can still be useful for a small set of child-independent properties.

## 3. Model B — Parent/layout-policy-owned metadata

Conceptually:

```text
PanelNode
 └── LayoutPolicy
       └── child metadata
            NodeId -> PolicyData
```

or an equivalent structure aligned with the parent's child order.

Qt provides a strong real-world example of this separation: layout managers own the managed layout items and expose layout-specific operations such as `QGridLayout::addItem(... row, column, ...)`. `QGridLayout` owns the layout item, while the widget itself remains a separate object. citeturn766470search3turn766470search6

### Strengths

- metadata lifetime follows the parent-child relationship;
- removal naturally removes or invalidates the relationship metadata;
- reparenting naturally establishes a new layout relationship;
- layout policy directly owns the data it interprets;
- no specialized `GridNode` / `FlexNode` child type is needed;
- invalidation target is explicit: changing Grid data invalidates Grid.

### Weaknesses

- public API may naturally be parent-oriented (`grid.setPlacement(child, ...)`);
- child lookup must be efficient during layout;
- custom policy needs an explicit metadata storage convention;
- if implemented as a map keyed by `NodeId`, lookup cost may be unnecessary when child order already provides a stable index.

### Ownership assessment

Semantically strongest for the current retained-mode model.

## 4. Model C — Separate relationship object

Conceptually:

```text
NodeTree
   Parent
      │
      ├── LayoutRelationship ── Child A
      ├── LayoutRelationship ── Child B
      └── LayoutRelationship ── Child C
```

A relationship object could contain:

```text
Parent reference / identity
Child identity
layout metadata
possibly measurement cache
possibly relationship flags
```

### Strengths

- explicitly models parent-child relationship as a first-class concept;
- can group all relationship-specific state together;
- potentially useful if the framework later needs rich relationship data beyond layout;
- can isolate parent-specific data from both Node and LayoutPolicy.

### Weaknesses

For the current problem, it may be an extra layer without additional semantic value.

If the object only contains:

```text
child + GridPlacement
```

then it is essentially a more indirect representation of parent-owned metadata.

It introduces:

- another runtime object/lifetime;
- another level of indirection;
- another ownership synchronization problem;
- possible allocation overhead if each child gets a separate object;
- more complexity around NodeTree mutation.

The current Phase 1 model already represents parent/child ownership directly through the NodeTree and NodeId. Creating a second relationship object solely to store Grid/Flex values may duplicate an existing runtime concept.

### Ownership assessment

Architecturally meaningful only if the relationship itself acquires substantial responsibilities beyond a small set of layout metadata.

For the current Phase 2 scope, that additional abstraction is not yet justified.

## 5. Reparenting test

Assume:

```text
Grid
  └── Button
      row=2, column=1
```

The Button moves to:

```text
Flex
  └── Button
```

### Child-owned

Grid metadata remains attached to Button unless explicitly cleared.

This requires extra semantics:

```text
ignore?
clear?
remember for future Grid parent?
```

### Parent-owned

The Grid relationship disappears; the Flex relationship starts with Flex defaults/metadata.

This is the cleanest semantic interpretation.

### Relationship object

The old relationship object must be destroyed/detached and a new one created or attached.

This is correct but introduces another lifecycle operation that is unnecessary if the parent itself already owns the metadata.

## 6. Removal / destruction test

When a Node is removed from the tree:

```text
NodeTree
  removes Node
```

The desired invariant is:

> no layout metadata remains authoritative for the removed relationship.

Parent-owned metadata makes this natural.

A relationship object can also satisfy it, but only if its lifetime is perfectly synchronized with the tree mutation.

Child-owned metadata requires explicit cleanup if the values should cease to exist.

## 7. Hot-path comparison

Layout traversal is a repeated operation. Property configuration is comparatively infrequent.

### Child-owned metadata

Direct field/typed-store access can be fast, but a generic layout-data dictionary risks runtime lookup/type erasure on every child.

### Parent-owned metadata

A policy can keep compact data adjacent to its child iteration structure:

```text
children[i]
layoutData[i]
```

or keyed by stable NodeId when necessary.

This can make layout access deterministic and allocation-free during traversal.

### Relationship objects

The strategy may have to dereference:

```text
child -> relationship -> metadata
```

or:

```text
policy -> relationship object -> child
```

The additional indirection is not automatically harmful, but there is no demonstrated benefit yet that compensates for it.

## 8. API ergonomics vs storage ownership

An important result is that the public API does not have to mirror internal ownership.

For example, internal storage may be:

```text
GridPolicy
    child metadata
```

while the public API could expose:

```cpp
Grid::setRow(child, 2);
Grid::setColumn(child, 1);
```

or eventually a child-oriented facade.

Therefore we should not choose internal ownership just to make the public syntax look CSS-like.

The internal model should follow semantics and runtime efficiency first.

## 9. Relation to custom layouts

Parent-owned metadata also fits the custom-layout extension idea.

A custom layout policy can own:

```text
CustomLayout
  configuration
  child-specific metadata
  optional measurement cache
```

while the framework retains NodeTree ownership and lifecycle.

A separate relationship object is only justified if custom layout requires relationship state that cannot reasonably live in the policy.

## 10. Comparison with established frameworks

### WPF

WPF's attached-property design keeps layout-specific values out of common element base classes while allowing the parent layout to consume them. citeturn574267search1turn574267search7

This supports the semantic conclusion that specialized layout data should not be forced into every Node.

### SwiftUI

SwiftUI separates custom layout behavior into a `Layout` type and gives the layout access to child-specific values through `LayoutValueKey` and `LayoutSubview`. citeturn574267search0turn574267search5turn574267search4

This supports separating:

```text
runtime child identity
```

from:

```text
layout-specific child metadata
```

### Qt

Qt's layout hierarchy directly owns layout items and exposes layout-specific APIs such as `QGridLayout::addItem(... row, column, ...)`; layout objects manage geometry without turning each widget into a different layout-specific widget class. citeturn766470search3turn766470search6turn766470search11

This provides strong evidence that a parent/container-owned model is natural in a retained-mode framework.

## 11. Strongest current conclusion

The three models are not equally motivated.

### Child-owned metadata

Best when the metadata is genuinely intrinsic to the Node regardless of parent.

Poor fit for `GridRow`, `FlexGrow`, `DockSide` and similar relationship values.

### Parent-owned metadata

Best fit for parent-specific layout state in the current NodeTree architecture.

It aligns ownership, lifetime, invalidation target and algorithm responsibility.

### Separate relationship object

Useful only if the parent-child relationship becomes a first-class subsystem with substantial state or behavior beyond layout metadata.

For the current Phase 2 requirements, it appears to add complexity without solving a problem that parent-owned metadata cannot solve.

## 12. Provisional architectural principle

Without committing to a concrete API, the research currently supports:

```text
Node
 └── universal, parent-independent state

Parent / LayoutPolicy
 └── relationship-specific child metadata

NodeTree
 └── owns runtime Node lifetime and parent/child structure
```

This avoids:

- a Grid/Flex subclass hierarchy;
- a generic God-object Node property set;
- repeated `dynamic_cast` discovery;
- a second ownership system for relationships.

## 13. What should not be decided yet

The research does not yet establish:

- whether metadata should be stored by NodeId, child index, or a compact child entry;
- whether the public API should be parent-oriented or expose an ergonomic facade;
- whether all policy configuration should live in a separate policy object;
- how custom layout metadata should be represented;
- whether universal alignment is really parent-independent;
- whether a future reparent operation should preserve any relationship values.

These should be decided only after the layout policy architecture itself is specified.
