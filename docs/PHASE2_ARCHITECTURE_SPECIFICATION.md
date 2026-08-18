# Phase 2 Architecture Specification

> **Status:** proposed architecture for review; no source implementation implied
> **Date:** 2026-08-19

This document consolidates the Phase 2 research into one target architecture. It is intentionally narrower than a general-purpose CSS/WPF layout system and is designed around the current C++ retained-mode framework and the lessons from its previous layout iterations.

## 1. Goal

Phase 2 should establish a small, framework-owned layout system that solves the currently demonstrated needs:

```text
Text measurement
Container composition
One-dimensional flow
Size / min / max
Padding / border
Position / position mode
Alignment
Gap
Visibility / layout participation
Automatic invalidation
```

The client should configure components and compose them through framework-provided APIs. The client should not implement layout algorithms or participate directly in layout invalidation/scheduling.

## 2. Explicit non-goals

The first Phase 2 architecture does **not** attempt to implement:

```text
Grid
Full CSS Flexbox compatibility
Flex wrapping
General intrinsic-size query APIs
Public CustomLayoutStrategy
Client-side Measure/Arrange contracts
CSS-style dynamic property dictionaries
WPF-style dependency properties
Universal Margin semantics
ControlNode solely for architectural symmetry
```

These features can be reconsidered only when a concrete requirement demonstrates their necessity.

## 3. Core runtime hierarchy

The framework should retain at least:

```text
Node
  ↓
PanelNode
```

### Node

`Node` is the base runtime/component element.

Responsibilities:

```text
runtime identity
parent / ownership references
lifecycle
common state
common geometry properties
visibility / enabled / focus-related runtime state
events
render/update hooks
framework-owned mutation entry points
```

`Node` should not own children.

### PanelNode

`PanelNode` is the structural container base.

Responsibilities:

```text
child ownership / storage
child attachment / removal
child traversal
container participation in NodeTree
```

`PanelNode` should not require client-defined layout algorithms.

The existing NodeTree already treats `PanelNode` as the structural capability for child ownership and traversal. fileciteturn203file0

## 4. ControlNode

`ControlNode` remains deferred.

Current evidence does not justify it as a universal base. `Node` already provides enabled/focusable/capturable state and event storage, while the current legacy `ControlNode` implementation does not match the current Node rendering API. fileciteturn224file0

A separate ControlNode should be introduced only if several framework components later demonstrate a stable responsibility that cannot be naturally owned by `Node` or a more specific component base.

## 5. Framework-provided concrete components

The framework should provide concrete components that combine component semantics with supported layout semantics.

Examples for the initial scope:

```text
Text
Button
StackPanel / LinearPanel
Modal or dialog-like framework components as needed
```

A client can inherit from these framework components to customize behavior/state without implementing layout.

Example:

```cpp
class PrimaryButton : public Button
{
    // custom component behavior/state
};
```

The framework-provided Button keeps its measurement/layout semantics.

## 6. Closed layout engine

Layout execution remains entirely framework-owned.

Conceptually:

```text
NodeTree
   ↓
LayoutManager
   ├── proposal generation
   ├── constraint resolution
   ├── content measurement
   ├── one-dimensional container layout
   ├── absolute/overlay placement if enabled
   ├── arrangement
   └── geometry commit
```

The client does not receive public APIs for:

```text
measure()
arrange()
markLayout()
invalidateMeasure()
invalidateArrange()
LayoutManager
layout queue
```

The current virtual `Node::measure()` / `arrange()` methods may remain temporarily as an internal migration bridge, but they are not part of the intended client contract. fileciteturn224file0

## 7. Layout container model

The first built-in layout should be a one-dimensional flow model.

Conceptually:

```text
Linear / Stack panel
    orientation: horizontal | vertical
    gap
    main-axis distribution
    cross-axis alignment
```

This single algorithm can express:

```text
Row
Column
Vertical Stack
Horizontal Stack
simple toolbar
simple forms
header/content/footer
button content rows
```

