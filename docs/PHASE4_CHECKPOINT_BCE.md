# Phase 4 Checkpoint — A / B / C / D / E / F

## Status

This checkpoint records the agreed source-level state after reviewing Phase 4 rendering items A, B, C, D, E and F. It is intended to restore context without repeating the Phase 1–3 audit.

Current branch:

```text
phase4-rendering
```

`main` must remain unchanged until Phase 4 is complete.

Compilation, automated tests and runtime validation remain deferred until Phase 6.

---

## A — Geometry → Rendering Boundary

**Source-level architecture accepted; no code change required at this checkpoint.**

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

`position_` is layout input, `desiredSize_` is the measurement result, and `actualPosition_` / `actualSize_` are final arranged geometry consumed by rendering.

`PositionMode::Absolute` remains parent-content-relative and is not screen-space positioning. This is existing Phase 2 semantics and is not changed for rendering convenience.

No transform matrices, coordinate-space wrapper types, or layout redesign are introduced by A.

---

## B — Render Traversal

**Source-level architecture accepted / no implementation expansion required at this checkpoint.**

Top-level priority:

```text
roots        → forward order
overlays     → forward order, excluding top modal
top modal    → last
```

`roots_` and `overlays_` contain the same `Node` type; they are separate top-level priority containers, not separate component classes.

Within a subtree, children render in structural forward order. Hit-testing follows the reverse effective paint order. Update follows `roots → overlays`.

No explicit `zIndex`, global paint priority, or stacking-context abstraction is introduced. Structural order plus top-level priority layers are sufficient for current scope.

---

## C — Renderer State / Clipping

**Source-level complete.**

`RendererStateScope` isolates/restores render target, viewport, scale, clip state, draw color, blend mode and color scale. `NodeTree::drawSubtree()` scopes node subtrees with RAII.

`Overflow::HIDDEN` creates a clip from final node geometry and intersects it with the active parent clip. Zero-area intersections skip the subtree. Renderer state is restored on scope exit.

C does not expand into dirty regions, partial redraw, generic backend abstraction, animation infrastructure or transform-aware clipping without a concrete requirement.

Runtime/compile validation remains deferred to Phase 6.

---

## D — Node State / Geometry / Visual Semantics

**Architecture accepted at source level. No new visual-property implementation is required merely to close D.**

Node properties are classified by impact:

```text
Node property
   ├── Layout / geometry
   ├── Hit-test / interaction
   └── Rendering / visual output
```

A framework-defined property must have one canonical framework interpretation. `virtual Node::hitTest()` is an extension point for genuinely custom geometry, not an invitation to reimplement standard framework semantics differently.

### Property classes

Layout-affecting:

```text
position
size
auto/fixed sizing inputs
padding
borderWidth
```

Interaction + rendering/clipping:

```text
visibility
overflow
```

Visual-only:

```text
opacity
colors / appearance values
```

`opacity` does not change layout or hit-testing by itself. An opacity-zero node may remain geometrically present and interactive; `visible`/`enabled` and other input semantics control interaction separately.

Visual-geometry:

```text
borderRadius
future transform / translation / scale / rotation
```

These do not change the layout box but alter effective visual geometry and therefore belong to the same canonical-geometry problem class:

```text
layout geometry
    ↓
visual geometry
    ├── rendering shape
    ├── hit-test shape
    └── clip shape where applicable
```

`borderRadius` keeps the rectangular layout box but, once defined by the framework, should use canonical rounded geometry for standard hit-testing rather than requiring each component author to duplicate the math.

Parent clipping is framework-owned subtree semantics. A child does not need to know its parent’s clip. Future rounded clipping and transforms must be handled centrally.

Future transform handling should follow the same model as `borderRadius`: transform the effective geometry for rendering, hit-testing and clipping, with inverse transformation on input as appropriate.

Scroll is not treated as a trivial `Overflow::SCROLL` enum. It is a future cross-subsystem viewport/content feature involving content extent, viewport geometry, scroll offset, clipping, hit-test coordinate conversion and input behavior.

---

## E — Render Backend

**Architecture accepted; no backend abstraction expansion required at this checkpoint.**

The framework is intentionally **SDL3-only**.

Current dependency delivery:

```text
ui_framework/
    external/
        SDL3/
        SDL3_image/
        SDL3_ttf/
```

The framework supplies those dependencies. The application owns runtime lifecycle:

```text
Application
    → SDL_Init / SDL_Quit
    → SDL_Window lifetime
    → SDL_Renderer lifetime
    → renderer creation/destruction

UI Framework
    → uses SDL3 APIs
    → receives required SDL handles/context
    → performs rendering orchestration
    → does not own externally-created SDL runtime objects
```

