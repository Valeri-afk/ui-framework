# Architecture — Phase 5 Review Copy

This document is a proposed replacement for `ARCHITECTURE.md` after Phase 5.
It is a review copy only. The original `ARCHITECTURE.md` remains unchanged until
this version is manually reviewed and accepted.

The source code is the authoritative source of truth for current behavior.
This document describes current responsibilities, ownership, lifecycle,
dependencies, invariants and architectural boundaries. Planned development
order is defined separately in `ROADMAP.md`.

---

## 1. Architectural Overview

The framework is organized around a central `NodeTree`.

`NodeTree` owns the live UI hierarchy and currently coordinates node ownership,
lifetime, traversal, deferred mutation, lifecycle, layout invalidation, update,
rendering and hit-testing.

`UIManager` is the public runtime facade and coordinates the major managers.

The architecture is intentionally not a strictly layered architecture.
Several subsystems depend directly on `NodeTree` because it is the authority
for the live hierarchy and Node identity.

```text
                         UIManager
                             |
        +--------------------+--------------------+
        |                    |                    |
    NodeTree            InputManager        LayoutManager
        |                    |                    |
        |              EventDispatcher      StackPanelNode
        |
        +------ Node
        +------ PanelNode
        +------ active components
        +------ lifecycle
        +------ traversal
        +------ mutation
        +------ layout invalidation
        +------ update
        +------ hit-testing
        +------ rendering
        +------ overlays

    ModalManager
        |
        +------ NodeTree
        +------ InputManager
```

`ModalManager` is existing runtime infrastructure/preparation. It must not be
confused with the final Phase 6 `Modal` component design.

---

## 2. Repository Scope

### 2.1 Active runtime/core

The active core is primarily located in:

```text
include/ui_framework/
    types.hpp
    event_types.hpp
    core/
        node.hpp
        panelnode.hpp
        ui_manager.hpp
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
    stackpanelnode.cpp
    primitives.cpp
```

`ControlNode` is intentionally absent. The former `ControlNode` experiment was
removed and is not an accepted universal component base.

### 2.2 Active component layer

The component layer is part of the active framework architecture:

```text
include/ui_framework/components/
src/components/
```

The current standard component set is:

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
Checkbox
RadioButton
Slider
Dropdown
```

These are framework components with generic UI contracts. Their existence does
not imply that the framework should contain every component found in a typical
UI toolkit.

### 2.3 Removed legacy component architecture

The following concepts are not active architecture:

```text
Widget
ControlNode
old component implementations based on Widget
old Modal component implementation based on Widget
```

Removed files and legacy source are not authoritative for current contracts.

---

## 3. Component Architecture

Phase 5 establishes a small standard component layer above the existing runtime,
layout, input/event and rendering infrastructure.

The framework does not use a universal component hierarchy merely for symmetry.
A component chooses its base according to responsibility.

```text
Node
├── leaf / self-contained standard components
└── PanelNode
     └── composite components that genuinely own child Nodes
