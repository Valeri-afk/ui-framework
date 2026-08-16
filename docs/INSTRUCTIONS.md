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

## 3. Initial Analysis Output

The initial repository analysis must establish a high-level understanding
of the current codebase before implementation work begins.

The readiness report should include:

- repository structure;
- existing major modules;
- responsibility of each major module;
- major dependency relationships;
- ownership and lifecycle model;
- current development phase;
- implemented parts of the current phase;
- known incomplete areas;
- relevant architectural risks.

The report must distinguish between:

- facts confirmed by source code;
- information described by documentation;
- planned or hypothetical architecture.

Do not treat planned architecture as existing implementation.

## 4. Phase-Based Development

The framework is developed in independent phases.

Do not mix phases unless a dependency makes it necessary.

The current phase is defined by ROADMAP.md.

When the user specifies a phase:
- use ROADMAP.md to determine its scope;
- use ARCHITECTURE.md to understand its dependencies;
- inspect the actual source code before proposing changes.

## 5. Before Architectural Changes

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

## 6. Refactoring Rules

- No giant refactors.
- Modify one fundamental architectural layer at a time.
- Do not introduce abstractions prematurely.
- Do not change unrelated modules.
- Preserve existing public API unless there is a concrete reason to change it.
- Do not solve future problems prematurely.

## 7. Implementation Safety

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

## 8. Testing / Verification

For significant changes consider:

- normal case;
- nested subtree;
- mutation during callback;
- deletion of current node;
- reparenting;
- detached node;
- interaction with neighboring modules.

## 9. Documentation

Update documentation only when the actual architecture changes.

Do not document hypothetical architecture as existing architecture.

ROADMAP describes future development.

ARCHITECTURE describes the current system.
