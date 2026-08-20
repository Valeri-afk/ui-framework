# Phase 5 — Big Documentation Manual Review

## Purpose

This document is a temporary manual patch plan for the two large documents that should be edited before concrete Phase 5 component implementation.

Files intentionally not modified automatically:

```text
docs/ARCHITECTURE.md
docs/FRAMEWORK_SCOPE.md
```

The repository already has the authoritative Phase 5 component architecture in:

```text
docs/PHASE5_COMPONENT_ARCHITECTURE.md
```

The source remains authoritative.

---

# 1. ARCHITECTURE.md

## A. Active core / ControlNode

Current architecture lists `controlnode.hpp` / `controlnode.cpp` inside the active core and subsequently describes `ControlNode` as a current runtime element.

### Required change

Keep the file reference if the source still exists, but classify it explicitly as:

```text
Historical / experimental:
    ControlNode
```

Do not describe `ControlNode` as the base component architecture.

Add the Phase 5 rule:

```text
ControlNode is a historical WPF-inspired experiment.
It is not an accepted universal base class for framework components.
Current component architecture derives from concrete responsibilities and
may use Node or PanelNode without requiring ControlNode.
```

This matches the Phase 5 architecture checkpoint and prevents readers from
mistaking an existing source file for an accepted architecture.

## B. Node responsibilities

The existing Node section is broadly correct and should remain the factual source-level description.

Add a short Phase 5 clarification after the current responsibility list:

```text
Node is the base runtime/component object. Its framework-recognized state is
not intended to encode every component-specific semantic property.

Generic properties such as visibility, enabled state, geometry, padding,
border and overflow are interpreted by framework subsystems. Concrete
components keep their domain state in the component itself.
```

Do not introduce `FrameworkProps` / `ComponentProps` into the architecture document.

## C. PanelNode responsibilities

The existing `PanelNode` description is correct at the runtime level, but it should be clarified that PanelNode is a structural/layout primitive, not a generic visual-content base.

Replace/extend the current summary:

```text
Node
  +
child ownership
  +
container behavior
```

with:

```text
Node
  +
structural child ownership
  +
child/layout composition capability
```

and add:

```text
PanelNode is selected when a concrete component genuinely requires
owned child Nodes, child geometry management, layout flow or structural
composition. The presence of text, icons or multiple visual primitives
alone does not require PanelNode.
```

## D. Hit-test section — outdated

The current document still contains an old statement that `NodeTree::hitTest()` calls `Node::hitTest()` on top-level nodes and that the implementation does not recursively traverse PanelNode children.

That is no longer the current Phase 4 contract.

### Required replacement

The section must describe the current canonical model:

```text
InputManager
      |
      v
NodeTree::hitTest()
      |
      +-- reverse effective top-level paint order
      |
      +-- recursive subtree hit-test
             |
             +-- reverse child order
             +-- descendant-first target selection
             +-- ancestor clipping/overflow constraints
```

The accepted Phase 4 behavior is:

```text
roots → overlays → modal presentation order
reverse order for hit-test
recursive subtree hit-test
Overflow::HIDDEN acts as an ancestor clipping boundary
```

Do not preserve the old wording that says recursive child hit-testing is not established.

Reference: `docs/PHASE4_FINAL_CHECKPOINT.md`.

## E. Rendering / backend section

The architecture should explicitly retain:

```text
SDL3 is the current and only concrete backend.
The application owns SDL runtime lifetime.
The framework consumes SDL3 types and the supplied renderer.
No generic RenderContext/backend interface exists as a current layer.
```

Do not elevate `RenderContext`, `IRenderBackend`, `BackendFactory` or a second backend into current architecture.

## F. Resource boundary

Add a concise current rule:

```text
Renderer-bound resources are owned by the Node/component that needs them.
There is no generic ResourceManager in the current architecture.
Semantic resources and backend-bound representations remain conceptually
separate, but no additional generic resource subsystem is required yet.
```

This keeps the architecture consistent with Phase 4 F.

## G. Animation

No architecture change required, but add one sentence if the section exists:

```text
Animation is not a current global rendering subsystem. It is a mechanism
for changing semantic/visual state through normal framework semantics.
```

Do not add an AnimationManager.

## H. Component architecture cross-reference

Near the end of `ARCHITECTURE.md`, add:

```text
Phase 5 component architecture is documented separately in
`docs/PHASE5_COMPONENT_ARCHITECTURE.md`.

That document defines the current component-design rules, including the
Node/PanelNode boundary, semantic content vs structural children, component
state ownership, primitive-node criteria and the rule that new abstractions
must emerge from concrete requirements.
```

This keeps `ARCHITECTURE.md` focused on implemented system architecture while providing a controlled link to the current Phase 5 design contract.

---

# 2. FRAMEWORK_SCOPE.md

The existing scope document is substantially compatible with the Phase 5 architecture. It should not be turned into a detailed component API specification.

## A. Framework responsibilities — add component execution boundary

Under framework responsibilities, after layout/render/input infrastructure, add:

```text
Component/runtime support:

- framework-recognized Node state and generic execution semantics;
- reusable framework primitives where centralized low-level behavior is justified;
- provided components that implement application-facing UI semantics on top of
  the runtime.
```

Clarify that the framework owns mechanics while components own domain semantics.

## B. Client responsibilities — distinguish application client from custom component author

Keep the existing application/client responsibility section, but add:

```text
The framework also supports custom component authors. A custom component may
own its own semantic/domain state and presentation, but should use the
framework's existing ownership, lifecycle, layout, input, hit-test, event and
rendering infrastructure rather than reimplementing those generic systems.
```

This is important because Phase 5 architecture concerns developers creating
custom components, not only the application that consumes provided controls.

## C. Required capabilities — refine component wording

The current scope lists common interactive controls as a required capability.
Keep that, but clarify:

```text
Common controls are provided framework components, not evidence that every
control needs a universal control base class. Component hierarchy must emerge
from concrete responsibilities.
```

## D. Minimalism / non-goals — add explicit component-system non-goals

Add to the non-goal principles:

```text
- universal "everything is content of everything" composition;
- CSS-style declarative property interpretation;
- mandatory generic Control/Interactive/Selectable/Composite base classes;
- mandatory service objects for ordinary component authors;
- feature parity with Qt/WPF/Material UI.
```

These are now explicit Phase 5 design constraints.

## E. Document roles — fix stale references

The current document still refers to:

```text
`docs/PHASE1_RUNTIME.md`
```

as a maintained document.

That reference should be removed or replaced with the actual current Phase 1
records:

```text
`docs/PHASE1_FINAL_DECISIONS.md`
```

Add the Phase 5 document role:

```text
`docs/PHASE5_COMPONENT_ARCHITECTURE.md`

Defines the current Phase 5 component architecture and is the primary design
reference for new component work.
```

Optionally mention the focused Phase 5 notes as historical/supporting records,
not competing architecture sources.

---

# 3. Important non-changes

Do NOT modify these large documents to introduce:

```text
Button hierarchy
IconButton inheritance
ImageNode
IconNode
ScrollNode
ActionNode
FrameworkProps / ComponentProps
```

Those remain Phase 5 concrete/implementation questions or intentionally rejected abstractions.

Do NOT add a generic content model to `ARCHITECTURE.md`.

Do NOT turn `FRAMEWORK_SCOPE.md` into a component API reference.

---

# 4. Final documentation hierarchy after manual edits

```text
FRAMEWORK_SCOPE.md
    → why the framework exists / intended scope

ROADMAP.md
    → phase order and current phase

ARCHITECTURE.md
    → current implemented architecture

PHASE1_FINAL_DECISIONS.md
PHASE2_CONSTRAINT_SEMANTICS.md
PHASE2_NUMERICAL_LAYOUT_CASES.md
    → completed phase contracts

PHASE4_*.md
    → completed Phase 4 checkpoints

PHASE5_COMPONENT_ARCHITECTURE.md
    → current Phase 5 component design authority
```

The older Phase 5 notes can be removed after their information has been confirmed as preserved in the consolidated checkpoint.
