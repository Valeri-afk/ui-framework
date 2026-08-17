# Phase 1 — Runtime

## 1. Purpose

This document records the current Phase 1 runtime analysis, the runtime contracts derived from the current source code, known implementation problems, and the proposed direction for stabilizing the runtime.

It is a working design and implementation reference for Phase 1.

It does not replace `ROADMAP.md` or `ARCHITECTURE.md`:

- `ROADMAP.md` defines the development scope and exit criteria.
- `ARCHITECTURE.md` describes the implemented architecture.
- this document records the Phase 1 stabilization decisions, unresolved questions, and implementation work derived from the current source.

The current source code remains authoritative for existing behavior.

---

## 2. Phase 1 Scope

Phase 1 covers:

- `NodeTree`
- `UIManager`
- `Node`
- `PanelNode`
- ownership
- lifetime
- `NodeId`
- lifecycle
- traversal
- mutation
- attach / detach / reparent semantics

The following are outside the primary Phase 1 implementation scope:

- Layout architecture and layout correctness
- Input architecture
- Event propagation architecture
- Component architecture
- Modal / navigation architecture
- Rendering / backend architecture
- legacy component directories

Later subsystems may be inspected when necessary to validate runtime decisions, but they should not be redesigned as part of Phase 1.

---

## 3. Current Runtime Model

The current runtime is already substantially implemented. Phase 1 is therefore a stabilization task, not a greenfield runtime implementation.

The central ownership model is:

```text
NodeTree / PanelNode
        |
    unique_ptr<Node>
        |
       Node
```

`Node` stores non-owning references to its parent and owning `NodeTree`.

`NodeTree` owns root and overlay nodes.

`PanelNode` owns child nodes.

`NodeTree` maintains a live-node registry:

```text
NodeId -> Node*
```

Structural mutation is protected by a deferred mutation queue and nested mutation scopes.

Traversal uses `NodeId` snapshots and resolves nodes against the current structure before invoking callbacks.

---

## 4. Client Contract

A framework client may:

- construct `Node`, `PanelNode`, and derived node types;
- create custom node types through inheritance;
- override runtime hooks such as `update`, `draw`, `measure`, `arrange`, `onMount`, and `onUnmount`;
- change node state from callbacks;
- change other nodes from callbacks when the public API permits it;
- add or detach children from callbacks;
- request structural mutations from lifecycle callbacks and event callbacks;
- detach the current node from its own callback;
- keep non-owning `Node*` references to other nodes.

The framework does not attempt to prevent client code from performing legitimate public mutations merely because they occur inside a callback.

The framework is responsible for protecting its internal ownership, lifetime, registry, and traversal invariants.

The client does not directly own or mutate the following runtime structures:

- `NodeTree::roots_`
- `NodeTree::overlays_`
- `NodeTree::liveNodes_`
- `NodeTree::mutationQueue_`
- `PanelNode::children_`
- `Node::owner_`
- `Node::parent_`

---

## 5. Ownership and Lifetime Contract

### 5.1 Ownership

`std::unique_ptr<Node>` is the ownership mechanism.

A live node is owned by exactly one runtime/container location:

- `NodeTree` as a root;
- `NodeTree` as an overlay;
- `PanelNode` as a child.

`Node::parent_` and `Node::owner_` are non-owning references.

### 5.2 Attached node

For a live node:

```text
findNode(node.id()) == &node
```

must hold.

Non-root nodes have a parent. Root/overlay nodes do not have a parent.

The complete attached subtree is registered in `liveNodes_`.

### 5.3 Detached node

After a successful detach:

```text
owner_ == nullptr
parent_ == nullptr
node is absent from liveNodes_
```

The object may remain alive if the caller owns the returned `std::unique_ptr<Node>`.

A detached subtree remains a valid object hierarchy and may later be attached again.

### 5.4 Raw pointers

`Node*` is a non-owning access reference.

It does not extend the lifetime of a node and must not be treated as an ownership handle.

