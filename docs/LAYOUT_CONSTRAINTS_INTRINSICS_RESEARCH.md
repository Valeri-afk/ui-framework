# Layout Constraints and Intrinsic Sizing Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-18

This document analyzes constraint/proposal semantics, intrinsic sizing and cycles for the retained-mode layout system.

## 1. The fundamental direction

A layout system needs a directional relationship:

```text
parent proposal / constraints
        ↓
child chooses a feasible / desired size
        ↓
parent allocates final geometry
```

Flutter states this explicitly as "constraints go down. Sizes go up. Parent sets position." citeturn922848search12

SwiftUI uses proposed sizes and lets views choose their size in response; its layout containers may query subviews with different proposals when determining flexibility. citeturn922848search0turn922848search3

Compose uses parent-supplied constraints, measures children, decides the parent's size and places children. It deliberately restricts normal layouts from measuring a child more than once in a single measurement pass. citeturn922848search2turn922848search15

The common invariant is more important than the exact API:

> **the parent proposes limitations; the child does not directly choose its position in the parent.**

## 2. Why a simple `availableSize -> Size` model is insufficient

The current framework's `MeasureContext::availableSize` is useful, but a single "available size" number can blur several distinct states:

```text
fixed width
bounded width
unbounded width
unspecified / intrinsic width
minimum width request
```

Text demonstrates the problem. For wrapped text, width is often the input that determines height. A parent may know the width but not the height.

A future internal constraint/proposal representation should therefore distinguish at least:

```text
finite bound
unbounded / unspecified dimension
```

without necessarily exposing a large public constraints API.

## 3. Why intrinsic sizing exists

Intrinsic sizing answers questions such as:

```text
What is the minimum size required by this content?
What is the ideal size if unconstrained?
How tall is this text if width is W?
```

Compose explicitly exposes intrinsic measurements for cases where a parent needs information before performing its normal measure pass. citeturn922848search2

SwiftUI uses proposals such as zero, infinity and unspecified to discover minimum, maximum and ideal behavior. citeturn922848search0turn922848search6turn922848search7turn922848search11

This does **not** mean ui-framework needs a full intrinsic-measurement API in Phase 2.

It means the architecture should not make intrinsic behavior impossible later.

## 4. The text wrapping dependency

Consider:

```text
Text
width = Auto
height = Auto
```

and a parent that has a finite width.

The natural dependency is:

```text
parent available width
        ↓
text measurement
        ↓
text desired height
```

This is not a circular dependency because width comes from an ancestor constraint rather than from the text's own desired height.

The danger appears when both dimensions are mutually dependent.

## 5. The real cycle problem

A pathological layout could conceptually create:

```text
Parent desired width
    depends on child height

Child desired height
    depends on parent width
```

or:

```text
Grid Auto row
    depends on child height

Child height
    depends on width allocated by Grid

Grid width
    depends on total row/column geometry
```

A retained-mode framework must avoid letting the layout algorithm perform arbitrary recursive re-entry until such dependencies stabilize.

The simplest safety boundary is:

> **layout is a staged computation; a measurement query cannot synchronously start a new global layout pass.**

This is consistent with the framework-owned scheduling direction established by the invalidation research.

## 6. Single-pass measurement as the safe default

Compose explicitly forbids measuring a child more than once in normal measurement because multi-pass measurement makes dependency tracking and complexity harder. citeturn922848search15

This is attractive for ui-framework because it gives a predictable rule:

```text
one child
one proposal
one measurement result
per ordinary measurement pass
```

However, this rule alone is restrictive for advanced intrinsic/flexible layouts.

## 7. Why SwiftUI allows more proposals

SwiftUI's `sizeThatFits` may be called multiple times with different proposals, and custom layouts can ask each subview for sizes under different proposals. It also supports a cache for reuse between layout methods. citeturn922848search3turn922848search9

This enables layouts to discover flexibility, for example:

```text
minimum size
ideal size
maximum size
```

before selecting a final proposal.

The cost is a more complex measurement model and stronger pressure for caching.

For ui-framework, this is useful future capability but likely unnecessary as a Phase 2 starting point.

## 8. A useful minimal model for this framework

A promising internal model is:

```text
Constraint / Proposal
    width:  finite | unbounded | unspecified
    height: finite | unbounded | unspecified
```

The framework can still normalize this internally to the existing min/max constraints when invoking content.

A content measurement operation then becomes conceptually:

```text
measure(content, proposal) -> Size
```

with the invariant:

```text
result satisfies the proposal / constraints
```

The client need not see this representation directly.

## 9. Fixed values vs proposals

A fixed `width = 300` on a node and a proposal `width = 300` are not necessarily the same semantic thing.

A fixed node property may constrain the node's final geometry.

A parent proposal means:

> "This is the space I am willing to consider for your measurement."

The distinction should be explicit in the internal engine even if the public API remains small.

## 10. `Auto` should not mean a proposal of infinity

`Auto` is a property semantics, not a constraint semantics.

For example:

```text
width = Auto
```

