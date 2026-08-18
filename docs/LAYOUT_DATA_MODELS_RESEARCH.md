# Layout Data Models Research

> **Status:** comparative research / no implementation decision
> **Date:** 2026-08-18
>
> This document examines concrete ways a C++ retained-mode UI framework can represent universal layout properties and parent-specific child layout metadata without forcing every layout type into the Node inheritance hierarchy or using repeated `dynamic_cast` during layout.

## 1. Problem statement

The framework wants all of the following simultaneously:

- one stable runtime `Node` abstraction;
- no requirement that every component carry every layout/container property;
- no framework-wide `dynamic_cast` dispatch just to discover layout capabilities;
- framework-owned invalidation and scheduling;
- CSS-like property-driven configuration for normal users;
- support for parent-specific layout metadata such as Grid row/column or Flex grow/shrink;
- optional advanced extension for layouts/content not yet covered by the framework.

No single C++ language feature solves all of these requirements. The choice is primarily a data-model and ownership decision.

---

## 2. Model A — Put every layout property directly on Node

```text
Node
 ├── width / height
 ├── min / max
 ├── padding / margin
 ├── alignment
 ├── Grid row / column / span
 ├── Flex grow / shrink / basis
 ├── absolute position
 └── future layout properties
```

### Advantages

- no type discovery during layout;
- trivial property access from the engine;
- easy to make properties framework-owned;
- excellent for a property-driven client API.

### Problems

- Node becomes progressively larger;
- unrelated components carry irrelevant properties;
- new layout models require modifying core Node API;
- future layout extensions become coupled to the Node type;
- property namespaces become difficult to reason about.

### Assessment

This solves the runtime dispatch problem but recreates the God-object concern that motivated earlier redesigns.

It may still be appropriate for a **small set of universal properties**, but not for every container-specific feature.

---

## 3. Model B — Specialized Node subclasses

```text
Node
 ├── GridNode
 ├── FlexNode
 ├── StackNode
 └── ...
```

### Advantages

- strong compile-time type separation;
- specialized properties live near specialized behavior;
- no type-erased storage required.

### Problems

- a widget's UI role becomes coupled to its parent's layout role;
- the same child type may need different layout metadata under different parents;
- framework dispatch can require concrete type discovery;
- a Button would become awkward if Grid-specific parent metadata were represented through its own subclass.

### Assessment

This is the model most likely to recreate the historical WPF-like class-hierarchy problem in the framework's current architecture.

A key reason is that **layout role belongs to the parent-child relationship**, not necessarily to the intrinsic type of the child.

---

## 4. Model C — Attached properties / parent-defined child metadata

WPF uses attached properties so a child can carry values defined by its parent panel, such as `DockPanel.Dock` or `Canvas.Left`, without the child having a corresponding property in its class. The property provider owns the meaning, while the child stores the effective value through the framework property system. citeturn481843search0turn481843search6turn481843search13

Conceptually:

```text
Grid
  defines:
      Row
      Column
      RowSpan
      ColumnSpan

child
  stores values assigned to those properties
```

### Advantages

- child does not become `GridNode`;
- layout metadata belongs conceptually to the parent layout;
- no concrete child type is required;
- aligns strongly with CSS/XAML property-driven thinking.

### Problems

- a full dependency-property/attached-property system is substantial;
- value lookup, metadata, inheritance, serialization and change notification can become a framework unto themselves;
- if implemented as a generic string/object dictionary in C++, type safety can degrade.

### Assessment

This is a **very strong conceptual fit** for the framework's problem, but reproducing all of WPF's dependency-property machinery would be excessive.

The important idea to preserve is simply:

> parent-defined layout values can live on a stable Node without requiring a specialized Node subclass.

---

## 5. Model D — Layout-value keys / typed metadata

SwiftUI has a direct analogue. A custom layout can define a `LayoutValueKey`, a view can attach a value for that key, and a `LayoutSubview` proxy can retrieve the value while the custom layout runs. citeturn481843search1turn481843search2turn481843search5

Conceptually:

```text
Node
  + universal properties
  + keyed layout values

Grid layout
  reads:
      GridRowKey
      GridColumnKey
      GridRowSpanKey

Flex layout
  reads:
      FlexGrowKey
      FlexShrinkKey
```

### Advantages

