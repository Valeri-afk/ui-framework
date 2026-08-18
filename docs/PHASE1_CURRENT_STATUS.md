# Phase 1 — Current Status

> **Status snapshot:** 2026-08-18
>
> This file records the current state of the Phase 1 analysis. It is intentionally a point-in-time decision/status document, not a replacement for the broader roadmap or architecture documents.

## 1. Where We Are

Phase 1 has completed the main **source-level runtime analysis** of ownership, structural mutation, traversal, lifecycle callbacks, and event callback boundaries.

The current work is being developed in:

```text
phase1-worktree/
```

The original implementation remains untouched so that the baseline can later be compared directly with the proposed implementation.

The framework has **not yet been independently built or runtime-tested**. All conclusions so far are based on source inspection and scenario analysis.

---

## 2. Core Decision: Framework-Owned Live Nodes

The selected ownership model is:

```text
add(std::unique_ptr<Node>)
remove(Node&)
```

Once a node becomes live, the framework owns it.

The client does not receive ownership back when a live node is removed.

### Consequences

- public client-facing `detach()` is not part of the target Phase 1 API;
- `remove()` destroys the node after deferred structural mutation is safely applied;
- `Node*` remains a non-owning reference;
- a client-held raw pointer may become invalid after removal;
- `NodeId` provides identity/liveness resolution but does not provide ownership or lifetime extension;
- preserving a removed node for later reinsertion is not a Phase 1 capability.

The historical `detach()` design made sense when the framework had direct `unique_ptr` ownership transfer but no deferred mutation or `NodeId` machinery. With the current runtime, public ownership transfer would create a second lifetime model and complicate callback safety without a demonstrated requirement.

---

## 3. Deferred Remove Model

The selected model is deferred removal:

```text
remove(node)
    |
    | guarded scope active
    v
queue NodeId
    |
    | current callback/traversal continues
    v
scope ends
    |
    v
flushMutationQueue()
    |
    v
resolve NodeId
    |
    v
unmount
    |
    v
unregister / clear owner
    |
    v
destroy framework-owned unique_ptr
```

This makes self-remove safe:

```cpp
void MyNode::update(float)
{
    remove(*this);
    // `this` remains alive until the relevant guarded scope ends.
}
```

`NodeId` and the mutation queue solve different problems:

```text
NodeId            -> which live node is this?
mutation queue    -> when may the structural/lifetime transition happen?
```

`NodeId` alone cannot make immediate destruction of `this` safe during a member callback.

---

## 4. Traversal Analysis — Result

The traversal model is snapshot + live re-resolution.

The runtime can capture `NodeId` values, then resolve them against the current structure before invoking the callback.

The following guarantees are now considered part of the target contract:

- callbacks may request structural mutation;
- the current callback remains safe until the applicable scope ends;
- mutations do not rewrite the current snapshot retroactively;
- newly attached nodes do not enter the current snapshot;
- removed/stale IDs are skipped safely;
- root/overlay traversal follows the same mutation-safety contract as child traversal.

The internal recursive `traversePreOrder()` / `traversePostOrder()` functions do not create their own mutation scopes. Their callers already establish the required scope.

### Early termination

Public traversal callbacks can terminate early without bypassing the mutation flush. The guard ends before the final flush.

---

## 5. Lifecycle Callback Analysis — Result

The runtime removal lifecycle is:

```text
attach
  -> register
  -> onMount (pre-order)

remove
  -> onUnmount (post-order)
  -> unregister
  -> destroy
```

`onMount()` and `onUnmount()` may request additional mutations.

Those mutations remain deferred until the surrounding lifecycle scope ends.

The important rule is:

> A lifecycle callback cannot cause the node currently executing that callback to be destroyed in the middle of the callback.

---

## 6. Event/Input Boundary — Result

A real bug was found in the event dispatch boundary.

The old order was effectively:

```cpp
ScopedMutationGuard guard(nodeTree);
EventDispatcher::dispatch(...);
nodeTree.flushMutationQueue();
```

The flush happened while the mutation scope was still active, so it could not actually drain the queue.

The worktree now uses:

```cpp
{
    NodeTree::ScopedMutationGuard guard(nodeTree);
    EventDispatcher::dispatch(...);
}

nodeTree.flushMutationQueue();
```

This makes event callback mutation consistent with update, draw and lifecycle callback mutation.

The outer `processEvent()` flush remains acceptable as an additional synchronization point.

---

## 7. Reparenting — Deferred, Not Required for Phase 1

Reparenting was deliberately separated from `detach()`.

We do **not** currently need public:

```cpp
reparent(node, newParent);
```

for Phase 1.

This is not because reparenting is universally useless. Larger UI systems can need it for docking, drag-and-drop, document/tab movement, workspace movement, and similar features.

The current target chess application has not established such a requirement.

If a real requirement appears later, the preferred design is a dedicated framework-owned `reparent()` operation, not restoration of public ownership-transfer `detach()`.

---

## 8. Shutdown — Deliberately Reopened Question

This is the **one major runtime-lifetime question currently left open**.