```

### 3.1 Node as component base

`Node` is the default base when a component can own its semantic state and
presentation without structurally owning child Nodes.

Examples include:

```text
Checkbox
RadioButton
Slider
```

### 3.2 PanelNode as component base

`PanelNode` is selected when child ownership and structural composition are
part of the component's semantics.

Examples include components such as:

```text
Menu
Dropdown
TabControl
```

The fact that a component contains multiple visual elements does not by itself
require `PanelNode`.

### 3.3 Component responsibilities

Components own:

```text
semantic state
component-specific properties
presentation
component-specific interaction semantics
intentional structural composition
```

Components do not reimplement:

```text
NodeTree ownership/lifetime
layout orchestration
hit-test traversal
EventDispatcher propagation
focus/capture infrastructure
render traversal
framework clipping
framework scrolling mechanics
text-input infrastructure
resource ownership infrastructure
```

---

## 4. Node

`Node` is the base runtime/component object.

It provides common framework state such as:

```text
Node::Id
parent relationship
tree ownership reference
visibility
enabled state
focusability
capturability
position / position mode
requested size
min/max size
desired size
actual position / size
padding
border
overflow
event handlers
```

It exposes lifecycle and presentation hooks including:

```text
update()
draw()
onMount()
onUnmount()
hitTest()
```

Layout measurement and arrangement remain framework-owned by `LayoutManager`.

### 4.1 Ownership

A Node does not own itself. Ownership is represented by:

```text
NodeTree root/overlay ownership
PanelNode child ownership
std::unique_ptr<Node>
```

`parent_` and `owner_` are non-owning references.

### 4.2 Identity

Every live node has a `Node::Id` and is registered by `NodeTree`.

The core identity invariant is:

```text
findNode(node.id()) == &node
```

for every live node.

---

## 5. PanelNode

`PanelNode` derives from `Node` and is the generic structural child container.

It provides:

```text
child ownership
child insertion/removal
child traversal
reverse traversal
visible-child access
hierarchy validation
```

It does not own the Measure/Arrange lifecycle.

A child cannot be attached when it already has a parent, already belongs to a
NodeTree, or would create a hierarchy cycle.

---

## 6. NodeTree

`NodeTree` is the central runtime structure.

It manages:

```text
roots
overlays
node ownership
attachment/detachment
live-node registration
NodeId lookup
lifecycle
traversal
deferred mutation
layout invalidation
update traversal
render traversal
hit-testing
```

The tree has two top-level owning collections:

```text
roots_
overlays_
```

Overlays are rendered and hit-tested above normal roots.

### 6.1 Mutation model

Structural mutation during protected traversal/callback execution is deferred.
`NodeTree` uses a mutation queue and snapshot-swap processing. A nested
`ScopedMutationGuard` prevents unsafe immediate structural changes.

This invariant applies to traversal and runtime operations that may invoke
callbacks capable of mutating the hierarchy.

### 6.2 Traversal

Internal traversal supports pre-order/post-order and:

```text
Continue
SkipChildren
Stop
```

Traversal works with mutation protection and live-node validation.

---

## 7. Lifecycle

The current lifecycle is approximately:

```text
Detached Node
     |
     | attach
     v
Owned by NodeTree / PanelNode
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
```

Roots, overlays and PanelNode children follow the same ownership and live-node
registration invariants.

---

## 8. Layout

`LayoutManager` is the framework-owned authority for measurement and
arrangement.

The current pipeline is:

```text
NodeTree layout queue
        |
        v
LayoutManager
        |
        +-- measureRecursive()
        |
        +-- arrangeRecursive()
```

Layout is currently invalidated at the nearest tree root and processed through
the framework-owned queue.

Clients do not implement or invoke Measure/Arrange lifecycle methods.

### 8.1 StackPanelNode

`StackPanelNode : PanelNode` provides the current one-dimensional flow model:

```text
Vertical
Horizontal
```

It measures and arranges children sequentially and stretches them across the
cross axis according to the current Phase 2 contract.

Full Grid/CSS/Flexbox compatibility is not part of the current architecture.

### 8.2 Positioning

The current positioning modes are:

```text
PositionMode::Layout
PositionMode::Absolute
```

Absolute positioning is a framework layout capability. It is not itself an
overlay system.

---

## 9. Input and Events

`InputManager` converts SDL input into framework interaction state and events.

Current responsibilities include:

```text
hover
focus
pointer capture
pressed state
drag state
mouse enter/leave
click
drag
mouse wheel
keyboard events
modal input restriction
```

Tracked nodes use Node pointers together with NodeIds so stale references can
be reconciled against the live NodeTree.

### 9.1 Hit testing

The current hit-test order is based on effective paint order, with overlays
above roots and reverse traversal selecting the topmost valid target.

`Overflow::HIDDEN` is an ancestor clipping boundary for hit testing and
rendering.

Clipping must not be confused with scrolling.

### 9.2 Focus

Focus is managed by InputManager.
Focus transitions dispatch:

```text
FocusLostEvent
FocusGainedEvent
```

The active modal root acts as a focus boundary in the current runtime
preparation.

### 9.3 Pointer capture

Pointer capture keeps the captured node as the primary pointer target during
an interaction even when the pointer leaves its bounds.

The current drag model uses:

```text
pressed node
captured node
press position
threshold
dragging state
```

The default drag threshold is currently `5.0f`.

### 9.4 EventDispatcher

Event propagation uses a target-to-root ancestry path and supports:

```text
TUNNELING
TARGET
BUBBLING
```

The propagation path stores NodeIds and resolves the current live Node at each
step. This works together with deferred mutation.

Current event families include mouse, drag, wheel, keyboard and focus events.

---

## 10. Rendering

SDL3 is the current concrete backend. There is no independent RenderContext or
backend abstraction.

The current render flow is approximately:

```text
NodeTree::draw()
      |
      +-- roots
      +-- overlays
      +-- top modal presentation
      |
      v