- keeps Node runtime identity stable;
- layout-specific metadata does not require subclassing;
- a layout strategy reads only the keys it understands;
- works naturally with custom layouts;
- strongly supports property-driven usage.

### Problems

- C++ must choose how keys are represented and type-checked;
- a generic key/value registry can become dynamic and harder to debug;
- invalidation metadata must also be tied to property changes.

### Assessment

This is currently one of the **most interesting conceptual models** for ui-framework because it solves the exact `GridNode` / `FlexNode` problem without forcing a new Node subclass for every layout model.

It may be possible to implement a much smaller C++ version than SwiftUI's full system.

---

## 6. Model E — Layout object / policy owns metadata

Qt's separate `QLayout` model shows another route: widget identity remains distinct from the layout manager, and the layout object manages the relationship among child layout items. Qt's custom layout API centers on layout objects and layout items rather than making every widget subclass a new layout-specific widget. citeturn481843search7turn481843search0

Conceptually:

```text
PanelNode
    └── LayoutPolicy
          ├── Stack
          ├── Grid
          └── Flex
```

Child-specific metadata could live either:

```text
on Node as typed layout values
```

or:

```text
inside the active LayoutPolicy keyed by NodeId
```

### Advantages

- separates container algorithm from Node type;
- avoids `GridNode` / `FlexNode` subclasses;
- layout policy can own algorithm-specific state.

### Problems

If child metadata lives only inside the layout policy, client-facing code may need to address it through the parent:

```cpp
panel.grid().setRow(child, 2);
```

rather than:

```cpp
child.layout().gridRow = 2;
```

The latter is more CSS-like; the former may be more explicit about the parent relationship.

### Assessment

A strong candidate for container behavior, but not sufficient alone to solve the data-model question.

---

## 7. Model F — Generic type-erased property storage

C++ can represent layout metadata using a registry keyed by a type or property ID:

```text
PropertyKey<T>
    ↓
value of T
```

or conceptually:

```text
Node
  property store
      Key<GridRow> -> int
      Key<GridColumn> -> int
      Key<FlexGrow> -> float
```

### Advantages

- stable Node class;
- arbitrary future layout properties without changing Node header for every property;
- typed access can be layered over erased storage;
- supports custom extension more naturally.

### Problems

- type erasure introduces runtime machinery;
- storage/performance/debugging complexity;
- property identity and ownership must be designed carefully;
- can become a "dictionary inside Node" that recreates a different form of God object.

### Assessment

Potentially powerful, but should be treated cautiously. A generic property system is justified only if the framework actually needs open-ended extensibility.

---

## 8. Model G — External per-container metadata keyed by NodeId

A layout policy can maintain:

```text
NodeId -> GridPlacement
```

or:

```text
NodeId -> FlexItemData
```

### Advantages

- Node remains minimal;
- no type erasure on Node;
- metadata lifetime can follow the container;
- layout policy has direct ownership of its own data.

### Problems

- child layout properties are less directly visible on the child;
- APIs tend toward:

```cpp
container.setGridPlacement(childId, ...)
```

- metadata must be cleaned when children are removed;
- reparenting/child moves must update policy state;
- NodeId liveness must be handled carefully.

### Assessment

Technically attractive inside the current Phase 1 architecture because NodeId is already a stable identity mechanism. However, it sacrifices some of the declarative/CSS-like feel.

It may still be an excellent internal implementation even if the public API looks property-based.

---

## 9. Model H — Composition of dedicated layout state objects

Instead of one huge Node property set:

```text
Node
 ├── RuntimeState
 ├── LayoutStyle
 ├── ContentMeasurementState
 └── EventState
```

The universal layout state could be a compact object:

```text
LayoutStyle
 ├── width
 ├── height
 ├── min/max
 ├── margin
 ├── padding
 ├── alignment
 └── position
```

Container-specific metadata could then be a separate optional structure.

### Advantages

- keeps the public Node API conceptually organized;
- avoids the appearance of one giant flat property bag;
- preserves one runtime Node identity;
- lets framework subsystems remain internally modular.

### Problems

- still does not by itself solve parent-specific metadata;
- can simply move the God object from Node into `LayoutStyle` if too many properties are added.

### Assessment

This is useful independently of the final metadata mechanism. It is probably better to organize universal properties as a coherent layout state object even if a future implementation chooses attached/keyed values for specialized metadata.

---

## 10. Cross-model comparison