Current SDL-facing hooks remain valid:

```cpp
Node::draw(SDL_Renderer*)
UIManager::runFrame(dt, SDL_Renderer*)
NodeTree::draw(renderer, ...)
```

No second backend, `IRenderBackend`, `BackendFactory`, generic renderer interface or premature `RenderContext` is introduced.

---

## F — Resource Boundary

### Status

**Source-level architecture accepted / F closed for the current framework scope.**

F is defined around ownership and backend representation, not around a generic resource-management subsystem.

### Ownership categories

Borrowed/external resources:

```text
SDL_Renderer*
TTF_Font*
```

The framework uses these but does not own their runtime lifecycle. `TextNode::setFont()` accepts a borrowed `TTF_Font*` and `TextNode` does not destroy it.

Framework-owned renderer-bound cache:

```text
TTF_TextEngine*
TTF_Text*
```

`TextNode` creates and destroys these objects itself. They are bound to the current `SDL_Renderer*` and are recreated when the renderer changes.

Conceptually:

```text
semantic text state
    ↓
borrowed TTF_Font*
    ↓
renderer-bound TTF_TextEngine / TTF_Text
    ↓
SDL3 renderer path
```

### Resource abstraction decision

No generic `ResourceManager`, `TextureManager`, `FontManager`, `AssetManager`, `IResource` hierarchy or public opaque resource-handle system is introduced at this stage.

The useful abstraction is semantic rather than managerial:

```text
semantic resource
    ↓
backend-bound representation
    ↓
SDL3
```

This distinction is already sufficient for current `TextNode` behavior.

### API path differences

The framework does **not** need to support every rendering path exposed by SDL3/SDL3_ttf merely because SDL supports them.

The framework selects a canonical SDL3 renderer path compatible with its backend contract:

```text
TextNode
    ↓
TTF_Text
    ↓
renderer-bound SDL_ttf text engine
    ↓
SDL_Renderer
```

A texture used as the current SDL render target can still be rendered through the same SDL renderer path. Direct-to-window and render-to-texture therefore do not require separate framework text abstractions.

Other SDL_ttf engines (surface/GPU/OpenGL) are not part of the current framework contract simply because they exist; supporting them would be a backend expansion not required by the current SDL3-only architecture.

### Future texture/offscreen resources

A future persistent `SDL_Texture` representation would be a renderer-bound resource whose lifetime and recreation semantics need explicit treatment. It should only be introduced with a concrete requirement such as persistent raster cache, image rendering or offscreen composition.

Likewise, `RenderTarget` would be a distinct semantic concept rather than merely another generic resource.

### Resource/lifecycle principle

```text
Dependency delivery
    ≠
resource ownership

semantic resource
    ≠
backend representation
```

The framework may supply SDL3/SDL_ttf dependencies while application/client code owns externally created assets such as fonts. Framework-owned caches remain internal implementation details.

### Explicitly not implemented by F

- generic resource manager;
- global asset cache;
- opaque font handles;
- persistent text-to-texture cache;
- general texture pipeline;
- offscreen render-target subsystem.

These remain future extensions driven by concrete rendering requirements.

---

## Current Phase 4 boundary after A / B / C / D / E / F

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
   semantic resources
        ↓
backend-bound representation
        ↓
      SDL3
```

Top-level traversal:

```text
roots → overlays → top modal for final presentation
```

Backend/lifecycle:

```text
framework supplies SDL3
application owns SDL runtime lifecycle
```

Resource ownership:

```text
application/external owner
    → SDL runtime + borrowed assets such as TTF_Font*

framework/node
    → renderer-bound caches such as TTF_TextEngine* / TTF_Text*
```

### Next remaining Phase 4 work

- G — Animation Boundary.

Animation system implementation remains out of scope unless a concrete requirement expands Phase 4.

### Resume rules

1. Read this checkpoint together with `PHASE4_START_CONTEXT.md` and `ai_bootstrap.md`.
2. Do not repeat the Phase 1–3 audit.
3. Treat A–C as source-level settled unless a concrete contradiction appears.
4. Treat D as the canonical Node state / geometry semantics checkpoint.
5. Treat E as the accepted SDL3-only backend/dependency/lifecycle boundary unless a new concrete requirement appears.
6. Treat F as the current resource ownership/representation boundary.
7. Continue with G — Animation Boundary.
8. Do not begin Phase 5 implementation.
9. Do not run compilation, tests or runtime validation before Phase 6.
10. Do not modify `main` before Phase 4 completion.