Node::draw()
      |
      v
recursive PanelNode rendering
```

`NodeTree::drawSubtree()` manages visibility and overflow clipping and preserves
renderer state around subtree rendering.

### 10.1 Clipping

`Overflow::HIDDEN` establishes an SDL clip rectangle from the node bounds.
Nested clipping rectangles are intersected.

This is a rendering/hit-test capability, not a complete scroll subsystem.

### 10.2 Animation

Animation is not a global mandatory subsystem. It is currently treated as a
mechanism for changing semantic/visual state through normal framework
semantics.

---

## 11. Primitives

The primitives layer provides low-level geometric rendering operations.

Conceptually:

```text
component presentation
        |
        v
primitives
        |
        v
SDL3 renderer
```

Primitives are intentionally not:

```text
component semantics
resource management
texture ownership
layout orchestration
input routing
```

Simple geometric component presentation should prefer primitives because this
allows resolution-independent sizing and dynamic state changes without texture
lifetime management.

Complex texture-based presentation should use a future Image/resource layer.
A component may eventually combine primitives and images.

---

## 12. Standard Component Layer

Phase 5 establishes the following active standard components:

```text
Button
ToggleButton
Menu
MenuItem
TabControl
TabItem
Checkbox
RadioButton
Slider
Dropdown
```

They are intentionally a small subset of a typical UI toolkit.

### 12.1 Checkbox

Checkbox owns checked state and presents the control using framework rendering
primitives. The checkmark is presentation, not a separate framework component.

### 12.2 RadioButton

RadioButton owns selected state. Group coordination remains explicit and is
not implemented through a global registry unless a future requirement proves
that such infrastructure belongs in the framework.

### 12.3 Slider

Slider owns numeric range/value/step state and uses the existing pointer
capture/drag infrastructure. Track, fill and thumb are presentation details,
not separate framework components.

### 12.4 Dropdown

Dropdown is a composite component using a trigger and menu semantics. Its
current implementation uses existing child composition and absolute
positioning.

It does not imply a global overlay subsystem.

A global popup/overlay system becomes justified only if concrete requirements
appear for behavior such as escaping parent clipping, root-level placement,
cross-subtree outside-click handling or global popup ordering.

---

## 13. Deferred Component Requirements

Phase 5 deliberately deferred components whose correct implementation depends
on unresolved framework infrastructure.

### 13.1 TextField / Input

Deferred until the framework defines:

```text
text-input events
composition / IME
caret
selection
editing commands
clipboard
focus/input lifecycle
```

KeyDown/KeyUp alone are not a sufficient text-input architecture.

### 13.2 Image

Deferred until the framework defines the smallest useful resource/texture
ownership contract.

Potential requirements include:

```text
resource lifetime
shared references/handles
texture loading boundary
source rectangle
fit/crop/scale
opacity/tint
flip/rotation
```

A generic ResourceManager must not be introduced merely as a container for
unrelated resources.

### 13.3 List

Deferred because no sufficiently distinct generic semantic contract has yet
been established beyond existing PanelNode/layout composition.

### 13.4 IconButton

Deferred until a stable image/icon resource contract exists.

### 13.5 Scroll / ScrollArea

Deferred until framework-level scrolling is designed.
The required concerns are:

```text
viewport
content extent
offset/range
coordinate conversion
clipping
wheel/gesture/drag routing
hit-test interaction
layout integration
nested scrolling
```

`Overflow::HIDDEN` alone is not scrolling.

### 13.6 Modal

The final Modal component is deferred to Phase 6.
Existing `ModalManager` infrastructure already provides preparation for:

```text
modal stack
active modal boundary
focus restoration
pointer cancellation
modal input restriction
```

The final Modal component must be designed on top of this infrastructure rather
than reviving the removed Widget-based implementation.

---

## 14. Non-Components

The following names are deliberately not standalone framework components:

```text
Paper
Label
Card
```

`Paper` is a surface/style concept.
`Label` is covered by `TextNode` / text presentation.
`Card` is a composition/style pattern that can be built from existing panels,
layout and visual properties.

These may be application-level compositions.

---

## 15. Modal Runtime Infrastructure

`ModalManager` maintains active modal sessions and coordinates with NodeTree
and InputManager.

A session records the modal NodeId and previous focus NodeId.

The top modal establishes the current modal interaction boundary.

Opening a modal can:

```text
remember current focus
cancel pointer interaction
find a valid focusable descendant
move focus into the modal
```

Closing a modal can:

```text
cancel pointer interaction
clear current focus
restore previous focus when valid
otherwise choose another valid focus target
```

These are current runtime capabilities/preparation. The final component-level
Modal API remains a Phase 6 concern.

---

## 16. UIManager and Frame Flow

`UIManager` is the public runtime facade and owns the major managers.

A normal frame is approximately:

```text
UIManager::runFrame()
        |
        +-- synchronize viewport
        +-- prepare tree operation
        +-- NodeTree::update()
        +-- LayoutManager::processLayoutQueue()
        +-- synchronize input/modal state
        +-- NodeTree::draw()
        +-- final synchronization
