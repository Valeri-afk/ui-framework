# Phase 2 Recovery Checkpoint

> **Status:** Phase 2 source-level work complete; runtime/build validation intentionally pending
> **Date:** 2026-08-19
> **Branch:** `phase2-layout-migration`
> **Recovery point:** `68aaa431ec4063023ff801fc508e78f738209c14`

## Current state

Phase 2 layout migration has completed its source-level implementation and consistency work. The branch currently contains the final Node cleanup change and the previously applied layout-invalidation fix.

## Completed areas

- Layout architecture and ownership boundaries established.
- Constraint semantics finalized.
- Linear layout measurement/allocation finalized.
- Text measurement separated from container layout.
- Absolute children kept outside normal-flow aggregation.
- Legacy layout API removal completed.
- Numerical acceptance semantics finalized in `PHASE2_NUMERICAL_LAYOUT_CASES.md`.
- Node sanitation/constraint normalization audited.
- Layout invalidation fixed so a mutated child queues its top-level layout root rather than itself.
- Obsolete `measurePreferredSize()` and `subtractPaddingBorder()` helpers removed from `src/core/node.cpp`.

## Canonical constraint semantics

```text
Fixed → measurement proposal + final size
Max   → measurement proposal + final size
Min   → final size only
Auto  → intrinsic measurement / parent allocation
```

For width-sensitive content, `maxWidth` narrows the measurement proposal before measurement. Minimum constraints do not implicitly become intrinsic measurement proposals.

## Canonical invalidation semantics

```text
Node property mutation
    ↓
deferLayoutMutation()
    ↓
insertLayoutQueueById(nodeId)
    ↓
walk parent chain to layout root
    ↓
queue root/overlay id
```

Structural queue insertion already resolves a node to its top-level root before entering the queue. The `insertLayoutQueueById()` path now applies the same root ownership rule.

The queue is therefore a collection of layout-root IDs, not arbitrary child IDs.

## Recovery-critical commits

- `279ad72e6b6a9704fbe502735fcaed83d7295643` — Apply child constraints during Linear measurement.
- `643ed9ea8687c81c54820eaba1af517fa6a75e1a` — Finalize Phase 2 numerical acceptance semantics.
- `68aaa431ec4063023ff801fc508e78f738209c14` — Remove obsolete Node preferred-measure helpers.

The current branch tip is `68aaa431ec4063023ff801fc508e78f738209c14` immediately before this checkpoint commit.

## Deliberately deferred

The following have not been introduced into Phase 2:

```text
flex-grow
flex-shrink
flex-basis
flex-wrap
order
margin
CSS-style absolute edge constraints
Grid track sizing
multi-pass intrinsic track resolution
content-dependent stretch remeasurement
```

## Validation status

Static/source-level audit: **complete**.

Runtime/build/test verification: **not performed yet**.

Do not interpret Phase 2 as runtime-validated until the project build and relevant tests have been executed.

## Next action after context loss

1. Read this checkpoint.
2. Inspect branch `phase2-layout-migration` at the checkpoint tip.
3. Treat Phase 2 source changes as complete unless runtime validation reveals a concrete defect.
4. Perform build/test validation before declaring Phase 2 fully closed.
