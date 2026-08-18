# Layout Models Research

> **Status:** comparative research / no implementation decision
> **Date:** 2026-08-18
>
> This document surveys established layout approaches and the problem each approach was designed to solve. It deliberately does not select an architecture for this framework.

## 1. Why compare layout models

The framework's Phase 2 is not simply a choice between StackPanel, Flex and Grid. The deeper question is how a C++ framework should divide responsibility among:

- layout properties;
- intrinsic/content measurement;
- parent constraints;
- child arrangement;
- invalidation/scheduling;
- framework runtime;
- custom layout extensions.

The goal of this research is to understand why existing systems converged on particular models and which parts of those models address the problems already encountered in this framework.

---

## 2. Model A — Property-driven / declarative layout

### Representative systems

- CSS
- WPF's property-oriented layout model
- many XAML-style UI systems

### Core idea

The user mostly describes **what** geometry should look like through properties and chooses a layout container. The engine determines the actual geometry.

CSS formalizes this through properties such as width, height, min/max sizing, padding, margin and intrinsic sizing. The CSS box model separates content, padding, border and margin, while intrinsic sizes distinguish content-driven dimensions from context-driven dimensions. citeturn859574search8turn859574search3turn859574search5

WPF similarly emphasizes property-driven layout: framework properties such as Width, Height and Margin are evaluated, panel-specific logic is applied, then content is arranged. WPF explicitly describes its Measure/Arrange model as a way to support a flexible, extensible layout model driven by property values rather than imperative layout logic in normal application code. citeturn859574search1turn859574search4

### Problem this model solves

It minimizes what ordinary users need to know. A developer thinks in terms of:

```text
width
height
margin
padding
alignment
container type
```

rather than:

```text
run invalidation
measure child
store desired size
arrange child
mark dirty
```

### Architectural cost

A property-driven system needs a rich engine and a well-defined property model. The framework must understand enough layout semantics to make those properties meaningful.

### Relevance to ui-framework

This is the closest match to the desired long-term user experience described in `LAYOUT_EVOLUTION.md`.

---

## 3. Model B — Measure / Arrange with framework orchestration

### Representative systems

- WPF
- Flutter RenderBox
- Jetpack Compose
- SwiftUI Layout

### Core idea

Layout is decomposed into two conceptual responsibilities:

```text
Measure
    determine desired / feasible size

Arrange / Place
    assign final geometry
```

WPF explicitly performs a measure pass followed by arrange; the child exposes `DesiredSize`, and the parent later provides the child's final layout slot. citeturn859574search1

Flutter passes constraints downward and expects a render object to choose a size within those constraints; a render object also lays out its children and can declare whether its own size depends on a child's size. citeturn293323search0turn293323search9

Jetpack Compose likewise gives each node constraints, asks it to measure, determines its size, and then places children. Its scopes restrict when measurement and placement operations are legal, which reduces misuse of the protocol. citeturn293323search6

SwiftUI's custom `Layout` protocol separates `sizeThatFits` from `placeSubviews`, and its layout subviews are proxies that expose sizing and placement capabilities rather than framework internals. citeturn293323search2turn293323search3turn293323search4

### Problem this model solves

The key problem is **intrinsic content sizing under parent constraints**.

Examples:

```text
text needs a height derived from width
image has intrinsic dimensions
button size depends on content + padding
container size depends on children
```

A simple one-way "parent assigns rectangle" system cannot express these cases cleanly. Measure/Arrange separates content-driven measurement from parent-driven allocation.

### Important observation

Measure/Arrange does **not** inherently require that every application developer implement Measure/Arrange.

Different frameworks expose different levels of the protocol:

- WPF exposes custom panel overrides.
- Flutter exposes render-object layout methods.
- Compose exposes scoped custom layout functions.
- SwiftUI exposes the `Layout` protocol and layout values.

Therefore the protocol itself is not the same thing as the public client contract.

### Relevance to ui-framework

This is likely the strongest candidate for the internal layout protocol because it addresses the text/content measurement problem that motivated the current design.

The remaining question is how much of this protocol should be exposed to normal C++ component authors.

---

## 4. Model C — Centralized layout managers / layout objects

### Representative system

Qt Widgets.

Qt has a separate `QLayout` hierarchy. Layouts expose geometry, size hints, minimum/maximum size and invalidation behavior, while widgets are layout items managed by the layout system. Qt's custom layout example requires a custom layout manager to implement operations such as `sizeHint()` and `setGeometry()`. citeturn859574search0turn859574search2turn859574search7

### Problem this model solves

It separates:

```text
widget identity/state
```

from:

```text
container layout algorithm
```

A widget does not have to become a different widget subclass just because its parent uses a different layout manager.

### Architectural cost

The layout object needs an association with the child/widget hierarchy. Custom layout authors still need to understand the layout protocol. Qt also exposes invalidation (`invalidate()`) as part of custom layout management. citeturn859574search0turn859574search7

