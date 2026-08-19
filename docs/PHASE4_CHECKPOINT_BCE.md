# Phase 4 Checkpoint — A / B / C / D / E

## Status

This checkpoint records the agreed source-level state after reviewing Phase 4 rendering items A, B, C, D and E. It is intended to restore context without repeating the Phase 1–3 audit.

Current branch:

```text
phase4-rendering
```

`main` must remain unchanged until Phase 4 is complete.

Compilation, automated tests and runtime validation remain deferred until Phase 6.

---

## A — Geometry → Rendering Boundary

### Status

**Source-level architecture accepted; no code change required at this checkpoint.**

Current pipeline:

```text
position_ / requested layout state
        ↓
measurement
        ↓
desiredSize_
        ↓
arrange
        ↓
actualPosition_ / actualSize_
        ↓
rendering
        ↓
toSDLRect()
        ↓
SDL coordinates
```

Terminology:

```text
position_                    = requested/layout input
desiredSize_                 = measurement result
actualPosition_ / actualSize_ = final arranged geometry
```

Rendering consumes final arranged geometry rather than layout intent. Current source uses simple `LayoutPosition`/`LayoutSize` values rather than dedicated coordinate-space wrapper types.

`PositionMode::Absolute` is currently parent-content-relative and primarily means leaving normal linear flow. It is not screen-space positioning and must not be changed for rendering convenience.

No transform matrices, new coordinate-space wrapper types, or layout redesign are introduced by A.

---

## B — Render Traversal

### Status

**Source-level architecture accepted / no implementation expansion required at this checkpoint.**

`NodeTree` uses deterministic top-level processing priorities rather than a general global z-order system.

```text
NodeTree
├── roots_       base top-level priority
├── overlays_    elevated top-level priority
└── top modal    modal presentation / interaction boundary
```

`roots_` and `overlays_` contain the same `Node` type. They are separate containers because priority differs; they are not different component classes.

Rendering:

```text
roots        → forward order
overlays     → forward order, excluding top modal
top modal    → last
```

Within a subtree, parent rendering is followed by children in structural forward order.

Hit-testing is the reverse of effective paint order:

```text
overlays (reverse)
    ↓
roots (reverse)
```

Update traversal follows `roots → overlays`, maintaining deterministic top-level runtime ordering.

No explicit `zIndex`, global paint priority, or stacking-context abstraction is introduced. The current structural order plus top-level priority layers are sufficient for the supported scope.

---

## C — Renderer State / Clipping

### Status

**Source-level complete.**

`RendererStateScope` isolates/restores:

- render target;
- viewport and viewport-set state;
- scale;
- clip rectangle/state;
- draw color;
- blend mode;
- color scale.

`NodeTree::drawSubtree()` scopes each node subtree with this RAII mechanism.

For `Overflow::HIDDEN`:

```text
node final geometry
        ↓
node clip rectangle
        ↓
intersect with active parent clip
        ↓
child subtree inherits resulting clip
```

Zero-area intersections skip that subtree. Renderer state is restored when the subtree returns.

C does not expand into dirty regions, partial redraw scheduling, a draw invalidation queue, generic backend abstractions, animation infrastructure, or transform-aware clipping absent a concrete future requirement.

C is closed at source level; runtime/compile validation stays deferred to Phase 6.

---

## D — Node State / Geometry / Visual Semantics

### Status

**Architecture accepted at source level. No new visual-property implementation is required merely to close D.**

D is not treated as a simple list of visual properties. The important contract is the **impact classification** of Node properties across three subsystems:

```text
Node property
   ├── Layout / geometry
   ├── Hit-test / interaction
   └── Rendering / visual output
```

A property may affect one, two or all three.

### Canonical framework semantics

If the framework defines the geometry semantics of a property, that semantics must have one canonical framework interpretation. A custom Node should not reimplement the same semantics differently merely because `hitTest()` is overridable.

This prevents divergence such as:

```text
framework rendering shape != component hit-test shape
```

Multiple correct behaviors are appropriate only when the framework intentionally supports different geometry semantics (for example, a genuinely custom shape), not when one standard property already has a defined meaning.

### Current / planned property classification

#### Layout-affecting

```text
position
size
auto/fixed sizing inputs
padding
borderWidth
```

These influence layout geometry and therefore can affect final arranged geometry, rendering and hit-testing through that geometry.

`borderWidth` is already part of the current border-box/content-box layout calculations.

#### Interaction + rendering / clipping

```text
visibility
overflow
```

`visibility` affects traversal, rendering and interaction availability.

`Overflow::HIDDEN` defines a subtree clipping/interaction boundary but does not itself change the node's layout dimensions.

#### Visual-only

```text
opacity
colors / appearance values
```

`opacity` is explicitly treated as a rendering-only property:

```text
opacity
    → rendering/composition
    ✕ layout
    ✕ hit-test by itself
```

A node with `opacity == 0` is still geometrically present and may remain an input target. Interaction state is controlled separately through visibility/enabled/focus/capture semantics. A fade animation therefore does not implicitly change hit-testing.

#### Visual-geometry properties

```text
borderRadius
future transform / translation / scale / rotation
```

These are not layout-box changes. They alter effective visual shape/geometry and therefore belong to the same architectural problem class:

```text
layout geometry
    ↓
visual geometry
    ├── rendering shape
    ├── hit-test shape
    └── clip shape where applicable
```

### `borderRadius`

