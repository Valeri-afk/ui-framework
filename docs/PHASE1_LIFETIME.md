# Phase 1 — Framework Lifetime and Shutdown Contract

> **Status:** architecture decision
> **Date:** 2026-08-18
>
> This document records the lifetime model selected after the Phase 1 runtime analysis. It supplements `PHASE1_CURRENT_STATUS.md` and should be treated as the current decision for framework lifetime and shutdown.

## 1. Scope

The UI framework is an embedded retained-mode UI framework/runtime used by an application. It is not responsible for the application's global process lifetime or main loop.

The target architecture is:

```text
Application
├── application lifetime
├── main/game loop
├── SDL runtime / window / renderer
├── ChessEngine
└── UIManager
      └── UI framework runtime
          ├── NodeTree
          ├── InputManager
          ├── ModalManager
          └── LayoutManager
```

The application decides when to create and destroy `UIManager`. The framework owns its internal UI runtime for as long as that `UIManager` exists.

## 2. Ownership

`UIManager` owns the UI runtime subsystems through `std::unique_ptr`:

```cpp
std::unique_ptr<NodeTree> nodeTree_;
std::unique_ptr<InputManager> inputManager_;
std::unique_ptr<ModalManager> modalManager_;
std::unique_ptr<LayoutManager> layoutManager_;
```

`NodeTree` owns the live framework nodes.

The application owns the `UIManager` instance and the SDL runtime resources. The UI framework depends on SDL3/SDL_image/SDL_ttf, but dependency does not imply ownership of the SDL runtime.

## 3. Application Loop

The application remains responsible for the outer loop:

```cpp
while (running)
{
    pollEvents();
    ui.processEvent(event);

    updateApplication(dt);
    ui.runFrame(dt, renderer);

    present();
}
```

`UIManager::runFrame()` is the public UI-frame boundary. The client chooses **when** to give the framework a frame; the framework determines **what happens inside that frame and in what order**.

The framework does not need its own `while` loop for this architecture.

## 4. Construction

Normal C++ construction is sufficient:

```cpp
UIManager ui;
```

Construction creates the internal UI runtime subsystems. No separate `initialize()` operation is required by the current Phase 1 model.

## 5. Destruction

Normal C++ RAII destruction is also sufficient.

`UIManager` has an out-of-line defaulted destructor:

```cpp
UIManager::~UIManager() = default;
```

The destructor is declared in the public header and defined in `ui_manager.cpp`, where the concrete subsystem types are complete.

This is intentional: the framework does not need a separate public `shutdown()` operation merely to release its own C++ objects.

## 6. Destruction Order

The members are declared in this order:

```text
nodeTree_
inputManager_
modalManager_
layoutManager_
```

C++ destroys members in reverse declaration order:

```text
~UIManager()
    ↓
~LayoutManager()
    ↓
~ModalManager()
    ↓
~InputManager()
    ↓
~NodeTree()
```

This is currently safe based on the Phase 1 source inspection:

- `LayoutManager` stores viewport state and does not own or retain a `NodeTree` object.
- `ModalManager` stores modal sessions as node IDs, not ownership of nodes or a `NodeTree`.
- `InputManager` stores tracked `Node*`/`NodeId` state, but its destruction does not perform runtime operations against `NodeTree`.
- `NodeTree` is therefore destroyed after the managers that coordinate runtime state around it.

No destructor should attempt to call back into the already-destroying framework runtime unless a future design explicitly requires such behavior.

## 7. No Special Shutdown State in Phase 1

Do **not** introduce any of the following solely for framework destruction:

```text
shuttingDown_
shutdown mutation queue
forced shutdown traversal
mandatory shutdown() API
```

The reason is semantic: destruction of the entire `UIManager` is not the same operation as removing one node from an otherwise-running UI runtime.

During ordinary application shutdown there is no next UI frame, no requirement to process new input, and no requirement to preserve the runtime after `UIManager` destruction.

RAII destruction is therefore the default shutdown mechanism.

