# Phase 2 Numerical Layout Cases

> **Status:** research / pre-specification validation
> **Date:** 2026-08-19

This document walks through concrete numerical examples for the proposed minimal Phase 2 layout system. The purpose is to expose hidden constraints, second-pass requirements and semantic ambiguities before implementation.

## 1. Assumptions used for the examples

For the examples below, use these temporary semantics:

```text
Node width/height = optional preferred/fixed size
Min/Max = outer size constraints
Padding/Border = part of node box
Container layout = one-dimensional horizontal or vertical flow
Gap = spacing between participating flow children
Alignment = container distribution/alignment
PositionMode = Normal or Absolute
Text = proposal-driven content measurement
```

These are validation assumptions, not final API decisions.

## 2. Case A — Fixed-width Text

Tree:

```text
Root width = 400
└── Vertical Panel
      └── Text
```

Assume the Panel has no padding and Text has no explicit width.

Text intrinsic measurement under width 400 returns:

```text
Text desired content = 400 × 80
```

Panel desired size becomes:

```text
width  = 400
height = 80
```

Arrangement:

```text
Text rect = (0, 0, 400, 80)
```

### Result

One measurement pass is enough.

No intrinsic-query subsystem is required.

## 3. Case B — Text plus Button with gap

Tree:

```text
Vertical Panel, width = 400, gap = 10
├── Text
└── Button
```

Assume:

```text
Text measured under width 400 → 400 × 80
Button desired size            → 120 × 40
```

Panel desired height:

```text
80 + 10 + 40 = 130
```

Panel desired size:

```text
400 × 130
```

Placement:

```text
Text   = (0,   0, 400, 80)
Button = (0,  90, 120, 40)
```

### Result

The same algorithm handles multiple content types without knowing their internals.

## 4. Case C — Padding on the Panel

Tree:

```text
Panel width = 400
padding = left/right 20, top/bottom 10
└── Text
```

Content width available to Text:

```text
400 - 20 - 20 = 360
```

Text measurement:

```text
360 × 80
```

Panel outer desired size:

```text
width  = 360 + 40 = 400
height = 80 + 20 = 100
```

### Result

Padding is naturally handled as common Node geometry around container content.

## 5. Case D — Fixed Text width smaller than parent

Parent content width:

```text
400
```

Text explicit width:

```text
200
```

Text is measured with width 200.

Assume:

```text
200 × 140
```

The panel's height contribution is 140.

The remaining horizontal space is an alignment question, not a Text question.

For center alignment:

```text
x = (400 - 200) / 2 = 100
```

Placement:

```text
Text = (100, 0, 200, 140)
```

### Result

Fixed child size and parent alignment remain separate responsibilities.

## 6. Case E — Min/max on Text

Parent content width:

```text
400
```

Text natural result under width 400:

```text
400 × 80
```

Node constraints:

```text
minHeight = 120
maxHeight = 200
```

Effective desired/allocated height becomes at least 120.

```text
Text size = 400 × 120
```

### Important question

The framework must decide whether clamping is applied to the content measurement result before it is used by the parent, or after box composition but before final parent allocation.

The existing code already has Node-level min/max normalization and clamping helpers. fileciteturn221file0

The research favors keeping this normalization framework-owned rather than exposing it to content measurement implementations.

## 7. Case F — Button with Text and padding

Assume:

```text
Button content: Text
Text desired under proposal 200 → 160 × 24
Button padding: 10 left/right, 8 top/bottom
```

Button desired size:

```text
width  = 160 + 20 = 180
height = 24 + 16 = 40
```

The parent sees:

```text
Button desired = 180 × 40
```

The parent does not care that Text created the result.

### Result

This is a strong argument for keeping content measurement below container layout.

## 8. Case G — Parent width changes and text wraps differently

Initial:

```text
Panel width = 400
Text → 400 × 80
```

Resize:

```text
Panel width = 250
```

New proposal:

```text
Text width = 250
```

Suppose the new text result is:

```text
250 × 128
```

The panel now reports:

```text
width  = 250
height = 128
```

No Text mutation occurred.

The new measurement is caused solely by the changed proposal.

### Result

This confirms that proposal propagation is separate from invalidation mutation.

## 9. Case H — Vertical container with center alignment

Container content height:

```text
300
```

Children:

```text
A = 50
B = 70
```

gap:

```text
20
```

Required height:

```text
50 + 20 + 70 = 140
```

Remaining free space:

```text
300 - 140 = 160
```

With center alignment:

```text
leading offset = 160 / 2 = 80
```

Placement:

```text
A = y 80
B = y 150
```

### Result

Main-axis center alignment needs no intrinsic measurement beyond the first pass.

## 10. Case I — Cross-axis center

Horizontal container:

```text
height = 100
```

Child:

```text
size = 60 × 40
```

Cross-axis center gives:

```text
y = (100 - 40) / 2 = 30
```

Placement:

