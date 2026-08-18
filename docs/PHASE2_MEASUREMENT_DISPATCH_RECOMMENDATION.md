# Phase 2 Measurement Dispatch Recommendation

> **Status:** research / provisional recommendation
> **Date:** 2026-08-19

This document narrows the intrinsic measurement dispatch problem after reviewing the current Node API and the closed-layout architecture.

## 1. Current source shape

The current `Node` has protected virtual `measure()` and `arrange()` methods with a zero-size default, while `LayoutManager` is a friend and performs recursive layout. fileciteturn224file0

The important observation is that the default zero measurement already gives us a useful semantic:

```text
ordinary Node
    → no intrinsic content requirement
    → zero intrinsic content size unless sized by Node properties/parent policy
```

Therefore not every Node needs a measurement capability.

## 2. Provisional architecture

The strongest current direction is:

```text
Node
    common runtime/geometry state

PanelNode
    child structure

Framework content components
    framework-owned measurement semantics

Framework container components
    framework-owned layout semantics

LayoutManager
    orchestration + algorithms
```

This does not require a public `MeasurableNode` base.

## 3. Why the current zero default is valuable

A generic custom Node such as:

```cpp
class Marker : public Node { };
```

can be treated as a geometry element whose size comes from:

```text
explicit size
min/max
parent allocation policy
```

It does not need to invent intrinsic measurement logic.

This directly reduces the client contract.

## 4. Framework content components can retain specialized measurement

A framework Text component can have an internal measurement implementation.

Conceptually:

```text
Text
  state: text/font/wrap
  internal measurement
       proposal → Size
```

A framework Button can have similarly specialized desired-size behavior.

A user-derived Button inherits that behavior.

## 5. Avoiding `std::function` in every Node

A per-Node generic callback such as:

```cpp
std::function<LayoutSize(const Proposal&)> measure;
```

would be flexible but is not currently justified.

It can add:

- type erasure;
- allocation opportunities;
- larger Node state;
- less obvious lifetime semantics.

For the current closed framework, concrete framework component behavior is enough.

## 6. Three realistic internal implementations

### A. Concrete component dispatch

The framework layout subsystem recognizes known framework content types and invokes their measurement implementation.

This is the simplest strict closed-world solution but couples central layout code to content types.

### B. Internal measurement operation descriptor

A framework-created component can carry a compact internal operation descriptor:

```text
MeasurementOps
    measure(node, proposal)
```

This is not a public interface.

It can be a function pointer/static descriptor rather than a `std::function`.

This would allow layout orchestration to remain generic while still avoiding a new public inheritance hierarchy.

### C. Internal content object

Components such as Text own an internal content object that provides measurement.

This is attractive for complex content backends but should not be required for every Node.

## 7. Current preference

The research currently prefers **B or a component-local equivalent of B**, provided it can be implemented without turning Node into a type-erased object container.

The exact storage location remains open.

The important architectural rule is:

> The measurement mechanism is an internal framework capability, not a user-facing inheritance contract.

## 8. Why concrete-type dispatch may still be sufficient

If Phase 2 only has:

```text
Text
Button
Image
Panel
```

then a small closed-world dispatch is reasonable.

The framework should not introduce an abstraction solely to avoid a few explicit branches.

The criterion should be maintainability, not pattern purity.

## 9. Proposed first implementation boundary

For Phase 2, the framework can initially define internal measurement semantics for the components it actually ships.

For example:

```text
Text
    measureText(proposal)

Button
    measureButton(proposal)

Panel
    measureChildren(proposal)
```

These are framework-internal operations. They are not virtual hooks that client subclasses must implement.

## 10. Derived component behavior

A client class:

```cpp
class PrimaryButton : public Button
{
};
```

inherits the framework Button measurement semantics automatically.

A client class:

```cpp
class Marker : public Node
{
};
```

has no intrinsic measurement and can rely on explicit sizing/parent allocation.

This creates a clear distinction between:

```text
component customization
```

and:

```text
measurement implementation
```

## 11. Custom intrinsic content remains deferred

A client that eventually needs a custom RichText or chart may justify a narrow extension:

```text
proposal → Size
```

But this is not required to build the Phase 2 core.

The framework should not expose the extension merely because it could be useful someday.

## 12. Important consequence for `Node::measure()`

The current virtual method is useful during migration, but the final architecture should not depend on every client Node implementing it.

A safe migration is:

```text
Phase 2 initial implementation:
    keep existing internal hooks temporarily

Once framework-owned dispatch is proven:
    move algorithms out of concrete Node overrides

Final public architecture:
    no client layout implementation contract
```

Whether the virtual functions remain as private/internal compatibility hooks after migration is an implementation detail to be decided later.

## 13. Current recommendation

The framework should **not** add a new public base class solely for intrinsic measurement.

It should first use framework-owned measurement semantics for the concrete components it actually provides.

Only a real requirement for third-party custom intrinsic content should trigger design of a narrow custom measurement extension.

## 14. Remaining question before implementation

The last unresolved design detail is the exact internal storage of framework-owned measurement behavior:

```text
concrete-type branch
vs
compact internal operation descriptor
vs
internal content object
```

This choice should be made after the final Phase 2 component inventory is written down.

No source implementation is implied by this document.