## 8. `onMount` / `onUnmount` Semantics

These hooks describe **membership in the active UI runtime**, not raw C++ object lifetime.

The normal node lifetime is:

```text
construct object
    ↓
attach to active NodeTree
    ↓
onMount()
    ↓
node participates in UI runtime
    ↓
remove()
    ↓
onUnmount()
    ↓
node leaves active runtime
    ↓
framework-owned object is destroyed
```

### `onMount()`

`onMount()` means:

> The node has entered the active UI runtime and its runtime relationships are now established.

This is intentionally distinct from the constructor. A constructed node can exist before it is attached to the tree.

### `onUnmount()`

`onUnmount()` means:

> The node is leaving the active UI runtime as part of a normal structural removal.

It is not defined as a synonym for the C++ destructor.

This gives `onUnmount()` a useful role even though Phase 1 `remove()` normally leads to destruction of the removed node. It is a runtime departure hook and the last lifecycle callback before the framework releases the node from the active tree and destroys it.

## 9. Framework Destruction vs Normal Removal

These operations are intentionally different:

### Normal removal

```text
remove(node)
    ↓
deferred mutation
    ↓
onUnmount()
    ↓
unregister / release tree ownership
    ↓
destroy node
```

The framework is still alive after this operation. Other nodes can update, render, receive input, and perform future mutations.

### Framework destruction

```text
~UIManager()
    ↓
subsystem destruction
    ↓
NodeTree destruction
    ↓
node object destruction
```

The entire UI runtime is ending. There is no requirement to model this as a sequence of ordinary `remove()` operations.

## 10. Does Framework Destruction Call `onUnmount()`?

**Phase 1 decision: no special shutdown lifecycle is required.**

`onUnmount()` is guaranteed for the normal runtime removal path, not as a universal promise that every node receives an unmount callback when the entire `UIManager` is destroyed.

This distinction prevents shutdown from acquiring a second, much more complicated mutation/lifecycle protocol.

If a future feature requires deterministic shutdown callbacks for application resources, that should be designed explicitly as a separate shutdown contract rather than silently redefining `onUnmount()`.

## 11. Why `onUnmount()` Is Still Useful

The existence of framework-owned destruction does not make `onUnmount()` redundant.

The following states are different:

```text
C++ object exists
        ↕
node belongs to active UI runtime
```

A node can be constructed before attachment, and a node can be in the process of leaving the active runtime before its C++ object is destroyed.

Therefore:

```text
constructor/destructor
    = C++ object lifetime

onMount/onUnmount
    = active UI runtime membership
```

This separation should remain stable even if the ownership model changes in a future phase.

## 12. `detach()` and Reparenting

Phase 1 does not restore public ownership-transfer `detach()`.

A removed node is framework-owned and is normally destroyed after deferred removal.

Reparenting is also not required for Phase 1. If a future application requirement establishes a need to move a live node between parents, introduce an explicit framework-owned `reparent()` operation rather than reintroducing ownership transfer as a side effect of `detach()`.

## 13. Current Contract Summary

```text
Application owns:
    UIManager lifetime
    application main loop
    SDL runtime
    ChessEngine

UIManager owns:
    UI runtime subsystems

NodeTree owns:
    live UI nodes

runFrame():
    one framework-controlled UI frame

onMount/onUnmount:
    active UI runtime membership hooks

constructor/destructor:
    C++ object lifetime

remove():
    normal deferred runtime removal + destruction

~UIManager():
    normal RAII shutdown of the UI runtime

shutdown():
    not required in Phase 1
```

## 14. Remaining Verification

The architecture is now decided. The remaining Phase 1 work is implementation verification rather than another lifetime design search:

1. Build the `phase1-worktree` independently.
2. Verify the new `UIManager::~UIManager() = default` definition links correctly.
3. Run the relevant runtime scenarios, especially mutation during update/draw/event/lifecycle callbacks.
4. Compare the worktree against the untouched baseline.
5. Reconcile the Phase 1 documentation if implementation details differ from the decisions above.
