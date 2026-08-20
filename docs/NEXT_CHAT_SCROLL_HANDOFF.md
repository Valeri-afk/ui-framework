# Handoff — Continue Current Framework Development

This document is a continuation checkpoint for a new chat. It is not a replacement for `ARCHITECTURE.md` and should not be used to rewrite the architecture document.

Repository:

```text
Valeri-afk/ui-framework
```

Working branch:

```text
phase5-components
```

## 1. Development context

Phase 5 was intentionally scoped as a **component-development phase** rather than an attempt to create a huge generic UI toolkit.

The framework is being developed independently of the chess application. The chess application is only used as a reference for determining the minimum reusable framework capabilities that could theoretically be required.

The target philosophy is:

```text
minimal reusable framework
        ↓
only infrastructure that has a concrete reusable responsibility
        ↓
application-specific composition stays in the client application
```

Do not expand the framework merely because a feature exists in another UI toolkit.

## 2. Phase 5 component scope

The component set discussed/implemented during Phase 5 was intentionally limited to basic standard UI components that can plausibly support the target applications.

The important principle is that framework components are ordinary reusable UI building blocks, while application-specific widgets/compositions remain client-side.

The wider candidate list discussed at the end of Phase 5 was:

```text
Scroll
Modal
Checkbox
RadioButton
Input / TextField
Slider
Dropdown
Image
Paper
Label
Card
```

The current conclusions were:

```text
Checkbox / RadioButton / Slider / Dropdown
    → standard component territory

Modal
    → do NOT create a standalone Modal component yet
    → modality should primarily be a framework service

Scroll
    → framework-level behavior/infrastructure first
    → standalone Scroll/ScrollArea component only after the infrastructure contract is stable

Label
    → text primitives/text nodes are sufficient; no need for a heavy Label subsystem

Card / Paper
    → composition/style patterns, not mandatory framework infrastructure

Image
    → defer until minimal resource/texture ownership infrastructure exists

Input / TextField
    → defer until framework text-input/editing infrastructure exists
```

## 3. Modality status

Modality was developed as a framework service through `ModalManager` rather than as a public `Modal` component.

The intended service responsibilities are:

```text
modal stack/order
focus restriction
pointer interaction restriction
Escape routing
outside-click interception
backdrop interaction policy
backdrop visual layer
backdrop fade
backdrop lifecycle
nested modal focus restoration
deferred mutation safety
direct modal invalidation cleanup
```

Outside-click policy:

```text
BackdropClickBehavior::Consume
BackdropClickBehavior::Close
```

Backdrop is an **internal framework overlay node**, not a public UI component.

Current architectural rule:

> Do not add a standalone `Modal` component unless later evidence shows that the service-level modality API is insufficient.

## 4. Viewport model — important current decision

The framework now treats the SDL renderer's **logical presentation size** as the framework UI coordinate space.

Example client-side configuration:

```cpp
SDL_SetRenderLogicalPresentation(
    renderer,
    1280,
    720,
    SDL_LOGICAL_PRESENTATION_LETTERBOX
);
```

Framework meaning:

```text
Framework viewport
    = current SDL logical presentation size
    = logical UI coordinate space
```

The physical monitor size is not the UI coordinate system.

The physical window size and physical render-output size may be different from the framework UI space, especially on high-DPI displays or when using logical presentation.

The framework should obtain the current logical presentation through SDL itself rather than requiring the client to call a `UIManager::setViewportSize()` method every frame.

Fallback rule:

```text
if logical presentation is configured:
    use logical width/height
else:
    use current render-output size as compatibility fallback
```

The important source-level implementation already observed in the current code is in `LayoutManager::syncViewportFromRenderer()`.

`UIManager::setViewportSize()` / `UIManager::getViewportSize()` were intentionally removed from the public `UIManager` API because they create a second source of truth.

`LayoutManager` remains the internal owner of the current viewport size and provides it to framework subsystems.

Do not reintroduce a client-driven viewport setter unless a new concrete requirement proves that necessary.

## 5. Scroll architecture already agreed upon

Scroll is **framework-level behavior**, not primarily a visual widget.

The scroll behavior should be based on a separate scroll offset/state rather than rewriting child layout positions.

