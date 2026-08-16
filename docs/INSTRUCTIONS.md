# Framework Development Instructions

## 1. Source of Truth

Current source code is the authoritative source of truth.

Documentation describes the intended understanding of the codebase,
but must never override actual implementation.

If documentation conflicts with source code:
source code wins.

## 2. Initial Repository Analysis

Before modifying code:

1. Inspect the repository structure.
2. Read the relevant documentation.
3. Analyze the current source code.
4. Reconstruct the current architecture.
5. Identify module responsibilities and dependencies.
6. Identify the current development phase.
7. Identify known limitations and unfinished areas.

Do not modify code during this initial analysis.

After analysis, provide a concise readiness report.

## 3. Phase-Based Development

The framework is developed in independent phases.

Do not mix phases unless a dependency makes it necessary.

The current phase is defined by ROADMAP.md.

When the user specifies a phase:
- use ROADMAP.md to determine its scope;
- use ARCHITECTURE.md to understand its dependencies;
- inspect the actual source code before proposing changes.

## 4. Before Architectural Changes

For every significant change determine:

- current behavior;
- current responsibility;
- current dependencies;
- invariants;
- public contract;
- affected modules;
- unaffected modules;
- target behavior;
- minimal required change.

## 5. Refactoring Rules

- No giant refactors.
- Modify one fundamental architectural layer at a time.
- Do not introduce abstractions prematurely.
- Do not change unrelated modules.
- Preserve existing public API unless there is a concrete reason to change it.
- Do not solve future problems prematurely.

## 6. Implementation Safety

Preserve:

- ownership;
- lifetime;
- NodeId/live-node invariants;
- mutation safety;
- traversal correctness;
- detached node lifecycle;
- event behavior;
- layout invalidation;
- rendering behavior.

## 7. Testing / Verification

For significant changes consider:

- normal case;
- nested subtree;
- mutation during callback;
- deletion of current node;
- reparenting;
- detached node;
- interaction with neighboring modules.

## 8. Documentation

Update documentation only when the actual architecture changes.

Do not document hypothetical architecture as existing architecture.

ROADMAP describes future development.

ARCHITECTURE describes the current system.
