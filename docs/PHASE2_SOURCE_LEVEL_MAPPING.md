# Phase 2 Source-Level Mapping

> **Status:** implementation preparation / no Phase 2 source changes yet
> **Date:** 2026-08-19

This document maps the approved Phase 2 architecture specification onto the current repository. It identifies which existing files align with the target, which require redesign, and which current source should remain untouched.

## 1. Current repository shape relevant to Phase 2

The current source tree separates:

```text
src/core
src/components
```

The historical `src/components` area is treated as obsolete and is **not** an implementation target. Its Label/Button code is usable only as architectural evidence.

The current core contains:

```text
Node
PanelNode
StackPanelNode
GridNode
NodeTree
LayoutManager
UIManager
InputManager
ModalManager
ControlNode (legacy/intermediate)
```

`GridNode` is currently present in `src/core`, which conflicts with the deliberately reduced Phase 2 scope. It must therefore be explicitly classified before implementation begins: either archived/deferred, or retained but not integrated into the new Phase 2 path. The target architecture itself does not require Grid. fileciteturn227file0turn230file0

## 2. `Node`

### Current role

`Node` currently owns:

```text
identity
parent / owner
visibility / enabled / focusable / capturable
size / min / max
position / position mode
padding / border / overflow
actual / desired geometry
events
update / draw
mount / unmount
protected measure / arrange hooks
mutation deferral
```

The current public header confirms this structure and shows that `measure()` / `arrange()` are still protected virtual hooks. fileciteturn224file0

### Target role

Keep the bulk of the common runtime/geometry state.

Remove the **architectural responsibility** for client-defined layout algorithms, but do not remove the current virtual hooks until the framework-owned replacement exists.

### Required change class

**Major architectural change, but potentially incremental.**

The immediate migration should not rewrite all of `Node`. The priority is to make `measure/arrange` internal implementation seams rather than client-defined semantics.

### Manual editing suitability

`node.hpp` is medium-sized. Surgical changes are preferable.

`node.cpp` is larger but still manageable; avoid wholesale rewrite.

## 3. `PanelNode`

### Current role

`PanelNode` already owns the structural responsibility we want:

```text
children_
add/remove
child traversal
visible-child traversal
attachment validation
```

It currently also makes `measure()` and `arrange()` pure virtual. fileciteturn225file0

### Target role

```text
PanelNode = structural container base
```

It should continue to own children and structural semantics.

It should no longer require a client-visible custom layout implementation contract.

### Required change

Eventually remove:

```cpp
virtual LayoutSize measure(MeasureContext&) = 0;
virtual void arrange(ArrangeContext&) = 0;
```

but only after the new framework-owned layout dispatch path is functional.

### Manual editing suitability

`panelnode.hpp/.cpp` are suitable for targeted edits.

## 4. `StackPanelNode`

### Current role

`StackPanelNode` stores `orientation_` and currently owns the full stack measurement and arrangement algorithm. fileciteturn229file0

### Target role

The persistent data should remain small:

```text
orientation
future gap/alignment when implemented
```

The algorithm should move into framework-owned layout execution.

### Important discovery

Current Stack does **not** yet implement the intended Phase 2 semantics:

```text
gap
main-axis distribution
cross-axis start/center/end/stretch
```

It simply accumulates desired sizes and then fills the cross-axis with the full container content size. fileciteturn229file0

Therefore the new Phase 2 layout code should not be treated as a tiny patch to the existing Stack algorithm. Its semantics need to be rewritten around the new specification.

### Manual editing suitability

`stackpanelnode.cpp` is small enough for direct framework editing. This should be treated as a semantic rewrite, not a patch-by-patch extension of the old algorithm.

## 5. `GridNode`

### Current role

A substantial Grid implementation already exists, including:

```text
rows
columns
track definitions
placements
row/column spans
measure
arrange
```

The source is non-trivial and maintains its own layout algorithm. fileciteturn230file0

### Target role

Grid is intentionally **outside the first Phase 2 target**.

Therefore there should be no assumption that the current Grid implementation is part of the new architecture.

### Required action

Before changing layout code, explicitly choose one of:

```text
A. leave Grid source untouched but exclude it from Phase 2 integration;
B. move/archive Grid as deferred work;
C. later migrate Grid after the one-dimensional layout is stable.
```

The recommended current choice is **A**, unless the build/public export path makes the existing Grid presence actively interfere with the new architecture.

### Important warning

Do not let the existence of Grid force a universal `PanelNode` layout-property model or a generic strategy interface.

## 6. `LayoutManager`

### Current role

`LayoutManager` already owns:

```text
viewport
layout queue processing
recursive measurement
recursive arrangement
content/border conversion
min/max normalization
geometry commit
```

The current implementation delegates the actual node-specific algorithm to virtual `Node::measure()` / `arrange()`. fileciteturn228file0turn231file0

### Target role

This becomes the **central owner of closed layout execution**.

It should own or coordinate:

```text
constraint resolution
content measurement dispatch
one-dimensional layout algorithm
absolute placement
arrangement
geometry outputs
```

### Most important current mismatch

The current measurement pipeline applies fixed/min/max rules after calling `node.measure()`:

```text
node.measure(ctx)
    ↓
toBorderBoxSize()
    ↓
applyMeasureRules()
```

But the numerical analysis showed that width/min/max can affect the *measurement proposal itself*, especially for wrapped text.

Therefore this code path needs redesign rather than merely moving the Stack algorithm. fileciteturn228file0

### Manual editing suitability

This is a large and central file. It is one of the files where the user's previously stated preference for manual edits is appropriate if the redesign becomes extensive.

The migration should first be designed as precise steps before touching it.

## 7. `NodeTree`

### Current role

