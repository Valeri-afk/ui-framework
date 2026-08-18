# Intrinsic Measurement Dispatch Analysis

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document evaluates how the closed Phase 2 `LayoutManager` can obtain intrinsic/content measurement from framework components without introducing a public `MeasurableNode` base class or a client-visible layout contract.

## 1. The problem

The target architecture needs to support at least:

```text
Text → content size under proposal
Button → content-driven desired size
Image → intrinsic size
Panel → size derived from children
```

while avoiding:

```text
public measure()/arrange()
public CustomLayoutStrategy
TextEngine service for clients
a new inheritance tree of capability bases
```

## 2. Candidate A — Keep protected virtual `Node::measure()`

This is the current implementation model.

```text
LayoutManager
    ↓
Node::measure()
```

### Advantage

Very simple runtime dispatch and no additional object model.

### Problem

It remains a client extension contract because a client-derived `Node` can override the virtual and is expected to understand measurement semantics.

Making the method `protected` reduces surface exposure but does not eliminate the conceptual contract.

### Assessment

Good migration bridge, poor final architectural boundary.

## 3. Candidate B — Framework-owned internal virtual hook

The framework could keep a virtual internal hook whose documentation and access semantics clearly classify it as runtime implementation rather than public layout API.

This still leaves inheritance as the mechanism, which means advanced client subclasses can potentially override it.

### Assessment

Possible transitional step, but it does not fully achieve the desired contract break.

## 4. Candidate C — Framework-managed measurement callback/descriptor

Conceptually:

```text
Node
  └── internal measurement descriptor/callback

LayoutManager
  └── invoke descriptor
```

A framework Text constructor/configuration installs Text measurement behavior. A client subclass of Text inherits that installed semantics automatically.

A generic client Node can have no intrinsic measurement behavior and rely on explicit size or container allocation.

### Advantages

- no new inheritance capability base;
- layout algorithm remains closed;
- measurement is an internal framework capability;
- framework components can carry their own measurement semantics.

### Risks

- another internal function-pointer/object slot on Node;
- lifecycle of the descriptor/callback must be simple;
- a generic `std::function` would likely be unnecessarily heavy in the Node hot path.

### Assessment

Architecturally promising, especially if implemented as a compact static descriptor/function pointer rather than type-erased heap state.

## 5. Candidate D — Separate internal content object

Conceptually:

```text
TextNode
   └── TextContent
         measure(proposal)
         render(...)
```

LayoutManager asks the Node for its content measurement object internally.

### Advantages

- strong separation of content from Node runtime state;
- text implementation can own font/shaping/rendering resources;
- useful if components eventually have rich internal content models.

### Risks

- adds another object and lifetime relationship;
- may be unnecessary for simple leaf components;
- could recreate the old Component/Node layering if applied universally.

### Assessment

Strong for complex content types, but premature as a mandatory representation for every Node.

## 6. Candidate E — LayoutManager knows concrete framework components

For example:

```text
if TextNode → text measurement
if ImageNode → image measurement
if ButtonNode → button measurement
```

### Advantages

- no callback storage;
- no capability interface;
- very explicit closed-world behavior.

### Problems

- LayoutManager becomes coupled to every framework content type;
- adding every new intrinsic component modifies central layout code;
- risks turning LayoutManager into a content registry.

### Assessment

Acceptable only for a tiny, fixed set of components. Not attractive as the long-term framework architecture.

## 7. Candidate F — Explicit internal measurement kind + data

A Node could carry an internal framework-owned content kind and associated state:

```text
contentKind = Text
contentState = ...
```

The manager dispatches based on kind.

This is essentially a tagged-union content system.

### Advantages

- no RTTI;
- deterministic dispatch;
- data-oriented storage can be efficient.

### Problems

- risks turning Node into a universal content sum type;
- custom framework components require expanding the central union;
- poorly aligned with normal C++ inheritance and user-defined component behavior.

### Assessment

Too restrictive for the current component model.

## 8. Current source evidence

The current `Node` already has a generic `measure()` default implementation that returns zero, while `LayoutManager` recursively executes it. fileciteturn212file0turn201file0

This means a generic Node can already conceptually represent zero intrinsic content and be sized by explicit Node properties or a parent layout.

This observation is important:

