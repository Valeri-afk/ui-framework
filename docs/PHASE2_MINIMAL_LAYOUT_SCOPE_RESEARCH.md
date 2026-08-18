# Phase 2 Minimal Layout Scope Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document narrows Phase 2 to the smallest useful layout capability supported by the current framework requirements. It deliberately avoids assuming that a full CSS/WPF/Flutter-scale layout system is necessary.

## 1. Current target scope

The current requirements can be expressed as:

```text
Text measurement
Container layout
Basic geometry
Basic spacing
Basic positioning
Basic alignment
Framework-owned invalidation
```

At this stage there is no demonstrated requirement for:

- Grid;
- full Flexbox compatibility;
- wrapping flex lines;
- complex intrinsic query APIs;
- masonry;
- arbitrary custom client layout strategies;
- CSS-level value systems.

This is an important architectural constraint: the framework should solve the actual current UI problem rather than building a general-purpose browser layout engine.

## 2. Why a one-dimensional layout is a strong candidate

Flexbox was designed for one-dimensional interface layout and provides main-axis distribution, cross-axis alignment and optional grow/shrink behavior. citeturn296030search2turn296030search1

This covers common UI patterns:

```text
Row
Column
Toolbar
Navigation bar
Header/content/footer
Button content
Horizontal/vertical groups
```

The current historical `FlexPanel` also demonstrates that this family of capabilities was useful in the project, although the legacy implementation should not dictate the new architecture. fileciteturn117file0

## 3. Full Flexbox is not the same as a minimal one-dimensional layout

CSS Flexbox includes:

```text
flex direction
main-axis justification
cross-axis alignment
align-self
flex-grow
flex-shrink
flex-basis
wrapping
multi-line alignment
order
```

and its sizing semantics become substantially more complex when intrinsic content, shrink behavior, wrapping and indefinite dimensions interact. citeturn296030search0turn296030search4turn296030search6

The framework does not currently need to implement all of this.

A minimal Phase 2 container can initially support:

```text
orientation: horizontal / vertical
main-axis alignment: start / center / end / maybe space-between
cross-axis alignment: start / center / end / stretch
fixed gap
```

Grow/shrink should be introduced only if a real UI requirement needs free-space allocation inside children.

Wrapping should be deferred unless a concrete layout requires multiple lines.

## 4. Why Stack + one-dimensional Flex may be one system rather than two

A conventional Stack can be viewed as a restricted one-dimensional layout:

```text
orientation
fixed child sizes
fixed gap
alignment
no grow/shrink
```

A Flex-like layout generalizes it with free-space distribution.

This suggests that the framework may not need separate core concepts for:

```text
Stack
Row
Column
Flex
```

unless their APIs or semantics become materially different.

One internal one-dimensional layout algorithm could potentially express the simple Stack case as a special configuration.

This is a research observation, not a naming/API recommendation.

## 5. Text remains a first-class content measurement case

Text should be tested as the primary non-trivial content type.

The required behavior is:

```text
proposal / available width
        ↓
text measurement
        ↓
generic desired size
```

The historical Label already measured wrapped text according to available width and returned `LayoutSize`, confirming that the basic mechanism fits the project well. fileciteturn120file0

The framework should own when measurement occurs and when it is invalidated, while text-specific measurement remains inside the text implementation.

## 6. Minimal universal geometry

The current Node already contains a significant set of geometry state: size, min/max size, position, position mode, padding, border, overflow, visibility and actual geometry. fileciteturn130file0

The first Phase 2 architecture should resist adding additional universal properties merely because CSS exposes them.

A conservative core should focus on properties with demonstrated need:

```text
Size
Min/Max size
Padding
Position
Position mode
Alignment
Visibility
```

Margin should be treated carefully because its semantics are often relationship-dependent; it should not be added automatically just to match CSS.

## 7. Alignment should be defined through the active container

Alignment is one of the places where a CSS-like interpretation can become misleading.

In flexbox, `justify-content` controls the main-axis distribution of the group, while `align-items` controls cross-axis alignment of children; `align-self` overrides a child's cross-axis alignment. citeturn296030search0turn296030search5

Therefore the framework should first determine whether it actually needs:

```text
container main alignment
container cross alignment
per-child alignment override
```

It should not automatically add all three properties to every Node.

A simpler first system may use:

```text
container: main/cross alignment
child: optional alignment override
```

or even container alignment only if current UI needs do not require per-child control.

## 8. Gap vs margin