`NodeTree` is already the runtime authority for:

```text
live Node registry
ownership assertions
mount/unmount
mutation queue
layout queue
root/overlay ownership
```

It also uses `PanelNode` RTTI to validate structural parent capabilities. fileciteturn223file0

### Target role

Keep this responsibility.

The new layout system should not move lifetime or child ownership into layout-specific objects.

### Required changes

Likely limited to invalidation/scheduling integration once the new dirty semantics are defined.

The current deferred mutation architecture is a strong foundation and should be preserved.

### Manual editing suitability

`nodetree.cpp/.hpp` are large. Avoid wholesale edits. Only make targeted changes after the layout invalidation contract is fully specified.

## 8. `UIManager`

`UIManager` already owns `NodeTree`, `InputManager`, `ModalManager` and `LayoutManager`, exposing frame/event operations while keeping those subsystems private. fileciteturn235file0

This is already a good top-level architecture for a closed layout engine.

Likely Phase 2 impact:

```text
minimal
```

unless frame sequencing changes after the layout migration.

## 9. `ControlNode`

Current `ControlNode` remains a legacy/intermediate candidate and is not justified by the Phase 2 architecture.

Do not introduce a new replacement `ControlNode` merely to fill a hierarchy gap.

Whether the current source should be deleted, archived, or simply left unused should be handled separately from the layout migration after checking include/build reachability.

## 10. `src/components`

Do not modify.

Use only as historical reference for:

```text
Label measurement
Button composition
old Component measurement boundary
```

The current Phase 2 architecture intentionally does not resurrect the old `Component::arrange()` client contract.

## 11. `types.hpp`

The common layout vocabulary currently includes values such as:

```text
LayoutSize
LayoutConstraints
Alignment
PositionMode
Padding
Border
Overflow
MeasureContext
ArrangeContext
```

The existing types are usable as a starting point, but `Alignment` currently mixes container distribution semantics with child alignment semantics.

Therefore `types.hpp` is likely to require a small semantic cleanup during implementation.

Do not redesign the entire type system prematurely.

## 12. `inputmanager.cpp`

The user already manually updated this large file during Phase 1.

No direct architectural change is currently required by the Phase 2 layout specification.

Avoid touching it unless the layout migration changes hit testing or absolute positioning semantics.

## 13. Current Phase 2 source mismatch summary

| File / area | Current state | Phase 2 action |
|---|---|---|
| `node.hpp/cpp` | Common state + virtual layout hooks | preserve state, remove client layout contract incrementally |
| `panelnode.hpp/cpp` | children + pure virtual layout hooks | keep children, remove layout obligation after migration |
| `stackpanelnode.hpp/cpp` | orientation + full algorithm | keep configuration, move algorithm to framework layout |
| `gridnode.*` | full Grid implementation | defer/exclude from first Phase 2 |
| `layoutmanager.*` | orchestration + node virtual dispatch | major redesign; central closed layout owner |
| `nodetree.*` | mutation/layout scheduling | mostly preserve; targeted invalidation changes later |
| `ui_manager.*` | subsystem orchestration | likely minimal change |
| `controlnode.*` | legacy/intermediate | defer/remove only after reachability check |
| `src/components/*` | obsolete | do not touch |
| `types.hpp` | mostly usable common types | small semantic cleanup likely |
| `inputmanager.*` | Phase 1 changes already made | do not touch for layout unless required |

## 14. Recommended migration order

### Step 1 — Freeze current semantics

Do not change source yet. Define the first closed layout API/types precisely.

### Step 2 — Establish framework-internal measurement boundary

Implement framework-owned content measurement for the first actual framework content component set.

### Step 3 — Move one-dimensional algorithm into LayoutManager-side code

Do this while keeping existing virtual `measure/arrange` hooks temporarily if that reduces migration risk.

### Step 4 — Add the intended container semantics

Add:

```text
orientation
gap
main distribution
cross alignment
```

without adding full Flexbox.

### Step 5 — Correct proposal/constraint resolution

Ensure Node width/min/max semantics participate in the content measurement proposal before intrinsic measurement.

### Step 6 — Make invalidation semantics explicit internally

Keep NodeTree's deferred mutation model, then introduce the minimum Measure/Arrange/Render distinction needed for correct scheduling.

### Step 7 — Remove client layout obligation

Once framework-owned layout is stable, remove the pure virtual `PanelNode::measure/arrange` obligation and retire concrete panel algorithm overrides.

### Step 8 — Validate with the numerical acceptance suite

Use the cases documented in `PHASE2_NUMERICAL_LAYOUT_CASES.md`.

## 15. Files likely to require the user's manual involvement

Given the user's stated preference for large-file manual changes:

```text
HIGH MANUAL VALUE:
    src/core/layoutmanager.cpp
    src/core/nodetree.cpp
    src/core/node.cpp (depending on scope)

LIKELY SAFE FOR SURGICAL AUTOMATED EDITS:
    panelnode.hpp/.cpp
    stackpanelnode.hpp/.cpp
    small type headers

DO NOT TOUCH:
    src/components/*
    inputmanager.cpp unless layout semantics specifically require it
```

This is a preparation assessment, not a command to edit yet.

## 16. Current source-level conclusion

The repository already contains most of the structural pieces required by the target architecture.

The main architectural seam to change is:

```text
Current:
Node/PanelNode hierarchy owns layout algorithms

Target:
Node/PanelNode hierarchy owns component/structure
LayoutManager owns layout algorithms
```

The second major change is:

```text
Current:
Node constraints mostly normalize measured result

Target:
Node constraints participate in deriving measurement proposal
```

The rest of the migration can largely preserve Phase 1 ownership, mutation and runtime architecture.

No implementation changes are made by this mapping.