Separate `RowNode` and `ColumnNode` implementations are not necessary unless the public API later benefits from distinct names.

## 8. Initial one-dimensional semantics

### Orientation

```text
Horizontal
Vertical
```

### Gap

A container-owned spacing value between participating flow children.

`gap` is preferred over adding general `margin` semantics in the first version.

### Main-axis distribution

Start with:

```text
Start
Center
End
```

`SpaceBetween` can be added if required and should be treated as container distribution rather than universal Node alignment.

### Cross-axis alignment

Start with:

```text
Start
Center
End
Stretch
```

Per-child alignment override is deferred until a real UI requirement demonstrates its need.

## 9. Common Node geometry properties

Strong candidates for universal `Node` state are:

```text
size
min size
max size
padding
border
position
position mode
visibility
```

The framework interprets these in the context of the active layout algorithm.

### Padding

Padding belongs to the Node because it describes the relationship between the Node's outer box and its own content/children.

### Margin

Margin is intentionally not part of the first universal property model. Its semantics depend on the parent layout relationship and are not needed for the demonstrated Phase 2 cases.

### Position

Position data can remain Node state while its meaning is controlled by `PositionMode` and the framework's positioning semantics.

## 10. Position modes

The existing `PositionMode` concept distinguishes normal layout participation from absolute positioning. fileciteturn213file0

The first Phase 2 model should preserve this distinction, but define absolute children as a separate framework geometry step:

```text
normal-flow pass
      ↓
absolute/overlay placement pass
```

Absolute children do not participate in normal-flow accumulation.

Exact coordinate origin/content-box semantics must be specified in implementation tests before finalizing the API.

## 11. Text and content measurement

Text is a content measurement problem, not a container layout problem.

Conceptually:

```text
effective measurement proposal
        ↓
Text measurement implementation
        ↓
desired content size
        ↓
framework box composition/layout
```

The framework must own how/when this measurement occurs.

Text state such as:

```text
text
font
wrapping
text alignment
```

belongs to the Text component and should trigger framework-owned measurement invalidation when geometry can change.

The old `components/Label` implementation is useful historical evidence for proposal-driven text measurement, but the old components architecture should not be restored. fileciteturn215file0

## 12. Intrinsic measurement is optional

A generic Node does not need intrinsic measurement.

The current Node already returns zero from its default `measure()` implementation. fileciteturn224file0

The target semantics may therefore be:

```text
ordinary Node
    → no intrinsic content by default

framework Text/Button/Image/etc.
    → framework-owned intrinsic measurement
```

This avoids introducing a public `MeasurableNode` capability hierarchy.

## 13. Measurement proposal resolution

This rule is critical:

> Node size/min/max semantics must participate in deriving the effective content measurement proposal, not merely clamp the final intrinsic result.

Conceptually:

```text
parent proposal
      ↓
Node constraints
(size/min/max/position semantics)
      ↓
effective content proposal
      ↓
content measurement
      ↓
desired content size
      ↓
box composition
```

This is necessary for width-dependent text wrapping.

Example:

```text
parent width = 500
Node maxWidth = 300
```

Text should ultimately be measured under an effective width of 300, not first measured at 500 and merely have its result clamped afterward.

## 14. Compound framework components

A component such as Button may internally combine content measurement and component geometry semantics.

Conceptually:

```text
Button
  ├── content state
  ├── visual/input state
  ├── padding/border
  └── framework-owned content measurement
```

The parent container sees the resulting Button desired size and does not need to know whether it came from Text, Image or another content implementation.

## 15. Measurement dispatch

The framework should not introduce a public measurement base class solely for Phase 2.

The initial implementation can use a framework-internal mechanism for the small set of framework components that actually provide intrinsic measurement.

The exact mechanism remains an implementation choice among:

```text
closed concrete-component dispatch
compact internal measurement descriptor/function pointer
internal content object
```

The architectural rule is more important than the exact mechanism:

> Content measurement is an internal framework capability, not a client-facing layout contract.

