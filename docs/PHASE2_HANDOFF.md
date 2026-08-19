# Phase 2 Handoff / Current Context

> **Purpose:** canonical recovery document for continuing Phase 2 in a new chat.
> **Date:** 2026-08-19
> **Active branch:** `phase2-layout-migration`
> **Current branch HEAD:** `a87a3ff3d6c64742eef7b8d443ca1ccbf9e10d34`
> **Build/tests:** intentionally deferred until the end of Phase 6 by project decision.

## 1. Phase 2 objective

Move layout ownership from the Node inheritance hierarchy into the framework layout subsystem while preserving the retained-mode runtime and NodeTree ownership model.

Target ownership:

```text
Node / PanelNode
    -> runtime state + structure

LayoutManager
    -> measurement + constraints + container layout + arrangement

NodeTree
    -> lifetime + mutation + scheduling
```

No public `LayoutStrategy`, `CustomLayoutStrategy`, `MeasurableNode`, or client-defined measure/arrange contract is part of Phase 2.

## 2. Important project decisions

- `src/components/*` is legacy reference material and must not be modified.
- `inputmanager.cpp` remains untouched unless a concrete Phase 2 dependency is demonstrated.
- Grid is deferred; an existing Grid implementation must not force a generic layout abstraction.
- ControlNode is deferred.
- Full Flexbox is deferred (`flex-grow`, `flex-shrink`, `flex-basis`, wrap, order, etc.).
- Margin is deferred.
- Build and runtime tests are not to be run during Phase 2 development. Final compilation/tests are planned only after the core framework phases are formed, per project decision.

## 3. Current architectural model

### Node

Owns common geometry/configuration/state:

```text
size
min/max
padding
border
position
position mode
visibility
actual/desired geometry
runtime/event state
```

It does not own children and no longer exposes legacy `measure()` / `arrange()` virtual layout hooks.

### PanelNode

Owns structural children and traversal. It is not intended to require a client-defined layout algorithm.

### StackPanelNode

Now acts primarily as persistent configuration:

```text
orientation
gap
main-axis alignment
cross-axis alignment
```

The one-dimensional algorithm is in internal `linear_layout` code and is dispatched by `LayoutManager`.

### LayoutManager

Owns the current framework layout pipeline:

```text
root proposal
  -> constraint proposal resolution
  -> border/content box conversion
  -> framework content measurement
  -> Linear child measurement/allocation
  -> final constraint resolution
  -> arrangement
  -> actual geometry
```

### NodeTree

Still owns live nodes, identity, lifecycle, mutation queue, traversal, roots/overlays and layout scheduling. It does not own layout mathematics.

## 4. Constraint semantics currently established

The critical distinction is:

```text
measurement proposal != final size
```

Current semantics:

```text
Fixed size
    -> constrains measurement proposal and final geometry

Max size
    -> may narrow measurement proposal and final geometry

Min size
    -> constrains final geometry

Auto
    -> intrinsic measurement / parent allocation determines size
```

Padding and border translate between border-box and content-box.

For width-dependent content:

```text
parent proposal 500
maxWidth 300
Text

measure using effective width 300
```

not:

```text
measure at 500
then clamp to 300
```

A minimum does not automatically narrow the intrinsic measurement proposal.

## 5. Current implementation state

The following internal files are active:

```text
include/ui_framework/core/linear_layout.hpp
src/core/linear_layout.cpp
include/ui_framework/core/layout_constraints.hpp
src/core/layout_constraints.cpp
```

`LayoutManager` currently integrates both.

### Constraint fixes already committed

1. `cd9f0ca2686be6772eba75ceda72188cdc56b385`
   `Apply min and max constraints to fixed final sizes`

   Final explicit sizes are now clamped by min/max instead of bypassing final constraints.

2. `f574c3386fccbc7e7f13f70e94ed3326f2dd6e0d`
   `Bound fixed measurement proposals by max size`

   Fixed proposals are also bounded by max before content measurement, which is required for wrapped text correctness.

### Absolute positioning fix

3. `2b148b876d2e511957f7a5b0af1233bc7279d7dc`
   `Use parent content allocation for absolute children`

   Absolute children are removed from Linear flow, measured independently, positioned relative to the parent content origin, and final size is resolved through the common constraint resolver. Auto falls back to desired size; explicit size/max are resolved through the common path.

### Legacy layout seam removal

4. `727335baa16f06b84605cf2206c16e65238540f3`
   `Remove legacy Node layout migration seam`

   Removed `Node::measure()` and `Node::arrange()`.

5. `a87a3ff3d6c64742eef7b8d443ca1ccbf9e10d34`
   `Remove legacy layout context types`

   Removed `MeasureContext` and `ArrangeContext`.

Repository search after these changes found no remaining `MeasureContext`, `ArrangeContext`, `measure(` or `arrange(` references in the accessible repository search.

