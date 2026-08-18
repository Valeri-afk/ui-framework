# Client Component Inheritance Analysis

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document tests the proposed `Node` / `PanelNode` split against the way client-defined components should be created when layout algorithms are fully framework-owned.

## 1. Current runtime hierarchy

The current source already has:

```text
Node
  └── PanelNode
        └── StackPanelNode
```

`Node` owns runtime identity, common state, events and lifecycle; `PanelNode` owns child structure and traversal; `StackPanelNode` adds orientation and currently implements Measure/Arrange. fileciteturn193file0turn192file0turn210file0

This hierarchy is structurally useful even if the layout hooks are later moved out of the client-visible component contract.

## 2. Desired client contract

Under a closed layout system, an ordinary user-defined component should be able to inherit from one of the framework's base components without implementing layout internals.

Conceptually:

```cpp
class MyButton : public Button
{
    // component-specific behavior only
};
```

or:

```cpp
class MyPanel : public SomeFrameworkPanel
{
    // component-specific behavior only
};
```

The client should not need to implement:

```text
measure
arrange
markLayout
layout queue interaction
```

## 3. Leaf component inheritance

`Node` is a suitable base for leaf-like components when they do not structurally own children.

Examples:

```text
Button
Image
simple Text leaf
background surface
```

A framework `Button` can contain internal implementation state without making its user-facing base class responsible for a container layout algorithm.

The fact that Button may visually consist of text, icon and padding does not imply that Button's public inheritance base must itself be a general layout container. Its desired size can be determined by framework-owned content measurement and component state.

## 4. Container component inheritance

A user-defined component that structurally owns children should inherit from `PanelNode` or from a more specialized framework-provided panel type.

The key distinction is:

```text
PanelNode inheritance gives child ownership.
It should not grant ownership of a new layout algorithm.
```

This is the architectural change that removes the historical client/layout contract.

## 5. Three possible semantics for a user-derived PanelNode

### A. Plain PanelNode has no layout semantics

```cpp
class MyPanel : public PanelNode
{
};
```

The Node can contain children, but the framework has no default layout for it.

**Advantage:** very explicit separation.

**Problem:** a client cannot create a custom container component by simply inheriting from PanelNode unless it composes an existing layout container.

### B. PanelNode has a framework-owned default layout

```cpp
class MyPanel : public PanelNode
{
};
```

The framework automatically applies a default one-dimensional or overlay layout.

**Advantage:** maximal client simplicity.

**Problem:** the default semantics become part of the base class contract and must be suitable for arbitrary panels.

### C. PanelNode is a structural base; framework provides concrete layout bases

```text
PanelNode
  ├── StackPanelNode
  ├── FlexPanelNode
  └── future framework panels
```

A client chooses the nearest framework-provided layout container and derives from it.

**Advantage:** layout semantics remain explicit and closed.

**Problem:** user-defined container inheritance becomes coupled to one built-in layout family.

## 6. Current strongest direction

For the first closed layout system, the most robust approach appears to be:

```text
PanelNode = structural container base
Framework Panel types = layout-bearing components
```

This means a user can derive a custom component from:

```text
Node
```

or from a suitable framework panel type, while never implementing the layout algorithm.

A completely generic user-defined container that needs a unique layout algorithm is intentionally not supported by the first public API.

## 7. Why this is not as restrictive as it initially sounds

A user can still create custom components through composition.

For example, a complex custom widget can inherit from `Node` and internally own framework-provided layout children through a framework-defined component composition mechanism, or it can inherit from a framework-provided panel when it genuinely behaves as that layout family.

This allows the user to add behavior without forcing a new layout model.

## 8. Example: custom button

A framework Button could be conceptually:

```text
Button : Node
    text/content state
    visual state
    input behavior
```

A user can then derive:

```text
PrimaryButton : Button
```

without thinking about measurement or arrangement.

Framework-owned layout computes the Button's size from its content + padding + constraints.

## 9. Example: custom vertical panel

A user wants:

```text
InventoryPanel
    ├── item 1
    ├── item 2
    └── item 3
```

If its layout is simply vertical one-dimensional flow, it can derive from a framework-provided vertical/one-dimensional panel rather than implementing a new measure/arrange algorithm.

```text
InventoryPanel : VerticalPanel
```

The custom class adds behavior/state, while the framework continues to own layout.

## 10. Example: custom modal

A modal is structurally a panel, but its positioning semantics may be different from a normal flow container.

This suggests that a framework-provided `Modal` should own its special geometry behavior internally while exposing a component base for customization.

A user should be able to derive:

```text
MyModal : Modal
```

without learning the modal's internal layout algorithm.

## 11. Component type vs layout type

A useful distinction is:

```text
Component type
    = what behavior/style/interaction this component provides

Layout type
    = what framework geometry behavior this container uses
```

The first is naturally extensible through inheritance.

The second is intentionally closed during early framework development.

This prevents the C++ inheritance hierarchy from becoming a mechanism for injecting arbitrary layout algorithms into the runtime.

## 12. Why `ControlNode` still lacks justification

The current `ControlNode` implementation only adds `StyleProps` and a `drawSelf` hook that does not match the current `Node` drawing API. fileciteturn185file0turn191file0

The current `Node` already owns enabled/focusable/capturable state and event registration. fileciteturn193file0

Therefore the research still does not identify a stable responsibility that requires a universal `ControlNode` base.

The roadmap also explicitly says `ControlNode` must not be introduced merely for symmetry. fileciteturn190file0

## 13. Relationship to closed layout

This hierarchy gives the desired contract:

```text
Framework
    defines Node
    defines PanelNode
    defines Button/Modal/Text/etc.
    defines all supported layout semantics

Client
    derives components
    sets properties
    composes framework components
    does not implement layout
```

The client contract becomes primarily component behavior/state rather than framework layout participation.

## 14. Important limitation

This model deliberately does not solve arbitrary custom containers.

That is intentional.

If later a real requirement appears such as:

```text
Masonry
Graph layout
Radial layout
specialized timeline
```

the framework must decide at that future point whether:

1. a new built-in framework panel should be added;
2. an advanced custom layout extension is justified;
3. composition can express the requirement.

There is no reason to make that decision before a real use case exists.

## 15. Current conclusion

The most coherent client-facing inheritance model currently appears to be:

```text
Node
  → leaf / non-container component base

PanelNode
  → structural container base

Framework-provided concrete panels/components
  → contain the supported layout semantics

LayoutManager
  → executes all layout algorithms
```

The framework remains closed with respect to arbitrary layout algorithms, while still providing normal C++ inheritance and component customization.

No implementation decision is made by this document.
