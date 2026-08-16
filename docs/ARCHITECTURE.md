# Architecture

This document describes the current architecture of the framework as implemented
in the source code.

The source code is the authoritative source of truth.

This document describes current responsibilities, ownership, lifecycle,
dependencies, invariants and existing architectural boundaries.

Planned architecture and future development order are defined separately in
`ROADMAP.md`.

---

## 1. Architectural Overview

The framework is currently organized around a central `NodeTree`.

`NodeTree` owns the live UI hierarchy and is responsible for node ownership,
lifetime, traversal, mutation, lifecycle, layout invalidation, update,
rendering and hit-testing.

`UIManager` acts as the public runtime facade and coordinates the major
subsystems.

The current architecture is not a strictly layered architecture. Several
subsystems have direct dependencies on `NodeTree`, and some responsibilities
are intentionally centralized there.

The current high-level structure is:

```text
                         UIManager
                             |
          +------------------+------------------+
          |                  |                  |
      NodeTree          InputManager      LayoutManager
          |                  |                  |
          |            EventDispatcher    StackPanelNode
          |
          +------ Node
          |
          +------ PanelNode
          |
          +------ traversal
          |
          +------ lifecycle
          |
          +------ mutation
          |
          +------ update
          |
          +------ layout invalidation
          |
          +------ hit-testing
          |
          +------ rendering
          |
          +------ overlays
                         |
                    ModalManager


This diagram is intentionally high-level.

Actual source-level dependencies are more tightly coupled than this diagram
suggests.

In particular:

InputManager depends directly on NodeTree, EventDispatcher, SDL3 input
types and modal state.
LayoutManager operates directly on NodeTree and Node.
ModalManager coordinates directly with NodeTree and InputManager.
rendering is currently SDL3-specific;
NodeTree contains several responsibilities beyond pure tree ownership.

These are properties of the current implementation.

2. Repository Scope
2.1 Active Core

The active framework core is primarily located in:

include/ui_framework/
    types.hpp
    event_types.hpp
    core/
        node.hpp
        panelnode.hpp
        ui_manager.hpp
        controlnode.hpp
        stackpanelnode.hpp
        event_handler_storage.hpp
        event_handler_storage.inl


src/core/
    node.cpp
    nodetree.cpp
    nodetree.hpp
    panelnode.cpp
    ui_manager.cpp
    inputmanager.cpp
    inputmanager.hpp
    event_dispatcher.hpp
    layoutmanager.cpp
    layoutmanager.hpp
    modalmanager.cpp
    modalmanager.hpp
    controlnode.cpp
    stackpanelnode.cpp
    primitives.cpp

The exact public/private header placement is a repository organization detail;
the runtime implementation also uses internal headers from src/core.

2.2 Excluded Legacy Components

The following directories are currently outside the active architectural scope:

src/components/
include/ui_framework/components/

These directories contain older component implementations that are not
maintained as part of the current framework core.

They are not authoritative for:

current runtime contracts;
current lifecycle semantics;
current input semantics;
current layout semantics;
current component architecture.

They may be revisited during a later component-development phase.

Their presence in the repository does not imply that the current framework
actively depends on them.

2.3 Primitive Rendering Support

The following files are retained as low-level rendering support:

primitives.hpp
primitives.cpp

They provide primitive drawing functionality such as borders, rounded corners
and other low-level rendering operations.

They are not a central runtime subsystem.

They should therefore be considered part of the rendering/backend area rather
than part of:

NodeTree ownership;
lifecycle;
traversal;
input;
event propagation;
layout orchestration.

Their relevance becomes significant when rendering functionality is being
modified.

2.4 SDL3 Backend

SDL3 is the current platform, input and rendering backend.

SDL3 itself is not vendored into this repository.

The framework currently depends directly on SDL3 in several areas, including:

UIManager;
InputManager;
node rendering;
viewport synchronization;
panel rendering and clipping;
primitive rendering.

There is currently no independent rendering/backend abstraction.

A future RenderContext or additional backend is therefore a future
architectural direction rather than a current architectural layer.

3. Node

Source:

include/ui_framework/core/node.hpp
src/core/node.cpp

Node is the base object representing an element of the UI hierarchy.

It contains common state and behavior shared by node types.

Responsibilities

Node currently owns or exposes:

unique Node::Id;
parent relationship;
tree ownership reference;
visibility;
enabled state;
focusability;
capturability;
logical position;
position mode;
requested size;
minimum and maximum size;
desired size;
actual position;
actual size;
padding;
border;
overflow mode;
event handlers.

It also provides virtual hooks for:

update()
draw()
measure()
arrange()
onMount()
onUnmount()
hitTest()

Therefore Node currently combines several responsibilities:

runtime state
layout state
input-related state
event handling
lifecycle hooks
rendering hook

This is the current implementation boundary.

3.1 Node Ownership

A Node does not own itself.

Ownership is established through the tree/container hierarchy.

A node can be owned by:

NodeTree as a root;
NodeTree as an overlay;
PanelNode as a child container.

Node stores:

Node *parent_;
NodeTree *owner_;

These are non-owning references.

Actual ownership is represented by:

std::unique_ptr<Node>

stored by NodeTree or PanelNode.

3.2 Node Identity

Every node receives a globally generated Node::Id.

NodeTree maintains a registry:

NodeId -> Node*

for currently live nodes.

The important identity invariant is:

findNode(node.id()) == &node

for every live node.

4. PanelNode

Source:

include/ui_framework/core/panelnode.hpp
src/core/panelnode.cpp

PanelNode derives from Node and is the current generic child-container node.

It owns child nodes through:

std::vector<std::unique_ptr<Node>>
Responsibilities

PanelNode provides:

child ownership;
child insertion;
child removal;
child traversal;
reverse child traversal;
visible-child access;
visible-child indexing;
hierarchy validation.

It also establishes the generic container-side measure() / arrange()
contract.

Therefore its current role is:

Node
  +
child ownership
  +
container behavior
  +
layout-container interface
4.1 Hierarchy Invariants

A child cannot be attached when:

it already has a parent;
it already belongs to a NodeTree;
the attachment would create a hierarchy cycle.

When a PanelNode belongs to a tree, structural child mutation is coordinated
with NodeTree.

5. NodeTree

Source:

src/core/nodetree.hpp
src/core/nodetree.cpp

NodeTree is the central runtime structure.

It is currently the most important architectural subsystem.

Responsibilities

NodeTree currently manages:

root nodes;
overlay nodes;
node ownership;
child attachment/detachment;
live-node registration;
node identity lookup;
lifecycle;
traversal;
deferred mutation;
layout invalidation;
update traversal;
rendering traversal;
hit-testing.

NodeTree therefore currently acts as more than a pure ownership container.

5.1 Root and Overlay Containers

The tree maintains two top-level collections:

roots_
overlays_

Both contain owning:

std::unique_ptr<Node>

Overlay nodes are rendered and hit-tested above normal roots.

5.2 Live Node Registry

NodeTree maintains:

std::unordered_map<NodeId, Node*> liveNodes_

A node becomes live when its subtree is attached to the tree.

The complete subtree is registered recursively.

When detached, the subtree is recursively unregistered.

The registry is used throughout the framework to validate pointer identity and
lifetime.

The central invariant is:

findNode(node.id()) == &node

for every live node.

6. Ownership and Lifecycle

The current lifecycle can be summarized as:

Detached Node
     |
     | attach
     v
Owned by NodeTree
     |
     | register subtree
     v
Live Node
     |
     | mount
     v
Mounted Node
     |
     | detach
     v
Unmount
     |
     | unregister
     v
Detached Node

The exact attachment path depends on whether the node is:

a root;
an overlay;
a child of a PanelNode.

When attached to a tree, ownership and live-node registration are established
recursively for the subtree.

Mount/unmount callbacks are executed through subtree traversal.

7. Mutation Model

The framework uses deferred structural mutation to protect traversal and
callback execution from changes to the hierarchy.

NodeTree maintains a mutation queue.

Mutations are executed through:

flushMutationQueue()

The queue uses snapshot-swap semantics.

If execution of one batch creates additional mutations, those mutations are
placed into the next batch and processed during the same flush operation.

A nested mutation scope is represented by:

ScopedMutationGuard

Structural operations performed while a mutation scope is active are deferred
instead of modifying the hierarchy immediately.

This mechanism is used by runtime operations including:

traversal;
event dispatch;
update;
rendering;
layout;
input processing.

Deferred mutation is therefore one of the fundamental runtime invariants.

8. Traversal

NodeTree provides internal pre-order and post-order traversal.

Traversal supports:

Continue
SkipChildren
Stop

Traversal is used for:

mounting;
unmounting;
update;
ownership registration;
ownership removal;
other subtree operations.

Top-level roots and overlays can also be traversed in forward or reverse order.

Traversal operates together with mutation guards and live-node validation where
required by the operation.

9. Update

The normal update path is:

UIManager
    |
    v
NodeTree::update()
    |
    +-- roots
    |
    +-- overlays
           |
           v
      pre-order traversal
           |
           v
      Node::update(dt)

Invisible or disabled nodes are skipped together with their descendants.

Structural mutations generated during update are deferred and processed after
the traversal according to the mutation model.

10. Layout

Source:

src/core/layoutmanager.hpp
src/core/layoutmanager.cpp
include/ui_framework/types.hpp

LayoutManager currently owns the orchestration of the Measure / Arrange
process.

The current layout system is recursive.

NodeTree layout queue
        |
        v
LayoutManager
        |
        +-- measureRecursive()
        |
        +-- arrangeRecursive()

Layout is currently invalidated at the root level.

When a node requires layout, NodeTree walks from that node toward its root and
queues the root.

The queue is deduplicated using:

layoutQueue_
layoutQueueSet_

The current invalidation granularity is therefore:

affected node
      |
      v
nearest tree root
      |
      v
full root layout

The current system does not implement fine-grained subtree layout invalidation.

11. Measure

MeasureContext provides:

availableSize
measureChild()

The available size represents the content-box size.

The current layout implementation converts between:

content-box
border-box

using padding and border values.

The recursive process is approximately:

measure parent
    |
    +-- measure child
    |      |
    |      +-- measure descendants
    |
    +-- calculate desired size

The resulting desired size is stored on Node.

Fixed sizes and min/max constraints are applied by the current layout
implementation.

12. Arrange

ArrangeContext provides:

contentPosition
contentSize
placeChild()

The arrange process recursively assigns:

actualPosition_
actualSize_

to child nodes.

Child sizes are clamped against their min/max constraints before being
committed.

13. StackPanelNode

Source:

include/ui_framework/core/stackpanelnode.hpp
src/core/stackpanelnode.cpp

StackPanelNode derives from:

PanelNode

and implements a concrete Measure / Arrange strategy.

It supports:

Vertical
Horizontal

orientation.

The current implementation:

measures children sequentially;
accumulates their sizes along the main axis;
uses the maximum child size on the cross axis;
arranges children sequentially;
stretches children across the cross axis.

StackPanelNode is therefore an existing layout container.

It is not part of the current runtime stabilization scope and is expected to be
revisited during the Layout phase.

14. Positioning

Node currently exposes:

PositionMode::Layout
PositionMode::Absolute

However, the presence of PositionMode::Absolute does not mean that a complete
absolute-positioning system currently exists.

Absolute positioning is currently an incomplete layout concern.

Its semantics belong to the Layout phase rather than to the runtime ownership
model.

15. InputManager

Source:

src/core/inputmanager.hpp
src/core/inputmanager.cpp

InputManager converts SDL input into framework-level interaction state and
events.

Responsibilities

It currently manages:

hovered node;
focused node;
captured node;
pressed node;
drag state;
pointer tracking;
focus transitions;
pointer capture;
mouse enter/leave;
mouse click;
mouse drag;
mouse wheel;
keyboard events;
modal input restriction.

Tracked nodes use both:

Node*
optional<Node::Id>

This allows cached pointers to be validated against the current NodeTree.

16. Hit Testing

Hit testing is currently implemented by NodeTree.

The process is:

InputManager
      |
      v
NodeTree::hitTest()
      |
      +-- topmost overlays
      |
      +-- topmost roots
      |
      v
hitTestSubtree()

Child nodes are tested in reverse order so that later children are considered
visually above earlier children.

For an active modal root, hit testing is restricted to that subtree.

Visibility and enabled state are respected.

Overflow::HIDDEN also affects hit testing.

Input targeting is therefore currently shared between:

NodeTree
InputManager
ModalManager

rather than isolated in a dedicated hit-test subsystem.

17. Focus

Focus is managed by InputManager.

A node must satisfy the relevant state requirements before receiving focus.

Focus transitions dispatch:

FocusLostEvent
FocusGainedEvent

Focus state is validated against the live-node registry.

When a tracked node disappears or becomes invalid, InputManager::syncState()
clears or repairs the corresponding state.

18. Pointer Capture

Pointer capture is managed by InputManager.

The captured node becomes the primary target for pointer movement and release
events even when the pointer is no longer over that node.

Capture is used by the drag system.

The current drag model contains:

pressed node
captured node
press position
drag threshold
dragging state

The current default drag threshold is:

5.0f
19. Event System

The event system consists of:

event_types.hpp
EventHandlerStorage
Node::dispatchEvent()
EventDispatcher
InputManager

The framework currently defines events including:

MouseMove
MouseDown
MouseUp
MouseClick
MouseWheel
MouseEnter
MouseLeave
MouseDragBegin
MouseDrag
MouseDragEnd


KeyDown
KeyUp


FocusGained
FocusLost
20. EventHandlerStorage

EventHandlerStorage stores handlers by event type.

Handlers receive:

Event&
Node&

Handlers receive a token when registered.

Handlers can be:

added;
removed;
cleared.

The storage uses event-type-indexed tables.

The intended dispatch model uses a snapshot of handlers so that mutation of
handlers during dispatch does not invalidate the active iteration.

21. EventDispatcher

Source:

src/core/event_dispatcher.hpp

EventDispatcher is responsible for event propagation along the node ancestry.

The propagation path is constructed from:

target -> parent -> ... -> root

It supports:

TUNNELING
TARGET
BUBBLING

The dispatcher allows the caller to independently enable tunneling and
bubbling.

The propagation path stores NodeId values rather than relying exclusively on
raw pointers.

After dispatching to a node, the dispatcher checks whether the corresponding
node is still live.

This provides protection against node deletion during event callbacks.

22. ModalManager

Source:

src/core/modalmanager.hpp
src/core/modalmanager.cpp

ModalManager maintains a stack of active modal sessions.

A modal must:

be live;
belong to the overlay container;
be visible;
be enabled.

Each modal session stores:

modal NodeId
previous focused NodeId

Conceptually:

overlay nodes
      |
      v
ModalManager
      |
      v
modal stack
      |
      v
top modal

Only the top modal defines the current modal interaction boundary.

23. Modal Input Boundary

When a modal is active, InputManager receives the top modal as its modal
root.

Hit testing is then restricted to that modal subtree.

The resulting interaction boundary is:

Top Modal
   |
   +-- descendants can receive pointer input
   |
   +-- outside nodes cannot be targeted

The modal input boundary therefore depends directly on:

NodeTree hit testing;
InputManager state;
overlay ownership;
ModalManager.
24. Modal Focus

When a modal opens:

current focus is remembered;
pointer interaction is cancelled;
the first valid focusable descendant is searched;
focus is moved into the modal.

When a modal closes:

pointer interaction is cancelled;
current focus is cleared;
previous focus is restored if still valid;
otherwise another valid focusable node is selected.

Invalid modal sessions are removed during synchronization.

25. Rendering

Rendering is currently SDL3-specific.

The primary rendering traversal is owned by NodeTree.

The current order is approximately:

roots
  |
  v
non-top overlays
  |
  v
top modal

This ensures that the top modal is rendered last among the overlay layer.

NodeTree::drawSubtree() currently:

checks visibility;
applies overflow clipping where required;
calls Node::draw();
recursively renders panel children.

Renderer state is temporarily saved/restored around subtree rendering.

26. Clipping

Overflow::HIDDEN causes NodeTree to establish an SDL render clip
rectangle corresponding to the node's actual bounds.

Nested clipping rectangles are intersected.

Clipping is therefore currently implemented inside the SDL rendering traversal
rather than through an independent rendering abstraction.

27. ControlNode

Source:

include/ui_framework/core/controlnode.hpp
src/core/controlnode.cpp

ControlNode currently exists in the repository and derives directly from
Node.

Its current implementation is limited primarily to style/self-rendering
behavior, including:

StyleProps;
background color;
border color;
border width;
border radius.

It is not currently the foundation of a complete component system.

ControlNode is therefore an existing but deferred part of the framework.

Its current implementation should not be interpreted as the final component
architecture.

Its further responsibility is to be determined when the Component Model phase
is addressed.

28. Component System

A complete component architecture does not currently exist in the active core.

The repository also contains legacy component directories:

src/components/
include/ui_framework/components/

These are outside the active architectural scope.

The current active core should therefore not be represented as an established
hierarchy such as:

Node
├── PanelNode
│   └── StackPanelNode
└── ControlNode
    ├── Button
    ├── Toggle
    └── ...

The hierarchy above is only a possible future direction.

Currently implemented classes are the only classes that should be treated as
existing architectural components.

29. Runtime Frame Flow

The public runtime facade is UIManager.

The normal frame flow is approximately:

UIManager::runFrame()
        |
        +-- synchronize viewport
        |
        +-- prepare for tree operation
        |
        +-- NodeTree::update()
        |
        +-- LayoutManager::processLayoutQueue()
        |
        +-- synchronize input/modal state
        |
        +-- NodeTree::draw()
        |
        +-- synchronize state

Input events follow a separate path:

SDL_Event
    |
    v
UIManager::processEvent()
    |
    v
InputManager
    |
    +-- hit-test
    +-- focus/capture state
    +-- event generation
    |
    v
EventDispatcher
    |
    v
Node handlers

Tree mutations may be deferred during these operations and flushed before the
runtime proceeds to the next stable state.

30. UIManager

UIManager is the main public runtime facade.

It currently exposes operations including:

frame execution;
SDL event processing;
root attachment/detachment;
overlay attachment/detachment;
viewport configuration;
modal open/close;
modal queries.

It owns the major runtime managers through std::unique_ptr:

NodeTree
InputManager
ModalManager
LayoutManager

The individual managers remain separate implementation subsystems.

31. Dependency Structure

The current architecture can be summarized as:

                         UIManager
                             |
          +------------------+------------------+
          |                  |                  |
          v                  v                  v
      NodeTree          InputManager      LayoutManager
          |                  |                  |
          |                  v                  v
          |            EventDispatcher    StackPanelNode
          |
          +------ Node
          |
          +------ PanelNode
          |
          +------ lifecycle
          |
          +------ traversal
          |
          +------ mutation
          |
          +------ update
          |
          +------ layout invalidation
          |
          +------ hit-testing
          |
          +------ rendering
          |
          +------ overlays
                         |
                         v
                    ModalManager

Important direct dependencies include:

UIManager
  -> NodeTree
  -> InputManager
  -> LayoutManager
  -> ModalManager


InputManager
  -> NodeTree
  -> EventDispatcher
  -> ModalManager state
  -> SDL3


LayoutManager
  -> NodeTree
  -> Node


ModalManager
  -> NodeTree
  -> InputManager


NodeTree
  -> Node
  -> PanelNode
  -> SDL3 rendering

This is a description of the current dependency structure, not a target
layered architecture.

32. Core Invariants

The following invariants are fundamental to the current implementation.

32.1 Node Lifetime

A live node must be registered in NodeTree.

For a live node:

findNode(node.id()) == &node

must hold.

32.2 Ownership

A node attached to a tree has:

owner_ == that NodeTree

for the complete owned subtree.

32.3 Parent Relationship

A child attached to a PanelNode has that panel as its parent.

32.4 Mutation Safety

Structural mutations performed during guarded traversal are deferred.

32.5 Detached State

A detached subtree must no longer belong to the previous NodeTree.

Its live-node registrations must also be removed from that tree.

32.6 Traversal Safety

Traversal must tolerate structural changes initiated by callbacks according to
the current deferred-mutation and live-node validation mechanisms.

32.7 Layout Invalidation

A layout-affecting mutation currently queues the containing root for layout
processing.

32.8 Input Safety

InputManager must not continue using invalid node references after the
corresponding node has been removed from the live tree.

32.9 Modal Boundary

When a modal is active, pointer targeting must remain inside the top modal
subtree.

33. Current Architectural Boundaries

The current implementation establishes the following responsibility
boundaries.

Runtime / Tree

NodeTree owns:

runtime hierarchy;
ownership;
lifecycle;
mutation;
traversal.
Layout

LayoutManager orchestrates Measure / Arrange.

Individual layout containers implement their own layout behavior.

Input

InputManager owns interaction state and converts SDL input into framework
input events.

Events

EventDispatcher owns event propagation mechanics.

EventHandlerStorage owns handler registration and storage.

Modal

ModalManager owns modal stack semantics and coordinates modal-related focus
behavior.

Rendering

Rendering is currently implemented directly through SDL3 and the
NodeTree rendering traversal.

Components

A complete component architecture does not currently exist in the active core.

34. Deferred Areas

The following areas exist in the repository or are represented by current
types, but are not currently stabilized architectural layers:

ControlNode;
StackPanelNode;
absolute positioning;
advanced alignment;
Grid;
complete component hierarchy;
rendering abstraction;
backend abstraction;
second rendering backend;
resource abstraction.

These areas are not necessarily architectural problems.

They are simply not part of the currently stabilized runtime foundation.

Their development order is defined in ROADMAP.md.

35. Current Architecture Summary

The current framework should be understood as a centralized runtime architecture
built around NodeTree:

                         UIManager
                             |
          +------------------+------------------+
          |                  |                  |
      NodeTree          InputManager      LayoutManager
          |                  |                  |
          |                  v                  |
          |            EventDispatcher          |
          |                                     |
          +------ Node / PanelNode       StackPanelNode
          |
          +------ ownership
          |
          +------ lifecycle
          |
          +------ mutation
          |
          +------ traversal
          |
          +------ update
          |
          +------ layout invalidation
          |
          +------ hit-testing
          |
          +------ rendering
          |
          +------ overlays
                         |
                         v
                    ModalManager

The important architectural characteristic is that NodeTree is currently
the central runtime authority rather than a narrowly scoped tree container.

The framework is therefore not yet a fully separated layered architecture.

Further separation of responsibilities should be based on concrete requirements
and the active development phase rather than introduced solely for architectural
symmetry.

The current source code remains the authoritative definition of all behavior.
