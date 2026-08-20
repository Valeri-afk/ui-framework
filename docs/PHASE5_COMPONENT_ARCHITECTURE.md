# Phase 5 — Component Architecture

## Status

This is the **consolidated Phase 5 architecture checkpoint** for component design.

It contains the decisions we can currently treat as design guidance, plus explicitly unresolved implementation questions. It supersedes scattered discussion as the primary context document for new component work, while the more focused Phase 5 notes remain useful as supporting records.

Current branch:

```text
phase5-components
```

No compilation/tests/runtime validation before Phase 6.

---

# 1. Architectural goal

The framework is a retained-mode C++/SDL3 UI framework. It is not intended to become a browser DOM/CSS system, Qt, or WPF clone.

The architecture should provide enough centralized infrastructure that component authors do not need to reimplement generic UI mechanics, while keeping the framework small enough that custom component authors do not need to learn or manually coordinate a large collection of framework services.

The target balance is:

```text
Framework owns generic UI mechanics
            +
Components own domain semantics and presentation
            +
Minimal public contracts
            +
Concrete evidence before new abstractions
```

---

# 2. Framework vs component responsibility

## Framework responsibilities

The framework owns generic mechanisms whose correctness depends on the coordinated runtime:

```text
Node/NodeTree lifecycle and traversal
layout and geometry processing
hit-test traversal
input/event dispatch and propagation
focus/capture
render traversal and renderer orchestration
clipping
invalidation/update scheduling
backend/resource mechanics
scroll mechanics if/when scrolling is implemented as framework infrastructure
```

A component must not duplicate these mechanisms simply because it is complex.

## Component responsibilities

A provided component may:

```text
own domain/semantic state
implement component-specific state transitions
react to framework events
coordinate intentional internal children
choose component-specific presentation
contain substantial domain-specific logic
```

The key boundary is:

```text
component domain complexity
    !=
generic UI infrastructure
```

A component may be complicated without becoming a second framework.

---

# 3. Visual component definition

A visual component is a component whose primary responsibility is presenting state visually:

```text
state
  ↓
presentation
  ↓
render
```

Examples may include `TextNode`, future `ImageNode`, `IconNode`, and `Scrollbar`.

Not every component is purely visual. `Button`, `ToggleButton`, `Menu`, `TabControl`, `ListBox`, `Accordion`, and `Dialog` may own semantic/interactive/composite logic while continuing to use framework-owned generic mechanics.

---

# 4. Node and PanelNode

## Node

`Node` is the base runtime/component object.

It owns framework-recognized state such as:

```text
visible
enabled
focusable
capturable
position
positionMode
size
min/max size
padding
border
overflow
event handlers
actual/desired geometry
```

These properties are part of the generic Node contract and are interpreted by framework subsystems.

## PanelNode

`PanelNode` is a **structural/layout primitive**:

```text
PanelNode
    = Node
    + owned child structure
    + child/layout composition
```

It is not:

```text
VisualNode
ControlNode
universal content host
base class for every component with visual content
```

### PanelNode decision rule

Use `PanelNode` when the component genuinely needs:

```text
owned framework children
child geometry management
layout flow / child arrangement
structural composition
child-level composition semantics
```

Do **not** use `PanelNode` merely because a component has:

```text
text
icon
image
multiple visual primitives
complex visual appearance
```

Thus a `Button` may remain a `Node` even when it represents text + icon.

`ButtonGroup`, `Menu`, `TabControl`, `ListBox`, `Accordion`, and `Dialog` are plausible `PanelNode` candidates because structural child composition is central to their semantics.

The exact type of each concrete component follows implementation evidence.

---

# 5. Structural child vs semantic content

These are separate concepts.

```text
structural child
    = Node owned by a PanelNode

semantic content
    = content/child role intentionally defined by a specific component
```

Therefore:

```text
structural child != semantic content
```

The framework intentionally does **not** adopt:

```text
"everything is content of everything"
```

or a universal arbitrary-content tree.

A `PanelNode` can structurally own a `TextNode`, `MenuItem`, `Button`, or other Node if normal ownership rules permit it. Whether that relationship is semantically meaningful is determined by the containing component.

Examples:

```text
Menu
    → MenuItem has menu-specific meaning

TabControl
    → TabItem has tab-specific meaning

Button
    → may use TextNode internally if its design chooses to

Dialog
    → may contain Buttons/Text/other components
```

Generic `PanelNode` should not contain a registry of which component types are semantically legal children.

---

# 6. Specialized components remain Nodes

A specialized component such as:

```text
Button
IconButton
ToggleButton
MenuItem
TabItem
ListItem
Section
```

