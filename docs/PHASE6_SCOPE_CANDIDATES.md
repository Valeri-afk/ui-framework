# Phase 6 — Scope Candidates from Phase 5

This document is the handoff from component development to framework-subsystem development. It records requirements discovered while designing the Phase 5 standard component layer.

It is a candidate scope, not the final Phase 6 architecture.

## 1. Modality

### Evidence

`Modal` could not be finalized without framework-level control over active modal roots and input routing.

### Candidate responsibilities

```text
active modal registration
modal stack/order
exclusive hit-testing
input routing restrictions
focus/capture policy
Escape routing
background interaction blocking
focus restoration after close
optional scroll-lock policy
modal/overlay rendering order
```

The final Modal component should remain a semantic/presentation component. It should not implement these low-level rules itself.

Reference: `PHASE6_MODALITY_REQUIREMENTS.md`.

## 2. Scrolling

### Evidence

`Scroll / ScrollArea` crosses component and framework boundaries.

### Candidate responsibilities

```text
viewport bounds
content extent
scroll offset/range
coordinate conversion
clipping
wheel/gesture/drag input routing
hit-test through transformed/clipped content
layout integration
nested scrolling policy
scrollbar interaction/presentation hooks
```

The final `Scroll` component API should be derived from this infrastructure rather than defining it prematurely.

Reference: `SCROLL_ARCHITECTURE.md`.

## 3. Text input / editing

### Evidence

`TextField / Input` cannot be implemented correctly from the current `KeyDown/KeyUp` event path alone.

### Candidate responsibilities

```text
text input events
composition / IME
caret position
selection range
editing commands
clipboard interaction
keyboard navigation inside text
repeat/backspace/delete behavior
focus integration
text input lifecycle
```

A future `TextField` should sit on top of this infrastructure and own field semantics/presentation, not platform text-input routing.

## 4. Image / resource management

### Evidence

`Image` requires a stable texture/resource ownership model. The current framework intentionally has rendering primitives and renderer-state helpers but no generic texture/resource API.

### Candidate responsibilities

```text
resource ownership/lifetime
shared texture references
texture loading/import boundary
safe renderer/resource relationship
TextureHandle or equivalent resource abstraction
source rectangle
fit/crop/scale modes
opacity/tint/flip/rotation
image presentation lifecycle
```

The exact resource architecture must remain separate from `primitives`.

Reference: `PRIMITIVES_ROLE.md`.

## 5. Overlay / popup infrastructure

### Evidence

The current `Dropdown` implementation is deliberately local: it owns its `Menu` child and uses existing absolute positioning. This is sufficient only while the menu can remain inside its parent tree/clip context.

### Candidate trigger

Introduce a framework overlay subsystem only when a concrete requirement appears for:

```text
escaping parent clipping
root-level popup placement
global popup ordering
outside-click dismissal across unrelated subtrees
popup collision/placement policy
```

Do not add overlay infrastructure merely because `Dropdown` exists.

This candidate may become part of modality work if the final interaction model proves they are the same framework concern.

## 6. Validation and stabilization

Phase 6 should also absorb the technical validation intentionally deferred from earlier phases:

```text
compile validation
automated component tests
runtime interaction tests
NodeTree/input/layout/render integration tests
full accumulated build validation
```

Validation is a phase responsibility, not a component-specific concern.

## 7. Explicit non-goals unless new evidence appears

```text
full theme system
generic animation manager
generic resource manager as a dumping ground
universal content model
CSS/Flexbox compatibility
large widget catalog
application-specific chess components
```

## 8. Final scope decision rule

Before Phase 6 implementation begins, review these candidates against the large architecture document and current source. Promote only the responsibilities that have a concrete framework-level justification.

A candidate should enter Phase 6 only when:

```text
Phase 5 component requirements
        +
framework-level responsibility
        +
concrete reusable contract
        +
clear ownership boundary
```

The purpose of Phase 6 is to build the missing infrastructure, not to retroactively absorb every deferred component into the framework.
