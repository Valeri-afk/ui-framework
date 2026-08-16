# Architecture

This document describes the current architecture of the framework as implemented
in the source code.

The source code is the authoritative source of truth.

This document must not describe planned architecture as if it already exists.
Future development direction belongs in `ROADMAP.md`.

---

## 1. Architectural Overview

The framework is currently organized around a central `NodeTree` that owns the
live UI hierarchy and coordinates ownership, lifetime, traversal, mutation,
layout invalidation, update, rendering and hit-testing.

`UIManager` acts as the public runtime facade and coordinates the major
subsystems:

```text
                         UIManager
                             |
          +------------------+------------------+
          |                  |                  |
      NodeTree          InputManager      LayoutManager
          |                  |
          |            EventDispatcher
          |
          +----------------------+
          |
      ModalManager
          |
      InputManager


The actual dependency relationships are more tightly coupled than this
high-level diagram suggests.

The current architecture is not yet a fully separated layered architecture.
In particular:

NodeTree contains several runtime responsibilities in addition to
ownership/tree management;
InputManager depends directly on NodeTree, EventDispatcher,
SDL3 event types and modal state;
LayoutManager operates directly on NodeTree and Node internals;
ModalManager coordinates directly with both NodeTree and InputManager;
rendering is currently SDL3-specific;
some core headers are kept in src/core rather than the public include tree.

These are properties of the current implementation, not necessarily the final
architecture.

2. Repository Scope
2.1 Active Core

The actively maintained framework core is primarily located in:

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

The files in src/core are part of the current implementation even when their
headers are not exposed through include/ui_framework/core.

2.2 Excluded Legacy Components

The following directories are outside the active architectural scope:

src/components/
include/ui_framework/components/

They contain older component implementations which are not maintained as part
of the current framework core.

They must not be used as the source of truth for:

current component contracts;
current runtime architecture;
current lifecycle semantics;
current input semantics;
current layout semantics.

They may be revisited during a later component-development phase.

2.3 Primitive Rendering Support

primitives.hpp and primitives.cpp are retained as low-level rendering
support code.

They are related to primitive drawing functionality such as borders, rounded
corners and other SDL rendering operations.

They are not a central architectural subsystem of the current runtime.

Unless a task explicitly concerns rendering or primitive drawing, they should
not be treated as a dependency of the core runtime design.

2.4 SDL3 Backend

SDL3 is the current rendering and platform/input backend.

SDL3 itself is not vendored into this repository.

The framework currently depends directly on SDL3 in several areas, including:

Node rendering interfaces;
UIManager;
InputManager;
layout viewport synchronization;
PanelNode rendering/clipping;
primitive rendering.

Backend abstraction is therefore not currently established as an independent
layer.

A future RenderContext or second backend is a roadmap concern, not part of
the current architecture.

3. Node

Source:

include/ui_framework/core/node.hpp
src/core/node.cpp

Node is the base object representing an element of the UI tree.

It owns the common state and behavior shared by all node types.

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

Node therefore currently combines:

runtime state;
layout state;
input-related state;
event handling;
lifecycle hooks;
rendering hook.

This is the current implementation boundary.

Ownership

A Node does not own itself.

Ownership is established through:

NodeTree root/overlay containers;
PanelNode child containers.

Node stores:

Node *parent_
NodeTree *owner_

These are non-owning references.

Actual ownership is represented by std::unique_ptr<Node> stored by the
tree/container.

Identity

Every node receives a globally generated Node::Id.

NodeTree maintains a registry:

NodeId -> Node*

for all currently live nodes.

The live-node registry is a central invariant of the current architecture.

4. PanelNode

Source:

include/ui_framework/core/panelnode.hpp
src/core/panelnode.cpp

PanelNode is the current container node.

It derives from Node and owns child nodes through:

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

It also establishes the abstract layout-container contract:

measure()
arrange()

Therefore PanelNode is both:

Node
 +
container ownership
 +
layout-container interface
Hierarchy invariants

A child cannot be attached if:

it already has a parent;
it already belongs to a NodeTree;
attaching it would create a hierarchy cycle.

When a panel belongs to a tree, child mutation is routed through NodeTree.

5. NodeTree

Source:

src/core/nodetree.hpp
src/core/nodetree.cpp

NodeTree is the central runtime structure.

It is currently the most important architectural subsystem in the framework.

Responsibilities

NodeTree currently manages:

root nodes;
overlay nodes;
child attachment/detachment;
node ownership;
live-node registration;
node identity lookup;
lifecycle;
traversal;
deferred mutation;
layout invalidation;
update traversal;
rendering traversal;
hit-testing.
Root and Overlay Containers

The tree maintains two top-level containers:

roots_
overlays_

Both contain owning:

std::unique_ptr<Node>

Overlays are rendered and hit-tested above normal roots.

Live Node Registry

NodeTree maintains:

std::unordered_map<NodeId, Node*> liveNodes_

A node becomes live when its subtree is attached to the tree.

A subtree is registered recursively.

A subtree is unregistered recursively when detached.

The registry is used throughout the framework to validate pointer identity and
lifetime.

The common invariant is effectively:

findNode(node.id()) == &node

for a live node.

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

The exact attach/detach path depends on whether the node is:

a root;
an overlay;
a child of a PanelNode.

NodeTree assigns tree ownership recursively and registers the complete
subtree in the live-node registry.

Mount and unmount callbacks are executed through subtree traversal.

7. Mutation Model

The framework uses deferred mutation to protect traversal and callback
execution from structural changes.

NodeTree maintains a generic mutation queue.

Mutations are executed through:

flushMutationQueue()

The queue uses snapshot-swap semantics.

If executing one batch creates additional mutations, those mutations are placed
into the next batch and processed during the same flush operation.

A nested mutation scope is represented by:

ScopedMutationGuard

Structural operations performed while a mutation scope is active are deferred
rather than modifying the hierarchy immediately.

This model is used by:

traversal;
event dispatch;
update;
rendering;
layout;
input processing.

This mutation model is one of the fundamental runtime invariants.

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

Top-level roots and overlays can also be traversed in forward or reverse
order.

Traversal generally works together with mutation guards and live-node
validation.

9. Update

The runtime update path is:

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

Mutations generated during update are deferred and flushed after traversal.

10. Layout

Source:

src/core/layoutmanager.hpp
src/core/layoutmanager.cpp
include/ui_framework/types.hpp

LayoutManager owns the framework's current Measure / Arrange orchestration.

The current layout system is recursive.

Layout Flow
NodeTree layout queue
        |
        v
LayoutManager
        |
        +-- measureRecursive()
        |
        +-- arrangeRecursive()

Layout is currently invalidated at the root level.

When a node requires layout, NodeTree walks from that node to its root and
queues the root.

The queue is deduplicated using:

layoutQueue_
layoutQueueSet_

Therefore the current invalidation granularity is:

affected node
      ↓
nearest tree root
      ↓
full root layout

This is important: the current system does not implement fine-grained subtree
layout invalidation.

11. Measure

MeasureContext provides:

availableSize
measureChild()

The available size represents the content-box size.

LayoutManager converts between:

content-box
border-box

using padding and border values.

The recursive process is:

measure parent
    |
    +-- measure child
    |      |
    |      +-- measure descendants
    |
    +-- calculate desired size

The resulting desired size is stored on Node as:

desiredSize_

Fixed sizes and min/max constraints are applied by LayoutManager.

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

and implements the current Measure / Arrange contract.

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

StackPanelNode is therefore an existing layout container, but it should be
considered part of the layout area rather than part of the current runtime
stabilization work.

Its implementation is expected to be revisited during the Layout phase.

14. Positioning

Node currently exposes:

PositionMode::Layout
PositionMode::Absolute

However, the current architecture does not yet represent a complete
absolute-positioning layout system.

The existence of PositionMode::Absolute should therefore not be interpreted
as evidence that a complete absolute-positioning model already exists.

Absolute positioning remains a layout concern.

15. InputManager

Source:

src/core/inputmanager.hpp
src/core/inputmanager.cpp

InputManager converts SDL input into framework-level input state and events.

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

The state stores both:

Node*
optional<Node::Id>

for tracked nodes.

This allows the manager to validate cached pointers against the current
NodeTree.

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

For a modal root, hit testing is restricted to that subtree.

Visibility and enabled state are respected.

Overflow::HIDDEN also affects hit testing.

Therefore input targeting is currently a responsibility shared between:

NodeTree
InputManager
ModalManager

rather than being isolated into a dedicated hit-test subsystem.

17. Focus

Focus is managed by InputManager.

A node must satisfy the relevant state requirements before receiving focus.

Focus transitions dispatch:

FocusLostEvent
FocusGainedEvent

Focus state is validated against the live-node registry.

If a tracked node disappears or becomes invalid, InputManager::syncState()
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

The default drag threshold is currently 5.0f.

19. Event System

The event system consists of:

event_types.hpp
EventHandlerStorage
Node::dispatchEvent()
EventDispatcher
InputManager
Event Types

event_types.hpp defines framework events including:

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

The storage uses an event-type-indexed table.

The intended dispatch model uses a snapshot of handlers so that handler
mutation during dispatch does not directly invalidate the active iteration.

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

The current implementation allows the caller to independently enable
tunneling and bubbling.

The dispatcher stores node IDs rather than relying exclusively on raw pointers
during propagation.

After each handler/node dispatch it checks whether the node is still live.

This is intended to make event dispatch safe against node deletion during
callbacks.

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
Modal Stack

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

Only the top modal controls the current modal interaction boundary.

23. Modal Input Boundary

When a modal is active, InputManager receives the top modal as its modal
root.

Hit testing is then restricted to that modal subtree.

This creates the current interaction boundary:

Top Modal
   |
   +-- descendants can receive pointer input
   |
   +-- outside nodes cannot be targeted

The modal system therefore depends directly on:

NodeTree hit testing
InputManager state
overlay ownership
24. Modal Focus

When a modal opens:

current focus is remembered;
pointer interaction is cancelled;
the first valid focusable descendant is searched;
focus is moved into the modal.

When a modal closes:

pointer interaction is cancelled;
current focus is cleared;
the previous focus is restored if still valid;
otherwise another valid focusable node is selected.

Invalid modal sessions are removed during synchronization.

25. Rendering

Rendering is currently SDL3-specific.

The primary rendering traversal is owned by NodeTree.

The current order is:

roots
  ↓
non-top overlays
  ↓
top modal

This ensures the top modal is rendered last among the overlay layer.

NodeTree::drawSubtree():

checks visibility;
applies overflow clipping when necessary;
calls Node::draw();
recursively renders panel children.

Renderer state is temporarily saved/restored around subtree rendering.

26. Clipping

Overflow::HIDDEN currently causes NodeTree to establish an SDL render clip
rectangle corresponding to the node's actual bounds.

Nested clipping rectangles are intersected.

Clipping is therefore currently implemented inside the SDL rendering traversal,
not through an independent rendering abstraction.

27. ControlNode

Source:

include/ui_framework/core/controlnode.hpp
src/core/controlnode.cpp

ControlNode currently exists in the repository.

It derives directly from Node.

Its current responsibility is limited to style properties and self-rendering:

StyleProps
background color
border color
border width
border radius

It is not currently the foundation of the component system.

It should therefore be treated as a deferred component-layer concern.

Its current implementation must not be assumed to represent the final
ControlNode architecture.

28. Component System Scope

The repository contains older component directories:

src/components/
include/ui_framework/components/

These are currently excluded from the active architecture.

The current core does not yet contain a complete component hierarchy such as:

Node
├── PanelNode
│   └── StackPanelNode
└── ControlNode
    ├── Button
    ├── Toggle
    └── ...

Only the currently implemented classes should be treated as existing.

Future components belong to the Component Model phase in ROADMAP.md.

29. Runtime Frame Flow

The public runtime facade is UIManager.

The normal frame flow is approximately:

UIManager::runFrame()
        |
        +-- synchronize viewport
        |
        +-- prepareForTreeOperation()
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

Tree mutations may be deferred during these operations and are flushed before
the runtime proceeds to the next stable state.

30. Public Facade

UIManager is the main public runtime facade.

It currently exposes:

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

31. Dependency Graph

The current dependency structure can be summarized as:

                         UIManager
                            |
            +---------------+----------------+
            |               |                |
            v               v                v
        NodeTree      InputManager     LayoutManager
            |               |                |
            |               v                |
            |        EventDispatcher         |
            |               |                |
            +---------------+----------------+
            |
            +------ PanelNode
            |          |
            |          +------ child ownership
            |
            +------ Node
            |
            +------ hit-test
            |
            +------ update
            |
            +------ draw
            |
            +------ layout invalidation


        ModalManager
             |
             +------ NodeTree
             |
             +------ InputManager

A more accurate conceptual dependency chain is:

Node
  ↑
PanelNode
  ↑
NodeTree
  ↑
+-----------------------------+
|                             |
UIManager                 LayoutManager
|                             |
InputManager              StackPanelNode
|
+---- EventDispatcher
|
ModalManager

This graph is conceptual. Actual source-level dependencies are not fully
layered and include direct cross-module dependencies.

32. Core Invariants

The following invariants are fundamental to the current implementation.

Node lifetime

A live node must be registered in NodeTree.

findNode(node.id()) == &node

must hold for a live node.

Ownership

A node attached to a tree has:

owner_ == that NodeTree

for the complete owned subtree.

Parent relationship

A child attached to a PanelNode has that panel as its parent.

Mutation safety

Structural mutations during guarded traversal are deferred.

Detached state

A detached subtree must no longer belong to the previous NodeTree.

Traversal safety

Traversal code must tolerate node deletion and structural changes initiated
by callbacks.

Layout invalidation

A layout-affecting mutation queues the containing root for layout processing.

Input safety

InputManager must not retain invalid live-node pointers after a node has been
removed or disabled.

Modal boundary

When a modal is active, pointer targeting must remain inside the top modal
subtree.

33. Current Architectural Boundaries

The following boundaries currently exist:

Runtime / Tree

NodeTree owns the runtime hierarchy, lifecycle and mutation model.

Layout

LayoutManager orchestrates Measure / Arrange.

Individual layout containers implement their own layout behavior.

Input

InputManager owns interaction state and converts SDL input into framework
events.

Events

EventDispatcher owns propagation mechanics.

EventHandlerStorage owns handler registration and storage.

Modal

ModalManager owns modal stack semantics and modal-related focus behavior.

Rendering

Rendering is currently directly implemented through SDL3 and node/tree
rendering traversal.

Components

A complete component architecture does not yet exist in the active core.

34. Deferred Architectural Areas

The following areas exist in the repository but should not be treated as
currently stabilized architectural layers:

ControlNode;
StackPanelNode;
absolute positioning;
advanced alignment;
Grid;
component hierarchy;
rendering abstraction;
backend abstraction;
second rendering backend;
resource abstraction.

Their future development is governed by ROADMAP.md.

35. Current Architecture vs Future Architecture

The current architecture should be understood as:

                    UIManager
                        |
        +---------------+---------------+
        |               |               |
    NodeTree        InputManager   LayoutManager
        |               |
        |         EventDispatcher
        |
        +---- Node / PanelNode
        |
        +---- mutation / lifecycle
        |
        +---- traversal
        |
        +---- layout invalidation
        |
        +---- update
        |
        +---- hit-test
        |
        +---- rendering
        |
        +---- overlays
                    |
               ModalManager

This is the current implementation.

It should not be prematurely transformed into a more abstract architecture
simply because such an architecture may be desirable in the future.

Future separation of responsibilities must be driven by concrete requirements
and by the active development phase.
