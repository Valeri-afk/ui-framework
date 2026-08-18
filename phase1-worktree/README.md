# Phase 1 Working Copies

This directory contains snapshot copies of files that are planned to change during Phase 1 implementation.

## Purpose

The original repository files remain the authoritative baseline until the Phase 1 implementation is finalized.

During implementation, work may be performed against the copies in this directory. The copies are intentionally outside the normal `include/` and `src/` trees, so they are not part of the framework build.

## Planned source copies

- `include/ui_framework/core/nodetree.hpp`
- `include/ui_framework/core/panelnode.hpp`
- `include/ui_framework/core/ui_manager.hpp`
- `src/core/nodetree.cpp`
- `src/core/panelnode.cpp`
- `src/core/ui_manager.cpp`
- `docs/ARCHITECTURE.md`

## Rules

1. The copies start as snapshots of the current `main` branch.
2. No implementation change is considered final merely because it exists in this directory.
3. After the Phase 1 implementation is complete, the working copies will be compared against the original files and reviewed together as one coherent change.
4. Only after that comparison will the final implementation be propagated into the active source tree.
5. Unrelated changes must not be introduced into the working copies.

## Baseline

The copies are based on commit `786f1a6d512e8bec2a0448325088f9300b4bac0e`.
