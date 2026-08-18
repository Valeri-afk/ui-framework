# Phase 1 — Final Architecture Decisions

> **Status:** architecture decided
> **Date:** 2026-08-18
>
> This document is the concise authoritative snapshot of the Phase 1 architecture decisions reached during source analysis. It is intended for future development contexts.

## 1. Framework boundary

The project is an **embedded retained-mode UI framework/runtime**, not an application framework.

The application owns:

- application lifetime;
- the main/game loop;
- SDL runtime resources and event polling;
- ChessEngine and other application/domain systems.

The UI framework owns:

- the UI runtime state;
- the node tree;
- node ownership and lifetime while nodes are live;
- UI input/event routing;
- lifecycle callbacks;
- layout coordination;
- modal coordination;
- mutation scheduling;
- traversal/rendering order;
- the internal UI-frame protocol.

The application decides **when** to give the UI framework a frame. The UI framework decides **what happens inside that frame and in what order**.

## 2. Application loop and `runFrame()`

The framework does not own the application's main `while` loop.

The intended integration is:

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

`UIManager::runFrame()` is the public UI-frame boundary. Its internal phase ordering remains framework-controlled and should not be exposed as arbitrary public calls such as `update()`, `layout()` and `draw()`.

If future requirements need client code between UI phases, prefer explicit extension points over exposing the internal phase machinery.

## 3. SDL relationship

The UI framework is SDL-backed and depends on:

- SDL3;
- SDL3_image;
- SDL3_ttf.

The dependency does **not** imply ownership of the SDL runtime.

The application owns SDL initialization, window and renderer lifetime, and event polling. The framework consumes SDL events and an SDL renderer supplied by the application.

## 4. Node ownership

The selected public ownership model is:

```cpp
add(std::unique_ptr<Node>)
remove(Node&)
```

Once a node is live, the framework owns it.

Public ownership-transfer `detach()` is not part of the target Phase 1 API.

A client-held `Node*` is non-owning and may become invalid after removal. This remains client responsibility.

## 5. Deferred removal

`remove()` is deferred when the runtime is inside a guarded mutation scope.

The contract is:

```text
remove(node)
    -> queue NodeId
    -> current callback continues
    -> guarded scope ends
    -> flush
    -> resolve NodeId
    -> unmount
    -> unregister / clear ownership
    -> destroy framework-owned object
```

Self-remove is valid. `NodeId` supplies identity/liveness resolution; the mutation queue supplies safe mutation timing. They solve different problems.

## 6. Lifecycle hooks

`onMount()` / `onUnmount()` describe **membership in the active UI runtime**, not raw C++ object lifetime.

```text
construct object
    -> attach
    -> onMount
    -> live in active UI tree
    -> remove
    -> onUnmount
    -> object destruction
```

`onMount()` is distinct from construction because a node may exist before attachment.

`onUnmount()` is distinct from destruction because it means the node is leaving the active UI runtime. It is guaranteed for normal runtime removal, but it is **not a universal promise that every node receives an unmount callback when the entire framework is destroyed**.

## 7. Traversal and mutation

Traversal uses snapshot + live re-resolution:

```text
capture NodeIds
    -> callback-capable traversal under mutation scope
    -> resolve each NodeId before callback
    -> stale/non-live ids are skipped
    -> flush after leaving the guard
```

Public root/overlay traversal follows the same mutation-safety contract as child traversal.

Internal recursive pre-order/post-order traversal does not create another mutation scope; its callers already own the appropriate scope.

## 8. Event callback mutation

The common event dispatch boundary uses:

```cpp
{
    NodeTree::ScopedMutationGuard guard(nodeTree);
    EventDispatcher::dispatch(...);
}

nodeTree.flushMutationQueue();
```

The guard must be destroyed before flushing. Client event callbacks may request structural mutations safely under this contract.

## 9. Reparenting

Public `reparent()` is not required for Phase 1.

The absence of reparenting is a scope decision, not a claim that larger UI systems never need it.

If a concrete future requirement appears, use a dedicated framework-owned reparent operation rather than restoring ownership-transfer `detach()`.

## 10. Framework lifetime

`UIManager` is the public owner of the UI runtime and is created/destroyed by the application.

Normal C++ RAII is the lifetime mechanism:

```cpp
UIManager ui;
```

with:

```cpp
UIManager::~UIManager() = default;
```

No public `shutdown()` API or `shuttingDown_` state is required by Phase 1.

Current member destruction order is:

```text
~LayoutManager()
~ModalManager()
~InputManager()
~NodeTree()
```

This is the result of reverse destruction of the `UIManager` member declarations and is consistent with the current subsystem dependencies.

Framework destruction is conceptually different from normal node removal: the entire UI runtime is ending, so teardown does not need to be modeled as a sequence of public `remove()` operations.

## 11. `NodeTree` destruction

`NodeTree` remains an ownership container whose live node hierarchy is ultimately destroyed through its `unique_ptr` containers.

Phase 1 does not introduce a special shutdown lifecycle solely to force `onUnmount()` during destruction of the entire framework.

## 12. What Phase 1 has resolved

Resolved at the architecture/source level:

- ownership-transfer `detach()` removal;
- framework-owned deferred `remove()`;
- `NodeId` identity/liveness role;
- raw pointer responsibility;
- self-remove semantics;
- root/overlay traversal mutation safety;
- lifecycle callback mutation safety;
- event dispatch mutation boundary;
- `runFrame()` as the UI-frame integration boundary;
- application-owned main loop;
- SDL dependency/ownership boundary;
- `onMount/onUnmount` semantics;
- framework RAII lifetime;
- no special shutdown protocol;
- no Phase 1 public reparenting requirement.

## 13. Verification policy

The project intentionally does **not** require compilation or runtime tests between architecture phases.

Phase 1 development is being performed from source analysis, scenario analysis and architectural invariants. The framework will be built and empirically tested only after the planned six architecture phases are complete.

Therefore the current Phase 1 completion criterion is:

> the architecture and intended implementation are internally coherent and documented.

Compilation/runtime verification is a later project-level validation stage, not a Phase 1 architecture gate.

## 14. Remaining Phase 1 work

The remaining work before moving on is limited to:

1. final source-level reconciliation of `phase1-worktree` against the untouched baseline;
2. final documentation reconciliation so older Phase 1 documents do not contradict these decisions;
3. then proceed to the next architecture phase.

No new runtime abstraction should be introduced unless the final reconciliation exposes a concrete contradiction with these decisions.
