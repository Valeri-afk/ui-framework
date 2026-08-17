# Phase 1 — Runtime

## 1. Purpose

This document records the Phase 1 runtime contracts, identified problems, implementation decisions, and completion checklist derived from the current source code.

It complements:

- `ROADMAP.md` — phase scope and exit criteria;
- `FRAMEWORK_SCOPE.md` — why the framework exists and which capabilities belong in it;
- `ARCHITECTURE.md` — the implemented architecture;
- `PHASE1_RUNTIME_DECISIONS.md` — focused design decisions and rejected alternatives.

The current source remains authoritative for implemented behavior. This document describes the target Phase 1 runtime contract where it explicitly says a decision has been made.

---

## 2. Scope

Phase 1 stabilizes:

- `NodeTree`;
- `UIManager`;
- `Node`;
- `PanelNode`;
- ownership and lifetime;
- `NodeId`;
- lifecycle;
- traversal;
- structural mutation;
- attach/remove semantics.

Phase 1 does not redesign layout, input, event propagation, rendering, modal architecture, higher-level controls, or legacy components except where those systems must be checked against the runtime contract.

---

## 3. Runtime Model

The framework is a retained-mode UI runtime in which live nodes are owned by the framework:

```text
Client
  |
  | std::unique_ptr<Node>
  v
add / attach
  |
  v
NodeTree / PanelNode
  |
  | owns
  v
live Node
```

The runtime maintains:

- owning `std::unique_ptr<Node>` containers;
- `Node*` parent/owner references that are non-owning;
- `NodeId -> Node*` live-node registry;
- deferred mutation queue;
- nested mutation scopes;
- traversal snapshots.

---

## 4. Client Contract

The client may:

- construct custom `Node` / `PanelNode` descendants;
- override runtime and lifecycle callbacks;
- change node state from callbacks;
- add nodes from callbacks;
- request removal from callbacks;
- mutate other nodes through public APIs;
- store non-owning `Node*` references.

The framework protects ownership, lifetime, lifecycle, live-node registration, traversal and mutation timing. Client callbacks are not assumed to be mutation-free.

---

## 5. Ownership and Lifetime Contract

### 5.1 Framework-owned live nodes

A live node has exactly one owner:

- `NodeTree` for a root;
- `NodeTree` for an overlay;
- `PanelNode` for a child.

The framework is the sole owner of live nodes.

### 5.2 `Node*`

`Node*` is a non-owning access reference. It does not extend lifetime.

After `remove()` destroys the node, previously stored client raw pointers become invalid.

### 5.3 `NodeId`

`NodeId` is an identity/liveness token used internally for:

- traversal snapshots;
- deferred mutation resolution;
- event propagation;
- cached pointer validation;
- live registry lookup.

`NodeId` does not own or extend lifetime and does not replace `Node*` in the normal public client API.

### 5.4 Detached object hierarchies

A `PanelNode` that has never been attached to a `NodeTree` may still own children through ordinary C++ `unique_ptr` ownership.

This standalone/detached object state is an ordinary construction state, not a second live-runtime ownership domain.

Once a node becomes live, ownership belongs to the framework until destruction.

---

## 6. Lifecycle Contract

The target lifecycle is:

```text
client-created object
        |
        | add / attach
        v
framework-owned + registered
        |
        | onMount (pre-order)
        v
live
        |
        | remove
        v
onUnmount (post-order)
        |
        | unregister / owner clear
        v
unique_ptr destruction
```

Lifecycle callbacks may request further mutations.

A lifecycle callback cannot cancel its own lifecycle transition.

---

## 7. Mutation Contract

Structural mutations are deferred while a guarded runtime scope is active.

Examples:

- add/attach;
- remove;
- future runtime structural operations if explicitly introduced.

The intended sequence is:

```text
callback / traversal
    |
    +-- request mutation
    |
    +-- current objects remain alive
    |
callback ends
    |
flush
    |
structural/lifetime change
```

Mutations are applied in request order. Mutations generated while draining the queue are processed in later batches during the same flush.

The queue is an ordered deferred-command mechanism, not a final-state optimizer.