is still a Node or PanelNode according to its own composition needs.

It can therefore be a structural child wherever the generic Node ownership model permits.

A component may intentionally define stronger semantic expectations for its own children, without asking generic Node infrastructure to understand those domain rules.

---

# 7. Primitive Node rule

Not every visual primitive needs its own Node type.

A visual primitive should become a separate Node/component when it has sufficiently independent, reusable, or complex framework semantics that centralizing it prevents repeated implementation.

Current evidence:

```text
TextNode
    justified
    text measurement/layout
    font/alignment/wrapping
    SDL_ttf rendering/resource behavior

Border
    not a separate Node currently
    represented through Node box state
```

Future candidates:

```text
ImageNode
IconNode
```

These should be introduced only when concrete image/icon requirements establish a useful independent contract. Do not create them solely for symmetry.

---

# 8. Property ownership and Props structures

The framework does not need a public split such as:

```text
FrameworkProps
ComponentProps
```

The important architectural distinction is **semantic ownership and execution responsibility**, not necessarily physical storage.

For component authors, these are simply Node APIs:

```text
visible
border
padding
overflow
...
```

The framework internally knows which subsystems are affected.

### Framework-defined state

Properties with generic semantics independent of component type should remain Node/framework state.

Examples:

```text
visible
enabled
focusable
capturable
position
size
min/max
padding
border
overflow
```

Future likely framework-level properties/capabilities may include:

```text
opacity
borderRadius
transform
scroll state/capability
focus/navigation state
```

only when their generic semantics are established.

### Component-defined state

Properties whose meaning belongs to a concrete component remain in that component:

```text
Button.pressed
ToggleButton.checked
Menu.selectedItem
TabControl.selectedTab
ListBox.selection
Accordion.expandedSections
Fab-specific variant/state
```

Do not add these to Node merely to make components look uniform.

### Props structures

A Props structure is not required simply because a group of properties belongs to one architecture category.

Use a separate structure when it represents a genuine reusable/cohesive concept, for example:

```text
Padding
Border
Layout constraints
Stack layout policy
future shared visual/appearance data if semantics are truly identical
```

Component-specific fields may remain direct members when a separate structure adds no real value.

---

# 9. Layout properties vs layout-policy properties

The current `Node` owns generic geometry/layout state.

`PanelNode` owns child structure, but does not need a separate universal `PanelProps` block merely because it is a container.

Concrete layout primitives may own their own policy properties.

Current example:

```text
StackPanelNode
    orientation
gap
    mainAlignment
    crossAlignment
```

Future layout primitives may have independent policies such as grid definitions or other flow rules.

This keeps layout policy at the primitive that actually implements it and avoids turning `Node` into a CSS-like universal layout object.

---

# 10. Visual/box properties

Some generic visual properties are also framework-relevant geometry properties.

Current/future examples:

```text
background
border
borderRadius
opacity
transform
```

Do not classify all of them as pure cosmetic style.

In particular:

```text
border
borderRadius
transform
```

may affect geometry, clipping, or hit-test behavior depending on the eventual implementation.

`opacity` is primarily rendering state and should not automatically imply `visible == false` or disable hit-testing.

The framework is responsible for consistent generic consequences once these properties exist.

---

# 11. Component state → framework consequences

The intended retained-mode pattern is:

```text
imperative mutation
    ↓
retained semantic state
    ↓
component-specific logic
    ↓
framework-recognized Node state/capabilities when needed
    ↓
normal update/layout/hit-test/render pipeline
```

The framework does not automatically interpret arbitrary component-defined properties as if they were CSS declarations.

Example:

```text
Accordion.expanded
    ↓
component changes its intended composition/framework state
    ↓
framework applies layout/visibility/render consequences
```

This provides a state-driven model while remaining an explicit C++/retained-mode architecture.

---

# 12. Common action components

The current working component family is:

```text
Button
    general one-shot action

IconButton
    specialized icon-only action candidate

ToggleButton
    action + persistent checked/selected state

Fab
    specialized action/presentation candidate
```

Their current candidate base is `Node`, not `PanelNode`, unless concrete composition requirements prove otherwise.

Do not introduce a generic `ActionNode` merely because these components respond to pointer input. Generic input/hit-test/focus/capture infrastructure is already a framework responsibility.

Shared internal implementation may be introduced later if concrete code demonstrates stable repeated behavior.

### Content variants

Do not automatically create:

```text
TextButton
TextIconButton
IconTextButton
```

merely because content differs.

A general Button may support text, icon, or text+icon as variants of one semantic Button.

