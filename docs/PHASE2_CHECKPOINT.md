# Phase 2 Recovery Checkpoint

> **Status:** Phase 2 source-level work complete; build/runtime validation intentionally deferred until the end of Phase 6
> **Date:** 2026-08-19
> **Branch:** `phase2-layout-migration`
> **Current recovery point:** `09f0775b11fe79b038c2fa6a18e7072cd97a4f14`

## Current state

Phase 2 layout migration is complete at source level. The branch contains the final layout invalidation fix, legacy preferred-measure cleanup, and the documentation checkpoint needed for recovery after context loss.

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
- Final source-level ownership/consistency audit completed.

## Canonical constraint semantics

```text
Fixed → measurement proposal + final size
Max   → measurement proposal + final size
Min   → final size only
Auto  → intrinsic measurement / parent allocation
```

For width-dependent content, `maxWidth` narrows the measurement proposal before measurement. Minimum constraints do not implicitly become intrinsic measurement proposals.

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

Structural queue insertion already resolves a node to its top-level root before entering the queue. The `insertLayoutQueueById()` path applies the same root ownership rule.

The queue is therefore a collection of layout-root IDs, not arbitrary child IDs.

## Recovery-critical commits

- `279ad72e6b6a9704fbe502735fcaed83d7295643` — Apply child constraints during Linear measurement.
- `643ed9ea8687c81c54820eaba1af517fa6a75e1a` — Finalize Phase 2 numerical acceptance semantics.
- `68aaa431ec4063023ff801fc508e78f738209c14` — Remove obsolete Node preferred-measure helpers.
- `5c77cb48185bcecf1c21d93dcd53efe49ab86ef0` — Add initial Phase 2 recovery checkpoint.
- `09f0775b11fe79b038c2fa6a18e7072cd97a4f14` — Finalize Phase 2 handoff documentation.

## Deliberately deferred

The following are outside Phase 2:

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
public/custom layout strategy API
client-side layout invalidation
```

## Validation status

```text
Source inspection              COMPLETE
Architectural consistency      COMPLETE
Numerical acceptance analysis  COMPLETE
Ownership/invalidation audit   COMPLETE

Build                          DEFERRED TO PHASE 6
Compilation                    DEFERRED TO PHASE 6
Runtime tests                  DEFERRED TO PHASE 6
```

The absence of build/runtime validation is intentional project policy and is not an unfinished Phase 2 task.

## Recovery procedure after context loss

1. Read this checkpoint and `PHASE2_HANDOFF.md`.
2. Inspect branch `phase2-layout-migration` at the current tip.
3. Treat Phase 2 source-level implementation as complete.
4. Do not run build, compilation or runtime tests before Phase 6.
5. Continue with Phase 3 unless a later source audit identifies a concrete Phase 2 defect.
