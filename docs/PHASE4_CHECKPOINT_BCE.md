# Phase 4 Checkpoint — A / B / C / E

## Status

This checkpoint records the agreed source-level state after reviewing Phase 4 rendering items A, B, C and E. It is intended to restore context without repeating the Phase 1–3 audit.

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

The current pipeline is:

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

Current terminology:

```text
position_
    = requested/layout input

desiredSize_
    = measurement result

actualPosition_
actualSize
    = final arranged geometry
```

Rendering consumes `actualPosition` / `actualSize` rather than layout intent. `toSDLRect()` converts the final geometry into SDL integer rectangle bounds.

The current source does not define dedicated coordinate-space types. `LayoutPosition` and `LayoutSize` are simple geometric values. The current rendering contract therefore treats final arranged geometry as directly usable in the renderer/viewport coordinate system.

This is sufficient for the current Phase 4 scope.

### Important non-change

`PositionMode::Absolute` currently means separation from normal one-dimensional flow and positions the child from the parent content position. It is **not** screen-space positioning. This is existing Phase 2 semantics and must not be changed for rendering convenience.

### Not introduced

- transform matrices;
- new coordinate-space wrapper types;
- screen-space reinterpretation of `Absolute`;
- layout redesign.

---

## B — Render Traversal

### Status

**Source-level architecture accepted / no implementation expansion required at this checkpoint.**

`NodeTree` has deterministic top-level processing priorities rather than a global z-order system.

Conceptual model:

```text
NodeTree
├── roots_
│   └── base top-level priority
│
├── overlays_
│   └── elevated top-level priority
│
└── top modal
    └── modal presentation / interaction boundary
```

`roots_` and `overlays_` contain the same `Node` type. They are separate top-level containers because their processing priority differs; they are not separate component classes.

Rendering:

```text
roots        → forward order
overlays     → forward order, excluding top modal
top modal    → last
```

Within a subtree:

```text
parent draw
    ↓
children in structural forward order
```

Later paint order means visually above earlier paint order.

Hit-testing is the reverse of effective paint order:

```text
overlays (reverse)
    ↓
roots (reverse)
```

Update traversal follows the same top-level priority `roots → overlays`, keeping the runtime ordering deterministic across stages where ordering matters.

### Z-order decision

No explicit `zIndex`, global paint priority, or stacking-context abstraction is introduced.

The structural order plus explicit top-level priority layers are sufficient for the current supported application class. A general z-order system would require additional stacking-context semantics and interaction rules that are not justified by a concrete requirement.

`roots_` / `overlays_` names are retained. Renaming is not a current architectural requirement.

---

## C — Renderer State / Clipping

### Status

**Source-level complete.**

`RendererStateScope` provides renderer-state isolation for:

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

Zero-area intersections skip that subtree. Renderer state is restored when the subtree returns, preventing state leakage across siblings or nested subtrees.

C is closed at source level. Runtime and compilation validation remain deferred to Phase 6.

### Explicitly excluded

- dirty-region rendering;
- partial redraw scheduling;
- new render invalidation queue;
- generic backend abstraction;
- animation infrastructure;
- transform-aware clipping beyond a future concrete requirement.

---

## E — Render Backend

### Status

**Architecture accepted; no backend abstraction expansion required at this checkpoint.**

The framework is intentionally **SDL3-only**.

The backend choice is not an open question. The remaining questions concern dependency delivery, runtime ownership and the exact SDL-facing framework contract.

### Dependency delivery

Current project model:

```text
ui_framework/
    external/
        SDL3/
        SDL3_image/
        SDL3_ttf/
```

The framework supplies these dependencies. The local CMake target model uses them through `find_package()` and links them through their CMake targets.

Conceptually:

```text
SDL3 target
    ↓
ui_framework target
    ↓
application target
```

This is dependency delivery/packaging, not a second runtime backend.

### Runtime ownership

The accepted ownership boundary is:

```text
Application
    → owns SDL lifecycle
      SDL_Init / SDL_Quit
      SDL_Window lifetime
      SDL_Renderer lifetime
      renderer creation/destruction

UI Framework
    → uses SDL3 APIs
      receives required SDL handles/context
      performs rendering orchestration
      does not own externally-created SDL runtime objects
```

The framework therefore supplies SDL3 availability but does not become the owner of the application window/renderer runtime.

This supports the intended single-SDL-project model and allows the application to use the same `SDL_Renderer*` for game and UI rendering.

### Current API direction

`Node::draw(SDL_Renderer*)` remains valid.

`UIManager::runFrame(dt, SDL_Renderer*)` / `NodeTree::draw(renderer, ...)` remain honest expressions of the current SDL-first architecture.

The renderer does not need to be stored in every Node/component. It may be supplied for the duration of the current rendering operation.

### RenderContext decision

No `RenderContext` is introduced merely to solve dependency delivery or to create an abstraction by symmetry.

A future SDL-specific context could become useful if the framework needs to pass a stable bundle of rendering state such as renderer, viewport, render target or other concrete backend data. That is a future API evolution question, not a current requirement.

It is also not required to hide the fact that the framework is SDL3-only.

### Resource/backend abstractions not introduced

- second backend;
- `IRenderBackend`;
- `BackendFactory`;
- generic renderer interface;
- `RenderContext` as premature abstraction;
- `ResourceManager`;
- `TextureManager`;
- `FontManager`.

The framework's own renderer policies (ordering, clipping, state isolation, geometry handling) remain framework semantics implemented on top of SDL3.

---

## Current Phase 4 boundary after A / B / C / E

```text
Application
    │
    │ owns SDL lifecycle
    ▼
  SDL3
    │
    │ SDL_Renderer*
    ▼
UI Framework
    │
    ├── geometry → rendering boundary
    ├── render traversal
    ├── clipping
    ├── renderer-state isolation
    └── SDL3 backend usage
```

Top-level processing priority:

```text
roots
  ↓
overlays
  ↓
top modal for final presentation
```

Current rendering geometry:

```text
layout final geometry
    ↓
actualPosition / actualSize
    ↓
render coordinates
```

### Next remaining Phase 4 work

The remaining context items to review are:

- D — Visual State;
- F — Resource boundary;
- G — Animation boundary.

Animation itself is **not** to be implemented as part of this discussion unless a later concrete requirement explicitly expands scope.

### Resume rules

1. Read this checkpoint together with `PHASE4_START_CONTEXT.md` and `ai_bootstrap.md`.
2. Do not repeat the Phase 1–3 audit.
3. Treat A, B and C as source-level settled unless a concrete contradiction appears.
4. Treat E as the accepted SDL3-only backend/dependency/lifecycle boundary unless a new concrete requirement appears.
5. Continue with D, F and G.
6. Do not begin Phase 5 implementation.
7. Do not run compilation, tests or runtime validation before Phase 6.
8. Do not modify `main` before Phase 4 completion.