An earlier analysis assumed that `NodeTree` destruction should reproduce the normal `remove()` lifecycle:

```text
pending mutations
 -> onUnmount
 -> unregister
 -> destroy
```

We have since identified that this assumption is not justified yet.

There are two conceptually different operations:

### Runtime removal

```text
remove(node)
```

means:

> the node leaves an otherwise-running UI runtime.

`onUnmount()` has a clear meaning here.

### Runtime destruction

```text
~UIManager()
    -> ~NodeTree()
```

means:

> the entire UI runtime is ceasing to exist.

At that point there may be no next frame, input event, layout pass, render pass, or further runtime operation.

Therefore ordinary C++ RAII destruction of the framework-owned `unique_ptr` hierarchy may be a completely valid design.

### Current position

**Do not add `shuttingDown_`, a special shutdown mutation queue, or a forced shutdown-unmount protocol yet.**

First determine the framework lifetime model and decide what `onUnmount()` semantically means:

- leaving an active tree; or
- final object destruction.

The current evidence favors treating `onUnmount()` as a **structural/runtime-removal callback**, not automatically as a destructor callback.

This decision is intentionally open pending the framework lifetime discussion.

---

## 9. Framework Lifetime Questions Still Open

Before closing Phase 1, answer these questions:

1. What object is the actual public owner of the UI runtime? Currently this appears to be `UIManager`.
2. Who normally constructs and destroys `UIManager` — the application/client, an application runtime object, or another owner?
3. What is the intended destruction order among:
   - `InputManager`;
   - `ModalManager`;
   - `LayoutManager`;
   - `NodeTree`?
4. Do any subsystems retain pointers/references into `NodeTree` during their own destruction?
5. Does `onUnmount()` mean “left an active tree” or “object lifetime is ending”?
6. Should lifecycle callbacks run at all when the entire UI runtime is being destroyed?
7. If callbacks do run during framework destruction, what guarantees are provided about creating/removing nodes at that point?
8. Does the framework need an explicit `shutdown()` operation, or is RAII destruction sufficient?

No code change should be made solely to answer these questions until their intended semantics are agreed.

---

## 10. Current API Direction

Target Phase 1 direction:

### PanelNode

```cpp
Node* add(std::unique_ptr<Node> child, size_t index);
void remove(Node& child);
```

### NodeTree

```cpp
Node* attachRoot(size_t, std::unique_ptr<Node>);
Node* attachOverlay(size_t, std::unique_ptr<Node>);
void removeRoot(Node*);
void removeOverlay(Node*);
Node* attachChild(PanelNode&, std::unique_ptr<Node>, size_t);
void removeChild(PanelNode&, Node&);
```

### UIManager

```cpp
Node* attachRoot(size_t, std::unique_ptr<Node>);
Node* attachOverlay(size_t, std::unique_ptr<Node>);
void removeRoot(Node*);
void removeOverlay(Node*);
```

Exact signatures still need final reconciliation with the current headers before merging the worktree changes.

---

## 11. Problems: Resolved vs Open

### Resolved by design

- public ownership-transfer `detach()` is unnecessary for the current target;
- framework-owned deferred `remove()` is the preferred lifetime model;
- `NodeId` is identity/liveness, not ownership;
- raw pointers remain non-owning;
- self-remove is safe under deferred mutation;
- reparenting is not required for Phase 1;
- traversal mutation is snapshot + live re-resolution;
- root/overlay traversal receives a mutation scope;
- lifecycle callbacks may request deferred mutation;
- event dispatch flush ordering is corrected in the worktree.

### Open

- framework/UIManager lifetime model;
- destruction order of runtime subsystems;
- whether framework destruction invokes `onUnmount()`;
- whether explicit shutdown is needed;
- final reconciliation of the worktree against baseline headers/source;
- standalone compilation;
- runtime tests.

There is currently **no other identified structural mutation problem comparable to the former deferred-detach issue**. The remaining architectural uncertainty is primarily framework lifetime/shutdown semantics.

---

## 12. Phase 1 Completion Gate

Phase 1 should not be declared runtime-verified until:

```text
architecture/source analysis       ✅
scenario analysis                  ✅
ownership decision                 ✅
remove model                       ✅
traversal model                    ✅
event mutation boundary             ✅
reparent scope                      ✅
framework lifetime decision         ☐
API/header reconciliation           ☐
worktree vs baseline comparison     ☐
standalone build                   ☐
runtime verification               ☐
documentation reconciliation       ☐
```

The next discussion should therefore focus on **what the framework runtime actually is, how it is created, who owns it, and how its lifetime ends**. That discussion should happen before implementing any shutdown-specific code.

---

## 13. Important Note About Existing Phase 1 Documents

`PHASE1_RUNTIME.md` and `PHASE1_RUNTIME_DECISIONS.md` contain earlier design decisions made during the analysis. In particular, older text may describe lifecycle-aware shutdown as already selected.

This current-status file supersedes that earlier shutdown assumption until the framework lifetime discussion is completed.

After that discussion, the older documents should be reconciled so that there is one authoritative shutdown decision.