means the node derives width from its layout/content rules.

It does **not** automatically mean:

```text
measure with infinite width
```

The parent decides whether a dimension is bounded, unbounded or fixed during the measurement pass.

This distinction is particularly important for Text.

## 11. StackPanel and unbounded proposals

A vertical StackPanel typically measures children with an unconstrained main-axis proposal because the children should report their natural vertical requirement before the parent resolves its own height.

Conceptually:

```text
Vertical Stack
    child proposal:
        width = bounded by parent
        height = unbounded
```

Then:

```text
children desired heights
        ↓
Stack desired height = sum
```

This is structurally compatible with the current framework's idea of allowing a measure axis to be effectively unbounded.

## 12. Grid and Auto tracks

Grid makes the proposal model more subtle.

For a fixed track:

```text
child receives approximately fixed width
```

For Auto:

```text
child may need to reveal its natural requirement
```

For Fr:

```text
track size depends on remaining available space
```

Therefore a compact Phase 2 Grid should avoid algorithms where the child measurement depends on a track size that itself depends recursively on the same child result.

A first implementation should prefer deterministic phases such as:

```text
fixed requirements
→ auto requirements
→ remaining flexible space
→ final placement
```

rather than attempting full CSS Grid track sizing.

## 13. Flex and intrinsic sizing

Flex-like algorithms face similar issues.

A simplified model is:

```text
child basis / desired size
       ↓
free space or deficit
       ↓
grow/shrink distribution
       ↓
final child sizes
```

Intrinsic text measurement can supply the initial desired/basis information, but Flex should not ask Text to perform the distribution itself.

The container remains responsible for allocation.

## 14. Cycles involving custom content

Suppose custom content measurement itself queries the parent layout to discover its width.

That would be a dangerous boundary:

```text
parent measures child
    ↓
child asks parent for layout
    ↓
parent measures child again
```

A safe content-measurement API should therefore be **purely proposal-driven**:

```text
content receives proposal
content returns size
```

It should not receive a callback that can synchronously ask the parent to re-layout.

This is another reason to keep scheduling/invalidation outside the measurement contract.

## 15. Intrinsic measurement should be added only when a real layout requires it

The research does not justify implementing a full intrinsic measurement subsystem immediately.

Instead:

1. Keep the core measure query proposal-driven.
2. Support the cases required by Text/Stack/Grid/Flex.
3. Add explicit intrinsic queries only when a concrete layout cannot be expressed correctly without them.

This follows Compose's distinction between ordinary measurement and intrinsic queries rather than making every layout pass intrinsically multi-pass. citeturn922848search2

## 16. Layout pass as a transaction-like phase

A useful conceptual safety model is:

```text
begin layout pass
    ↓
measure
    ↓
resolve container geometry
    ↓
arrange / place
    ↓
commit actual geometry
    ↓
end layout pass
```

During the pass:

- no synchronous global layout restart;
- no ownership mutation through the strategy;
- no lifecycle manipulation;
- measurements are queries;
- placement is mediated by the framework.

Deferred mutations can be queued for after the relevant layout boundary, consistent with the Phase 1 mutation model.

## 17. What this means for Text

Text does not need to know about Grid/Flex.

It needs a measurement function of the form:

```text
proposal(width, height)
        ↓
text engine
        ↓
size
```

The framework decides the proposals.

For wrapped text, a finite width proposal can determine height. For intrinsic/unconstrained cases, an unbounded or unspecified proposal can produce an ideal size.

This is the cleanest resolution so far of the historical TextEngine problem.

## 18. What this means for RichText

A future RichText component can use the same content-measurement boundary.

It can implement:

```text
RichText
    proposal -> size
```

without implementing:

```text
arrange children
invalidate parent
schedule layout
```

If RichText itself becomes a container of inline boxes, that internal layout is still a content-specific implementation detail unless it needs to expose its own child geometry to the framework tree.

## 19. Current conclusion

The research strongly supports the following constraints on a future Phase 2 architecture:

- measurement must be proposal/constraint-driven;
- final child position belongs to the parent layout strategy/framework;
- synchronous re-entry into the global layout engine must be prohibited;
- ordinary measurement should remain predictable and bounded;
- full intrinsic query machinery should be added only when required by a concrete layout;
- text measurement can remain content-specific while the layout engine remains generic.

The exact C++ representation of proposal/constraints is still open.

## 20. Open questions

1. Should ui-framework use a single constraint type for both current Node min/max and future proposals?
2. Should unspecified and unbounded dimensions be explicit values rather than very large finite sentinels?
3. Which built-in layouts actually need intrinsic queries beyond ordinary proposal-based measurement?
4. Can Grid Auto and Flex basis be implemented without multi-pass child measurement in the first version?
5. What should happen if a custom content measurement implementation cannot satisfy a proposal?
6. Should the framework cache measurement results by `(NodeId, proposal, relevant content state)`?
7. Can measurement caching be implemented without exposing cache semantics to the client?

No implementation decision is made by this document.