The framework does not attempt to make arbitrary client-held raw pointers automatically safe after destruction.

### 5.5 NodeId

`NodeId` is an internal identity/liveness mechanism, not an ownership handle.

Its roles include:

- traversal snapshots;
- deferred mutation resolution;
- live-node validation;
- cached pointer validation;
- event propagation path tracking.

`NodeId` does not replace `Node*` in the normal public client API.

---

## 6. Lifecycle Contract

The intended lifecycle is:

```text
Detached
   |
   | attach
   v
Owned + registered
   |
   | onMount (pre-order)
   v
Mounted / live
   |
   | detach
   v
onUnmount (post-order)
   |
   | unregister
   v
Detached
```

### Attach

The runtime establishes ownership, assigns subtree ownership, registers the subtree as live, and then runs `onMount()` in pre-order.

Mutations requested from `onMount()` are deferred.

### Detach

The runtime runs `onUnmount()` in post-order, unregisters the subtree, clears ownership, and transfers the owning `unique_ptr` when the detach operation is an ownership-transfer operation.

### Lifecycle callbacks

Lifecycle callbacks are client code and may request further mutations.

A lifecycle callback does not cancel the lifecycle transition currently being performed.

For example, `onUnmount()` must not be able to turn its own unmount into a successful re-attach during the same transition.

---

## 7. Mutation Contract

### 7.1 Structural mutation

Structural mutation includes changes to hierarchy or ownership topology, such as:

- attach root;
- attach overlay;
- attach child;
- detach root;
- detach overlay;
- detach child;
- reparent, when expressed as detach + attach.

When a guarded runtime scope is active, structural mutations are deferred.

### 7.2 Safe execution point

The intended model is:

```text
callback / traversal
      |
      +-- request structural mutation
      |
      +-- current objects remain live
      |
callback / traversal finishes
      |
flush mutation queue
      |
structural/lifetime change is applied
```

A node requested for detach therefore remains fully live, owned, and accessible until the mutation is actually flushed.

This protects C++ object lifetime while the current callback is executing.

### 7.3 Mutation ordering

Mutations are applied in request order.

The mutation queue uses snapshot-swap semantics. Mutations generated while draining one batch are processed in later batches during the same flush.

The mutation queue is not a declarative final-state optimizer. It is an ordered deferred-command mechanism.

### 7.4 Mutation and traversal

A mutation does not retroactively rewrite a traversal that has already started.

For example:

```text
A
├── B
├── C
└── D

B callback requests detach(C)
```

`C` remains live until the current guarded traversal completes, so `C` may still be visited during that traversal. The detach takes effect after flush.

Newly attached nodes do not participate in the current snapshot traversal.

### 7.5 NodeId and mutation queue

These mechanisms solve different problems:

```text
mutation queue
    -> controls when structural/lifetime changes are applied

NodeId
    -> identifies and re-resolves the intended live object
```

A `NodeId` cannot by itself make destruction during a currently executing C++ member function safe. Deferred mutation is therefore still required.

---

## 8. Traversal Contract

Traversal is snapshot-based with live re-resolution.

The runtime may capture a sequence of `NodeId` values, then resolve the current node for each ID before invoking the callback.

The important semantics are:

- traversal order is based on the snapshot;
- mutation is allowed during callbacks;
- current callback execution is protected by the mutation scope;
- a node is resolved again before callback execution rather than trusting an old pointer;
- mutations do not retroactively rewrite the current traversal;
- newly attached nodes are not added to the current snapshot;
- `Stop` stops the traversal;
- `SkipChildren` has meaningful semantics for pre-order traversal and does not require a separate post-order interpretation.

Public traversal APIs are expected to provide the same mutation-safety guarantees.

This includes the root/overlay traversal APIs and `PanelNode` child traversal APIs.

---

## 9. State Mutation Categories

Not every node state mutation needs the same runtime semantics.

The current implementation already distinguishes different categories.

### Deferred state/layout mutation

