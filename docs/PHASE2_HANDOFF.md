# Phase 2 Handoff / Current Context

> **Purpose:** canonical recovery document for Phase 2 after source-level completion.
> **Date:** 2026-08-19
> **Active branch:** `phase2-layout-migration`
> **Current HEAD:** `5c77cb48185bcecf1c21d93dcd53efe49ab86ef0`
> **Phase status:** source-level implementation complete; runtime/build validation intentionally deferred until the end of Phase 6.

## 1. Phase 2 result

Phase 2 moved layout ownership into the framework-owned layout subsystem while preserving the Phase 1 retained-mode runtime and NodeTree ownership model.

Current ownership:

```text
Node / PanelNode
    -> runtime state + structure

LayoutManager
    -> measurement + constraints + container layout + arrangement

NodeTree
    -> lifetime + mutation + scheduling
```

Clients do not implement layout algorithms, call layout invalidation, or participate in the internal measurement/arrangement lifecycle.

## 2. Completed implementation scope

The following Phase 2 areas are complete at source level:

- framework-owned constraint proposal/final-size semantics;
- one-dimensional Linear/Stack layout;
- width-sensitive text measurement integration;
- absolute-child separation from normal Linear flow;
- padding/border box-model conversion;
- alignment and gap handling;
- visibility/layout participation semantics;
- legacy `Node::measure()` / `Node::arrange()` seam removal;
- legacy `MeasureContext` / `ArrangeContext` removal;
- Node input sanitation and min/max normalization;
- framework-owned layout invalidation;
- obsolete preferred-measure helpers `measurePreferredSize()` and `subtractPaddingBorder()` removal;
- final source-level ownership/consistency audit.

## 3. Constraint contract

```text
Fixed size
    -> constrains measurement proposal and final geometry

Max size
    -> may narrow measurement proposal and constrains final geometry

Min size
    -> constrains final geometry; does not automatically narrow intrinsic measurement

Auto
    -> intrinsic measurement / parent allocation determines size
```

For width-dependent content, effective maximum width is applied to the measurement proposal before measurement.

Padding and border translate between border-box and content-box.

## 4. Invalidation contract

The layout queue contains layout-root IDs.

Property mutation follows:

```text
Node mutation
    -> deferLayoutMutation()
    -> insertLayoutQueueById(nodeId)
    -> walk parent chain to top-level root
    -> queue root/overlay ID
```

Structural queue insertion already resolves a node to its top-level root before queue insertion. Both paths therefore share the same root ownership rule.

This closes the Phase 2 invalidation defect where a child mutation could previously queue the child itself instead of the layout root.

## 5. Intentionally deferred scope

The following are not Phase 2 requirements and must not be reintroduced merely to expand the layout model:

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
public LayoutStrategy/custom layout API
client-side layout invalidation
```

Legacy `src/components/*` remains historical reference material and is not part of the Phase 2 architecture.

## 6. Recovery-critical commits

```text
279ad72e6b6a9704fbe502735fcaed83d7295643
    Apply child constraints during Linear measurement

643ed9ea8687c81c54820eaba1af517fa6a75e1a
    Finalize Phase 2 numerical acceptance semantics

68aaa431ec4063023ff801fc508e78f738209c14
    Remove obsolete Node preferred-measure helpers

5c77cb48185bcecf1c21d93dcd53efe49ab86ef0
    Add Phase 2 recovery checkpoint
```

## 7. Validation policy

Phase 2 validation is source-level only by project decision.

```text
Source inspection              DONE
Architectural consistency      DONE
Numerical acceptance analysis  DONE
Ownership/invalidation audit   DONE

Build                          DEFERRED
Compilation                    DEFERRED
Runtime tests                  DEFERRED
```

Build, compilation and runtime tests are intentionally scheduled for the end of Phase 6. Their absence from Phase 2 is not an unfinished Phase 2 task.

## 8. Documentation authority

Use this file together with:

```text
PHASE2_CHECKPOINT.md
PHASE2_CONSTRAINT_SEMANTICS.md
PHASE2_NUMERICAL_LAYOUT_CASES.md
```

Research and planning documents whose status says `research`, `provisional`, `proposed`, or `migration plan` are historical design evidence unless explicitly marked as current. They must not override the current source or this handoff.

`PHASE2_SOURCE_LEVEL_MAPPING.md` and older migration-plan documents describe the path to the implementation, not remaining work.

## 9. Next phase

Phase 2 source-level work is complete. The next implementation phase is **Phase 3 — Input / Events**.

No additional Phase 2 code work is planned unless later source inspection exposes a concrete defect or a dependency required by a later phase.
