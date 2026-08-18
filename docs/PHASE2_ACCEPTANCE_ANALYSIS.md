# Phase 2 Acceptance Analysis

> **Status:** research / no implementation decision
> **Date:** 2026-08-19

This document tests the reduced Phase 2 hypothesis against concrete UI scenarios. It is intentionally narrower than a general-purpose CSS/WPF layout system.

## 1. Acceptance model

Candidate core:

```text
Content measurement
        +
One-dimensional container layout
        +
Basic geometry
        +
Framework-owned invalidation
```

The goal is not to prove that this can express every future layout. The goal is to determine whether it provides a coherent, useful and internally consistent foundation.

## 2. Scenario: text with finite width and automatic height

Input:

```text
Text
width constrained to W
height derived from content
```

Expected flow:

```text
parent proposal W
    ↓
text measurement
    ↓
wrapped height H
    ↓
Text desired size = W × H
```

This is a direct fit for proposal-driven measurement and matches the historical Label behavior, which measured wrapped text using available width. fileciteturn120file0

### Result

**Pass.**

No parent-specific text logic is required.

## 3. Scenario: button = content + padding

Conceptual behavior:

```text
Button
  ├── content measurement
  ├── padding
  └── desired size
```

The framework can treat the button as a content-bearing leaf for layout purposes even if it has internal rendering/input behavior.

### Result

**Pass**, provided content measurement is a framework-owned capability and Button does not need to expose a general container layout contract.

## 4. Scenario: vertical container with gap

Input:

```text
Column
 ├── A
 ├── B
 └── C
 gap = G
```

Measurement:

```text
measure A
measure B
measure C
sum main-axis sizes + gaps
```

Arrangement:

```text
A
 ↓ G
B
 ↓ G
C
```

The current Stack implementation already follows the main-axis accumulation pattern. fileciteturn126file0

### Result

**Pass.**

No Grid-style track system is needed.

## 5. Scenario: horizontal container with gap

Same model, transposed:

```text
Row
 ├── A
 ├── B
 └── C
```

The algorithm remains one-dimensional.

### Result

**Pass.**

This supports the idea that Row/Column/Stack can share one core one-dimensional layout model.

## 6. Scenario: main-axis alignment

Potential modes:

```text
start
center
end
```

The container first knows:

```text
available main-axis size
sum child sizes + gaps
```

Then free space can be distributed according to the selected alignment.

### Result

**Pass.**

`space-between` or other distribution modes can be added later if real requirements justify them.

## 7. Scenario: cross-axis alignment

Potential modes:

```text
start
center
end
stretch
```

The container has a known cross-axis allocation and can place each child accordingly.

### Result

**Pass.**

Per-child cross-axis overrides should not be introduced unless required by actual UI cases.

## 8. Scenario: child min/max constraints

A child can report a desired size, while the framework applies its min/max constraints before the parent uses the result.

Conceptually:

```text
content desired size
        ↓
Node min/max rules
        ↓
layout size contribution
```

The current framework already has min/max state and size clamping. fileciteturn130file0

### Result

**Pass.**

The important architectural question is whether min/max belong to the content measurement phase, the parent arrangement phase, or both. The framework can own this normalization without exposing it to client layout code.

## 9. Scenario: parent size change causes text remeasurement

Example:

```text
Column
  width changes 500 → 300
  ↓
Text receives new width proposal
  ↓
wrapped height changes
```

The framework must:

```text
invalidate measurement
schedule layout
remeasure Text
rearrange container
```

The Text implementation does not need to know why its width changed.

### Result

**Pass conceptually.**

This is one of the most important tests because it validates the framework-owned invalidation boundary.

## 10. Scenario: text content changes

```text
setText(newValue)
        ↓
content state changes
        ↓
framework invalidates Measure
        ↓
layout pass
        ↓
new desired size
        ↓
parent re-layout if needed
```

No explicit `markLayout()` should be required from the user.

### Result

**Pass conceptually**, assuming text state is framework-managed or otherwise has a framework-owned change notification boundary.

## 11. Scenario: absolute / overlay child

The current Node model has `PositionMode`, so the reduced Phase 2 scope can preserve a simple distinction:

```text
normal flow
absolute/overlay
```

A normal-flow child participates in one-dimensional layout.

An absolute child can receive its own placement based on position data and need not contribute to normal flow size.

### Result

**Likely pass**, but exact semantics for parent padding/content box and size contribution must be defined before implementation.

## 12. Scenario: visibility

If a child becomes invisible, the framework must define whether it:

```text
participates in measurement/layout
```

or is removed from layout participation.

This is a semantic choice that should be made explicitly rather than inherited from CSS.

### Result

**Requires explicit policy definition**, but does not require a larger layout architecture.

## 13. Scenario: rendering-only change

Example:

```text
background color changed
```

Geometry remains valid.

The framework should be able to schedule Render without unnecessary Measure/Arrange work.

### Result

**Architecturally compatible**, even if Phase 2 initially uses a coarser layout queue.

## 14. Scenario: RichText not yet built into framework

A future custom RichText implementation only requires:

```text
proposal → desired size
```

if the content measurement boundary is kept narrow.

It can then live inside the built-in one-dimensional container system without a client-defined container layout.

### Result

**Pass conceptually.**

This is the main reason that closing container layout does not necessarily make early framework coverage a blocker.

## 15. Scenario: no client invalidation API

The acceptance goal is that normal code never needs:

```text
markLayout()
invalidateMeasure()
invalidateArrange()
queueLayout()
```

Instead:

```text
state/property mutation
      ↓
framework-owned invalidation
```

The current implementation already routes deferred layout mutations through `NodeTree` and queues layout work, although the semantics are currently coarse-grained. fileciteturn124file0

### Result

**Pass as architectural direction.**

## 16. What the scenarios expose as missing

The reduced model is coherent, but several concrete semantic decisions remain:

### A. Margin

Not necessary for the current acceptance set. Padding plus gap plus position may cover the immediate needs.

### B. Per-child alignment override

Useful but not proven necessary for the first version.

### C. Grow/shrink

Potentially useful for flexible free-space distribution, but not needed for the simplest one-dimensional layouts.

### D. Wrap

Not necessary unless a UI scenario requires multi-line flow.

### E. Absolute positioning details

Need explicit box/coordinate semantics.

### F. Visibility/layout participation

Needs explicit behavior.

## 17. Architectural stress result

The acceptance scenarios do **not** expose a fundamental contradiction in the reduced architecture.

The same overall pipeline can handle:

```text
Text
Button
Row/Column
Gap
Alignment
Size/min/max
Padding
Position
Content changes
Parent resize
```

without requiring:

- Grid;
- full Flexbox;
- public custom layout;
- TextEngine service;
- client-side invalidation.

This is strong evidence that a small closed layout core is feasible.

## 18. Remaining architectural boundary

The hardest remaining issue is not the set of basic properties. It is the internal relationship between:

```text
Node measurement
Container measurement
Final placement
Invalidation propagation
```

especially for width-dependent text and future flexible free-space allocation.

The current research suggests the framework should preserve a generic internal Measure/Arrange pipeline while not exposing it as the ordinary client API.

## 19. Current verdict

The minimal Phase 2 scope is currently **architecturally coherent** and appears sufficient to exercise the important problems discovered during the project's previous layout iterations.

The scope should remain deliberately small until real UI cases prove that additional primitives are required.

No implementation decision is made by this document.
