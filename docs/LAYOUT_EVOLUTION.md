# Layout Architecture Evolution

> **Status:** historical context and architectural rationale
> **Date recorded:** 2026-08-18
>
> This document records the evolution of the framework's layout architecture and the problems that motivated each transition. It exists so future development discussions can start from the accumulated reasoning rather than reconstructing it from memory.

## 1. Why this document exists

The current layout architecture was not chosen in a single step. It emerged from several iterations that each solved one problem while exposing another.

The important lesson is that the present Phase 2 discussion is not simply about choosing between Grid, Flex, StackPanel, Measure/Arrange, or a central LayoutManager. The deeper problem is the boundary between:

- framework-owned runtime/lifecycle/layout machinery;
- user-defined component behavior;
- text measurement/rendering;
- container-specific layout behavior;
- layout invalidation.

Future changes should evaluate this boundary explicitly.

---

## 2. First model — one large Widget

The earliest framework design used a single Widget abstraction that held many responsibilities together:

- lifecycle;
- parent/child relationships;
- runtime state;
- layout state;
- other UI properties.

There was also a separate layout engine.

This made framework-level processing straightforward because the framework always dealt with the same Widget abstraction.

The cost appeared on the client side: when new components were implemented by inheritance, derived components inherited and/or carried layout-related behavior and properties even when those properties were irrelevant to the component.

A Button, for example, could end up conceptually carrying container-oriented responsibilities simply because the common base object was responsible for everything.

This motivated investigation of a more WPF-like separation of concerns.

---

## 3. Second model — split Widget responsibilities

The next direction attempted to split the monolithic Widget into several more specialized base concepts so that lifecycle, layout, parent/child and other responsibilities could be represented separately.

The problem was framework-side processing.

The runtime largely operated on the Widget abstraction, so once behavior was distributed across unrelated or differently derived types, the framework needed ways to discover which capabilities a concrete object provided. This led toward repeated type checks and `dynamic_cast`-style probing.

That was considered architecturally undesirable because:

- framework code became coupled to concrete capability types;
- processing was no longer expressed cleanly through one runtime abstraction;
- the inheritance hierarchy became part of framework control flow.

The design therefore returned to a single primary runtime object rather than continuing with a fragmented Widget hierarchy.

---

## 4. Third model — remove the layout engine and let components define layout

The next experiment moved in the opposite direction.

Instead of a closed layout engine, individual user-defined components could describe their own layout algorithm. This gave component authors maximum freedom and allowed unusual components to define exactly the geometry behavior they needed.

This model was attractive because it also made special content types, including text, easy to reason about locally: a component could implement its own measurement/layout behavior without requiring the central layout engine to understand every component-specific case.

However, the model exposed a major problem: **layout invalidation became part of the client contract**.

A custom component author had to understand and correctly maintain framework-level rules such as:

- when layout becomes dirty;
- which ancestor must be invalidated;
- when to call `markLayout`/equivalent mechanisms;
- how layout mutations interact with traversal and runtime mutation;
- how the component's custom algorithm fits the framework lifecycle.

A correct component therefore required knowledge of internal framework contracts. A mistake could make layout stale or break other runtime behavior.

This was considered too much responsibility for normal framework users.

---

## 5. Text became the critical pressure point

The strongest practical example was text.

A closed layout engine initially became difficult to combine cleanly with text measurement and text-specific behavior. Text measurement depends on content, font, renderer/text backend state and wrapping constraints, while layout needs the measured result.

Trying to put both concerns into one closed layout engine caused responsibilities to mix.

The framework therefore experimented with a separate text engine service exposed to the client. This separated text measurement/rendering concerns technically, but it was inconvenient for framework users because clients had to know about and coordinate an additional text service.

That was not considered an ideal public model.

---

## 6. Text component experiment

A later experiment removed the explicit text-engine service from the client-facing model and introduced a dedicated text component/Label-like object.

The text component itself could own the text-related behavior and participate in layout. Instead of treating raw text as a special service-level concern, text became a UI component that could measure itself.

The historical `src/components/label.cpp` is evidence of this direction. It contains text measurement based on available width, wrapping behavior, text-specific alignment and renderer-backed text state. This code is legacy and is **not** part of the current architecture, but it records an important idea: text can be a component-local measurement concern rather than something the client must manually orchestrate through a separate text service. fileciteturn118file0

The component/Node architecture used by that implementation is also historical and should not be copied directly.