### Relevance to ui-framework

This model is especially relevant to the current discussion because it demonstrates a way to avoid the equation:

```text
GridNode == a new widget type
FlexNode == a new widget type
```

A panel/container can instead have a layout object/strategy.

However, Qt's exact API should not be copied blindly: the goal here is to determine whether the separation itself is useful.

---

## 5. Model D — Flexbox / flexible one-dimensional distribution

### Representative system

CSS Flexbox and flex-inspired engines.

Flexbox is a layout system optimized for interface design in one dimension. Children can be laid out horizontally or vertically, and free space can be assigned to or distributed among children through flex behavior. citeturn859574search9

The CSS specification also demonstrates why mature flex layout becomes technically deep: intrinsic sizing, content-based minimum sizes and interactions between definite/indefinite sizes can cause multiple constraint relationships. citeturn859574search13

### Problem it solves

The core use case is:

```text
linear sequence of children
+
free space
+
controlled expansion/shrinking/distribution
```

This covers common UI structures efficiently:

```text
toolbar
row
column
button groups
split layouts
navigation bars
```

### Architectural cost

A production-level flex algorithm becomes complex quickly once intrinsic sizing, shrink behavior, minimum sizes, wrapping and nested constraints interact.

### Relevance to ui-framework

The historical `flex_panel.cpp` confirms that these capabilities were useful enough to appear in the framework's earlier development. It is valuable evidence for required behavior, but its implementation should remain a legacy reference rather than an API template.

---

## 6. Model E — Grid / two-dimensional track layout

### Representative systems

- CSS Grid
- WPF Grid
- SwiftUI Grid-like containers

### Core idea

Instead of a single main axis, the parent defines two-dimensional tracks and places children into cells/spans.

### Problem it solves

Grid is useful when geometry is relational:

```text
row/column alignment
forms
boards
property panels
table-like structures
multi-region views
```

### Architectural cost

Intrinsic sizing becomes more complicated because row and column requirements can depend on children and spanning. CSS demonstrates how quickly a general grid specification becomes a complex sizing algorithm.

### Relevance to ui-framework

A compact Fixed / Auto / Fr model is likely enough for the target application class unless a concrete requirement proves otherwise. Full CSS Grid is not a reasonable Phase 2 scope.

---

## 7. Model F — Intrinsic/content measurement as a separate capability

### Representative evidence

CSS intrinsic sizing, WPF DesiredSize, Flutter intrinsic sizing APIs, SwiftUI `sizeThatFits`, and Qt `sizeHint` all expose a common idea: content can have a size contribution that is discovered by asking the content/layout item rather than hardcoding every content type into the central container algorithm. citeturn859574search5turn859574search1turn293323search0turn293323search4turn859574search7

### Problem it solves

It is especially important for:

```text
text
images
controls whose size depends on content
```

A central layout engine should not need a large switch over every content type merely to learn its desired size.

### Relevance to ui-framework

This is the most important research result for the historical TextEngine problem.

A plausible architecture is:

```text
Layout engine
    asks node/content for intrinsic size
    ↓
receives generic size information
    ↓
continues generic layout algorithm
```

The text engine can remain an internal implementation detail of the text-bearing component rather than a public client service.

This does not yet prove that the current Node inheritance API is the right mechanism; it only identifies a useful boundary.

---

## 8. Model G — Custom layout extension

### Representative systems

- WPF custom Panels using MeasureOverride / ArrangeOverride. citeturn293323search5
- Flutter custom RenderBox layout. citeturn293323search0
- SwiftUI custom `Layout`. citeturn293323search2turn293323search3
- Jetpack Compose custom `Layout`. citeturn293323search6
- Qt custom `QLayout`. citeturn859574search0turn859574search2

### Problem it solves

No built-in layout system can predict every domain-specific arrangement.

Examples include:

```text
circular menu
calendar grid with special rules
graph/diagram layout
chess board
radial controls
timeline
custom packing
```

### Important observation

All of the mature examples do **not** imply that ordinary application developers must write custom layout.

Custom layout is an escape hatch for layouts not expressible through built-in policies.

This strongly supports a possible model:

```text
closed by default
        +
optional custom layout extension
```

The important design question is how narrow the extension contract can be.

SwiftUI is especially interesting here because a custom Layout receives subview proxies and custom layout values instead of direct access to runtime ownership/lifecycle. citeturn293323search3

---

## 9. Model H — Immediate-mode / application-driven layout

Immediate-mode GUI systems are fundamentally different. Layout can be performed while constructing the frame and does not necessarily maintain the same retained object graph or invalidation semantics as retained-mode UI frameworks.

This model is useful when:

- UI state is cheap to rebuild each frame;
- layout is tightly coupled to immediate drawing;
- deterministic frame construction is more important than retained component identity.

