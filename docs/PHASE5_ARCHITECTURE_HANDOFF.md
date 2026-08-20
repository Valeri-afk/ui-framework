# Phase 5 → Architecture Manual Handoff

This document records the architectural statements in `ARCHITECTURE.md` that became stale or incomplete during Phase 5.

`ARCHITECTURE.md` is intentionally not edited automatically. This handoff is the checklist for the manual architecture review at the Phase 5 → Phase 6 boundary.

## 1. Repository scope / active components

`ARCHITECTURE.md` currently describes:

```text
src/components/
include/ui_framework/components/
```

as excluded legacy component directories.

This is no longer correct.

The `components` directories are now an active framework layer containing the standard Phase 5 component set:

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

The document should distinguish these active components from removed legacy implementations.

## 2. ControlNode

`ARCHITECTURE.md` currently documents `ControlNode` as an existing deferred class and references:

```text
include/ui_framework/core/controlnode.hpp
src/core/controlnode.cpp
```

Those files have been deliberately removed.

The manual revision should state that the legacy `ControlNode` experiment is no longer part of the active source tree and is not an accepted component base.

The design rule that remains valid is:

```text
Node
PanelNode
StackPanelNode
```

with concrete components choosing their base according to responsibility.

## 3. Component System section

The current wording says that a complete component architecture does not exist and presents `components/` as outside active architecture.

This section should be rewritten to reflect that Phase 5 established an active standard component layer while keeping the base runtime hierarchy intentionally small.

The architecture should describe component roles rather than inventing a universal hierarchy.

## 4. Modal

The architecture document already describes the current ModalManager mechanics in significant detail. Phase 5 did not remove that underlying runtime preparation.

However, the manual review should distinguish:

```text
existing Phase 6 preparation
```

from:

```text
final Modal component architecture
```

The old Modal component itself was removed from the active source tree because it depended on the legacy Widget model.

The Phase 6 modality requirements document remains the authoritative design input for the next implementation.

## 5. Scroll

The architecture document currently describes clipping as an existing rendering concern but does not define a final scroll subsystem.

The manual Phase 6 review should add only the scroll responsibilities that are actually accepted:

```text
viewport
content extent
offset/range
coordinate conversion
clipping interaction
input/wheel routing
hit-test interaction
layout integration
nested scrolling policy
```

Do not treat `Overflow::HIDDEN` as equivalent to a complete scroll implementation.

Reference: `SCROLL_ARCHITECTURE.md`.

## 6. Text input

The current architecture correctly lists keyboard events but does not define a text-input/editing subsystem.

Phase 5 established that `TextField / Input` must remain deferred until the framework has a proper text-input contract.

Candidate requirements for Phase 6:

```text
text-input events
composition / IME
caret
selection
editing commands
clipboard
focus/input lifecycle
```

Reference: `PHASE6_SCOPE_CANDIDATES.md`.

## 7. Image / resources

The architecture document currently says that renderer-bound resources remain local to the node/component and that no generic resource manager exists.

This remains a valid constraint for Phase 5.

Phase 6 should not automatically introduce a generic `ResourceManager`. The manual review should first determine the smallest useful resource abstraction required by `Image` or future icon/texture use.

The likely requirement is a resource handle/lifetime abstraction rather than a global dumping-ground manager.

## 8. Primitives

The current architecture describes primitives as low-level rendering support. This remains correct.

The manual review should preserve the explicit boundary:

```text
primitives
    ≠
resource system
    ≠
component system
```

`PRIMITIVES_ROLE.md` is the more detailed current reference.

## 9. Dropdown / overlays

The current architecture already has overlay roots and top-level render/hit-test ordering.

Phase 5 `Dropdown` does not currently require global overlay behavior; it remains a local composite with a child `Menu` using absolute positioning.

Do not assume that the existence of `Dropdown` alone justifies a new overlay subsystem.

Promote overlay infrastructure into Phase 6 only if concrete popup requirements require:

```text
escaping parent clipping
root-level placement
cross-subtree outside-click handling
global popup ordering
collision/placement policy
```

## 10. Animation

Phase 5 component development used animation only as a component-local state/presentation mechanism where existing infrastructure was sufficient.

The architecture should continue to avoid a mandatory global animation manager unless a concrete framework-level requirement emerges.

## 11. Phase 5 architectural conclusion

The manual architecture update should characterize Phase 5 as the establishment of a small standard component layer on top of the existing runtime/layout/input/event/rendering infrastructure.

It should not redefine Phase 5 as a new core subsystem phase.

## 12. Phase 6 handoff

After the manual `ARCHITECTURE.md` review, confirm the Phase 6 scope against:

```text
PHASE6_SCOPE_CANDIDATES.md
PHASE6_MODALITY_REQUIREMENTS.md
SCROLL_ARCHITECTURE.md
PRIMITIVES_ROLE.md
COMPONENT_DESIGN_GUIDE.md
```

Only after this review should Phase 6 implementation begin.