Avoid `std::function`-style generic type erasure in every Node unless a concrete requirement proves it necessary.

## 16. Layout algorithm: measurement phase

For a vertical container:

```text
parent proposal
      ↓
derive child proposals
      ↓
measure each participating child
      ↓
accumulate main-axis sizes
      ↓
max cross-axis size
      ↓
apply container box geometry
      ↓
container desired size
```

Horizontal layout exchanges the axes.

For the minimal Phase 2 cases, the normal flow algorithm should remain approximately linear in participating children.

## 17. Layout algorithm: arrangement phase

After the container has a final allocation:

```text
available main-axis space
        ↓
required child sizes + gaps
        ↓
free space
        ↓
main-axis distribution
        ↓
child coordinates
```

Cross-axis placement then uses the selected alignment policy.

The layout algorithm writes framework-owned geometry outputs rather than exposing direct geometry mutation to the client.

## 18. Stretch semantics

`Stretch` is safe only when the changed final cross-axis size does not invalidate content measurement semantics, or when the framework explicitly remeasures under the required final size.

The first implementation should avoid introducing broad stretch behavior that silently requires multiple measurement passes.

## 19. Deliberately deferred flexible allocation

The following are deferred:

```text
flex-grow
flex-shrink
flex-basis
wrap
multi-line distribution
```

These features can create width/height dependencies for Text and other intrinsic content that may require additional measurement stages.

They should enter the system only when a concrete requirement justifies the complexity.

## 20. Invalidation model

The framework owns invalidation and scheduling.

Conceptually:

```text
state mutation
      ↓
framework determines affected work
      ↓
deferred/coalesced mutation queue
      ↓
layout pass
```

The client does not call invalidation APIs.

The current `Node::deferLayoutMutation()` and `NodeTree` mutation queue already provide a useful correctness-first foundation. fileciteturn221file0turn223file0

## 21. Dirty categories

The internal system should be able to distinguish at least conceptually:

```text
Measure
Arrange
Render
```

The first implementation may remain conservative and re-layout a queued root/subtree rather than implementing a highly optimized dirty dependency graph.

The public contract should not depend on that optimization level.

## 22. Invalidation propagation

Local state changes produce local facts:

```text
Text content changed
    → Text measurement dirty

Container gap changed
    → container layout dirty

Background changed
    → render dirty
```

The framework then determines ancestor impact.

Do not make every property descriptor encode a universal rule about all ancestors.

The active parent layout determines whether child desired-size changes affect parent geometry.

## 23. Re-entrancy

Layout should behave as a deferred transaction:

```text
flush mutations
    ↓
begin layout pass
    ↓
measure
    ↓
arrange
    ↓
commit geometry
    ↓
end pass
```

Mutations that occur during the pass are deferred rather than causing synchronous recursive layout.

This builds on the existing `NodeTree` mutation-scope model. fileciteturn223file0

## 24. Geometry outputs

The following are framework outputs rather than ordinary client properties:

```text
desiredSize
actualSize
actualPosition
constraints/proposals
```

The framework should not re-trigger layout merely because an output geometry value changed during a correct layout pass.

## 25. Visibility

Current layout traversal already filters children through `getVisibleChild()`. fileciteturn203file0

Therefore Phase 2 should preserve the meaning:

```text
visible = false
    → does not participate in normal flow
```

Changing visibility is therefore layout-affecting as well as render/input-affecting state.

## 26. Client inheritance model

Normal client inheritance should look like:

```text
Node
  ↓
custom leaf/component
```

or:

```text
framework panel type
  ↓
custom component inheriting that supported layout family
```

The client should not derive from `PanelNode` and implement a new layout algorithm.

A generic `PanelNode` subclass without a framework-defined layout semantics remains an API decision that should be constrained deliberately; the first implementation may keep `PanelNode` primarily as a structural base and expose concrete framework panels for supported layout families.

## 27. Layout dispatch

The first closed engine does not need a public Strategy hierarchy.

A controlled framework-owned dispatch is acceptable:

```text
concrete framework container
        ↓
LayoutManager
        ↓
built-in algorithm
```

A container-level RTTI check is not equivalent to the historical per-child `dynamic_cast` problem.

If a few framework container types exist, explicit central dispatch is simpler and more auditable than adding a policy object hierarchy prematurely.

## 28. Relationship-specific layout state

Properties that depend on the parent layout should not be added universally to Node.

Examples deferred for future specialized systems:

```text
Grid row/column/span
Flex grow/shrink/basis/order
Dock side
```

The current minimal one-dimensional layout needs only container-owned state such as orientation, gap and distribution/alignment.

## 29. Custom layout policy

There is intentionally no public custom layout strategy in Phase 2.

If a future real use case cannot be expressed with the built-in layout system, the framework can decide later between:

```text
add a new framework layout
add a composition mechanism
introduce a narrow advanced extension
```

This avoids permanently committing to an extension contract before its necessity is proven.

## 30. Custom intrinsic content

A future advanced API for custom content measurement may be considered independently from custom layout:

```text
proposal → desired size
```

This is intentionally not required for the initial Phase 2 implementation.

It is a much narrower extension and does not allow clients to control sibling placement, layout scheduling or NodeTree ownership.

## 31. Current file/architecture migration direction

The current source already contains:

```text
Node
PanelNode
StackPanelNode
NodeTree
LayoutManager
```

and `LayoutManager` already owns recursive measurement/arrangement orchestration. fileciteturn201file0turn224file0

Therefore migration should be incremental.

Conceptually:

```text
1. Preserve the existing internal layout pipeline while stabilizing semantics.

2. Move concrete layout algorithm ownership from PanelNode overrides into the framework layout subsystem.

3. Reduce concrete framework panel classes to persistent configuration/state.

4. Keep NodeTree responsible for structural/lifetime mutation.

5. Make invalidation semantics framework-owned and progressively phase-aware.

6. Remove the old client-visible measure/arrange contract only after the internal replacement is proven.
```

Large current files such as `nodetree.*`, `inputmanager.cpp` and `layout_manager.cpp` should not be rewritten wholesale unless the migration proves that necessary. Small supporting types can be changed more surgically.

## 32. Phase 2 acceptance criteria

The architecture should pass at least:

```text
Text with bounded width and wrapping
Text with parent resize
Text content/font/wrap change
Button with content + padding
Vertical container with gap
Horizontal container with gap
Main-axis start/center/end
Cross-axis start/center/end/stretch under defined semantics
Min/max constraints
Padding/border box composition
Nested containers
Visibility removing child from flow
Absolute child under defined positioning semantics
No client-side invalidation calls
No client-side access to LayoutManager / NodeTree scheduling
No client-defined measure/arrange algorithm
```

## 33. Known complexity boundaries

The first architecture should stop before:

```text
intrinsic width-driven track systems
flex shrink with width-dependent text
multi-line wrapping layout
general intrinsic query APIs
cyclic layout equations
```

These are not architectural failures. They are later algorithmic capabilities that should be introduced when required.

## 34. Final Phase 2 architecture summary

```text
                         UI Framework
                              │
                   ┌──────────┴──────────┐
                   │                     │
                  Node               PanelNode
                   │                     │
          common runtime/geometry     children
                   │                     │
        ┌──────────┼──────────┐         │
        │          │          │         │
      Text       Button     Image   framework panels
        │          │          │         │
        └──────────┴──────────┘         │
                 content                │
              measurement         one-dimensional layout
                    │                      │
                    └──────────┬───────────┘
                               │
                         LayoutManager
                               │
                  ┌────────────┼────────────┐
                  │            │            │
             constraints    measure      arrange
                  │            │            │
                  └────────────┼────────────┘
                               │
                          geometry output
                               │
                           NodeTree
                     mutation / scheduling
```

The central architectural principle is:

> **The client describes components and properties; the framework owns layout execution and invalidation.**

No implementation decision is made merely by publishing this specification. It is the proposed architecture to review before source changes begin.