It is not a direct fit for the current framework because `ui-framework` is explicitly retained-mode and already has Node identity, lifecycle and framework-owned lifetime.

Therefore this model should be treated as a contrast case rather than a likely Phase 2 architecture.

---

## 10. What the comparison says about the historical problems

### Problem 1 — "A closed layout engine cannot easily handle text"

Partially valid, but the deeper problem is likely **content measurement coupling**, not closed layout itself.

Mature systems commonly let content participate in measurement through a generic intrinsic-size protocol while keeping the main layout algorithm generic. citeturn859574search1turn293323search0turn293323search4turn859574search7

### Problem 2 — "Custom layout forces users to own invalidation"

This is not inherent to custom layout. It is a consequence of exposing too much of the framework's layout lifecycle to the extension author.

A custom layout can be isolated to geometry decisions while the framework owns invalidation and scheduling. SwiftUI's subview proxy model is a useful example of narrowing the extension surface. citeturn293323search2turn293323search3

### Problem 3 — "Multiple specialized widget base classes force dynamic_cast"

This is a risk when framework dispatch is based on discovering capabilities after the fact. Qt's separate layout-item/layout-manager architecture and SwiftUI's strategy-like Layout protocol demonstrate another route: keep a stable runtime entity and attach layout behavior/metadata explicitly. citeturn859574search0turn293323search2

### Problem 4 — "A single Node becomes a God object"

That is a legitimate risk, but it is not caused merely by having one runtime class. Internal state can be composed into subsystem/value objects while the framework still exposes one stable runtime identity.

### Problem 5 — "A CSS-like property system is unrealistic in C++"

The research does not support that conclusion. C++ frameworks such as Qt already use property-like geometry APIs, while WPF and CSS demonstrate large property-driven layout systems. The difficult part is implementing the engine and its invalidation/measurement semantics, not whether C++ can expose the concept. citeturn859574search0turn859574search1turn859574search8

---

## 11. Most important cross-framework pattern

Across WPF, Flutter, Compose and SwiftUI, a recurring structure appears:

```text
parent supplies a proposal / constraints
        ↓
child/content reports a feasible or desired size
        ↓
container decides final allocation
        ↓
children are placed
```

The difference is primarily **who is allowed to implement which part** and **how much of the protocol is exposed publicly**. citeturn859574search1turn293323search0turn293323search6turn293323search4

That distinction is directly relevant to ui-framework.

---

## 12. Possible architectural families for ui-framework

Without selecting one, the research currently suggests these families deserve serious consideration:

### Family A — Fully closed property-driven engine

```text
Node properties
      ↓
framework LayoutEngine
      ↓
built-in Stack/Grid/Flex/Overlay/Text/etc.
```

Strength: smallest client contract.

Risk: engine must provide enough built-in semantics for the target application class, or it becomes a bottleneck.

### Family B — Closed engine + generic intrinsic measurement

```text
Node properties
      +
content intrinsic measurement
      ↓
framework LayoutEngine
      ↓
built-in layout models
```

Strength: directly addresses the historical text problem without exposing a TextEngine service.

Risk: the measurement boundary and invalidation of content changes must be designed carefully.

### Family C — Closed by default + narrow custom-layout escape hatch

```text
normal user
    -> properties / built-in layouts

advanced user
    -> geometry-only custom layout extension

framework
    -> invalidation / scheduling / lifecycle / ownership
```

Strength: best balance between safety and expressiveness if the extension surface can be kept small.

Risk: the custom-layout API must not leak internal runtime responsibilities.

### Family D — Fully open Measure/Arrange components

```text
component author owns measurement + arrangement
```

Strength: maximum flexibility.

Risk: repeats the invalidation/contract problem already experienced in this framework.

No evidence from this research suggests that Family D should be the default user model.

---

## 13. Research implications for text

Text should be used as a mandatory architectural test case before selecting the final Phase 2 model.

The candidate system must support:

```text
text content changes
font changes
available-width changes
wrapping
intrinsic width/height
padding/margins
alignment inside parent
placement in Stack/Grid/Flex-like containers
```

without making the ordinary client call a separate TextEngine service.

A likely internal boundary is:

```text
Text-bearing node/component
        ↓
content-specific measurement implementation
        ↓
generic size / intrinsic information
        ↓
framework-owned layout engine
```

This remains a research hypothesis, not an accepted design.

---

## 14. Research conclusion

The research does not identify a single universal "correct" layout architecture.

Instead, it shows a recurring separation of responsibilities:

```text
content measurement
        ≠
container arrangement
        ≠
layout scheduling/invalidation
        ≠
runtime ownership
```

The strongest candidate direction for further investigation is therefore not simply "central engine" or "custom layout". It is to determine whether ui-framework can make the **normal user experience property-driven and framework-owned**, while keeping content-specific measurement and any unavoidable custom layout capability behind narrow internal/advanced extension boundaries.

No implementation decision is made by this document.
