# Phase 5 → Architecture Manual Handoff

This document is a temporary checklist for the manual reconciliation of `ARCHITECTURE.md` after Phase 5.

`ARCHITECTURE.md` is intentionally not edited automatically. Phase 6 implementation has already started; this file only records the remaining manual architecture review points.

## Manual review points

### Active components

Ensure `ARCHITECTURE.md` distinguishes the active `components/` layer from removed legacy implementations.

Active standard components include:

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
Checkbox
RadioButton
Slider
Dropdown
```

`components/` is **not deprecated**.

### ControlNode

Remove/replace any current-architecture wording that presents `ControlNode` as an active base class.

The legacy `ControlNode` experiment is removed and is not an accepted universal component base.

### Component architecture

The architecture should describe the active component layer as concrete responsibilities built on the small runtime hierarchy rather than introducing a universal Control/Interactive/Composite hierarchy.

### Modal

Clarify the distinction between:

```text
ModalManager
    active framework-level modality infrastructure

legacy Modal component
    removed/deprecated

public Modal component
    deferred unless service-level modality later proves insufficient
```

### Scroll

Add only the responsibilities that are now actually accepted by the implementation:

```text
viewport/content extent
offset/range/clamping
coordinate transform
clipping interaction
wheel routing
hit-test interaction
layout-derived content extent
nested scrolling policy
```

Reference: `SCROLL_ARCHITECTURE.md`.

### Text input

Keep `TextField / Input` deferred until the framework has a proper text-input/editing contract covering the requirements that actually prove necessary.

### Image / resources

Do not introduce a generic `ResourceManager` merely because `Image` is a candidate. First determine the smallest resource/texture ownership abstraction required by the implementation.

### Primitives

Keep the boundary:

```text
primitives ≠ resource system ≠ component system
```

Reference: `PRIMITIVES_ROLE.md`.

### Dropdown / overlays

Do not promote `Dropdown` into a global overlay subsystem unless concrete requirements emerge around clipping escape, root-level placement, popup ordering, collision handling, or outside-click behavior.

### Animation

Do not introduce a mandatory global animation manager without a concrete framework-level requirement.

## Final Phase 6 references

After manually reviewing `ARCHITECTURE.md`, use these living documents as the current Phase 6 references:

```text
ROADMAP.md
FRAMEWORK_SCOPE.md
PHASE6_CORE_STATUS_CHECKPOINT.md
PHASE6_SCOPE_CANDIDATES.md
PHASE6_MODALITY_REQUIREMENTS.md
SCROLL_ARCHITECTURE.md
PRIMITIVES_ROLE.md
COMPONENT_DESIGN_GUIDE.md
```

Once the manual architecture review is complete, this handoff can itself be archived or removed. It should not become a second architecture source of truth.
