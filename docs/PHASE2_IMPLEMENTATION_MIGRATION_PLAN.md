# Phase 2 Implementation Migration Plan

> **Status:** proposed migration sequence; source changes not yet made
> **Date:** 2026-08-19

This plan translates the Phase 2 architecture specification into an incremental implementation sequence designed to preserve the working retained-mode runtime while moving layout ownership from concrete Node overrides into the framework.

## 1. Migration principle

Do not rewrite the entire layout system in one step.

The target is:

```text
Node / PanelNode
    → runtime + structure + component state

LayoutManager
    → all layout execution

NodeTree
    → lifetime + mutation + scheduling
```

The current virtual `measure/arrange` path should be used as a temporary bridge until the new framework-owned path is proven.

## 2. Migration slices

### Slice A — Freeze and isolate deferred layout families

- Do not modify `src/components`.
- Keep current Grid source untouched for now.
- Confirm Grid is not required for the first implementation path or accidentally wired into the new default path.
- Treat `ControlNode` as legacy/deferred.

No runtime behavior change is intended.

### Slice B — Define internal layout semantics

Introduce only the smallest internal concepts necessary for:

```text
orientation

gap
main-axis distribution
cross-axis alignment

measurement proposal
resolved constraints
```

Do not expose custom layout or custom Measure/Arrange APIs.

### Slice C — Move one-dimensional algorithm into LayoutManager

Extract the current Stack behavior into framework-owned helper logic inside `LayoutManager` or a private implementation unit.

At this stage it is acceptable for `LayoutManager` to temporarily recognize `StackPanelNode` while `StackPanelNode::measure/arrange` still exists for compatibility.

The goal is to establish:

```text
LayoutManager → stack algorithm
```

before deleting the old hooks.

### Slice D — Correct measurement proposal resolution

Change the internal measurement flow from:

```text
measure content
→ apply width/min/max
```

to:

```text
parent proposal
→ resolve Node size/min/max
→ effective content proposal
→ content measurement
→ box composition
```

This is essential for wrapped Text.

### Slice E — Add container semantics

Implement the first actual container properties:

```text
orientation
gap
main-axis start/center/end
cross-axis start/center/end
```

Add `stretch` only with an explicitly defined measurement rule.

Do not add flex grow/shrink/wrap.

### Slice F — Establish framework content measurement

For the actual Phase 2 content components, implement framework-owned intrinsic measurement without creating a public `MeasurableNode` hierarchy.

The exact mechanism can remain internal and minimal.

### Slice G — Move invalidation semantics into the framework

Preserve current deferred mutation/queue behavior.

Add the minimum internal separation necessary for:

```text
Measure
Arrange
Render
```

Avoid a full dependency graph until correctness and real performance needs justify it.

### Slice H — Remove client layout obligation

After the new framework-owned layout path is proven:

- remove `PanelNode` pure virtual layout requirement;
- remove `StackPanelNode` algorithm overrides;
- ensure framework-provided components inherit stable layout semantics without requiring client layout code;
- update documentation/examples.

This is the point at which the historical client contract is actually broken.

### Slice I — Acceptance validation

Run the numerical and semantic acceptance cases from:

```text
PHASE2_NUMERICAL_LAYOUT_CASES.md
```

including:

```text
Text width-dependent wrapping
Button content + padding
nested containers
gap/alignment
min/max before measurement
visibility
absolute positioning
parent resize
content mutation
no client invalidation calls
```

## 3. Exact priority of files

### Highest priority

```text
src/core/layoutmanager.cpp
include/ui_framework/core/layoutmanager.hpp
src/core/stackpanelnode.cpp
include/ui_framework/core/stackpanelnode.hpp
```

These define the current algorithm/dispatch boundary.

### Secondary

```text
include/ui_framework/core/panelnode.hpp
src/core/panelnode.cpp
include/ui_framework/core/node.hpp
src/core/node.cpp
include/ui_framework/types.hpp
```

These need targeted semantic changes after the new internal path is established.

### Later / targeted only

```text
src/core/nodetree.cpp
src/core/nodetree.hpp
src/core/ui_manager.cpp
src/core/ui_manager.hpp
```

Only touch these when the finalized invalidation/frame sequencing requires it.

### Do not touch for this migration

```text
src/components/*
inputmanager.cpp
```

unless a concrete integration issue is discovered.

## 4. Manual-edit recommendations

Because `layoutmanager.cpp`, `nodetree.cpp`, `nodetree.hpp` and some `node.*` files are large, implementation should favor a small number of explicit manual patches rather than generated wholesale rewrites.

The most suitable first manual patch is the LayoutManager redesign.

The smaller `panelnode.*`, `stackpanelnode.*`, and type headers can be edited surgically after the new internal path is established.

## 5. First implementation checkpoint

Before changing `PanelNode` or removing any virtual hooks, the following must work internally:

```text
LayoutManager
    → identify framework Stack container
    → derive effective child proposals
    → measure child
    → accumulate one-dimensional size + gap
    → resolve alignment
    → place child geometry
```

Only then should the legacy `Node::measure/arrange` contract be retired.

## 6. Important non-goals during implementation

Do not simultaneously:

- redesign NodeTree ownership;
- introduce ControlNode;
- implement Grid;
- introduce public LayoutStrategy;
- implement full Flexbox;
- rework InputManager;
- restore legacy components architecture.

Keeping these concerns out is necessary to make failures attributable to the Phase 2 layout migration.

## 7. Current implementation readiness

The architecture is sufficiently specified to begin implementation of the first migration slice, but the first source change should be limited to the internal LayoutManager path and the minimal types it requires.

The goal of the first code change is not to complete Phase 2. It is to prove that the framework can execute one-dimensional layout centrally while preserving the existing NodeTree runtime.
