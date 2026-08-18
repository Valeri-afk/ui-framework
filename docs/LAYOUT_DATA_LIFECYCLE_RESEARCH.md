# Layout Data Lifecycle Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-18

This document examines where layout metadata should live during the lifetime of a retained-mode node tree.

## 1. Key observation

A property such as:

```text
Grid row = 2
Grid column = 1
Flex grow = 1
Dock = Left
```

is meaningful only relative to a particular parent layout model.

The child can move from one parent to another and keep the same runtime identity while its layout role changes.

Therefore there is a strong semantic argument that **parent-specific layout metadata belongs to the parent/layout relationship**, not intrinsically to the child type.

WPF's attached properties are explicitly designed for this relationship: a parent layout defines a property that child elements can set, and the parent consumes it during layout. citeturn427518search0turn427518search4

SwiftUI's layout-value mechanism provides a related model in which a subview carries values that the active layout reads through a key. citeturn376916search0turn376916search5

## 2. Candidate ownership models

### A. Child-owned metadata

```text
Node
 └── GridData / FlexData / ...
```

The child stores all possible roles.

**Problem:** metadata survives independently of the relationship that gives it meaning. A Node may carry Grid data while it is inside Flex, Stack or no parent at all.

### B. Parent-owned metadata

```text
Panel
 └── NodeId -> LayoutItemData
```

The active parent owns the child's placement/configuration metadata.

**Advantages:**

- metadata lifetime follows the relationship;
- removal naturally discards metadata;
- reparenting creates a new relationship and can create new metadata;
- no specialized child subclass is needed;
- layout algorithm directly owns the data it interprets.

**Cost:**

- public API may naturally be parent-oriented;
- lookup must be efficient;
- metadata must be synchronized with child insertion/removal.

### C. Generic attached/keyed values stored on Node

```text
Node
 └── keyed layout values
```

The parent defines the meaning of the key and reads it during layout.

**Advantages:**

- child can express metadata declaratively;
- no specialized child type;
- values can survive moves if desired.

**Cost:**

- values can outlive the relationship that gives them meaning;
- generic type-safe storage requires framework machinery;
- stale values may accumulate unless cleanup semantics are explicit.

### D. LayoutPolicy-owned metadata

```text
Panel
 └── LayoutPolicy
      └── NodeId -> policy-specific data
```

The strategy owns both arrangement algorithm and child metadata.

This makes responsibility extremely explicit:

```text
GridPolicy owns Grid semantics
FlexPolicy owns Flex semantics
```

The trade-off is that the public API may naturally be strategy/parent-oriented instead of child/property-oriented.

## 3. Reparenting as the deciding test

Phase 1 explicitly defers reparenting, but the eventual design should still be tested against it.

Consider:

```text
Grid
  └── Button
      row = 2
      column = 3

Button moved to Flex
```

If metadata is parent-owned:

```text
Grid metadata disappears with the old relationship
Flex metadata begins with the new relationship
```

This is semantically clean.

If metadata is child-owned:

```text
Button still has Grid row/column values
```

Those values are either ignored, cleared, or accidentally interpreted later.

This is one reason parent-owned metadata is attractive in a retained-mode tree.

## 4. Removal / destruction

Phase 1 makes NodeTree the owner of live nodes and uses NodeId for liveness.

Any metadata model should therefore guarantee:

```text
Node removed
   ↓
no layout metadata remains authoritative
```

A parent-owned model can satisfy this naturally because the parent drops the child record/metadata when removal occurs.

A Node-owned map requires an explicit cleanup step tied to Node destruction.

A policy-owned NodeId map also requires removal hooks, but the policy already owns the relationship and can erase metadata while processing removal.

## 5. Default values

Parent-owned metadata has a useful property: the absence of metadata can simply mean the layout's default behavior.

For example:

```text
Grid child without placement
    → default row/column

Flex child without grow
    → grow = 0
```

No child-wide storage is required for defaults.

This is attractive for a framework where most children use default behavior.

## 6. Client API vs internal ownership

The framework does not need to expose internal ownership directly.

For example, a public API can be:

```cpp
panel.setGridPlacement(child, placement);
```

while internally the policy stores:

```text
NodeId -> GridPlacement
```

Alternatively, a child-oriented facade could expose:

```cpp
child.layout().setGridPlacement(...)
```

which forwards the value to the current parent/layout relationship.

The second form is more declarative, but the first is semantically more explicit about the fact that Grid placement belongs to the parent.

This is an API design question, not an ownership requirement.

## 7. No dynamic_cast requirement

A parent-owned model does not require the layout engine to ask:

```text
"Is this child a GridNode?"
```

The current container already knows which strategy is active.

The strategy asks for its own metadata:

```text
GridPolicy
    -> GridPlacement for child

FlexPolicy
    -> FlexItemData for child
```

This is explicit dispatch rather than runtime capability discovery.

## 8. Relation to current Phase 1 architecture

The current framework already has a strong retained-mode primitive for this purpose: `NodeId`.

The tree knows:

- which nodes are live;
- which parent owns a node;
- when a node is removed;
- when a node becomes invalid.

Therefore a parent/policy-owned metadata model fits naturally with the existing runtime identity model, without introducing a new global registry.

## 9. Potential internal shape

One possible internal concept to investigate later is:

```text
PanelNode
   ├── children (owned Node objects)
   └── layout policy
          └── child metadata
```

The exact storage might be:

```text
vector / small array keyed by child order
```

if metadata is compact and the policy can maintain it alongside children;

or:

```text
NodeId -> metadata
```

if stable identity is more convenient.

No decision should be made until the expected child-count/performance characteristics are understood.

## 10. Hot-path consideration

Layout traversal is a hot path compared with property configuration. A generic property bag lookup on every child during every layout pass may be unnecessarily expensive.

A policy-owned typed structure can provide direct access:

```text
GridPolicy
   -> vector/GridItemData aligned with children
```

while the public API can still be ergonomic.

This is an important reason not to choose a generic `unordered_map<string, any>` merely because it resembles CSS properties.

## 11. Current research conclusion

For parent-specific data, the strongest semantic default appears to be:

> **the active parent/layout strategy owns the metadata required to interpret its children.**

This does not require the public API to be parent-oriented, and it does not prevent a future attached/keyed property facade.

It also fits retained-mode lifecycle more naturally because metadata lifetime follows the parent-child relationship.

Universal geometry properties such as size constraints, padding, margin and position may still belong to the Node's common layout state because they make sense independently of the current parent.

Therefore a promising conceptual split is:

```text
Node
 └── universal layout state

Panel / LayoutPolicy
 └── relationship-specific child metadata
```

This is a research result, not a final API decision.

## 12. Open questions

1. Should the public API expose parent-owned metadata through the parent, child or both?
2. Can policy metadata be stored alongside children without invasive changes to current ownership containers?
3. Would NodeId-keyed lookup be fast enough, or should metadata align directly with child order?
4. How should a future reparent operation transfer or discard relationship metadata?
5. Can a custom layout policy own its own metadata without exposing any storage machinery to the client?
6. Which universal layout properties are truly parent-independent?
