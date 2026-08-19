# Phase 5 — Component Design Notes

## Status

This document records **working hypotheses and analysis**, not final framework contracts. It exists to preserve context while the first concrete Phase 5 components are designed.

Current branch:

```text
phase5-components
```

`main` is the Phase 4 source-level baseline and must not be changed directly during Phase 5 work.

---

## 1. Starting position

Phase 5 is the Component Model phase. Runtime, layout, input/events and rendering/backend infrastructure are considered established from Phases 1–4.

The current source already contains active-core `Node`, `PanelNode`, `StackPanelNode`, `TextNode` and `ControlNode` types, but their status is not equivalent:

- `Node`, `PanelNode`, `StackPanelNode` are established infrastructure;
- `TextNode` is an established concrete content node;
- `ControlNode` is a historical WPF-inspired experiment and is **not** an accepted final base-component architecture.

The legacy `src/components/*` and `include/ui_framework/components/*` implementations are historical evidence only and must not be mechanically treated as the Phase 5 architecture.

---

## 2. No final common component base has been chosen

The following hierarchy is intentionally **not** accepted as a framework contract:

```text
Node
  ↓
ControlNode
  ↓
Button / Toggle / ...
```

`ControlNode` currently adds `StyleProps` and rectangle-style drawing, which is not sufficient evidence that all future components need that common abstraction.

Similarly, an `InteractiveNode` base class has not been accepted.

The project should derive abstractions from concrete repeated semantics rather than from symmetry with WPF, Qt, React/Material UI, or another framework.

---

## 3. Capability analysis hypothesis

Initial Phase 5 capability groups are:

```text
Content / resource components
    Text
    Image

Interactive components
    Button
    Toggle

Composite/container components
    Menu / MenuItem
    TabControl / TabItem
    ListBox / ListItem
    Accordion / Section
    Scroll-related container/capability
    Modal / Dialog
```

This does not prescribe inheritance.

A shared capability may ultimately be implemented through:

- direct inheritance;
- composition;
- a helper/state object;
- existing framework infrastructure;
- no abstraction if the repeated behavior is too small or unstable.

The correct choice is determined from concrete component designs.

---

## 4. Component semantic state vs presentation

A useful **working hypothesis** is:

```text
Component semantic state
        ↓
component internal composition/state updates
        ↓
visual presentation
        ↓
rendering
```

The intended advantage is that a client interacts with a component through its public semantic API rather than manually synchronizing the visual properties of internal children.

Example hypothesis:

```text
Menu
    selectedItem / open state
        ↓
MenuItem semantic/presentation state
```

The same possible pattern may apply to:

```text
TabControl → TabItem
ListBox    → ListItem
Accordion  → Section
Modal      → Dialog/internal content
```

### Important qualification

This is **not yet a framework-wide contract**.

A component is allowed to expose whatever public API its design requires. A composite component may provide controlled access to the state of its internal/child components when that is natural for the component itself.

The framework must not require developers to follow one universal component-authoring pattern.

This approach should be evaluated by designing real components. It becomes an architectural pattern only if it consistently reduces complexity and remains natural across multiple components.

---

## 5. Client responsibility hypothesis

The intended experiment is:

```text
Client
    ↓
public semantic component state/actions
    ↓
Component
    ↓
internal state / child coordination
    ↓
rendering derived from current state
```

The client should not need to know how a provided framework component synchronizes its internal visual representation.

For example, a client-facing Button should expose button semantics rather than require the client to coordinate background, border, text and child-node state manually.

This is a design goal to test, not yet a universal framework rule.

---

## 6. Framework mechanics vs component responsibility

A key Phase 5 working model emerged from comparing browser-style UI environments with this SDL3/C++ retained-mode framework.

The browser provides a large amount of generic UI infrastructure through the platform/runtime: layout, style application, overflow/scrolling, focus, event routing, repaint/reflow and related mechanics. A component can often declare state and let that environment interpret the state.

In this framework, those mechanics are explicit subsystems. Therefore the framework should continue to own **generic UI execution mechanics**, while provided components define **domain semantics and presentation**.

Working model:

```text
                    FRAMEWORK
────────────────────────────────────────────
Node / PanelNode
Layout
Hit-test
Input / events
Focus / capture
Rendering traversal
Clipping
Scroll mechanics (when implemented)
Resource/backend mechanics
Invalidation
────────────────────────────────────────────
                       ↑
                       │
                framework state/capabilities
                       │
                       ↓
                    COMPONENT
────────────────────────────────────────────
Domain semantic state
Component-specific state transitions
Composition
Presentation
────────────────────────────────────────────
                       ↓
                    RENDER
```

### Component responsibility

A provided component may:

- own its semantic/domain state;
- implement component-specific state transitions;
- coordinate internal/composite children when its own API is designed to do so;
- derive presentation from current state;
- respond to framework events;
- contain meaningful domain-specific logic;
- use existing framework capabilities/properties.

A provided component should not duplicate generic framework infrastructure such as:

- NodeTree traversal;
- generic event dispatch/propagation;
- global focus/capture management;
- generic hit-test traversal;
- renderer-state orchestration;
- generic clipping;
- layout engine implementation;
- framework-owned scrolling mechanics.

This is a **working design principle**, not yet a mandatory constraint on custom component authors.

### Visual component

A visual component is a special case whose main responsibility is presentation:

```text
state
  ↓
presentation
  ↓
render
```