Examples include:

- visibility;
- position;
- size;
- min/max size;
- padding;
- border;
- overflow;
- position mode;
- other layout-affecting state.

These are currently routed through `NodeTree` deferred mutation handling.

### Immediate runtime state mutation

Examples currently include:

- enabled;
- focusable;
- capturable.

These currently change immediately.

This difference is not currently considered a Phase 1 architecture defect. It should be documented rather than artificially unified into one mutation class.

---

## 10. PanelNode Contract

`PanelNode` is the generic child-owning node.

For a detached `PanelNode`, child insertion/removal is local ownership manipulation.

For a live `PanelNode`, child insertion/removal is coordinated by `NodeTree` so that ownership, live-node registration, lifecycle, mutation, and layout invalidation remain consistent.

Hierarchy constraints include:

- a child cannot already have a parent;
- a child cannot already belong to a `NodeTree`;
- a child cannot create a hierarchy cycle.

`PanelNode::forEachChild()` uses snapshot IDs and a mutation guard for live trees.

---

## 11. Attach, Detach, and Reparent

### Attach

The framework accepts ownership through `std::unique_ptr<Node>`.

When the operation is applied, the node becomes part of the live hierarchy, is registered, and is mounted.

### Detach

The conceptual meaning of `detach` is:

> remove the node/subtree from the framework hierarchy and transfer ownership to the caller.

This is distinct from destruction.

The client may then:

- keep the subtree;
- reattach it later;
- destroy it by destroying the returned `unique_ptr`.

### Reparent

No atomic public `reparent()` primitive is required by the current design.

Reparenting is conceptually:

```text
detach from old parent
attach to new parent
```

For synchronous operations this can be expressed directly with ownership transfer.

For deferred callback-driven reparenting, the runtime still needs a safe implementation path that preserves ownership and lifecycle invariants. This remains an implementation issue to resolve during Phase 1.

---

## 12. UIManager / NodeTree Boundary

`UIManager` remains the public runtime facade and orchestration layer.

`NodeTree` remains the authoritative owner and runtime structure.

The intended split is:

```text
UIManager
    orchestration / public facade

NodeTree
    ownership
    lifetime
    traversal
    mutation
    live registry
```

No additional ownership abstraction is currently justified.

---

## 13. Known Problems

### P1 — `PanelNode::remove()` naming

Current behavior is ownership transfer and hierarchy detachment, not destruction.

**Proposed direction:** rename the operation to `PanelNode::detach()` so the public terminology matches `NodeTree::detach*()` and `UIManager::detach*()`.

### P2 — Deferred detach cannot currently return ownership

The current implementation defers `detach*()` inside an active mutation scope, immediately returns `nullptr`, and later discards the `std::unique_ptr<Node>` produced by the queued detach operation.

This conflicts with the ownership-transfer meaning of `detach()`.

**Status:** unresolved API/implementation issue.

### P3 — Deferred attach has context-dependent return semantics

An attach/add operation executed inside an active mutation scope returns `nullptr` even though the queued operation may succeed later.

Unlike detach, ownership is already transferred into the framework, so this is primarily a return-value/API consistency issue.

**Status:** requires explicit contract; no new request abstraction is currently justified.

### P4 — Root/overlay traversal does not consistently establish a mutation scope

`PanelNode` child traversal explicitly guards live traversal, while `NodeTree` root/overlay traversal helpers currently rely on callers to establish the appropriate scope.

This produces inconsistent guarantees across public traversal APIs.

**Proposed direction:** public callback-capable traversal helpers should provide the same mutation-safety contract.

### P5 — Deferred reparent during callbacks

Synchronous detach + attach is conceptually sufficient, but the current public ownership-transfer API does not provide a clean way to express the same operation from inside a guarded callback.

**Status:** unresolved implementation path. No atomic public `reparent()` abstraction is currently desired.

### P6 — `NodeTree` destruction is not currently lifecycle-aware

