# Phase 5 — Content / Composition Model

## Status

This document records the current **working architectural model** for component content and composition. It is not a final universal content contract.

Current branch:

```text
phase5-components
```

The purpose is to prevent future component implementations from accidentally assuming either a universal `content` model or a fixed leaf/container hierarchy before those choices are justified.

---

## 1. Why this question exists

The framework is a retained-mode C++/SDL3 system, not a browser DOM/CSS environment.

The browser can treat nearly everything as nested content while the platform/runtime handles layout, text, images, overflow, focus, rendering and other consequences.

This framework instead has explicit Nodes and explicit framework subsystems. Therefore a universal model in which every component may contain every other component would introduce substantial new semantics and is not required by the current architecture.

The current design intentionally favors:

```text
explicit Node tree
+
component-defined composition semantics
+
framework-owned low-level mechanics
```

rather than:

```text
universal "everything is content of everything" system
```

---

## 2. Structural child vs semantic content

These are related but distinct concepts.

### Structural child

A `PanelNode` can own child `Node` objects.

```text
PanelNode
    ↓
child Node
```

This is a runtime/ownership relationship.

### Semantic content

A component may define which children, primitives or state objects represent its own UI meaning.

```text
Menu
    → MenuItem children have menu meaning

Button
    → text/icon/content may have button meaning
```

Therefore:

```text
structural child
    ≠
semantic content
```

A framework does not need to globally prevent structurally legal combinations simply because they are semantically unusual.

---

## 3. Node and PanelNode

Current working definitions:

```text
Node
    = base runtime/component object

PanelNode
    = Node + structural child ownership/composition
```

`PanelNode` is not synonymous with:

- visual component;
- control;
- arbitrary content host;
- composite component;
- universal parent class for all components.

A component becomes a `PanelNode` when structural child composition is useful to its implementation or public semantics, not merely because it can display text or an image.

Likewise, a `Node` can represent a complete component without children.

---

## 4. Primitive components

A visual primitive does not automatically need its own Node type.

A primitive should become a distinct Node/component when its semantics are sufficiently independent or complex that a separate framework object provides clear value.

Current examples:

```text
Text
    → separate TextNode is justified by measurement, font, layout and SDL_ttf rendering/resource semantics

Border
    → currently remains Node state/properties rather than a separate BorderNode
```

The important distinction is not:

```text
visual = component
```

but:

```text
independent/complex reusable semantics
    → may justify a component/Node
```

---

## 5. TextNode as the current reference model

`TextNode` already demonstrates the desired framework direction:

```text
TextNode
    ↓
semantic text state
    ↓
framework measurement/layout support
    ↓
renderer-bound SDL_ttf representation
    ↓
render
```

The existence of `TextNode` is intentional. It keeps text-specific layout and rendering complexity inside framework-provided infrastructure so custom component authors do not need to reproduce text measurement and SDL_ttf handling manually.

This is not evidence that every component must contain a `TextNode`, nor that every content type must receive a universal content system.

---

## 6. A component may use another Node as internal composition

Examples that are considered architecturally valid:

```text
Button
    └── TextNode

Menu
    ├── MenuItem
    ├── MenuItem
    └── MenuItem

Dialog
    └── Button
```

The containing component decides whether the child is:

- an implementation detail;
- part of public component composition;
- a semantic child with component-specific meaning.

The generic NodeTree does not need to know all of these semantics.

---

## 7. Specialized components remain Nodes

A specialized component such as:

```text
MenuItem
TabItem
Section
Button
Toggle
```

can remain a normal `Node` or `PanelNode` according to its own implementation needs.

Therefore a specialized component is structurally eligible to be a child wherever normal `PanelNode` ownership permits:

```text
PanelNode
    └── MenuItem
```

This does **not** mean that the resulting structure is semantically meaningful for every component.

For example, `Menu` may define that its meaningful semantic children are `MenuItem`; generic `PanelNode` should not encode a global registry of such rules.

Component-specific semantic restrictions belong to the component design.

---

## 8. "Any primitive can be content" — qualified interpretation

The current working interpretation of the idea is:

> Any framework Node can be structurally composed as a child of a `PanelNode`, but whether that child is valid semantic content is decided by the containing component's own design.

Therefore these are conceptually distinct statements:

```text
TextNode may be a child of Button
```

and:

```text
TextNode is universal content for every component
```

The first is accepted as normal Node composition.
The second is deliberately not introduced.

Likewise:

```text
MenuItem can exist as a Node outside Menu
```