---

## 7. Fourth model — Measure / Arrange

The next direction generalized the useful part of the text-component idea.

Instead of forcing the central layout engine to understand every component's internal sizing needs, the framework adopted a Measure / Arrange model:

```text
Measure
    -> component reports desired size

Arrange
    -> framework/container assigns final geometry
```

This allowed a text component to implement component-specific measurement while allowing containers to implement their own child arrangement.

The model solved an important architectural problem:

- text measurement could remain component-specific;
- arbitrary containers could define custom geometry rules;
- the framework could still orchestrate the overall layout traversal.

However, it reintroduced the client/framework contract problem in a different form.

A custom component author now had to understand at least:

- MeasureContext;
- ArrangeContext;
- constraints;
- desired size;
- final size/position;
- layout invalidation;
- framework lifecycle/mutation boundaries.

The contract was narrower than in the completely open custom-layout model, but it still existed.

---

## 8. Current Phase 1 / Phase 2 starting point

Phase 1 subsequently stabilized the runtime around:

- one primary `Node` runtime abstraction;
- framework-owned node lifetime;
- `NodeId` identity/liveness;
- deferred mutation;
- mutation-safe traversal;
- `NodeTree` as the runtime authority.

The current layout foundation is built on top of that runtime.

It already contains concepts such as:

- Measure / Arrange;
- desired size;
- actual size and position;
- minimum/maximum size;
- padding/border;
- StackPanel;
- `PositionMode`;
- layout invalidation queue.

The open architectural question for Phase 2 is therefore not simply how to implement Grid or Flex. It is:

> **How much of the layout contract should be visible to user-defined components, and can the framework hide almost all of it while still supporting components that need custom measurement or custom container behavior?**

---

## 9. Desired direction

The long-term user-facing goal is CSS-like in one important sense:

> A normal framework user should configure layout through properties and container configuration and should not need to understand the layout engine itself.

The developer should ideally think in terms of:

- width/height;
- min/max;
- padding/margin;
- alignment;
- position;
- row/column/grid properties;
- stack/flex properties;
- text/content properties.

The framework should own:

- invalidation;
- layout scheduling;
- Measure/Arrange orchestration;
- constraint propagation;
- geometry calculation;
- lifecycle interaction;
- runtime/mutation safety.

A custom layout extension may still be required for advanced users, but it should be a narrow extension point that does **not** expose framework lifecycle or invalidation machinery.

---

## 10. Important architectural tension

There is a fundamental tension that future Phase 2 work must keep explicit:

```text
More framework-controlled layout
    -> less client knowledge
    -> safer and simpler client API

More user-defined layout algorithms
    -> more expressive custom components
    -> larger extension contract
```

The goal is not necessarily to remove all layout contracts from existence. The goal is to make the normal client contract property-based and framework-owned, while isolating any unavoidable custom-layout contract behind a specialized extension boundary.

---

## 11. Legacy references

The following legacy code may be useful for historical analysis but must not be treated as current architecture:

- `src/components/flex_panel.cpp` — historical flex-like layout implementation with main/cross-axis alignment, gaps, wrapping and grow/shrink behavior. It is useful for understanding previously needed layout capabilities and edge cases, but its implementation style predates the current runtime architecture. fileciteturn117file0
- `src/components/label.cpp` — historical text component combining text measurement, wrapping, text alignment and rendering. It is evidence for the component-local text measurement idea, not a current API contract. fileciteturn118file0

No legacy component file should be modified as part of Phase 2 merely to preserve compatibility with those experiments.

---

## 12. Current working hypothesis

The current architectural hypothesis to investigate is:

```text
Node / PanelNode
        |
        +-- universal layout properties
        |
        +-- container-specific layout metadata
        |
        +-- framework-owned layout strategy

LayoutEngine
        |
        +-- invalidation
        +-- scheduling
        +-- Measure
        +-- Arrange
        +-- constraints
        +-- geometry
```

This is a hypothesis, not yet an accepted implementation contract.

Before further Phase 2 implementation, this hypothesis must be tested against:

- text measurement;
- leaf components;
- Stack/Row/Column behavior;
- Grid/Flex behavior;
- custom container requirements;
- invalidation;
- absence of `dynamic_cast`-driven framework dispatch;
- avoidance of a monolithic Node API.

The Phase 2 implementation should follow whichever model survives that analysis, rather than treating the current Measure/Arrange implementation as automatically final.