Flexbox provides `gap` as spacing between adjacent items. MDN also notes that margins, padding, justification and gap all affect spacing but have different semantics. citeturn296030search3turn296030search4

For a small framework, `gap` is a strong primitive because it belongs clearly to the container layout and does not introduce relationship-specific margin collapsing or other CSS box-model rules.

This suggests:

```text
one-dimensional container
    -> gap
```

may be preferable to introducing `margin` immediately.

## 9. Positioning

Two basic positioning modes are enough for many early UIs:

```text
normal layout participation
absolute/overlay positioning
```

The current Node already has `PositionMode`, so the research does not justify creating a separate positioning subsystem before validating the existing concept. fileciteturn130file0

However, absolute positioning must be defined carefully relative to parent padding/content box and whether absolutely positioned children participate in parent measurement.

## 10. Padding

Padding is naturally universal because it affects the relationship between a Node's outer box and its own content/children.

The current framework already stores padding and the layout manager converts content size to border-box size. fileciteturn130file0

This makes padding a strong candidate for remaining a Node-level property rather than a container-specific property.

## 11. What the minimum container must do

The smallest useful container should be able to:

```text
1. own/contain child Nodes through NodeTree;
2. measure children under proposals;
3. compute its desired size;
4. assign final child rectangles;
5. support orientation;
6. support gap;
7. support main/cross alignment;
8. remain fully framework-managed for invalidation and execution.
```

This is enough for a substantial amount of UI without Grid or full Flexbox.

## 12. Custom layout is not required by this scope

The current scope provides no concrete reason to expose a user-defined layout strategy.

If the built-in one-dimensional container is sufficient for current UI requirements, then the framework can deliberately keep layout closed:

```text
Client
   -> built-in containers + properties

Framework
   -> all layout algorithms
```

A future custom-layout API can be added only if a real framework user scenario demonstrates a need that built-in layouts cannot satisfy.

This avoids introducing an extension contract prematurely.

## 13. RichText / missing content support

The absence of RichText does not necessarily require custom container layout.

A future RichText component can use the same content-measurement boundary:

```text
RichText
    proposal -> desired size
    ↓
built-in container
```

This preserves a closed container layout model while allowing new content implementations as the framework grows.

## 14. Minimal invalidation scope

The Phase 2 framework needs at least:

```text
Measure invalidation
Arrange invalidation
Render invalidation
```

It does not yet need a full global dependency graph.

The existing framework already coalesces layout work through a queued layout root; this can remain the correctness-first implementation while property semantics are clarified. fileciteturn124file0

## 15. Recommended architectural restraint

The research now strongly supports these exclusions from the first scope:

```text
No Grid
No CSS property registry
No public CustomLayoutStrategy
No full Flexbox wrapping
No general intrinsic-size query subsystem
No generic dynamic property dictionary
No second Node/relationship ownership graph
```

These are not statements that the features are bad. They are statements that the current requirements do not justify their architectural cost.

## 16. Phase 2 research target

A highly focused architecture can now be tested around:

```text
                Layout Engine
                     │
             ┌───────┴────────┐
             │                │
        Content measure   1D container
             │                │
            Text       horizontal / vertical
             │          gap + alignment
             │                │
             └────────┬───────┘
                      │
                Node geometry
               size / minmax
             padding / position
                      │
                 invalidation
```

This is substantially smaller than a CSS-like system while still exercising all of the difficult architectural boundaries discovered during the research.

## 17. Acceptance cases for the minimal scope

The architecture should be considered viable if it can correctly handle:

```text
1. Text with finite width and automatic height.
2. Text with changed content causing parent relayout.
3. Button whose size depends on text + padding.
4. Vertical container with gap.
5. Horizontal container with gap.
6. Main-axis start/center/end distribution.
7. Cross-axis start/center/end/stretch.
8. Child min/max constraints.
9. Parent size changes causing text remeasurement.
10. Absolute-positioned child if PositionMode remains supported.
11. No client-side invalidation calls.
12. No client-side access to NodeTree/layout queue.
```

## 18. Current conclusion

The current requirements do not justify building a general layout engine.

The strongest research direction is now a **small framework-owned one-dimensional layout system with content-specific measurement**, backed by a small universal geometry model and automatic invalidation.

The system can remain closed to client-defined layout algorithms.

This is not a claim that one-dimensional layout is universally sufficient. It is a claim that it is the smallest realistic architecture that exercises the important problems already discovered in this project:

- text measurement;
- container arrangement;
- retained-mode ownership;
- framework-owned invalidation;
- C++ type/ownership boundaries.

No implementation decision is made by this document.