`NodeId` resolves the intended live object; the mutation queue determines when the operation is allowed to change runtime state.

---

## 8. Add Contract

Public ownership enters the framework through `add`/`attach`:

```cpp
Node* add(std::unique_ptr<Node> child, size_t index);
```

and corresponding root/overlay APIs.

Outside a guarded mutation scope, a successful attach may return the live `Node*` immediately.

Inside a guarded scope, the operation may be queued and therefore cannot return the final live pointer synchronously; the current `nullptr` return in this case is accepted as the current contract.

Ownership has nevertheless already transferred to the framework when the caller passes the `unique_ptr`.

---

## 9. Remove Contract — Selected Phase 1 Model

The selected ownership model is:

```text
add(std::unique_ptr<Node>)
remove(Node&)
```

There is **no public client ownership-transfer `detach()` operation in the target Phase 1 API**.

`remove()` means:

> remove the node/subtree from the runtime and let the framework destroy it after the mutation is safely applied.

For a live node inside a guarded scope:

```text
remove(node)
    -> queue NodeId
    -> callback continues
    -> flush
    -> resolve NodeId
    -> unmount
    -> unregister
    -> clear owner
    -> destroy unique_ptr
```

Self-remove is therefore valid:

```cpp
void MyNode::update(float)
{
    remove(*this);
    // this is still valid until the current guarded scope ends.
}
```

The current callback must never observe `this` being destroyed in the middle of its own execution.

Repeated removal requests should resolve through live-node checks and become no-ops when the node is no longer live.

---

## 10. Reparenting

Reparenting is **not a Phase 1 public capability**.

The framework should not introduce `reparent()` merely for API symmetry or because larger UI toolkits provide it.

The current target application does not demonstrate a requirement for preserving an existing live node while moving it between unrelated parents.

If a future application requirement establishes that reparenting is needed, it should be introduced as a dedicated framework operation that preserves framework ownership rather than by restoring public `detach()`.

---

## 11. Traversal Contract

Traversal is snapshot-based with live re-resolution.

The runtime may capture `NodeId` values, then resolve each ID against the current structure before invoking the callback.

The intended guarantees are:

- mutation is allowed during callbacks;
- current callback execution remains safe;
- mutations do not retroactively rewrite the current traversal;
- newly attached nodes do not enter the current snapshot;
- removed/non-live IDs are skipped safely;
- public callback-capable traversal APIs should provide the same mutation-safety guarantees.

A node requested for removal remains live until the applicable flush, so it may still be encountered by the current traversal.

---

## 12. State Mutation Categories

State mutations do not all require structural-mutation semantics.

The current implementation distinguishes:

### Deferred layout-affecting mutation

Examples include position, size, padding, border, visibility and related layout state.

### Immediate runtime flags

Examples include enabled, focusable and capturable state.

This distinction is intentional and should be documented rather than forcing every state mutation through the structural mutation queue.

---

## 13. PanelNode Contract

`PanelNode` remains the generic child-owning node.

For a not-yet-attached `PanelNode`, `add()` and `remove()` operate on local `unique_ptr` ownership only.

For a live `PanelNode`, child operations are coordinated through `NodeTree` so that ownership, live registration, lifecycle, mutation and layout invalidation stay consistent.

A child cannot already have a parent, already belong to a tree, or create a hierarchy cycle.

---

## 14. Root / Overlay Contract

Roots and overlays are framework-owned nodes held directly by `NodeTree`.

Target public operations are:

```cpp
Node* attachRoot(size_t, std::unique_ptr<Node>);
Node* attachOverlay(size_t, std::unique_ptr<Node>);
void removeRoot(Node*);
void removeOverlay(Node*);
```

No root/overlay ownership transfer back to the client is required by the Phase 1 contract.

---

## 15. UIManager / NodeTree Boundary

`UIManager` remains the public facade/orchestration layer.

`NodeTree` remains authoritative for:

- ownership;
- lifetime;
- traversal;
- mutation;
- live registry.

The framework should not introduce a second ownership system in `UIManager`.

---

## 16. Known Phase 1 Problems and Decisions

### P1 — `detach()` public ownership transfer

**Decision:** remove it from the target public API.