Examples may include `Text`, `Image`, `Icon` and a future `Scrollbar`.

Not every component is purely visual. `Button`, `Toggle`, `Menu`, `TabControl`, `ListBox`, `Accordion` and `Dialog` may contain semantic/interactive/composite logic while still relying on framework mechanics.

The important boundary is:

```text
component domain complexity
    ≠
generic UI infrastructure
```

A component may be sophisticated without becoming a second framework.

---

## 7. Framework-recognized state/capabilities

The framework already recognizes a set of properties that have system-level semantics, for example:

```text
visible
enabled
focusable
capturable
position
size
padding
border
overflow
```

These are not merely component styling fields. The framework interprets them across its existing subsystems.

Working extension model:

```text
component-defined semantic state
        ↓
component logic
        ↓
framework-recognized state/capability changes
        ↓
existing framework subsystems execute consequences
```

Example:

```text
Accordion.expanded
    ↓
component changes relevant framework-visible state/composition
    ↓
framework layout / visibility / rendering semantics apply
```

The framework should not attempt to know every property of every future component. Only properties/capabilities with generic system-level meaning belong to framework infrastructure.

This is why a component-specific property such as `Button.variant` need not become a universal framework property.

---

## 8. Imperative retained-mode implications

The framework is not a browser DOM/CSS engine and does not parse arbitrary component-specific markup.

A client action such as:

```text
accordion.setExpanded(true)
```

must therefore update retained state explicitly. The framework does not infer arbitrary downstream semantics merely from the existence of the property.

A useful pattern is:

```text
imperative mutation
    ↓
retained semantic state
    ↓
component logic
    ↓
framework-recognized state/capability changes
    ↓
next normal framework update/layout/render pipeline
```

This is intentionally state-driven while remaining an imperative C++ API. It does not require reproducing React or browser declarative machinery.

The framework's job is to make the common low-level consequences reliable once a component maps its semantic state to recognized framework state/capabilities.

---

## 9. Scroll architecture — working hypothesis, not final decision

Scrolling surfaced a special case where the browser model is informative but cannot be copied directly.

The current working preference is:

```text
Framework subsystem
    → scroll mechanics

Node / container capability
    → declares/configures scrollability

Scrollbar
    → optional visual component

ScrollContainer / similar convenience component
    → optional composition helper
```

The framework subsystem would be responsible for generic low-level behavior such as:

- scroll offset;
- content extent;
- viewport extent;
- scroll range;
- scrolling input;
- coordinate conversion;
- clipping/translation integration;
- hit-test integration;
- layout integration.

A visual scrollbar should not own these mechanics. It would consume/represent scroll state.

### Open question to revisit

It is not yet decided whether scrollability should be represented directly through:

```text
Overflow::SCROLL
```

or through a separate scroll capability/state API. The latter may be semantically cleaner if scrolling needs to be independent of presentation-oriented overflow policy.

This question is intentionally preserved for later review.

### Important non-decision

Do not assume a `ScrollNode` must exist. Scrolling may be a framework capability of an existing container rather than a distinct fundamental Node type.

---

## 10. Material UI as reference material

Material UI may be used as a source of **concrete component behavior and composition ideas** when useful:

```text
Material UI example
    ↓
behavior/composition reference
    ↓
compare with this framework's retained-mode C++ architecture
    ↓
adapt or reject
```

Material UI is not treated as an architectural specification or hierarchy template.

Its React/DOM/CSS environment provides capabilities that this framework does not have automatically. Concrete examples therefore require adaptation rather than mechanical translation.

The same applies to other UI frameworks.

---

## 11. Architecture evidence gathered so far

Concrete cases explored:

```text
Button
    → own transient interaction/semantic state + presentation

Toggle
    → own persistent semantic state + interaction/presentation

Menu / MenuItem
    → parent-owned relation/selection semantics can naturally coordinate children

TabControl / TabItem
    → parent-owned single selection semantics

ListBox / ListItem
    → parent-owned single/multi selection semantics

Accordion / Section
    → parent-owned expansion semantics with layout consequences

Scroll
    → strong candidate for framework behavior + optional visual representation,
      rather than a self-contained visual scroll implementation
```

This evidence currently argues **against** introducing universal base classes such as:

```text
ControlNode
InteractiveNode
SelectableNode
CompositeNode
```

merely for hierarchy symmetry.

That conclusion is still provisional until concrete components are designed/implemented enough to reveal actual repeated implementation responsibilities.

---

## 12. Decision rule for Phase 5 abstractions

Do not introduce an abstraction because:

```text
another framework has it
```

or because:

```text
multiple components have superficially similar code
```

Introduce an abstraction only when concrete component designs demonstrate a stable shared semantic responsibility that benefits from a single implementation or contract.

The minimum sufficient abstraction is preferred.

Component hierarchy should emerge from concrete responsibilities rather than precede them.

---

## 13. Immediate continuation point

The next step is to stop expanding the abstract taxonomy and begin **concrete Phase 5 component design/implementation scope**.

The first candidates remain:

```text
Button
Toggle
Text / Image where needed
```

Composite components such as Menu, Tabs, ListBox and Accordion should be considered after the base interactive/content component behavior is concrete enough to determine what, if anything, is reusable.

Scrolling remains an architectural topic to revisit separately; its framework capability vs `Overflow::SCROLL` representation is explicitly unresolved.

No compilation/tests/runtime validation before Phase 6.