is structurally valid.

Whether `MenuItem` should expose useful independent semantics outside `Menu` is a separate component-level API decision.

---

## 9. The containing component defines composition semantics

A component may choose that a particular child type is part of its semantic design.

Examples:

```text
Menu
    → MenuItem

TabControl
    → TabItem

ListBox
    → ListItem

Accordion
    → Section
```

This does not require a generic `ContentNode`, `CompositeNode`, or content registry.

Similarly, a component can choose not to expose arbitrary child composition even if its implementation uses internal child Nodes.

The public API should reflect the semantics the component intentionally provides.

---

## 10. No universal content tree for now

The following architecture is intentionally deferred:

```text
Every component
    → accepts arbitrary content
        → any Node
            → any Node
                → ...
```

Such a model would require additional decisions around:

- ownership;
- measurement;
- arrangement;
- semantic validity;
- public child APIs;
- hit-test behavior;
- accessibility/focus semantics;
- replacement/removal rules;
- content typing.

Those costs are not currently justified by Phase 5 requirements.

The framework may evolve toward richer composition later if concrete requirements demonstrate the need.

---

## 11. Button implication

The `Button` leaf/container question is now understood as a consequence of the general composition model, not a Button-specific rule.

A Button could be implemented as:

```text
Button : Node
```

if its content is represented by internal semantic data/shared text/image machinery and it does not expose child composition.

Or:

```text
Button : PanelNode
```

if its implementation or public design naturally uses real child Nodes such as:

```text
Button
├── TextNode
└── ImageNode
```

Neither choice should be made solely because Button has text.

The concrete choice must follow the component's intended composition semantics.

---

## 12. Button variants

The current working model is:

```text
Button
    → one semantic button component
```

Text-only, icon-only or text+icon presentation should not automatically produce a proliferation such as:

```text
TextButton
IconButton
TextIconButton
```

An `IconButton` may still become a specialized component if it has distinct semantic or interaction requirements; that remains open.

The key distinction is:

```text
content/presentation variant
    ≠
semantic component type
```

The same distinction applies to other component families.

---

## 13. Text applies to many component families

The question of whether a component containing text should own a `TextNode` or internal text machinery is not unique to Button.

Potential examples include:

```text
Button
Toggle
MenuItem
TabItem
ListItem
Accordion header/Section
Dialog title/content
```

The current framework should not force every such component into one universal inheritance structure.

Instead, the component design determines whether text is:

- semantic component state;
- an internal `TextNode` child;
- shared internal text machinery;
- part of a broader component composition.

The framework should provide the reusable low-level text mechanics without requiring custom component authors to recreate them.

---

## 14. Framework vs component responsibility

The content model follows the Phase 5 working division:

```text
Framework
    → Node ownership/lifecycle
    → layout
    → hit-test
    → input/events
    → rendering traversal
    → clipping
    → renderer/resource mechanics
    → future scroll mechanics

Component
    → domain semantics
    → presentation
    → component-specific state transitions
    → intentional child composition
```

The component may contain substantial domain logic, but it should not become a second implementation of framework infrastructure.

---

## 15. Custom component freedom

This model is not a requirement that every custom component must use a specific composition style.

A developer may create:

```text
custom leaf Node
custom PanelNode component
component with internal child Nodes
component with no child Nodes
```

according to the actual component semantics.

Framework-provided components may adopt a coherent composition pattern when useful, but the framework does not require all custom authors to reproduce the same hierarchy.

---

## 16. Current open decisions

The following remain intentionally unresolved:

1. Whether common text-capable implementation should remain entirely internal or become a reusable component-level abstraction.
2. Whether certain content primitives such as Image should receive their own Node type.
3. Whether Button should be a `Node` or `PanelNode` once its content/composition API is designed concretely.
4. Whether `IconButton` is a presentation variant of Button or a separate semantic component.
5. How far component composition should be exposed publicly versus kept as implementation detail.
6. Whether scrolling should be a framework capability on existing containers or exposed through a dedicated container component.

These questions must be resolved from concrete implementation evidence rather than from hierarchy symmetry.

---

## 17. Decision rule

The Phase 5 component model should prefer:

```text
explicit Node structure
+
component-defined semantic composition
+
framework-owned generic mechanics
+
minimum sufficient abstraction
```

and avoid both extremes:

```text
"everything is content of everything"
```

and:

```text
"every component reimplements every content/rendering mechanism itself"
```

This is the current direction for further Phase 5 work.

No compilation/tests/runtime validation before Phase 6.
