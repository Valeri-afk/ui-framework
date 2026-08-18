# Layout Invalidation Research

> **Status:** research / no implementation decision
> **Date:** 2026-08-18

This document records the current analysis of layout invalidation and how it can remain fully framework-owned while supporting content measurement and parent-specific layout metadata.

## 1. Core observation

The historical framework problem was not simply that layout was custom. The problem was that custom components were expected to participate directly in invalidation.

Established systems show that these responsibilities can be separated.

WPF associates invalidation behavior with property metadata such as `AffectsMeasure`, `AffectsArrange`, `AffectsRender`, and also `AffectsParentMeasure` / `AffectsParentArrange`. When an effective property value changes, the property system can initiate deferred invalidation automatically. citeturn265224search0turn265224search6turn265224search3

Qt similarly asks a custom widget to notify the layout system when its size hints change (`updateGeometry()`), and coalesces repeated notifications. This is simpler than WPF but still keeps the layout recalculation in the framework's layout management rather than requiring the widget to run the layout itself. citeturn265224search4turn265224search11

Compose separates measurement and placement into layout phases and schedules layout when state read during those phases changes. It also gives measurement and placement different restart scopes. citeturn265224search5

The research therefore supports a strong architectural principle:

> **the framework should own invalidation; a component should only mutate framework-managed state.**

## 2. Invalidation is not one thing

A useful model is:

```text
property/state change
       ↓
what dependency does it affect?
       ├── Measure
       ├── Arrange
       ├── Render
       └── Input
```

A layout system can then derive the minimum required work.

Examples:

| Change | Likely effect |
|---|---|
| Text content | Measure + Arrange + Render |
| Font | Measure + Arrange + Render |
| Wrap width | Measure + Arrange + Render |
| Width / Height | Measure + Arrange |
| Min/Max | Measure + Arrange |
| Padding / Border | Measure + Arrange |
| Alignment | Arrange |
| Position | Arrange |
| Grid row/column | Parent Arrange, possibly Measure depending on algorithm |
| Flex grow/shrink | Parent Arrange / Measure depending on algorithm |
| Visibility | Parent Measure + Arrange + Render |
| Background color | Render |
| Focus | Input / Render |

These are conceptual categories, not an accepted property table yet.

## 3. Why parent invalidation matters

A child property can affect the parent rather than the child itself.

For example:

```text
Grid
  └── child.row = 2
```

Changing `row` may alter:

- which track receives the child's desired size;
- the size of an `Auto` row;
- other children's final positions.

Therefore invalidating only the child is insufficient.

WPF explicitly recognizes this class of dependency with `AffectsParentMeasure` and `AffectsParentArrange`. citeturn265224search3turn265224search6

This is a strong argument for making relationship-specific layout metadata part of the framework's invalidation-aware state model.

## 4. Text invalidation

Text is the most important practical example.

```text
text changes
    ↓
text desired size may change
    ↓
parent layout may change
    ↓
children may move
```

A text component should not call a generic `markLayout()` method that it has to understand.

Instead, a framework-managed text/content property should already be associated with the fact that its effective value can affect measurement.

Qt's width-dependent text example demonstrates the same concept: if a label's height depends on its width, the layout system needs to know that a sizing hint has changed. citeturn265224search4turn265224search11

## 5. Content measurement and invalidation

A custom content measurement implementation should therefore have no direct invalidation API.

Conceptually:

```text
setText()
   ↓
framework state changes
   ↓
framework knows: affects Measure
   ↓
invalidate appropriate layout relationship
   ↓
next layout pass
   ↓
content measured again
```

The measurement callback itself remains a pure-ish geometry query:

```text
proposal / constraints
      ↓
content measurement
      ↓
Size
```

This is cleaner than requiring the measurement implementation to manage dirty flags.

## 6. Parent-owned metadata and invalidation

If Grid/Flex metadata is parent-owned, its mutation naturally identifies the layout owner.

For example:

```text
GridPolicy
  setPlacement(child, newPlacement)
       ↓
GridPolicy knows its own layout semantics
       ↓
invalidate Grid parent/container
```