Conceptually:

```text
Node logical/layout position
        ↓
scroll transform
        ↓
render/hit-test coordinates
```

Do NOT implement scrolling by repeatedly mutating every child's original `position` on wheel input.

The logical/layout coordinates must remain stable.

Core scroll state:

```text
ScrollState
├── viewport size
├── content extent
├── scroll offset
└── max scroll offset
```

Expected range:

```text
maxOffsetX = max(0, content.width  - viewport.width)
maxOffsetY = max(0, content.height - viewport.height)
```

`scrollOffset` must be clamped to `[0, maxOffset]`.

For nested scroll containers, accumulated scroll is expected to be the sum of relevant ancestor offsets.

## 6. Border/padding rule for Scroll

Border and padding matter for computing the effective scroll viewport.

Do not invent a new border-box model specifically for Scroll.

Use the framework's existing layout model:

```text
border box
    ↓ remove border
padding box / content viewport relationship
    ↓
actual scroll viewport
```

The scroll viewport should represent the usable inner area of the scroll container, not blindly the full outer border box.

The existing `LayoutManager` already has helpers for converting between content size and border-box size and already sanitizes padding/border values.

Use those existing layout definitions instead of duplicating them inside Scroll.

## 7. Existing NodeTree/input behavior that Scroll must integrate with

Current `Node` already has:

```text
Overflow::VISIBLE
Overflow::HIDDEN
```

`Node` also has stable logical/layout positions and separate `actualPosition_` / `actualSize_` state.

Current `NodeTree::drawSubtree()` already applies clipping when a node has:

```cpp
Overflow::HIDDEN
```

via SDL render clip state.

Current `NodeTree::hitTestSubtree()` already checks `Overflow::HIDDEN` against the node's actual rectangle.

This is important:

> Scroll clipping should build on the existing `Overflow::HIDDEN` mechanism rather than inventing a second unrelated clipping system.

Current input wheel processing exists in `InputManager` as `MouseWheelEvent` and already performs hit testing before dispatch.

The relevant flow is approximately:

```text
SDL_EVENT_MOUSE_WHEEL
        ↓
InputManager
        ↓
NodeTree::hitTest()
        ↓
handleMouseWheelEvent()
```

Scroll should integrate at this level.

## 8. ScrollManager work already attempted

A framework-level `ScrollManager` was designed with the following responsibilities:

```text
registerScrollNode()
unregisterScrollNode()
isRegistered()
setViewportSize()
setContentSize()
setOffset()
scrollBy()
getState()
getOffset()
getMaxOffset()
getAccumulatedOffset()
handleWheel()
sync()
clear()
```

The important design idea is correct:

```text
ScrollManager owns scroll state.
NodeTree owns generic node hierarchy/layout/render/hit-test.
UIManager coordinates framework services.
```

However, because of previous branch/SHA mistakes during development, **the actual presence and exact revision of `scrollmanager.hpp` / `scrollmanager.cpp` on `phase5-components` must be verified before continuing**.

Do not assume that a previous chat's `create_file` operation succeeded just because the assistant reported it.

First verify:

```text
phase5-components/src/core/scrollmanager.hpp
phase5-components/src/core/scrollmanager.cpp
```

and inspect their exact contents and SHA.

There was a real incident where a ScrollManager file was created on the default branch because the `branch` parameter was omitted. Do not repeat this.

## 9. What the next chat must do first

Before changing code:

```text
1. Verify current branch = phase5-components
2. Verify scrollmanager.hpp exists on phase5-components
3. Verify scrollmanager.cpp exists on phase5-components
4. Inspect current UIManager / InputManager / NodeTree / LayoutManager
5. Check current working-tree state via GitHub, not an old response snapshot
```

Do not overwrite a file using a stale blob SHA.

When using `update_file`, first fetch the current file and use its current SHA.

## 10. Immediate Scroll implementation sequence

Continue Scroll in this order.

### Step A — Scroll state

Make sure the branch contains the final `ScrollManager` state implementation.

Required behavior:

```text
viewport/content relationship
offset
maxOffset
clamping
nested ancestor offset accumulation
```

No scrollbar visuals yet.

### Step B — Wheel routing