## 6. Text architecture checkpoint

Text measurement and rendering are now explicitly separated.

Measurement:

```text
TextNode
    text + font
        -> LayoutManager::measureTextNode()
        -> TTF_GetStringSizeWrapped()
```

Rendering:

```text
TextNode::draw()
    -> renderer-specific TTF_TextEngine / TTF_Text
    -> TTF_DrawRendererText()
```

`TextNode` header contains renderer-specific text objects but no wrap-width measurement property. The implementation no longer contains a `measure()` override or client-owned measurement state.

The previously suspected header/implementation mismatch is therefore resolved.

## 7. Box-model checkpoint

The current measurement path is:

```text
border-box proposal
    -> resolve Node proposal/max
    -> subtract padding + border
    -> content measurement
    -> add padding + border
    -> desired border-box size
```

Final geometry uses the common final resolver.

Absolute position uses:

```text
parent actual position
    + parent border + padding
    + child position offset
```

Negative offsets are allowed; no CSS-like left/right/top/bottom model is being introduced in Phase 2.

## 8. Current exact position in Phase 2

The project is **past the central Linear algorithm extraction, proposal/max correction, Text measurement integration, Absolute placement integration, and removal of the legacy Node measure/arrange contract**.

The remaining work is primarily consolidation and acceptance rather than another large architectural rewrite.

Current flow is approximately:

```text
                 Phase 2
                    |
    architecture / internal types       DONE
                    |
    Linear algorithm ownership          DONE
                    |
    constraint proposal/final semantics DONE
                    |
    Text intrinsic measurement          DONE
                    |
    Absolute placement baseline         DONE
                    |
    legacy measure/arrange removal      DONE
                    |
    >>> CURRENT CHECKPOINT <<<
                    |
    cleanup / consistency audit
                    |
    remaining Linear edge semantics
                    |
    invalidation/lifecycle audit
                    |
    numerical acceptance audit
                    |
    Phase 2 documentation/closure
```

## 9. Remaining work estimate

Do **not** interpret this as an exact commit count. The remaining work is better measured as architectural work units.

Expected remaining units:

```text
1. TextNode dead include/helper cleanup
2. Full static consistency audit of Linear + constraints + box model
3. Final Linear semantics audit:
       stretch + min/max
       gap/alignment edge cases
       zero/empty children
       visibility
4. Absolute/normal arrangement consistency audit
5. Invalidation/lifecycle audit against deferred mutation semantics
6. Numerical acceptance-case audit against PHASE2_NUMERICAL_LAYOUT_CASES.md
7. Final Phase 2 documentation/checkpoint cleanup
```

So the realistic remaining scope is roughly **6-7 focused work units**, not another major redesign. Some of these may collapse into the same commit if the code already satisfies the intended semantics.

Build/test execution is deliberately excluded from these Phase 2 units.

## 10. What must NOT be reintroduced

Do not reintroduce:

```text
Node::measure()
Node::arrange()
MeasureContext
ArrangeContext
public LayoutStrategy
public custom layout API
TextEngine service
client invalidation calls
CSS/Flexbox-wide property model
Grid implementation into the new generic path
```

## 11. Immediate next step

Before making the next code commit, this handoff itself is the recovery checkpoint.

The next code change is the already identified TextNode cleanup:

```text
remove unused measurement-related includes/helpers from src/core/textnode.cpp
```

This must not alter rendering or measurement behavior.

After that, continue with the static Linear/constraint acceptance audit.

## 12. Validation policy

Until Phase 6:

```text
NO BUILD
NO RUNTIME TESTS
```

Use only:

```text
source inspection
architectural consistency checks
static reasoning
numerical cases
ownership/lifecycle reasoning
documentation checkpoints
```

At the eventual final validation point, run the full build/test/compilation verification.

## 13. Recovery procedure for a new chat

Read this file first.

Then, in order:

```text
PHASE2_ARCHITECTURE_SPECIFICATION.md
PHASE2_SOURCE_LEVEL_MAPPING.md
PHASE2_CONSTRAINT_SEMANTICS.md
PHASE2_NUMERICAL_LAYOUT_CASES.md
PHASE2_LAYOUT_LIFECYCLE_INVALIDATION_ANALYSIS.md
INTRINSIC_MEASUREMENT_DISPATCH_ANALYSIS.md
PHASE2_MEASUREMENT_DISPATCH_RECOMMENDATION.md
```

Treat this handoff's current checkpoint as authoritative over older statements in the implementation migration plan that describe the migration as not yet started or that say legacy hooks still remain.

## 14. Last known branch state

```text
branch:
    phase2-layout-migration

HEAD:
    a87a3ff3d6c64742eef7b8d443ca1ccbf9e10d34
    Remove legacy layout context types

build/tests:
    intentionally deferred

next code action:
    TextNode dead-code/include cleanup
```
