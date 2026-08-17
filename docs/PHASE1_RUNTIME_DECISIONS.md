# Phase 1 — Runtime Design Decisions

This document records the current design discussion around node removal, ownership transfer, and reparenting. It complements `docs/PHASE1_RUNTIME.md` and is intentionally focused on unresolved design alternatives rather than general runtime architecture.

## 1. Current Question

The existing API uses ownership transfer for removal:

```text
attached node
    |
    | detach
    v
std::unique_ptr<Node> -> client
```

The current implementation also has deferred structural mutation. When `detach()` is requested while a mutation scope is active, the operation is queued and the public function currently returns `nullptr`; the later `unique_ptr` produced by the queued operation is not returned to the caller.

This is the main unresolved Phase 1 API problem.

## 2. Why Deferred Detach Is Difficult

A synchronous ownership-transfer API and a deferred structural mutation have conflicting requirements.

`detach()` returning `std::unique_ptr<Node>` means ownership has already left `NodeTree`/`PanelNode` when the function returns.

Deferred mutation requires the runtime to retain control of the node until the current callback/traversal scope finishes.

Returning ownership immediately while still treating the node as fully runtime-owned would require an additional lifetime/handle model.

The problem is especially important for self-detach:

```cpp
void MyNode::update(float)
{
    auto node = detach(*this);
    node.reset();
}
```

If ownership can be destroyed inside the currently executing member function, the function would continue executing after `this` has been destroyed. `NodeId` cannot make this safe.

Therefore a simple "return unique_ptr immediately, then finish the detach later" implementation is not considered safe.

## 3. Candidate Ownership Models

### Model A — add / remove

```text
add(std::unique_ptr<Node>)
remove(Node&)
```

The framework owns every live node until removal. `remove()` is deferred when required and the node is destroyed by framework-owned `unique_ptr` storage.

Benefits:

- one owner for every live node;
- no deferred ownership-transfer problem;
- simple lifetime semantics;
- simple deferred mutation semantics;
- smaller public API;
- fewer client lifetime responsibilities.

Cost:

- the client cannot preserve a detached subtree;
- removing a node destroys its object and custom state;
- existing node state cannot be reused by simply moving the node elsewhere.

### Model B — add / remove / reparent

```text
add(std::unique_ptr<Node>)
remove(Node&)
reparent(Node&, PanelNode&)
```

The framework still owns every live node. `reparent()` moves an existing live node between parents without transferring ownership to the client.

Benefits:

- retains the simpler framework-owned lifetime model;
- preserves node identity and custom state during moves;
- avoids exposing ownership transfer to normal client code;
- deferred reparenting can be represented naturally as a queued runtime operation;
- no separate detached-object lifetime state is required.

Cost:

- reparenting needs explicit semantics for lifecycle, layout, event ancestry, focus/capture and related runtime state;
- public API is larger than add/remove alone.

### Model C — add / detach / remove

```text
add(std::unique_ptr<Node>)
detach(Node&) -> std::unique_ptr<Node>
remove(Node&)
```

`detach()` transfers ownership to the client; `remove()` destroys the node through deferred framework ownership.

Benefits:

- maximum flexibility;
- detached subtrees can be preserved and later reattached;
- reparenting can be composed from detach + attach in synchronous contexts.

Costs:

- two lifetime ownership domains;
- more client responsibility;
- more lifetime edge cases;
- difficult deferred ownership transfer semantics;
- more complex documentation and API surface.

## 4. Current Assessment

The original reason for `detach()` was historical and practical: the framework used `unique_ptr`, there was no `NodeId`, and transferring the `unique_ptr` back to the client avoided immediate destruction.

The runtime has since gained:

- `NodeId` identity;
- live-node registry;
- deferred mutation;
- lifecycle management;
- traversal snapshots;
- stronger runtime ownership invariants.

This removes some of the original motivation for exposing ownership transfer to the client.

At present there is no demonstrated Phase 1 client requirement that a node removed from the UI must remain alive outside the framework.

Therefore Model C should not be retained merely because `detach()` has historically existed.

## 5. Reparenting Assessment

Reparenting is a legitimate capability in mature retained-mode UI toolkits, but it is not a requirement for every UI application.

Examples from established toolkits show that parent changes are a normal capability in richer UI systems, while ordinary UI composition often keeps elements attached to stable parents. Qt uses parent-child object trees and provides parent-changing capabilities; WPF exposes logical and visual tree concepts and APIs for tree manipulation; GTK provides child insertion/removal and child reordering operations. These examples demonstrate that reparenting is a valid capability, not that every framework must expose it in its minimal runtime.

For this framework, reparenting should therefore be justified by an actual client/runtime need rather than introduced for API symmetry.

Potential future use cases include:

- drag-and-drop between containers;
- tab/document transfer;
- docking or workspace movement;
- moving an existing item between panels while preserving its state;
- overlay/popup transitions;
- other cases where destroying and recreating a node would be undesirable.

These are legitimate future capabilities but are not automatically Phase 1 requirements.

## 6. Current Direction

Two models are now considered the strongest candidates:

### A. add / remove

Choose this if the framework does not need preserved live-node movement between parents and a removed node can simply be destroyed.

### B. add / remove / reparent

Choose this if preserving an existing node while moving it between parents is an important framework capability.

The current `detach`-based ownership-transfer model is no longer preferred by default because its additional complexity has not yet been justified by a concrete client requirement.

## 7. Remaining Design Question

The key question is no longer:

> How do we make deferred `detach()` return a `unique_ptr`?

The more fundamental question is:

> Does the framework need client-visible ownership transfer for live nodes at all, or should the framework remain the sole owner of live nodes and provide a dedicated `reparent()` capability when moving an existing node is required?

Until this is resolved, `detach()`/`remove()` implementation changes should not be committed.

## 8. Working Principle

The runtime should not expose a capability solely because it is technically possible or because another toolkit has it.

Every public lifecycle/ownership operation should correspond to a real responsibility of the UI framework and should minimize the number of lifetime models the client has to understand.
