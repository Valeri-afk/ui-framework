# Phase 6 — Modality Requirements from Legacy Modal

This document records requirements extracted from the legacy `Modal` implementation. It is a design input for Phase 6, not a Phase 5 implementation plan.

## 1. What the legacy Modal actually contained

The old component combined several responsibilities:

```text
visual
    modal background/color
    border radius/padding
    backdrop rendering
    transition/fade effect

interaction
    close on mouse click
    close on Escape

framework behavior
    modal-exclusive input
    focus/capture interaction
    interaction with the rest of NodeTree
    potential scroll-lock behavior
```

These responsibilities should not remain mixed in the new architecture.

## 2. Component responsibilities

The future `Modal` component may own:

```text
visible/open presentation state
background and border visual properties
backdrop visual configuration
transition enabled/disabled
transition duration/progress
close callback / semantic close action
```

The component should render its visual state and expose semantic operations such as `open`, `close`, or equivalent public state.

It should not implement its own hit-testing or input routing.

## 3. Phase 6 framework responsibilities

The modality subsystem must provide the low-level behavior required by an active modal:

```text
active modal root registration
modal stack/order if multiple modals are supported
exclusive hit-testing against the active modal root
input routing / event dispatch restrictions
focus/capture policy for modal interaction
Escape routing policy
background interaction blocking
scroll-lock policy, if supported
```

The exact API is intentionally unresolved until Phase 6 design work.

## 4. Legacy behavior that must not be copied literally

The old Modal handled `MouseClickEvent` and `KeyDownEvent` directly in the component. This should not be treated as the target architecture.

The component should declare the relevant semantic behavior; framework modality infrastructure should determine where those events can go.

The old implementation also mixed backdrop alpha animation into `backgroundColor`. The new implementation should keep visual transition state separate from generic framework state where practical.

## 5. Phase boundary

Phase 5 should not attempt to provide a final `Modal` implementation.

Phase 5 may keep a future Modal contract/reference, but final implementation depends on Phase 6 modality infrastructure.

The legacy implementation remains useful as a source of visual requirements and behavioral cases, not as the architectural base.
