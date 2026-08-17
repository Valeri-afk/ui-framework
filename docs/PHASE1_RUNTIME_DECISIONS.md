# Phase 1 — Runtime Design Decisions

This document records the design decision around node ownership, removal, deferred mutation, and reparenting. It complements `docs/PHASE1_RUNTIME.md`.

## 1. Decision

The selected Phase 1 ownership model is:

```text
add(std::unique_ptr<Node>)
remove(Node&)
```

The framework owns every live node.

A live node is destroyed by the framework when `remove()` is applied. Client code does not receive ownership of a live node after removal.

The public Phase 1 API therefore does **not** expose `detach()` as an ownership-transfer operation.

## 2. Why `detach()` Is Rejected

The historical reason for `detach()` was reasonable: the framework originally relied on `unique_ptr`, did not have `NodeId` or a deferred mutation system, and returning ownership to the client avoided immediate destruction.

The runtime has since gained:

- `NodeId` identity;
- live-node registry;
- deferred mutation;
- nested mutation scopes;
- traversal snapshots;
- lifecycle management.

The original need for client-visible ownership transfer is therefore no longer established.

More importantly, deferred structural mutation and synchronous ownership transfer conflict.

Consider:

```cpp
void MyNode::update(float)
{
    auto node = detach(*this);
    node.reset();
}
```

If the node can be destroyed while its member function is still executing, the callback continues with a destroyed `this`. `NodeId` cannot make that C++ lifetime situation safe.

Returning `unique_ptr` immediately while postponing the actual structural transition would require an additional lifetime/handle model. That complexity is not justified by a demonstrated framework requirement.

## 3. Why `remove()` Is Better

With framework-owned lifetime:

```text
remove(node)
    |
    | guarded callback
    v
queue NodeId
    |
    | callback completes
    v
flush
    |
    v
resolve NodeId
    |
    v
unmount
    |
    v
unregister
    |
    v
destroy framework-owned unique_ptr
```

This gives the runtime one ownership model for live nodes and makes deferred removal a natural mutation operation.

It also makes self-removal safe:

```cpp
void MyNode::update(float)
{
    remove(*this);
    // object remains alive until the guarded scope has completed.
}
```

## 4. Consequences

The client can no longer:

- preserve a removed subtree through `unique_ptr`;
- remove a node and later reattach the same object through ownership transfer;
- use `detach + attach` as a synchronous reparenting mechanism.

These are intentional consequences, not accidental limitations.

The client can still construct arbitrary custom nodes and pass ownership into the framework through `add`/`attach`.

Client-held `Node*` references remain non-owning and may become dangling after removal. This is the client's responsibility; the framework does not attempt to turn raw pointers into owning or automatically-updating handles.

## 5. Reparenting Decision

Reparenting is **not a Phase 1 public capability**.

Reparenting is useful in richer UI systems, for example for:

- drag-and-drop between containers;
- tab/document movement;
- docking/workspace movement;
- moving an existing item while preserving its state;
- some overlay/popup transitions.

These are valid framework capabilities, but they are not requirements demonstrated by the current target chess application.

If a concrete requirement appears later, reparenting should be implemented as a dedicated runtime operation:

```cpp
reparent(Node&, PanelNode&);
```

The operation should preserve framework ownership and queue safely when requested from callbacks. It should not restore client-visible `detach()` ownership transfer merely to implement movement.

## 6. Alternatives Considered

### Model A — add / remove

```text
add(unique_ptr)
remove(node)
```

**Selected.**

Advantages:

- one owner for every live node;
- simple deferred lifetime semantics;
- simple self-removal;
- smaller API;
- fewer client lifetime responsibilities;
- existing `NodeId` and mutation machinery remain useful without additional lifetime handles.

Disadvantages:

- removed objects are destroyed;
- preserved subtree movement is unavailable without a future reparent capability.

### Model B — add / remove / reparent

```text
add(unique_ptr)
remove(node)
reparent(node, parent)
```

**Not selected for Phase 1, retained as a future capability.**

This is the preferred direction if the framework later proves that moving an existing node while preserving identity/state is required.

### Model C — add / detach / remove

```text
add(unique_ptr)
detach(node) -> unique_ptr
remove(node)
```

**Rejected.**

It creates two lifetime domains for live UI objects and preserves the deferred ownership-transfer problem without a demonstrated requirement that justifies the complexity.

## 7. API Direction

Target Phase 1 public operations are:

### `PanelNode`

```cpp
Node* add(std::unique_ptr<Node> child, size_t index);
void remove(Node& child);
```

### `NodeTree`

```cpp
Node* attachRoot(size_t, std::unique_ptr<Node>);
Node* attachOverlay(size_t, std::unique_ptr<Node>);
void removeRoot(Node*);
void removeOverlay(Node*);
Node* attachChild(PanelNode&, std::unique_ptr<Node>, size_t);
void removeChild(PanelNode&, Node&);
```

### `UIManager`

```cpp
Node* attachRoot(size_t, std::unique_ptr<Node>);
Node* attachOverlay(size_t, std::unique_ptr<Node>);
void removeRoot(Node*);
void removeOverlay(Node*);
```

Exact overloads and convenience forms are implementation details to be reconciled with the current headers.

## 8. What `NodeId` Does Not Do

`NodeId` and the mutation queue are complementary:

```text
NodeId
    identifies / re-resolves the intended node

mutation queue
    determines when structural/lifetime changes occur
```

`NodeId` alone cannot make immediate destruction of `this` safe during a member callback. Deferred removal remains necessary.

## 9. Working Principle

The framework should expose capabilities because they are responsibilities of the target UI runtime, not because they are technically possible or because another toolkit provides them.

For Phase 1 this means:

- framework owns live nodes;
- `add` enters ownership;
- `remove` ends ownership by framework destruction;
- `Node*` is non-owning;
- `NodeId` is internal identity/liveness support;
- deferred mutation protects active callbacks/traversals;
- `reparent` remains a future capability;
- public client ownership transfer is not part of the runtime contract.

## 10. Implementation Gate

This decision authorizes design work toward the `add/remove` model but does **not** authorize arbitrary source modifications.

Before implementation, the current headers and source must be reconciled against this contract and a file-by-file change plan must be reviewed.