An `IconButton` may still be a distinct public component when it offers a genuinely specialized semantic/presentation contract. Its inheritance relationship with `Button` remains an implementation decision, not an architectural requirement.

---

# 13. Composite/action groups

Group components are different from action components because structural child layout is central to them.

Examples:

```text
ButtonGroup : PanelNode
ToggleButtonGroup : PanelNode
Menu : PanelNode
TabControl : PanelNode
ListBox : PanelNode
Accordion : PanelNode
Dialog : PanelNode
```

The exact implementation should follow the concrete component semantics.

Parent-owned semantic state may naturally coordinate child state:

```text
Menu.selectedItem
TabControl.selectedTab
ListBox.selection
Accordion.expandedSections
```

This does not require a generic selection/composite base class.

---

# 14. Custom component developer experience

A central quality criterion is the burden placed on **custom component authors**, not only end clients.

Avoid architectures where custom authors must:

```text
create/configure generic services
inject those services into components
manage service lifetime
coordinate renderer/layout/input subsystems manually
```

For example, a public `TextService` dependency that each custom component must instantiate is considered an undesirable direction.

Reusable low-level implementations may exist internally:

```text
TextNode / Button / Toggle / MenuItem
    ↓
shared internal text machinery
```

but the author of a normal component should not need to know or manage that machinery unless the public contract genuinely requires it.

Similarly, the framework should centralize generic scrolling mechanics rather than requiring component authors to implement scrolling coordinate conversion, clipping, hit-testing, and input themselves.

### Complexity budget

Every framework-wide abstraction should justify its new developer-facing contract:

```text
What repeated problem does it remove?
What correctness does centralization provide?
How many components benefit?
What new rules must custom component authors learn?
Can a smaller internal helper solve it?
```

Prefer the minimum sufficient abstraction.

---

# 15. Scroll architecture

Scrolling remains an explicit open implementation topic.

Current working preference:

```text
Framework subsystem
    → scroll mechanics

Node/container state or capability
    → declares/configures scrollability

Scrollbar
    → optional visual component

ScrollContainer
    → optional convenience composition
```

The framework may need to coordinate:

```text
scroll offset
content extent
viewport extent
scroll range
input
coordinate conversion
clipping
hit-test integration
layout integration
render translation
```

Do not assume a fundamental `ScrollNode` type is required.

### Open question

The representation is still unresolved:

```text
Overflow::SCROLL
```

vs

```text
separate scroll capability/state
```

Return to this question when the concrete scrolling implementation is designed.

---

# 16. Material UI as reference material

Material UI may be used as a source of concrete component behavior and composition examples.

It is not an architectural specification.

Its React/DOM/CSS environment provides capabilities that this framework does not automatically have, so examples must be adapted to the retained-mode C++ runtime.

Useful questions to extract from references include:

```text
What semantic states exist?
Which variants are genuinely separate components?
Which differences are merely content/presentation variants?
Which pieces are parent-owned state?
Which parts are structural composition?
```

---

# 17. Deliberate non-goals

The current framework is not trying to become:

```text
full CSS cascade engine
universal declarative content system
selector system
computed-style engine
property registry
capability registry
mandatory service-injection architecture
Qt/WPF-scale control hierarchy
```

Do not introduce these merely to reproduce browser/framework features.

---

# 18. Decision rules for future abstractions

1. Start from concrete component requirements.
2. Introduce a shared abstraction only after multiple concrete cases demonstrate stable shared semantics or implementation responsibility.
3. Do not create inheritance merely because another UI framework has a corresponding class.
4. Do not make `PanelNode` the common base of visually rich components merely because they contain multiple primitives.
5. Do not introduce a universal content tree to solve a small number of content cases.
6. Prefer internal reusable helpers over public services when the capability is implementation machinery rather than component semantics.
7. Keep framework-wide semantics where correctness depends on coordinated subsystems.
8. Keep component-specific semantics in the component.
9. Keep the custom component author contract small.
10. Prefer the smallest architecture that still provides correct reusable behavior.

---

# 19. Current implementation status

Established foundation:

```text
Node
PanelNode
StackPanelNode
TextNode
```

Historical/experimental and not accepted as final component architecture:

```text
ControlNode
legacy src/components/* implementations
```

Concrete component design candidates:

```text
Button
ToggleButton
IconButton
Fab
Menu/MenuItem
TabControl/TabItem
ListBox/ListItem
Accordion/Section
Dialog/Modal
```

Primitive candidates requiring concrete validation:

```text
ImageNode
IconNode
```

Separate framework topic:

```text
scrolling
```

No component implementation is considered final until it has been derived from the above rules and concrete requirements.