This is more reliable than asking the child to understand that `GridRow` affects its parent's layout.

In other words, parent-owned metadata gives the framework an explicit invalidation target.

## 7. Reparenting interaction

Phase 1 currently defers reparenting, but future design should maintain the invariant:

```text
old parent relationship ends
        ↓
old layout metadata becomes irrelevant

new parent relationship begins
        ↓
new layout metadata/defaults become authoritative
```

This strongly favors parent-owned relationship metadata because invalidation follows the relationship that actually interprets the data.

## 8. Root-level invalidation vs fine-grained invalidation

The current framework queues the nearest root and relayouts the subtree.

This is intentionally simple and correctness-oriented.

The research suggests that finer-grained invalidation is possible, but it is not necessary to solve the fundamental contract problem.

A future optimization could distinguish:

```text
measure dirty subtree
arrange dirty subtree
render dirty subtree
```

but Phase 2 does not need this merely to achieve framework-owned invalidation.

## 9. Coalescing

Invalidation should be deferred/coalesced rather than forcing immediate layout on every property change.

Qt explicitly documents that consecutive `updateGeometry()` calls are coalesced into one layout recalculation. citeturn265224search4

This matches the existing Phase 1 preference for deferred mutation and batched work.

## 10. Avoiding a dependency-property framework

WPF's property metadata is powerful but large. The framework should not blindly reproduce the entire dependency property system merely to obtain automatic layout invalidation.

A smaller C++ model might associate each framework-managed property with a dependency category:

```text
Property metadata
    affectsMeasure
    affectsArrange
    affectsRender
    affectsParentMeasure
    affectsParentArrange
```

Only the categories actually needed by ui-framework should exist.

The mechanism could be compile-time, centralized setter metadata, or explicit framework-owned state objects. The exact form remains open.

## 11. What must remain framework-owned

Regardless of the final storage model, the following should not be part of normal client responsibilities:

- deciding when layout is dirty;
- selecting the invalidation root;
- managing the layout queue;
- synchronizing measure/arrange passes;
- deciding whether a parent also needs invalidation;
- triggering a layout pass immediately;
- interacting with NodeTree mutation guards.

This directly addresses the historical failure of the open Measure/Arrange model.

## 12. What client code should do

Normal client code should mutate state:

```text
set text
set font
set width
set padding
set grid placement
set flex value
```

The framework interprets those changes according to its property metadata and layout relationships.

No explicit:

```text
markLayout
invalidateMeasure
invalidateArrange
```

should be required for normal components.

## 13. Important architectural consequence

Once invalidation is framework-owned, the distinction between "closed layout engine" and "custom content/layout extension" becomes much less dangerous.

A custom content measurement extension can be called inside a framework-owned layout pass.

A custom container layout extension can also be called inside a framework-owned layout pass.

Neither extension needs to own the scheduler.

Therefore:

```text
custom capability
    ≠
custom invalidation
```

This is likely the most important correction to the assumptions of the framework's earlier open Measure/Arrange implementation.

## 14. Current research conclusion

The strongest current architectural principle is:

> **Properties and relationship metadata may be extensible, but invalidation is a framework concern.**

A likely conceptual split is:

```text
Property / relationship change
          ↓
framework-owned dependency classification
          ↓
coalesced invalidation
          ↓
Measure / Arrange / Render
```

The client participates by changing state, not by scheduling layout work.

## 15. Open questions

1. Should dependency classification be static metadata or explicit per-property callbacks?
2. How much of the WPF-style `AffectsParent*` model is actually required?
3. Can Grid/Flex metadata changes identify the parent layout directly without global dependency tracking?
4. Should text/font state live inside Node or in a composed content state object?
5. How should asynchronous/resource-driven content measurement invalidate layout when an asset changes?
6. Can the current root-level NodeTree queue support all needed invalidation semantics without introducing a dedicated dirty-tree structure?
7. What is the smallest framework property model that provides automatic invalidation without becoming a general dependency-property system?

No implementation decision is made by this document.
