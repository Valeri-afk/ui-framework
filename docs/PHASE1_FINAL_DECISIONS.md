# Phase 1 — Final Architecture Decisions

> **Status:** accepted into active `main`; source-level reconciliation complete
> **Date:** 2026-08-18
>
> This document is the authoritative snapshot of the Phase 1 architecture decisions. Phase 1 implementation has been promoted into the active `main` source tree. The `phase1-worktree/` directory is retained only as a historical/audit snapshot.

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

## 3. SDL relationship

The UI framework is SDL-backed and depends on SDL3, SDL3_image and SDL3_ttf. The application owns SDL initialization, window and renderer lifetime, and event polling. The framework consumes SDL events and an SDL renderer supplied by the application.

## 4. Node ownership

The selected public ownership model is:

```cpp
add(std::unique_ptr<Node>)
remove(Node&)
```

Once a node is live, the framework owns it. Public ownership-transfer `detach()` is not part of the Phase 1 API.

A client-held `Node*` is non-owning and may become invalid after removal. `NodeId` is used internally for identity/liveness resolution and does not own or extend lifetime.

## 5. Deferred removal

`remove()` is deferred when the runtime is inside a guarded mutation scope:

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

Self-remove is valid. `NodeId` supplies identity/liveness resolution; the mutation queue supplies safe mutation timing.

## 6. Lifecycle hooks

`onMount()` / `onUnmount()` describe membership in the active UI runtime, not raw C++ object lifetime.

```text
construct object
    -> attach
    -> onMount
    -> live in active UI tree
    -> remove
    -> onUnmount
    -> object destruction
```

Normal runtime removal guarantees the corresponding unmount transition. Framework-wide C++ destruction is conceptually separate from public `remove()` operations.

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

## 8. Event callback mutation

The event dispatch boundary uses:

```cpp
{
    NodeTree::ScopedMutationGuard guard(nodeTree);
    EventDispatcher::dispatch(...);
}

nodeTree.flushMutationQueue();
```

The guard must be destroyed before flushing. This is an existing runtime invariant; full event/hit-test stabilization remains Phase 3 work.

## 9. Reparenting

Public `reparent()` is not required for Phase 1. If a concrete future requirement appears, use a dedicated framework-owned reparent operation rather than restoring ownership-transfer `detach()`.

## 10. Framework lifetime

`UIManager` is created and destroyed by the application using normal C++ RAII. Phase 1 does not require a public `shutdown()` API or `shuttingDown_` state.

Framework destruction is conceptually different from normal node removal: the entire UI runtime is ending, so teardown does not need to be modeled as a sequence of public `remove()` operations.

## 11. What Phase 1 resolved

Phase 1 resolved and promoted into `main`:

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
- `onMount` / `onUnmount` semantics;
- framework RAII lifetime;
- no Phase 1 public reparenting requirement.

## 12. Explicit Phase 1 non-goals

The following remain outside Phase 1:

- recursive hit-testing stabilization;
- complete event-handler dispatch semantics;
- layout redesign;
- input architecture redesign;
- component architecture;
- modal/navigation redesign;
- rendering backend abstraction.

The current source's existing hit-testing and event infrastructure therefore must not be interpreted as fully stabilized merely because the Phase 1 runtime is accepted.

## 13. Verification policy

The repository does not yet have an active standalone framework build/test path. Phase 1 acceptance is therefore based on source reconciliation and architectural invariants, not empirical runtime verification.

A later project-level validation stage must establish standalone build and runtime tests. That later validation is separate from Phase 1 architectural acceptance.

## 14. Phase 1 completion state

**Phase 1 is accepted into the active `main` source baseline.**

The active `include/`, `src/` and `docs/` trees are authoritative for current development. `phase1-worktree/` is retained only as a historical/audit snapshot and must not be used as the source for new implementation.

The next architectural scope is **Phase 2 — Layout**.