Reason: no demonstrated requirement for client-owned live nodes, and ownership transfer complicates deferred mutation and lifetime safety.

### P2 — `PanelNode::remove()` currently returns `unique_ptr`

**Decision:** change it to `void remove(Node&)` with framework-owned destruction semantics.

### P3 — `NodeTree::detachRoot/Overlay/Child`

**Decision:** replace with `removeRoot`, `removeOverlay`, `removeChild` operations that keep ownership in the framework and destroy after safe mutation application.

### P4 — `UIManager::detachRoot/Overlay`

**Decision:** replace with `removeRoot`, `removeOverlay` facade operations.

### P5 — Root/overlay traversal mutation scope

**Decision:** public callback-capable root/overlay traversal should establish the same mutation-safety contract as child traversal.

### P6 — `NodeTree` shutdown lifecycle

**Decision:** shutdown should be explicitly lifecycle-aware: pending mutations are settled, live subtrees are unmounted, then final owning destruction occurs. Shutdown is a one-way transition and callbacks cannot cancel it.

### P7 — Reparent

**Decision:** not implemented in Phase 1. Future reparenting, if required, should preserve framework ownership and be introduced as a dedicated capability.

### P8 — Immediate vs deferred state mutation

**Decision:** retain the existing distinction and document it rather than forcing all node state changes through one queue.

---

## 17. Final Phase 1 Completion Checklist

Phase 1 is complete only when the following implementation and documentation work is finished.

### A. Ownership/removal API

- [ ] Replace public `PanelNode::remove()` returning `std::unique_ptr<Node>` with `void remove(Node&)`.
- [ ] Replace `NodeTree::detachRoot()` with `void removeRoot(Node*)`.
- [ ] Replace `NodeTree::detachOverlay()` with `void removeOverlay(Node*)`.
- [ ] Replace `NodeTree::detachChild()` with `void removeChild(PanelNode&, Node&)`.
- [ ] Replace `UIManager::detachRoot()` with `void removeRoot(Node*)`.
- [ ] Replace `UIManager::detachOverlay()` with `void removeOverlay(Node*)`.
- [ ] Remove the public client ownership-transfer `detach()` API.

### B. Internal removal implementation

- [ ] Replace `detachFromContainer()` with a removal path that keeps ownership inside the framework until destruction.
- [ ] Replace `detachInternal()` with a removal implementation that holds the `unique_ptr` locally while running `unmount`/unregister logic, then destroys it when the local owner goes out of scope.
- [ ] Replace `detachChildInternal()` with framework-owned removal semantics.
- [ ] Preserve the existing `detachOwnedSubtree()` helper or rename it only if implementation clarity requires it; its internal role is no longer public ownership transfer.
- [ ] Preserve the existing `unique_ptr` containers, `liveNodes_`, `NodeId` and lifecycle machinery rather than introducing a new ownership abstraction.

### C. Deferred mutation behavior

- [ ] Ensure `removeRoot`, `removeOverlay`, and `removeChild` queue the target `NodeId` while a guarded mutation scope is active.
- [ ] Ensure queued removal re-resolves the target through `NodeId` before applying the operation.
- [ ] Ensure removal from inside `update`, traversal, event callbacks, `onMount`, and `onUnmount` is safe.
- [ ] Ensure self-remove never destroys `this` before the current callback returns.
- [ ] Ensure repeated/stale removal requests become safe no-ops.
- [ ] Preserve ordered FIFO batch semantics of `mutationQueue_`.

### D. Traversal consistency

- [ ] Give `forEachRoot()` and `rForEachRoot()` the same mutation-safety contract as `PanelNode::forEachChild()`.
- [ ] Give `forEachOverlay()` and `rForEachOverlay()` the same mutation-safety contract.
- [ ] Preserve snapshot identity semantics and live re-resolution.
- [ ] Preserve the rule that mutations do not retroactively rewrite the current traversal.
- [ ] Verify that mutations created during traversal are flushed only after the guarded traversal scope ends.

### E. Lifecycle and shutdown

