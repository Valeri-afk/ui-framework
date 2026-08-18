# Phase 1 Working Copies — Historical Snapshot

This directory contains the snapshot copies that were used during Phase 1 implementation.

## Purpose

Phase 1 has been accepted into the active `main` source tree. The files in this directory are retained as a historical/audit snapshot of the staged implementation and are **not** part of the framework build.

The active `include/`, `src/` and `docs/` trees are now authoritative for the current framework state.

## Snapshot contents

- `include/ui_framework/core/nodetree.hpp`
- `include/ui_framework/core/panelnode.hpp`
- `include/ui_framework/core/ui_manager.hpp`
- `src/core/nodetree.cpp`
- `src/core/panelnode.cpp`
- `src/core/ui_manager.cpp`
- `docs/ARCHITECTURE.md`

## Historical status

1. The copies started as snapshots of the pre-Phase-1 `main` branch.
2. Phase 1 implementation was developed and reviewed in this directory.
3. The ownership, lifecycle, traversal and mutation portions of Phase 1 were promoted into the active source tree.
4. The snapshot remains only for historical comparison and audit purposes.
5. Future architectural work must be made against the active source tree, not against these working copies.

## Baseline

The original copies were based on commit `786f1a6d512e8bec2a0448325088f9300b4bac0e`.

## Important note

The snapshot may contain experimental or later changes that are outside the final active contract, including an earlier recursive hit-testing implementation. Those changes must not be treated as current behavior unless they are explicitly promoted into `main` in a later phase.