```

SDL events follow:

```text
SDL_Event
    |
    v
UIManager::processEvent()
    |
    v
InputManager
    |
    +-- hit-test
    +-- focus/capture
    +-- event generation
    |
    v
EventDispatcher
    |
    v
Node handlers
```

Tree mutations may be deferred during these operations and flushed at safe
points according to the mutation model.

---

## 17. Architecture Boundaries

The following boundaries are intentional:

```text
NodeTree
    ownership / lifetime / traversal / hierarchy authority

LayoutManager
    measurement / arrangement orchestration

InputManager
    input state / focus / capture / interaction routing

EventDispatcher
    propagation only

ModalManager
    active modal sessions and modal interaction boundary

Node / PanelNode
    runtime object and structural composition

Components
    reusable semantic UI behavior and presentation

Primitives
    low-level geometric rendering

SDL3
    concrete platform/input/rendering backend
```

No subsystem should absorb another subsystem merely to make a single component
implementation easier.

---

## 18. Current Architectural Non-Goals

The current framework does not attempt to provide:

```text
full CSS/Flexbox compatibility
large universal widget catalog
universal content model
global animation manager
generic ResourceManager without concrete need
multiple rendering backends
transform system
application-specific chess components
```

Future additions require concrete reusable framework justification.

---

## 19. Phase Boundary

Phase 5 is a component-development phase.
Its architectural result is the small standard component layer documented
above.

Phase 5 intentionally does not implement the unresolved infrastructure needed
by deferred components.

Current Phase 6 candidates are:

```text
Modality
Scrolling
Text input / editing
Image/resource management
Overlay/popup infrastructure if concretely required
full validation and testing
```

The final Phase 6 scope must be confirmed from the Phase 5 handoff documents
and source evidence before implementation begins.

This review copy intentionally does not promote every candidate into a
committed Phase 6 subsystem.