> **Not every Node needs a content measurement provider.**

Only framework components whose desired size comes from intrinsic content need one.

## 9. This changes the problem significantly

We do not need:

```text
Every Node must implement measurement.
```

We can instead have:

```text
Generic Node
    → default intrinsic size = empty/zero

Framework Text
    → has text measurement behavior

Framework Image
    → has image intrinsic behavior

Framework Panel
    → has child-layout behavior
```

This is a much narrower system.

## 10. Client-derived components

Consider:

```cpp
class MyButton : public Button { };
```

The framework-provided `Button` already has its measurement behavior, so `MyButton` inherits it.

Consider:

```cpp
class Marker : public Node { };
```

If Marker has no intrinsic content, explicit size or parent allocation can determine its geometry.

The client does not need to implement `measure()` merely because Marker is custom.

This is an important property of the closed layout model.

## 11. What if a client wants custom intrinsic content?

This is where a future narrow extension may be justified:

```text
CustomContentMeasure
    proposal → Size
```

But it should remain distinct from custom container layout.

Crucially, Phase 2 does not currently require this capability if the framework only promises framework-provided Text/Button/Image/etc.

Therefore this extension can be deferred.

## 12. Candidate G — Composition-based content measurement

A framework component may own a framework/internal content object that knows its measurement behavior.

For example:

```text
Button
  └── TextContent
```

The Node itself remains generic.

This is particularly attractive for composite controls, but it should not be generalized to every Node until there is a demonstrated need.

## 13. Current strongest direction

For Phase 2, the smallest justified model appears to be:

```text
Generic Node
    → no intrinsic measurement requirement by default

Framework leaf/content component
    → framework-owned measurement implementation

Framework Panel component
    → framework-owned child layout implementation

LayoutManager
    → orchestrates all of the above
```

This can initially use an internal framework mechanism without creating a public measurement interface.

## 14. Why not add `MeasurableNode` now?

A base class such as:

```cpp
class MeasurableNode : public Node
```

would immediately create a question for every component:

```text
Node?
MeasurableNode?
PanelNode?
ControlNode?
```

That is exactly the C++ capability-inheritance proliferation we have been trying to avoid.

If only framework components need intrinsic measurement, a public capability hierarchy is unnecessary.

## 15. Why not make every Node measurable?

Because a generic empty Node can reasonably have zero intrinsic content.

The framework can size it by explicit configuration or parent policy.

Forcing every custom Node to implement measurement would reintroduce the original contract.

## 16. Rendering relationship

Text measurement and text rendering may share implementation resources, especially with SDL_ttf as the current backend. The legacy Label owns text-engine/text resources and measures/render the same content. fileciteturn215file0

This supports keeping content-specific measurement and rendering together inside the framework component rather than making LayoutManager renderer-aware.

## 17. Invalidation relationship

If framework Text stores:

```text
text
font
wrap policy
```

its setters can be framework-owned state mutation points that mark measurement/layout work.

The measurement callback/object itself never needs direct access to the invalidation system.

## 18. Internal descriptor as a future implementation tool

If the framework eventually needs several content-measurement implementations without concrete-type dispatch, a compact internal descriptor could look conceptually like:

```text
MeasurementOps
    measure(Node, proposal) -> Size
```

The important point is that this is an **internal runtime mechanism**, not a public client contract.

It could be implemented with:

- a static function pointer;
- a small internal descriptor object;
- a framework-owned pointer to immutable operations.

`std::function` and heap-allocated type erasure should not be assumed necessary.

## 19. Current conclusion

The research currently favors a surprisingly simple principle:

> **Intrinsic measurement is optional capability of framework-provided content components, not a universal capability of every Node.**

This lets the framework keep:

```text
Node
PanelNode
closed LayoutManager
```

without adding a `MeasurableNode` base and without giving custom component authors a layout contract.

The old component architecture's measurement boundary is useful historical evidence, but the old `Component::arrange()` contract should not be restored.

## 20. Remaining implementation question

The remaining technical question is now narrow enough to prototype:

> What is the smallest internal mechanism by which a framework-provided leaf component can expose `proposal → Size` to `LayoutManager` while a normal client-derived `Node` remains completely unaware of that mechanism?

This should be solved only after the exact Phase 2 built-in component set is selected.
