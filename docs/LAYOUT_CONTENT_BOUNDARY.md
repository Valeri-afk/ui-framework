# Layout / Content Measurement Boundary

> **Status:** research and architecture notes; no implementation decision
> **Date:** 2026-08-18
>
> This document records the current investigation into the minimal boundary between content-specific measurement and framework-owned layout orchestration.

## 1. Core question

The framework wants normal users to think in terms of layout properties and built-in containers, not Measure/Arrange or invalidation mechanics.

At the same time, content such as text is not a generic geometric constant. Its desired size can depend on constraints such as available width, font and wrapping behavior.

The question is therefore:

> What is the smallest contract that lets content report geometry without turning the content author into a framework layout implementer?

---

## 2. Three concepts that should remain distinct

The research currently suggests separating:

```text
Proposal / Constraints
        ↓
Content Measurement
        ↓
Container Allocation / Placement
```

### Proposal / Constraints

The parent tells a child what geometric limits or proposal it is considering.

Examples:

- minimum / maximum width;
- minimum / maximum height;
- fixed width proposal;
- unbounded dimension;
- ideal / unspecified request.

Flutter's `BoxConstraints` is an explicit example: the parent passes min/max constraints and the child's size must satisfy them. citeturn150531search2

SwiftUI's `ProposedViewSize` represents a proposal and explicitly distinguishes unspecified, zero and infinite proposals. citeturn150531search9turn150531search7turn150531search8turn150531search12

### Content Measurement

A node/content element answers a generic question:

> Given this proposal/constraint context, what size would you like / can you use?

This is where Text, Image and Button content can remain specialized.

The measurement implementation can know about:

- text shaping;
- font;
- wrapping;
- intrinsic image size;
- content padding;
- component-specific content rules.

The generic layout engine does not need to know those implementation details.

### Container Allocation / Placement

The parent container decides:

- how much space a child receives;
- where the child is placed;
- how sibling sizes interact;
- how free space is distributed.

WPF's custom Panel model is explicit about this split: MeasureOverride measures children, while ArrangeOverride uses those results to assign child rectangles. citeturn755651search0turn755651search1

SwiftUI custom layouts use subview proxies: the layout can ask a subview for size/dimensions and then place the subview without receiving direct ownership of the runtime object. citeturn150531search0turn150531search4

---

## 3. Important result: Measure/Arrange need not be a client API

A previous iteration of this framework exposed too much of the layout protocol to custom component authors. The resulting contract included not only geometry decisions but also invalidation and framework integration.

The research suggests a different boundary:

```text
Framework
    owns:
        layout scheduling
        invalidation
        traversal
        constraints
        lifecycle
        runtime safety

Content implementation
    supplies:
        generic measurement information

Built-in/custom container policy
    supplies:
        child allocation / placement decisions
```

In other words, the engine can still internally use Measure/Arrange while keeping the ordinary component API property-driven.

---

## 4. Text as the decisive test

Consider:

```text
Text("a long paragraph")
width constraint = 200
height = Auto
```

The required height cannot be known without performing text measurement using the width constraint.

This means the layout engine must be able to ask for a content-dependent size.

Qt exposes width-dependent sizing through `heightForWidth`, while Compose explicitly supports intrinsic measurement queries and SwiftUI allows a layout to ask a subview for a size under a proposal. citeturn494672search1turn150531search3turn150531search4

### Consequence

A closed layout engine does not have to own the text engine.

A viable separation is:

```text
Text component
    |
    +-- owns text-specific measurement implementation
    |
    +-- reports generic size information
    |
    v
Framework Layout Engine
```

This addresses the historical TextEngine problem without requiring a public TextEngine service.

---

## 5. Button as the compound-content test

Button is useful because it may depend on text but also has its own padding, border and interaction state.

A reasonable conceptual flow is:

```text
button content measurement
        ↓
button content desired size
        ↓
button padding / border
        ↓
button desired border-box size
        ↓
parent allocation
```

The Button implementation should not need to implement a parent-level arrangement algorithm merely because its size depends on content.

This suggests a distinction between:

```text
content measurement
```

and:

```text
container layout
```

rather than treating every component with children or content as a layout algorithm.

---

## 6. RichText as the future escape-hatch test

Suppose the framework later lacks a sophisticated RichText engine.

A user may reasonably need to implement a RichText component without waiting for the framework to gain full RichText support.

The desired architecture is:

```text
RichText component
    ↓
custom content measurement
    ↓
generic framework geometry
```

not:

```text
RichText author
    ↓
write a full Measure/Arrange implementation
    ↓
learn layout invalidation
    ↓
learn runtime lifecycle
```

If the RichText component also needs a truly custom container algorithm, that is a separate problem and may require a custom-layout extension. It should not be conflated with content measurement.

---

## 7. Custom container test

A custom container such as:

```text
RadialMenu
ChessBoard
Timeline
Graph
```

needs more than content measurement. It needs a rule that determines child placement.

This is the point where a custom layout extension may be necessary.

A useful target is a restricted extension that receives something conceptually like:

```text
container proposal
child proxies
child measurements
child layout metadata
```

and can return/place child geometry.

SwiftUI's `LayoutSubview` is a strong example of this idea: custom layout receives proxies that expose dimensions, sizing proposals and layout values, while the layout container decides placement. citeturn150531search0turn150531search6

The important research question for ui-framework is whether such an extension can remain strictly geometry-focused and therefore avoid exposing lifecycle/invalidation/ownership.

---

## 8. Invalidation should remain outside the measurement contract

The historical framework problem was that a component author often needed to remember to trigger layout invalidation when a measurement-affecting property changed.

The desired separation is:

```text
content property changes
        ↓
framework-owned property/invalidation mechanism
        ↓
layout scheduled
        ↓
content measured when required
```

The measurement implementation itself should not be required to call `markLayout()` or directly manipulate the layout queue.

This is consistent with mature systems where property metadata or framework runtime can schedule layout work. For example, WPF uses property metadata such as `AffectsMeasure` and `AffectsArrange` to trigger invalidation when relevant property values change. citeturn755651search5

---

## 9. Important insight about invalidation granularity

A generic content measurement boundary suggests that the framework can know whether a property affects:

```text
measure
arrange
render
input
```

without the component author managing the queue.

Conceptually:

```text
Property
   ↓
Dependency category
   ├── Measure
   ├── Arrange
   ├── Render
   └── Input
```

This is not a proposed API yet. It is a research direction that could make the previous `markLayout` problem disappear from the user-facing contract.

---

## 10. Constraint protocol vs. desired-size protocol

There are at least two useful ways to express the measurement boundary.

### Model 1 — min/max constraints

Similar to Flutter:

```text
MinWidth
MaxWidth
MinHeight
MaxHeight
```

The content returns a size satisfying those bounds. citeturn150531search2

### Model 2 — size proposals

Similar to SwiftUI:

```text
width = fixed / unspecified / infinity
height = fixed / unspecified / infinity
```

The content chooses its size in response to the proposal. citeturn150531search9turn150531search12

### Model 3 — richer intrinsic queries

Similar to Compose/Flutter intrinsic APIs, the framework may sometimes ask a content element for minimum/maximum intrinsic sizes under specific axis constraints. citeturn150531search3turn150531search1

These models should not be conflated prematurely. The framework may eventually choose a hybrid internal representation.

---

## 11. Single-pass vs multi-query measurement

This matters for a future custom-layout extension.

Compose explicitly restricts normal custom layout measurement to a single measurement of each child per pass and uses separate intrinsic measurement APIs when more information is needed. citeturn150531search10turn150531search3

SwiftUI, by contrast, allows a custom layout to ask subviews for sizes under multiple proposals; its parent may also call `sizeThatFits` more than once with different proposals. citeturn150531search4turn150531search9

This shows that "Measure/Arrange" does not imply one universal measurement algorithm. It is a protocol with different evaluation semantics.

For ui-framework, this should be an explicit design decision rather than an accidental behavior of recursive functions.

---

## 12. Current strongest hypothesis

The most promising conceptual decomposition is currently:

```text
                         Framework Layout Engine
                                  |
                 ┌────────────────┼─────────────────┐
                 │                │                 │
             proposals       measurement       placement
                 │                │                 │
                 │                │                 │
                 v                v                 v
             constraints   content-specific     container
                           size calculation      algorithm
```

The normal client experience could then be:

```text
properties
    ↓
framework-owned invalidation
    ↓
framework-owned layout pass
```

while advanced extensions would only need to cross one of two narrow boundaries:

```text
1. content-specific measurement
2. custom container placement
```

These should not be automatically combined into one giant "custom layout" interface.

---

## 13. Remaining questions before implementation

1. Does a normal `Node` need an intrinsic measurement hook at all, or can the framework infer most component sizes from stored properties and only certain built-in content types need measurement capability?
2. Should content measurement be a virtual capability of `Node`, a composed state object, or a framework-owned measurement adapter?
3. Should the measurement proposal be min/max constraints, SwiftUI-style proposals, or a hybrid?
4. Does this framework require intrinsic min/max queries, or can Phase 2 remain single-proposal for simplicity?
5. How should text expose width-dependent height without exposing TextEngine internals?
6. How should RichText be allowed to extend measurement without becoming a layout participant?
7. How should Grid/Flex child metadata be represented without `dynamic_cast`?
8. How should framework-owned property invalidation be connected to measurement categories?
9. What is the smallest custom-container extension that can support genuinely missing layouts without reopening the old client/runtime contract?

No implementation decision has been made by this document.
