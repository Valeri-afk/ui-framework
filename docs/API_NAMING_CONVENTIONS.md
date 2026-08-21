# API Naming Conventions

This document defines the naming vocabulary for the framework API refactor. It is a naming/API contract only; it does not change framework behavior.

## Public API vocabulary

### Accessors

- `getX()` returns a value or object.
- `isX()` returns a boolean state.
- `hasX()` reports presence of an object/state.

### State mutation

- `setX(...)` changes a property/state value.
- `enableX(...)` enables a framework behavior.
- `disableX(...)` disables a framework behavior.

### Structural operations

- `addX(...)` adds an element to a public collection/hierarchy.
- `removeX(...)` removes an element from a public collection/hierarchy.
- `attachX(...)` / `detachX(...)` describe internal ownership/tree attachment semantics and should not be exposed unless that distinction is part of the public contract.

### Semantic actions

- `showX(...)` activates presentation/state that is conceptually shown.
- `closeX(...)` closes an active presentation/state.
- `activate()` performs a component's semantic activation action.

### Runtime operations

- `processX(...)` handles an input/runtime operation.
- `dispatchX(...)` delivers an event to its destination.
- `routeX(...)` selects/routes an event or input operation.
- `resolveX(...)` determines a target/result from runtime state.
- `updateX(...)` updates derived runtime state.
- `calculateX(...)` calculates a value.
- `measureX(...)` measures layout/content requirements.
- `arrangeX(...)` applies final layout placement.
- `applyX(...)` applies an already determined change.
- `reconcileX(...)` reconciles derived/runtime state with authoritative state.

Avoid generic names such as `handleX`, `sync`, `manage`, `doX`, or `data` when a more specific operation can be named.

## Public/private boundary

Public names describe framework semantics from the client's point of view. Private names may describe implementation mechanics.

Public framework concepts currently include:

- `UIManager`
- `Node`
- `ContainerNode` / `StackPanelNode` (final class naming remains subject to API review)
- framework components
- rendering primitives
- public event and value types

Internal systems are implementation details:

- `NodeTree`
- `InputSystem`
- `LayoutSystem`
- `ModalSystem`
- `ScrollSystem`
- event dispatch/storage machinery
- rendering state and primitive renderer internals
- mutation queues and coordinate-transform machinery

`UIManager` remains the public runtime facade. The internal systems should not become client dependencies merely because they implement a framework service.

## Naming consistency

Multi-word filenames use `snake_case`:

- `ui_manager.hpp`
- `node_tree.hpp`
- `input_system.hpp`
- `layout_system.hpp`
- `modal_system.hpp`
- `scroll_system.hpp`
- `event_dispatcher.hpp`
- `event_handler_storage.hpp`

Public accessors should consistently use `getX()` / `isX()` rather than mixing accessor styles such as `id()` and `parent()` with `getSize()`.

## SDL types

SDL3/SDL_ttf types are allowed in the public API where they are genuinely part of the framework/platform contract. The framework does not introduce wrapper types merely to hide `SDL_Renderer*`, `SDL_Texture*`, or `TTF_Font*`.

The distinction is between the external SDL contract and framework implementation types: SDL types may be public; `NodeTree`, layout/input/modal/scroll internals, rendering state, and event storage remain private.

## Rendering primitives

Rendering primitives are a deliberate public API layer. Primitive drawing should be convenient for client-specific rendering without exposing the framework's internal renderer state, clipping stack, batching, or other implementation machinery.

`TextPrimitive` and other primitive types therefore remain candidates for the public primitive API; their internal renderer/state dependencies remain private.