- [ ] Define and implement lifecycle-aware `NodeTree` shutdown.
- [ ] Settle pending mutation work before final runtime destruction.
- [ ] Unmount live subtrees in post-order during shutdown.
- [ ] Unregister nodes and clear runtime ownership metadata before final destruction.
- [ ] Ensure shutdown is one-way and lifecycle callbacks cannot cancel destruction.
- [ ] Verify that the shutdown path does not create unsafe recursive mutation/lifetime behavior.

### F. Runtime invariants

After implementation, verify these invariants by source inspection and eventually by standalone runtime tests:

```text
live node
    -> exactly one framework owner
    -> owner_ points to the active NodeTree
    -> NodeId is present in liveNodes_

removed node
    -> not present in liveNodes_
    -> no longer has a framework owner
    -> is destroyed unless held as an ordinary pre-attachment object

queued removal
    -> node remains alive until flush
    -> current callback can finish safely
    -> target is resolved by NodeId at execution time
```

Also verify:

- lifecycle order is mount pre-order / unmount post-order;
- child/root/overlay operations follow the same deferred structural-mutation rules;
- raw pointers remain explicitly non-owning;
- `NodeId` remains an identity/liveness mechanism rather than an ownership mechanism;
- `NodeTree` remains the authoritative runtime owner.

### G. Documentation reconciliation

- [ ] Update `ARCHITECTURE.md` to describe the actual implemented ownership/removal model.
- [ ] Update `PHASE1_RUNTIME_DECISIONS.md` to mark the `add/remove` decision as implemented rather than merely selected.
- [ ] Keep `FRAMEWORK_SCOPE.md` focused on framework purpose and capability boundaries.
- [ ] Keep `ROADMAP.md` high-level and do not turn it into an implementation changelog.

### H. Verification

- [ ] Establish a standalone build path for the framework independent of the chessengine/client integration.
- [ ] Build the framework with the chosen C++20/SDL dependency configuration.
- [ ] Add a minimal runtime verification path covering at least add, remove, self-remove, nested mutation, lifecycle order, traversal mutation and live-node resolution.
- [ ] Do not claim Phase 1 runtime correctness is verified until the standalone build and runtime checks have actually been performed.

---

## 18. Explicit Non-Goals for Phase 1 Completion

The following are intentionally **not** required for Phase 1 completion:

- public `detach()` ownership transfer;
- public `reparent()` capability;
- preserving arbitrary detached live subtrees;
- redesigning layout algorithms;
- redesigning event routing;
- redesigning input architecture;
- redesigning modal architecture;
- building the full chess client;
- implementing chess-domain behavior;
- solving unrelated defects in higher-level controls or legacy components.

If a future requirement makes one of these necessary, it should be evaluated against `FRAMEWORK_SCOPE.md` and a new design decision should be recorded before implementation.

---

## 19. Edge Cases Covered

The target contract explicitly covers:

- self-remove during update;
- sibling removal during traversal;
- ancestor removal during traversal;
- add during traversal;
- nested mutations;
- mutation during `onMount`;
- mutation during `onUnmount`;
- mutation during event callbacks;
- repeated remove requests;
- add/remove operations occurring in one ordered mutation batch.

The common rule is that the current callback remains safe until the applicable flush.

---

## 20. Verification Status

The repository currently has no active standalone build/test verification for the framework runtime.

The Phase 1 analysis is therefore source-based and scenario-based rather than runtime-verified.

This is sufficient for architectural design but not sufficient to claim implementation correctness.

A standalone framework build and minimal runtime verification path remain required before claiming the phase fully verified.

---

## 21. Implementation Direction

The selected implementation sequence is:

1. replace public detach/removal ownership-transfer APIs with framework-owned `remove` operations;
2. preserve the existing `unique_ptr` storage model;
3. reuse the existing lifecycle and live-node registry machinery;
4. make removal queue-safe using `NodeId` resolution;
5. normalize mutation-safety of public root/overlay traversal;
6. implement lifecycle-aware `NodeTree` shutdown;
7. reconcile `ARCHITECTURE.md` and the Phase 1 decision record with the resulting implementation;
8. establish standalone build/runtime verification;
9. revisit reparenting only when a concrete supported-application requirement appears.

No unrelated subsystem should be redesigned merely for symmetry or completeness.
