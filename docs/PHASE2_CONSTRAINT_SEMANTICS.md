# Phase 2 Constraint Semantics

> **Status:** research / provisional semantic contract
> **Date:** 2026-08-19

This document defines the provisional distinction between measurement proposals and final size constraints for the Phase 2 layout engine.

## 1. Why the distinction is necessary

The current `LayoutManager` applies explicit size and min/max partly after content measurement. This is insufficient for proposal-dependent content such as wrapped text.

Example:

```text
parent available width = 500
node maxWidth = 100
text measured at 500 → 500 × 60
final clamp → 100 × 60
```

This is incorrect when text wrapping depends on width.

The desired path is:

```text
parent proposal
    ↓
measurement proposal resolution
    ↓
content measurement at effective width
    ↓
desired content size
    ↓
final size resolution
```

## 2. Two different concepts

### Measurement proposal

The question given to content:

> "Under what available bounds should you determine your desired content size?"

It is driven by the parent layout, explicit size, and constraints that bound the available content space.

### Final size constraints

The rules applied to the resulting Node size:

```text
explicit size
min size
max size
parent allocation
```

A minimum size generally constrains the final result, but should not automatically force content to measure at that minimum when doing so changes intrinsic content behavior.

A maximum size can bound a measurement proposal because the content cannot meaningfully need more space than the Node is allowed to occupy.

## 3. Fixed explicit size

If a Node has a fixed width/height:

```text
width = 200
```

then the content should be measured under an effective width corresponding to that fixed box (after accounting for padding/border).

This is both a final constraint and a measurement proposal.

## 4. Maximum size

If:

```text
parent available width = 500
maxWidth = 300
```

the effective measurement width should be:

```text
300
```

because content cannot produce a meaningful layout wider than the Node's permitted maximum.

For wrapped text this is essential.

## 5. Minimum size

If:

```text
parent available width = 500
minWidth = 300
```

the intrinsic content may still be measured under 500 if the parent allows 500 and no fixed/max width restricts it.

The final Node width must nevertheless be at least 300.

This avoids forcing content to wrap differently merely because the Node has a larger minimum.

## 6. Combined min/max

If:

```text
minWidth = 300
maxWidth = 400
parent available = 500
```

effective measurement width should be 400 because maxWidth bounds the available content space.

Final width is then resolved within:

```text
300 … 400
```

according to intrinsic result and parent allocation.

## 7. Parent unbounded proposal

An unbounded main-axis proposal means the parent is asking:

> "How large would you like to be in this axis?"

For a vertical linear container, child height may therefore be unbounded during measurement while width remains bounded by the parent's content width.

This matches the existing Stack algorithm's basic behavior.

## 8. Minimum size does not create an artificial content proposal

For text:

```text
parent width = 500
minWidth = 300
natural content at 500 = 450 × 60
```

The Node may finalise at 450, not 500 or 300, because the minimum is already satisfied.

There is no reason to remeasure at 300 merely because `minWidth=300`.

## 9. Minimum larger than intrinsic result

If:

```text
parent width = 500
minWidth = 400
content natural result = 250 × 80
```

final width should be at least 400.

The initial Phase 2 model does not require content to reflow at 400 solely because of `minWidth`. The minimum is a final geometry rule.

If a future content type proves that its semantics require remeasurement at the forced final size, that can be added explicitly.

## 10. Padding and border

Node proposals are expressed in content-box terms to content measurement.

Conceptually:

```text
parent border-box proposal
        ↓
subtract padding + border
        ↓
content measurement proposal
```

The desired content size is then converted back:

```text
content desired size
        ↓
add padding + border
        ↓
Node desired border-box size
```

## 11. Auto size

`Auto` means the Node does not impose an explicit size on that axis.

It does not by itself mean:

```text
fill parent
```

The parent layout determines how an auto-sized child participates.

In the initial Linear layout:

```text
normal auto child
    → uses intrinsic/desired size
```

while cross-axis stretch may assign a larger final size during arrangement.

## 12. Fixed size vs stretch

If a child has fixed cross-axis size, stretch should not override it.

If the child is auto-sized in the cross axis and the container uses stretch, the framework may allocate the available cross size subject to the child's min/max constraints.

This distinction belongs to the framework layout phase, not the child component.

## 13. Final size resolution

After content measurement:

```text
intrinsic/desired border-box size
        ↓
explicit size if specified
        ↓
min/max clamp
        ↓
parent layout allocation
        ↓
final actual size
```

The exact order between parent allocation and local clamp must be implemented consistently, but every final size must respect the Node's explicit/min/max contract.

## 14. Why min/max should not be universal measurement constraints

This distinction keeps the framework from implementing accidental CSS-like behavior.

The framework only needs a small rule:

```text
max/fixed can narrow measurement proposal
min constrains final result
```

rather than a general-purpose constraint algebra.

## 15. Implications for Linear layout

For a vertical Linear container:

```text
child width proposal
    → parent's available content width
    → limited by child's fixed/max width

child height proposal
    → unbounded for intrinsic flow measurement
    → fixed/max may bound it where explicitly specified
```

For horizontal layout, the axes are exchanged.

The container should not directly inspect the child's content type.

## 16. Implications for Text

Wrapped Text depends on the effective width proposal.

Therefore:

```text
Text width = auto
Text maxWidth = 300
parent width = 500
```

means:

```text
measure Text at width 300
```

whereas:

```text
Text width = auto
Text minWidth = 300
parent width = 500
```

can still measure at 500 and only require the final width to be >= 300.

## 17. Implications for Button

A Button's content can be measured under the Button's effective content proposal.

Button-level padding and size constraints remain framework-owned geometry semantics.

The parent container sees only Button's resulting desired size.

## 18. Current recommended helper semantics

The internal LayoutManager should conceptually have two operations:

```text
resolveMeasurementProposal(node, parentProposal)
resolveFinalSize(node, desired, parentAllocation)
```

These should remain framework-internal implementation functions rather than public APIs.

## 19. What this does not solve yet

The model intentionally does not define:

```text
percent sizes
aspect ratio
flex grow/shrink
min-content/max-content queries
multi-axis intrinsic equations
```

Those require additional semantics and are outside the first Phase 2 model.

## 20. Current verdict

The Phase 2 constraint model should be deliberately small:

```text
Fixed size
    → constrains measurement + final geometry

Max size
    → can narrow measurement proposal + final geometry

Min size
    → constrains final geometry

Auto
    → no local explicit size; parent/layout decides allocation

Padding/border
    → translate between border-box and content-box
```

This is sufficient to support the first Text/Panel/Linear scenarios without introducing a general constraint solver or CSS-sized model.