The current destructor path relies on `unique_ptr` destruction of roots and overlays. It does not currently pass through the normal `onUnmount()` / unregister lifecycle path.

**Proposed direction:** make `NodeTree` shutdown explicitly unmount live subtrees before final ownership destruction, while treating shutdown as a one-way lifecycle transition that cannot be cancelled by callback code.

### P7 — Immediate vs deferred state mutation is not explicitly documented

The source already distinguishes layout-affecting deferred state changes from immediate runtime flags such as enabled/focusable/capturable.

**Proposed direction:** document this distinction instead of forcing all state mutations through the structural mutation queue.

---

## 14. Edge Cases Covered by the Contract

The following cases are intended to be valid runtime scenarios:

```text
self-detach / self-remove during update
remove sibling during traversal
remove ancestor during traversal
attach during traversal
nested mutations
mutation during onMount
mutation during onUnmount
mutation during event callback
repeat/remove of an already removed node
```

The intended general rule is that the current callback completes while the current object remains protected by the deferred structural mutation model.

---

## 15. Mutation Ordering Examples

### Add then remove in the same queue

```text
add(B)
remove(B)
```

The queue is ordered. The runtime should apply the operations in that order rather than optimize them into a final state.

This means B may receive its normal mount/unmount lifecycle before being destroyed if both operations are explicitly requested and both are valid when executed.

### Remove parent then mutate child

```text
remove(A)
mutate child of A
```

After `remove(A)` has been applied, subsequent queued work resolving nodes in A's removed subtree must use `NodeId` liveness checks and fail cleanly when those nodes are no longer live.

### Add child after parent removal

```text
remove(A)
add(B to A)
```

The first operation removes A. The later operation cannot resolve A as a live parent and therefore cannot attach B.

---

## 16. Verification Status

Phase 1 currently has no active automated build/test verification in the repository.

The framework was intentionally separated from the chessengine/client integration while the runtime architecture is being stabilized. The local framework `CMakeLists.txt` exists in the developer workspace, but it is not currently part of the repository's authoritative build/test setup.

Therefore the current analysis is based on:

- source inspection;
- architecture documentation;
- roadmap requirements;
- static reasoning through runtime scenarios.

This is sufficient for architectural design work but not sufficient to claim runtime correctness is verified.

Before Phase 1 is considered fully verified, the framework should eventually have a standalone build and a minimal runtime verification path independent of the chessengine/client integration.

---

## 17. Phase 1 Implementation Direction

The current preferred implementation sequence is:

1. Align `PanelNode::remove()` terminology with detach semantics.
2. Resolve the deferred detach ownership-transfer problem.
3. Normalize mutation-safety guarantees of public root/overlay traversal APIs.
4. Implement safe deferred reparenting internally without introducing a public atomic `reparent()` abstraction.
5. Define and implement lifecycle-aware `NodeTree` shutdown.
6. Document the immediate/deferred state mutation distinction.
7. Reconcile `ARCHITECTURE.md` with the resulting implementation.
8. Re-evaluate the Phase 1 exit criteria.

No unrelated subsystem should be redesigned merely because it currently contains incomplete or incorrect behavior.

---

## 18. Non-Goals

Phase 1 does not attempt to:

- redesign layout;
- stabilize hit-testing;
- redesign input/event propagation;
- stabilize `EventHandlerStorage` dispatch;
- redesign `ControlNode`;
- redesign `StackPanelNode` layout behavior;
- introduce rendering abstractions;
- redesign modal/navigation behavior;
- revive legacy component directories;
- introduce abstractions solely for architectural symmetry.

---

## 19. Relationship to Project Documentation

`ROADMAP.md` remains the high-level definition of Phase 1 and its exit criteria.

`ARCHITECTURE.md` remains the description of the implemented architecture and must be updated when the implementation changes.

This document records the stabilization work and decisions made during Phase 1.

When Phase 1 is completed, this document should remain as a phase-specific design/history record rather than becoming the authoritative description of future behavior.
