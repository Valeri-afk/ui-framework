# Layout Container Architecture Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document compares how a retained-mode C++ framework could represent a layout container and its layout algorithm without returning to either a specialized Node inheritance hierarchy or a God-object Node.

## 1. The central question

A layout container needs at least three things:

```text
1. Runtime identity in the Node tree
2. Child ownership / traversal
3. A policy that determines child geometry
```

The question is whether these should be represented by one C++ class, several inherited classes, or a Node plus a separate layout object.

Established systems demonstrate multiple successful splits. Qt gives widgets stable runtime identity and attaches a separate `QLayout` geometry manager; all `QWidget` subclasses can use layouts, and the layout takes charge of positioning/resizing children. citeturn172070search3turn172070search1 SwiftUI separates the view hierarchy from the `Layout` protocol and gives a custom layout subview proxies rather than direct mutable child views. citeturn172070search4turn172070search0 WPF keeps `Panel` as a container element but makes child measuring/arrangement pass through framework layout methods. citeturn172070search8

These models suggest that **container identity and layout algorithm do not have to be the same abstraction**.

## 2. Model A — Specialized Node subclasses

```text
Node
 ├── StackNode
 ├── GridNode
 ├── FlexNode
 └── OverlayNode
```

### Advantages

- straightforward C++ virtual dispatch;
- container configuration and algorithm live together;
- easy for a small framework to start with;
- no strategy pointer/indirection.

### Problems

The layout mechanism becomes part of the public inheritance hierarchy.

A component's class now says both:

```text
what the component is
```

and:

```text
how its children are arranged
```

This works for a dedicated Stack/Grid widget, but creates awkward questions for arbitrary user components that sometimes need children but do not want a specialized layout base.

It also makes changing layout type a structural type change rather than a configuration change.

### Assessment

Good for a small widget toolkit, but poorly aligned with the framework's historical goal of keeping component identity separate from layout responsibility.

## 3. Model B — Node + attached layout strategy

```text
PanelNode
 ├── children
 └── LayoutStrategy
      ├── Stack
      ├── Grid
      ├── Flex
      └── Custom
```

Qt demonstrates a related split: `QWidget::setLayout()` applies a separate layout manager to the widget, and all widgets can use layouts rather than becoming layout-specific subclasses. citeturn172070search3turn172070search1 SwiftUI likewise defines a `Layout` type that determines geometry for a collection of subviews rather than requiring each child to become a layout-specific type. citeturn172070search4turn172070search0

### Advantages

- separates runtime Node identity from layout algorithm;
- layout can potentially be changed/configured without changing Node dynamic type;
- no `GridNode` / `FlexNode` hierarchy required;
- natural location for parent-owned child metadata;
- custom layout becomes a policy object rather than a new Node runtime type.

### Problems

- adds one level of indirection;
- ownership/lifetime of the strategy must be clear;
- a `PanelNode` must own/coordinate the strategy and children.

### Assessment

Currently the strongest conceptual candidate.

The extra indirection is small and buys a substantial decoupling between runtime identity and layout behavior.

## 4. Model C — Layout strategy is the container itself

```text
LayoutContainer
   ├── runtime Node state
   ├── children
   └── algorithm
```

This resembles the Qt idea where a `QLayout` is itself the geometry manager, although Qt keeps widgets and layouts as different runtime objects. citeturn172070search1turn172070search3

### Advantages

- explicit container ownership;
- fewer conceptual layers than Node + Strategy;
- layout state and algorithm can be tightly coupled.

### Problems

If the object must also participate as a Node in rendering, input, lifecycle and tree ownership, it again becomes a multi-responsibility object.

The risk is returning to the earlier "Widget with everything" problem under a new name.

### Assessment

Useful only if the framework deliberately treats layout containers as a distinct runtime entity. That would need to be reconciled with the current NodeTree design and likely reintroduce the same framework-processing issues that motivated a single primary Node abstraction.

## 5. Model D — Policy as a stateless algorithm + container state elsewhere

```text
PanelNode
 ├── children
 ├── GridState
 └── GridAlgorithm
```

The algorithm object is effectively a function/strategy, while configuration is stored in the container.

### Advantages

- state ownership is explicit;
- algorithm can be reused;
- minimal runtime object for policy.

### Problems

- if algorithms require caches, the separation becomes more complicated;
- custom layout authors may need both a state object and an algorithm object;
- strategy lifetime can be harder to express cleanly.

### Assessment

Potentially useful internally for built-in layouts, but probably not necessary as the first abstraction exposed to clients.

## 6. Model E — Type/tag + central switch

```text
PanelNode
   layoutType = Grid

LayoutEngine:
   switch(layoutType)
      Stack
      Grid
      Flex
```

### Advantages

- trivial data ownership;
- no virtual strategy objects;
- easy to optimize for a small fixed set of built-in layouts.

### Problems

- LayoutEngine becomes coupled to every built-in layout type;
- custom layout support becomes awkward;
- adding new layout types modifies central engine dispatch;
- configuration and algorithm selection become globally coupled.

### Assessment

Reasonable for a very small closed toolkit, but weak for a framework intended to grow and potentially support advanced extension.

## 7. Children ownership

Regardless of strategy model, an important distinction remains:

```text
NodeTree / PanelNode
    owns child Nodes

LayoutStrategy
    reads children through a controlled interface
```

This mirrors SwiftUI's proxy approach: a custom layout receives `LayoutSubview` proxies instead of owning the underlying views. citeturn172070search0turn172070search2

For this framework, the analogous rule should be:

> the layout strategy must never own or destroy runtime Nodes merely because it lays them out.

This preserves the Phase 1 ownership model.

## 8. Could the strategy also own child metadata?

Yes.

A strong conceptual composition is:

```text
PanelNode
 ├── child ownership
 └── LayoutStrategy
       ├── algorithm
       └── relationship-specific child metadata
```

This combines the earlier ownership research with the container research.

The strategy therefore owns exactly the semantics it interprets:

```text
GridStrategy
 ├── Grid definitions
 └── child placements

FlexStrategy
 ├── Flex configuration
 └── child Flex data
```

The Node remains unaware of those details.

## 9. Changing layout type

One benefit of Node + Strategy is that layout type can conceptually change without changing the Node's runtime class:

```text
PanelNode
   layout = Grid

PanelNode
   layout = Stack
```

However, this should not automatically imply that changing layout type at runtime is a Phase 2 requirement.

It does show that the relationship:

```text
Node identity
    !=
layout strategy identity
```

is architecturally clean.

## 10. Rendering and event handling

A common concern is whether separating layout from Node makes rendering or input harder.

It should not if layout strategy is treated as a non-rendering subsystem:

```text
NodeTree
  ├── lifecycle
  ├── input/event
  ├── rendering
  └── layout strategy
```

The Node remains the single runtime entity through which rendering/input/lifecycle operate.

The layout strategy contributes only geometry.

This is an important difference from a hierarchy where Grid/Flex become alternative widget base classes.

## 11. Custom layout

Under the Node + Strategy model, custom layout becomes naturally:

```text
PanelNode
   +
CustomLayoutStrategy
```

The strategy can receive framework-controlled child proxies and return geometry decisions.

SwiftUI has essentially this public shape: a layout type defines `sizeThatFits` and `placeSubviews` for a collection of subview proxies. citeturn172070search4turn172070search0

The ui-framework version should remain more conservative because it is a C++ retained-mode runtime and has a smaller feature scope.

## 12. Container and layout policy need not be public separately

An important implementation principle is:

```text
internal separation
    ≠
public API separation
```

The user-facing API might simply expose:

```cpp
panel.setLayout(Grid{...});
```

while internally `PanelNode` owns a strategy object and strategy-owned metadata.

Alternatively a built-in concrete `GridPanel` API could be exposed if that is clearer. The internal model should not be selected based on surface syntax alone.

## 13. Performance considerations

A strategy indirection occurs per container, not per property or per child type discovery.

Typical layout traversal is:

```text
container
  → get strategy
  → iterate children
  → measure/place
```

This is fundamentally different from:

```text
for each child
   dynamic_cast through capability types
```

The cost of one strategy dispatch per container is unlikely to be architecturally significant compared with recursive child measurement and rendering, but actual performance should be measured later rather than assumed.

## 14. Invalidation ownership

With Node + Strategy:

```text
universal Node property changed
    → Node/framework invalidation

Grid placement changed
    → GridStrategy invalidates owning PanelNode layout

Grid column definition changed
    → GridStrategy invalidates owning PanelNode layout

Custom strategy configuration changed
    → strategy/container invalidates its owning PanelNode
```

No strategy needs to manipulate the global layout queue directly.

## 15. Relationship to current Phase 1

This model fits the existing retained-mode structure particularly well because:

- Node remains the stable runtime identity;
- NodeTree remains the lifetime/ownership authority;
- layout strategy does not become a second tree;
- NodeId can remain the identity used by internal policy metadata if needed;
- deferred mutation can remain framework-controlled.

This avoids creating a second ownership system for layouts.

## 16. Current strongest candidate

The research now favors a conceptual shape close to:

```text
Node
   │
   └── Panel capability / container state
          │
          ├── owns child Node relationships through NodeTree
          │
          └── owns one LayoutStrategy
                  │
                  ├── configuration
                  ├── relationship metadata
                  └── geometry algorithm
```

The exact representation of "Panel capability" remains open. It might be a concrete `PanelNode`, a component of Node state, or another composition mechanism.

The strategy itself should not own runtime Node lifetime.

## 17. What the research does NOT support

It does not currently support:

- a separate runtime `GridNode` hierarchy for every layout model;
- repeated `dynamic_cast` discovery of layout capabilities;
- a central `switch(layoutType)` as the only extensibility mechanism;
- a separate relationship object solely for trivial Grid/Flex metadata;
- exposing the strategy's internal cache/metadata storage to normal clients.

## 18. Provisional conclusion

The strongest architecture currently under investigation is:

```text
stable Node runtime identity
+
container/panel capability
+
framework-owned LayoutStrategy
+
strategy-owned relationship metadata
+
framework-owned layout orchestration/invalidation
```

This preserves retained-mode ownership while allowing layout behavior to be decoupled from the concrete widget type.

It also gives the framework a natural place for built-in Grid/Flex/Stack algorithms and a controlled advanced custom-layout extension point.

No implementation decision is made by this document.