| Model | Stable Node | No dynamic_cast | CSS-like client API | Specialized metadata | Extensibility | Complexity |
|---|---|---|---|---|---|---|
| Everything on Node | ✓ | ✓ | ✓ | ✓ | low | low initially / high later |
| Specialized subclasses | ✓ | sometimes | medium | ✓ | medium | hierarchy complexity |
| Attached properties | ✓ | ✓ | **strong** | **strong** | **strong** | framework property-system complexity |
| Typed layout keys | ✓ | ✓ | **strong** | **strong** | **strong** | moderate runtime/type machinery |
| Layout policy + external metadata | ✓ | ✓ | medium | **strong** | strong | moderate |
| NodeId → metadata maps | ✓ | ✓ | medium | **strong** | strong | moderate |
| Generic type-erased store | ✓ | ✓ | **strong** | **strong** | **very strong** | high |
| Composed LayoutStyle + metadata | ✓ | ✓ | **strong** | strong | strong | moderate |

---

## 11. Most important conceptual result

The research strongly suggests that **layout role is often a relationship property rather than an intrinsic property of the child type**.

For example:

```text
Button inside Grid
    -> has Grid row/column metadata

Button inside Flex
    -> may have Flex grow/shrink metadata

Button inside Stack
    -> may only need alignment
```

The Button itself did not fundamentally change type in those cases.

This is the key reason parent-defined/attached/keyed layout metadata is more compelling than `GridNode` / `FlexNode` child subclasses.

WPF's attached properties and SwiftUI's `LayoutValueKey` are strong evidence for this design pattern. citeturn481843search0turn481843search2

---

## 12. Important distinction: storage vs public API

The framework does not have to expose the same model it uses internally.

For example, the public API might look CSS-like:

```cpp
child.layout().width(...);
child.layout().margin(...);
child.layout().align(...);
```

while the internal layout system could store:

```text
LayoutStyle
+
parent-specific metadata
+
NodeId-indexed policy state
```

Likewise, a public API could make Grid placement look child-oriented while the internal Grid strategy stores placement by NodeId.

This separation is important because it prevents C++ implementation constraints from dictating the user-facing mental model.

---

## 13. Dynamic dispatch question

The problem to avoid is not simply the C++ keyword `dynamic_cast`.

The real problem is:

```text
layout engine sees arbitrary Node
    ↓
tries to discover which specialized layout capability it carries
    ↓
dispatches based on concrete runtime type
```

A healthier architecture is explicit:

```text
current layout strategy
    ↓
asks for the metadata it owns/defines
    ↓
operates on generic child Node/proxy
```

That keeps the **strategy** in control of what it needs rather than making the engine interrogate every child.

---

## 14. Current strongest candidates

At this stage, the most promising families for ui-framework appear to be:

### Candidate 1 — universal LayoutStyle + keyed/attached container metadata

```text
Node
 ├── LayoutStyle
 └── layout values

Grid strategy
 └── reads Grid keys

Flex strategy
 └── reads Flex keys
```

This has the strongest CSS-like conceptual fit.

### Candidate 2 — universal LayoutStyle + container-owned NodeId metadata

```text
Node
 └── LayoutStyle

Grid strategy
 └── NodeId -> GridPlacement

Flex strategy
 └── NodeId -> FlexData
```

This has the strongest fit with the current Phase 1 NodeId architecture.

### Candidate 3 — LayoutPolicy owns all specialized metadata

```text
Panel
 └── LayoutPolicy
      ├── algorithm
      └── child metadata
```

This makes the parent relationship explicit and keeps Node very small, but the public API may feel less CSS-like.

No candidate is accepted yet.

---

## 15. Questions for the next research stage

1. Can a small typed-key/attached-property mechanism give the framework enough extensibility without becoming a full WPF-style dependency property system?
2. Can NodeId-indexed metadata be exposed through a convenient property-like API so the client does not care how the data is stored?
3. Should Grid/Flex properties belong to the active parent only, or should a child be able to retain values for multiple possible parent layout types?
4. How should changing a parent-specific layout property automatically notify the framework which container needs relayout?
5. How should metadata behave across reparenting, removal, and Node destruction?
6. Can the same mechanism serve both built-in layouts and future custom layout extensions?
7. Can universal layout properties remain small enough that Node does not become a God object?
8. Can content measurement use a similarly explicit capability mechanism without forcing every Node to implement a text/image-specific API?

No implementation decision is made by this document.