Integrate `ScrollManager` with `UIManager` / `InputManager` so that:

```text
MouseWheel
    ↓
hit-test target
    ↓
find nearest registered scroll ancestor
    ↓
apply scroll delta
    ↓
clamp
```

If there is no scrollable ancestor, the wheel event continues through the normal event system.

Do not swallow every wheel event globally.

For nested scrolling, start with the nearest scroll ancestor. More advanced propagation between nested containers should only be added if the existing event semantics require it.

### Step C — Coordinate transform

Add generic framework support for applying accumulated scroll offsets when calculating render/hit-test coordinates.

Desired behavior:

```text
logical layout coordinate
        - accumulated scroll offset
        = effective coordinate
```

Do not rewrite the stored layout position.

A nested hierarchy should behave conceptually like:

```text
Scroll A
   offset A
     ↓
   Scroll B
      offset B
        ↓
      Content
```

Effective content transform:

```text
-A.offset - B.offset
```

### Step D — Clipping

Use the existing `Overflow::HIDDEN` render clipping mechanism as the base.

The scroll container's viewport should clip descendants to its usable inner area.

This may require a small extension to render/hit-test traversal, but it should not create a second unrelated clipping subsystem.

### Step E — Hit testing

Hit testing must operate in inverse-transformed content coordinates while still respecting the scroll viewport clip.

Conceptually:

```text
pointer / window logical coords
        ↓
viewport containment
        ↓
add scroll offset(s)
        ↓
content hit-test
```

### Step F — Layout/content extent

Once the transform works, determine how `ScrollManager::content` is populated from the layout system.

Do not ask the client to manually provide content height for every ordinary scroll container if the framework can derive it from measured/arranged descendants.

If a generic framework mechanism cannot derive it safely yet, document the limitation instead of inventing application-specific measurement logic.

### Step G — Scrollbar visuals

Only after behavior is stable, decide whether framework-level scrollbar presentation is necessary.

Do not create a full scrollbar component automatically.

A scroll container can work without a visible scrollbar.

## 11. What NOT to do

Do not:

```text
create a giant generic ScrollArea widget immediately
add scrollbar visuals before scroll behavior works
rewrite child layout positions every frame
store scroll state in arbitrary Node properties
make client manually push viewport size every frame
make NodeTree depend semantically on ScrollManager
create a second clipping system unrelated to Overflow::HIDDEN
add touch/gesture physics before basic wheel scrolling works
```

## 12. Current phase boundary

The current project is still in **core framework development**.

Do not switch to application-specific chess widgets.

The goal of the current Scroll work is to create only the reusable framework contract needed by future applications.

After Scroll, the other deferred core areas remain:

```text
text input / editing infrastructure
image/resource ownership infrastructure
final core integration review
```

Only after those are sufficiently complete should the project move into full validation/stabilization:

```text
full build
runtime smoke tests
input/render/layout integration tests
modal interaction tests
scroll interaction tests
lifetime/memory checks
source/include consistency checks
```

## 13. Important existing documents

Use these as context, but do not rewrite `ARCHITECTURE.md` automatically:

```text
PHASE6_CORE_STATUS_CHECKPOINT.md
SCROLL_ARCHITECTURE.md
PRIMITIVES_ROLE.md
COMPONENT_DESIGN_GUIDE.md
PHASE6_SCOPE_CANDIDATES.md
PHASE6_MODALITY_REQUIREMENTS.md
```

`PHASE6_CORE_STATUS_CHECKPOINT.md` records the broader framework status.

`SCROLL_ARCHITECTURE.md` contains the earlier scroll-specific architectural constraints and should be updated only if the new implementation produces a real architectural decision worth recording.

## 14. Final target for the next continuation

The next chat should aim to reach this concrete state:

```text
ScrollManager state                 ✓
Wheel routing                       → next
Coordinate transform               → next
Render clipping integration        → next
Hit-test integration                → next
Automatic content extent           → next
Scrollbar presentation             → optional/later
Standalone Scroll component        → only after infrastructure proves useful
```

The key principle is:

> Build Scroll as reusable framework behavior first. Let the eventual public Scroll component emerge from the infrastructure instead of using a component API to dictate the infrastructure.