```text
child = (x, 30, 60, 40)
```

### Result

Cross-axis alignment is a pure placement policy when the container has already determined its final size.

## 11. Case J — Stretch

Horizontal container:

```text
height = 100
```

Child desired:

```text
60 × 40
```

With stretch:

```text
child final height = 100
```

The important semantic question is whether the child should be measured using a 100-height proposal before arrangement or whether height may be overridden only at arrangement.

For non-text, these are often equivalent. For height-sensitive content, they may not be.

### Phase 2 recommendation

Use stretch only where the cross-axis dimension does not change the content's intrinsic measurement semantics, or explicitly remeasure if it does.

This keeps the initial algorithm deterministic.

## 12. Case K — Absolute child

Container:

```text
400 × 300
```

Absolute child:

```text
position = (50, 40)
size = 100 × 80
```

Final rect:

```text
(50, 40, 100, 80)
```

The absolute child should not contribute to normal-flow main-axis accumulation.

### Result

Absolute positioning can be treated as a separate framework geometry phase rather than a second layout algorithm exposed to clients.

## 13. Case L — Nested panels

Tree:

```text
Outer Vertical Panel width = 400
├── Text
└── Inner Horizontal Panel
    ├── Button A = 100 × 40
    └── Button B = 120 × 40
```

Inner panel desired size:

```text
width  = 100 + gap + 120
height = 40
```

With gap 10:

```text
230 × 40
```

Outer panel then measures:

```text
Text desired height = 80
Inner panel height   = 40
outer gap            = 10
```

Outer desired height:

```text
80 + 10 + 40 = 130
```

### Result

Nested containers compose without the outer layout knowing the inner algorithm.

## 14. Case M — Visibility

Children:

```text
A = 50
B = hidden
C = 70
```

If current semantics remain that invisible nodes do not participate in layout, required main-axis size is:

```text
50 + 70 + gap = 130
```

rather than including B.

The current `getVisibleChild()` model already filters children for layout traversal. fileciteturn203file0

## 15. Case N — Fixed parent height with text

Panel:

```text
width  = 300
height = 100 fixed
```

Text naturally measures to:

```text
300 × 140
```

The parent cannot expand beyond 100.

The framework therefore must define the relationship between:

```text
Text desired height = 140
Panel final height   = 100
```

Possible semantics are:

```text
clip
overflow
shrink content
```

The first Phase 2 model should not invent sophisticated text shrinking semantics. It should preserve the child desired size while the parent applies its own final constraint/overflow policy.

## 16. Case O — Min/max causing a width-dependent text change

This is the most interesting edge case.

Suppose Text natural measurement at width 500 gives:

```text
500 × 60
```

but the Node has:

```text
maxWidth = 300
```

A correct framework should ensure Text is eventually measured under an effective width of 300 if width affects wrapping, rather than first measuring at 500 and simply clamping the resulting width to 300 while leaving height 60.

A likely effective flow is:

```text
parent proposal
    ↓
combine with Node width/min/max semantics
    ↓
effective content proposal
    ↓
Text measurement
```

This is an important rule.

### Result

Min/max cannot be treated purely as a post-measure clamp for width-sensitive content. They participate in deriving the measurement proposal.

## 17. Consequence for the internal constraint model

The previous observation means the internal pipeline should conceptually be:

```text
parent proposal
        ↓
Node sizing rules
(size/min/max/position semantics)
        ↓
effective content proposal
        ↓
content measurement
        ↓
desired content size
        ↓
box composition
        ↓
parent layout aggregation
```

This is more precise than simply:

```text
measure content
→ clamp result
```

## 18. What remains single-pass

For the Phase 2 cases above, one can generally avoid a second global measurement pass if:

- the parent supplies the relevant width before measuring text;
- fixed/min/max width is incorporated into the proposal before content measurement;
- grow/shrink and wrapping are not part of the first algorithm;
- stretch does not require width/height-sensitive remeasurement;
- absolute children are handled separately.

This is a strong argument for keeping Phase 2 deliberately small.

## 19. What would force additional passes later

The numerical cases make the future boundaries clearer:

```text
flex shrink changes text width
wrapped flex lines
intrinsic width determines track width
multi-axis auto sizing
content-dependent stretch
```

These should be treated as later algorithmic extensions, not hidden inside the minimal engine.

## 20. Current verdict

The numerical cases support the proposed Phase 2 architecture:

```text
Node
  → common geometry constraints

PanelNode
  → child structure

framework content components
  → intrinsic measurement

one-dimensional framework layout
  → aggregation + placement

LayoutManager
  → proposal generation + box normalization + layout execution

NodeTree
  → mutation/invalidation scheduling
```

The most important new rule from the numerical analysis is:

> **Node size/min/max semantics must participate in deriving the content measurement proposal, not merely clamp the final intrinsic result.**

This rule should be incorporated into the formal Phase 2 specification before implementation.

No implementation decision is made by this document.