`borderRadius` does not change the layout box. A node may continue to occupy a rectangular `actualSize`, while rendering a rounded visual shape.

Once `borderRadius` is defined as framework semantics, the standard hit-test for that node should use the same canonical rounded geometry rather than requiring a Button/custom component author to duplicate rounded-rectangle mathematics.

The desired principle is:

```text
framework-defined rounded visual shape
        ↓
canonical rounded hit-test shape
```

A rectangular hit-test for a rounded node would not be considered an alternate equally-correct implementation once the framework explicitly defines rounded-shape interaction semantics.

### `Node::hitTest()` extension point

The current `virtual Node::hitTest(float, float)` remains a valid extension point.

Its role is:

```text
framework-defined standard geometry
    → default framework hit-test

 genuinely custom node geometry
    → custom Node override
```

It is not intended to make component authors reimplement standard framework geometry semantics manually.

This allows future custom shapes such as circles, triangles, stars or arbitrary components to define genuinely custom interaction geometry without forcing a full shape hierarchy into the core now.

### `Overflow::HIDDEN` + shape semantics

Parent clipping is a framework-level subtree rule, not a child responsibility.

Conceptually:

```text
parent clip boundary
        ↓
child subtree
        ↓
child own geometry / hit-test shape
```

A child with `borderRadius` must not need to know that its parent has a clipping boundary. The framework owns composition of parent clipping with child traversal.

The current `Overflow::HIDDEN` implementation is rectangular because current clipping is rectangle-based. A future rounded parent clipping boundary would require shape-based clipping semantics; this is the same architectural class of problem as transformed geometry and must be solved centrally rather than separately inside every child.

### Transform

Future transform properties should follow the same architectural treatment as `borderRadius`:

```text
layout geometry
    ↓
visual transform
    ↓
effective geometry
    ├── render
    ├── hit-test
    └── clip
```

For hit-testing this implies inverse transformation from input space into the node's effective/local geometry before testing the canonical shape. Transform must not be introduced until its coordinate and clipping semantics are explicitly defined.

### Scroll

Scroll is **not** treated as merely another value of `Overflow`.

A real scroll capability is a cross-subsystem feature involving at least:

```text
viewport geometry
content extent
scroll offset
clipping
hit-test coordinate conversion
input / wheel / drag behavior
```

The current `VISIBLE/HIDDEN` overflow contract remains unchanged. A future scroll feature should be designed as viewport/content semantics rather than as a trivial enum extension such as `Overflow::SCROLL`.

### Explicitly not implemented by D

D does not require adding `opacity`, `borderRadius`, transform or scroll to the core API immediately. The purpose of D is to establish canonical semantic ownership before implementation.

---

## E — Render Backend

### Status

**Architecture accepted; no backend abstraction expansion required at this checkpoint.**

The framework is intentionally **SDL3-only**.

Current delivery model:

```text
ui_framework/
    external/
        SDL3/
        SDL3_image/
        SDL3_ttf/
```

The framework supplies these dependencies. The application owns runtime lifecycle.

Ownership boundary:

```text
Application
    → SDL_Init / SDL_Quit
    → SDL_Window lifetime
    → SDL_Renderer lifetime
    → renderer creation/destruction

UI Framework
    → uses SDL3 APIs
    → receives needed SDL handles/context
    → performs rendering orchestration
    → does not own externally-created SDL runtime objects
```

Current API direction:

```cpp
Node::draw(SDL_Renderer*)
UIManager::runFrame(dt, SDL_Renderer*)
NodeTree::draw(renderer, ...)
```

No second backend, `IRenderBackend`, `BackendFactory`, generic renderer interface or premature `RenderContext` is introduced.

A future SDL-specific rendering context may be considered only if concrete API pressure appears (for example, a growing bundle of renderer/viewport/target/resource state). It is not required merely to hide SDL3.

The framework's own rendering semantics remain above SDL3:

```text
framework
    → ordering
    → clipping policy
    → state isolation
    → geometry interpretation
    → Node draw contract

SDL3
    → concrete rendering operations
```

---

## Current Phase 4 boundary after A / B / C / D / E

```text
Layout final geometry
        ↓
actualPosition / actualSize
        ↓
Framework visual/geometry semantics
   ┌────┼──────────────┐
   │    │              │
render hit-test     clipping
   │    │              │
   └────┼──────────────┘
        ↓
      SDL3
```

Top-level traversal priority:

```text
roots → overlays → top modal for final presentation
```

Backend/lifecycle:

```text
framework supplies SDL3
application owns SDL runtime lifecycle
```

### Next remaining Phase 4 work

The remaining context items to review are:

- F — Resource boundary;
- G — Animation boundary.

Animation itself is **not** implemented as part of this discussion unless a later concrete requirement expands scope.

### Resume rules

1. Read this checkpoint together with `PHASE4_START_CONTEXT.md` and `ai_bootstrap.md`.
2. Do not repeat the Phase 1–3 audit.
3. Treat A, B and C as source-level settled unless a concrete contradiction appears.
4. Treat D as the canonical Node state / geometry semantics checkpoint described above.
5. Treat E as the accepted SDL3-only backend/dependency/lifecycle boundary unless a new concrete requirement appears.
6. Continue with F and G.
7. Do not begin Phase 5 implementation.
8. Do not run compilation, tests or runtime validation before Phase 6.
9. Do not modify `main` before Phase 4 completion.
