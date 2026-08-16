# Architecture

This document describes the current architectural structure of the framework.

The source code is the ultimate source of truth.

If this document conflicts with the source code, the source code takes precedence.

This document describes:
- module responsibilities;
- major dependencies;
- ownership boundaries;
- important architectural invariants.

It does not prescribe implementation details that are not yet established.

---

# 1. Runtime

The runtime is the foundation of the framework.

It is responsible for:
- node lifetime;
- ownership;
- tree structure;
- traversal;
- mutation;
- runtime coordination.

## Node

Responsibility:

- Base UI node.
- Stores node-level state and relationships.
- Provides common behavior shared by UI elements.
- Participates in the NodeTree lifecycle.

Important responsibilities must remain independent from higher-level component semantics where possible.

## NodeTree

Responsibility:

- Owns live nodes.
- Maintains the UI hierarchy.
- Provides node lookup and traversal.
- Coordinates structural mutation.
- Maintains NodeId / live-node invariants.

Ownership:

- NodeTree is the authoritative owner of live nodes.

Lifecycle:

- Nodes may be attached, detached, removed, or reparented.
- Operations that mutate the tree must preserve live-node invariants.

Mutation:

- Mutation during traversal or callbacks must be handled safely.
- Deferred mutation mechanisms must preserve deterministic traversal behavior.

## UIManager

Responsibility:

- High-level coordination of framework subsystems.
- Provides the public facade used by application code where appropriate.

Dependencies:

- NodeTree
- runtime subsystems

UIManager must not duplicate ownership responsibilities already belonging to NodeTree.

---

# 2. Layout

Layout operates on top of the runtime hierarchy.

## LayoutManager

Responsibility:

- Coordinates layout processing.
- Performs measurement and arrangement.
- Processes layout invalidation / layout queues.

Dependencies:

- NodeTree
- Node

## Measure

Measure determines the desired size of a node from the available constraints.

The Measure contract must distinguish between:
- available size;
- desired size;
- content-box;
- border-box.

## Arrange

Arrange assigns final positions and sizes to nodes after measurement.

Arrange must operate on the established NodeTree hierarchy and must not bypass runtime ownership rules.

## Future layout components

Expected future components include:

- StackPanel
- Grid
- alignment
- absolute positioning

These are part of the layout roadmap and are not assumed to exist until implemented.

---

# 3. Input

Input operates on the runtime hierarchy and layout geometry.

## InputManager

Responsibility:

- Processes input state.
- Performs hit-testing.
- Manages focus.
- Manages pointer interaction / capture.
- Converts platform input into framework-level interaction.

Dependencies:

- NodeTree
- layout state
- EventDispatcher

## Hit-testing

Hit-testing determines the node that receives pointer interaction based on the current UI hierarchy and layout state.

The exact hit-testing rules are defined by the implementation.

---

# 4. Events

## EventDispatcher

Responsibility:

- Dispatches framework events through the node hierarchy.
- Supports tunneling and bubbling.
- Maintains event target/currentTarget semantics.
- Handles propagation cancellation.

Dependencies:

- NodeTree
- Node

Event dispatch must remain safe when callbacks mutate the tree.

---

# 5. Components

Components are higher-level UI elements built on top of the runtime, layout, and input systems.

## PanelNode

Responsibility:

- Container-oriented node behavior.
- Manages child nodes.

PanelNode is part of the current framework.

## ControlNode

ControlNode is a potential future abstraction.

It must not be introduced merely to create a symmetrical class hierarchy.

It becomes justified only if interactive controls require shared responsibilities that do not belong in Node or PanelNode.

## Planned components

Potential future components include:

- Button
- Toggle
- Text
- Image
- Scroll

These are roadmap items unless they already exist in the source tree.

---

# 6. Modal / Navigation

## ModalManager

Responsible for higher-level modal interaction.

Expected responsibilities include:

- modal stack;
- modal lifecycle;
- focus interaction;
- focus restoration;
- overlay interaction.

Modal behavior depends on the runtime and input systems.

---

# 7. Rendering

Rendering is currently based on SDL.

Expected responsibilities include:

- drawing nodes;
- clipping;
- resource handling.

Potential future abstraction:

- RenderContext
- additional rendering backend

These abstractions are optional and must be introduced only when justified by concrete requirements.

---

# 8. Dependency Overview

Conceptually:

Runtime
    |
    +---- Layout
    |
    +---- Input ---- Events
    |
    +---- Components
    |
    +---- Modal
    |
    +---- Rendering

The exact dependency relationships must follow the current source code.

Higher-level modules must not bypass runtime ownership and lifecycle contracts.

---

# 9. Core Architectural Invariants

The following invariants are considered fundamental:

- NodeTree owns live nodes.
- NodeId identifies a live node independently of raw pointer lifetime.
- Node lifecycle must remain valid during traversal and callbacks.
- Mutation must not invalidate framework traversal semantics.
- Subsystems must respect NodeTree ownership.
- Higher-level systems should build on lower-level contracts rather than duplicate them.
- Architectural abstractions should be introduced only when justified by concrete responsibility boundaries.
