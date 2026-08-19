# Framework Scope and Purpose

## 1. Why This Framework Exists

This project did not begin as an attempt to reproduce Qt, WPF, or another general-purpose UI toolkit.

It originated from the development of a chess application.

The application initially had a small amount of UI:

- a chess board;
- chess pieces;
- highlighted legal moves;
- check/checkmate indications.

As the application grew, it needed ordinary application UI such as:

- main menus;
- buttons;
- dropdowns;
- modal windows;
- settings screens;
- navigation between screens;
- timers;
- move history;
- captured-piece displays;
- rules screens;
- opening information/cards;
- other auxiliary UI.

At first these elements were implemented as independent client-side components. This approach became difficult to maintain because components independently wanted to participate in rendering, updating, input handling, and callbacks.

Synchronous code could still be coordinated by controlling call order. Callback-driven behavior exposed a deeper problem: components did not have a common runtime responsible for ownership, lifecycle, traversal, update order, rendering order, input routing, event dispatch, and safe structural mutation.

The UI framework emerged from that problem.

The framework is therefore not an arbitrary collection of widgets. Its primary purpose is to provide the runtime infrastructure that turns independently implemented UI objects into one coherent interactive UI system.

---

## 2. What This Project Is

This project is a **lightweight retained-mode C++ UI framework/runtime** for interactive graphical applications.

The framework provides infrastructure for:

- a hierarchical UI object tree;
- node ownership and lifetime;
- lifecycle callbacks;
- traversal;
- safe mutation during callbacks;
- update scheduling;
- rendering traversal;
- input coordination;
- event propagation;
- layout;
- reusable and user-defined UI nodes;
- higher-level UI components built on top of the runtime.

The framework is intentionally narrower than large general-purpose application frameworks such as Qt or WPF.

It should be designed around the application class it is intended to support rather than around feature parity with larger toolkits.

---

## 3. Target Application

The immediate target application is a chess application using a separate chess engine/domain layer and the UI framework as its presentation/runtime layer.

The intended application may contain screens and UI such as:

```text
Application
├── Main Menu
│   ├── New Game
│   ├── Settings
│   ├── Rules
│   ├── Openings
│   └── Exit
│
├── Game Screen
│   ├── Chess Board
│   ├── Chess Pieces
│   ├── Move Highlights
│   ├── Player Clocks
│   ├── Captured Pieces
│   ├── Move History
│   └── Auxiliary Controls
│
└── Auxiliary UI
    ├── Settings
    ├── Rules / Help
    ├── Opening Information
    ├── Dialogs / Modals
    └── Other application screens
```

The framework does not implement chess rules or chess-domain behavior.

A conceptual system boundary is:

```text
Chess Engine / Domain
        |
        v
Chess Client / Application
        |
        v
UI Framework
```

The chess engine remains responsible for chess state and rules. The client is responsible for application-specific meaning and behavior. The framework is responsible for the infrastructure needed to present and interact with that application state.

---

## 4. Framework Responsibilities

The framework should own the mechanisms required to make many UI objects operate as one runtime.

### Runtime structure

- hierarchical node ownership;
- live-node registration;
- node identity;
- lifecycle;
- traversal;
- structural mutation;
- safe mutation during callbacks.

### UI execution

- update traversal;
- rendering traversal;
- ordering guarantees;
- callback execution boundaries.

### Interaction infrastructure

- input routing;
- focus/capture behavior where required;
- hit testing;
- event propagation;
- event dispatch ordering.

### Layout infrastructure

- measurement;
- arrangement;
- layout invalidation;
- basic layout containers and mechanisms.

### Reusable UI building blocks

The framework may provide common controls and containers, but components are not its defining purpose.

The client should be able to create custom nodes by composition and inheritance without bypassing runtime ownership and lifecycle rules.

---

## 5. Client Responsibilities

The framework client is responsible for application-specific behavior and state.

Examples:

- chess rules;
- chess engine integration;
- game state;
- move semantics;
- clock/game-time rules;
- opening data;
- PGN/FEN/domain data;
- navigation intent;
- what a button or menu item means to the application;
- custom UI behavior implemented through framework extension points.

The framework should provide mechanisms, not application meaning.

For example:

```text
Framework:
    detect button click
    dispatch event
    invoke client callback

Client:
    interpret "New Game"
    reset chess session
    choose the next screen
```

---

## 6. Supported Application Class

The intended class of application is larger than a trivial one-screen game but smaller than a universal application platform.

The framework should comfortably support applications with:

- multiple screens or views;
- nested panels and containers;
- menus and dialogs;
- settings interfaces;
- lists and scrollable content;
- interactive controls;
- overlays and modal UI;
- continuously updated information such as clocks;
- dynamic content;
- custom drawing;
- application-specific node types;
- callback-driven interaction.

A chess application is the immediate concrete target, but the runtime should remain useful for other interactive desktop-style C++ applications that have similar UI infrastructure needs.

---

## 7. Capability Principles

The framework should not implement a capability merely because another UI toolkit implements it.

The preferred rule is:

> A capability belongs in the framework when it represents a real responsibility of the UI runtime or is repeatedly required by the supported application class.

This creates three useful categories.

### Required capabilities

Capabilities that are fundamental to the current application class and to the runtime model.

Examples:

- ownership/lifetime;
- node tree;
- lifecycle;
- traversal;
- mutation safety;
- update;
- layout;
- rendering coordination;
- input/event infrastructure;
- common interactive controls.

### Optional / later capabilities

Capabilities that may become useful for richer applications but are not required by the current target.

Examples:

- drag-and-drop;
- docking systems;
- richer document/editor behavior;
- advanced accessibility;
- sophisticated animation systems;
- arbitrary node reparenting if a real use case appears.

### Non-goals

Capabilities outside the framework's intended responsibility.

Examples:

- chess rules;
- chess-engine evaluation;
- PGN/FEN semantics;
- opening databases;
- application data models;
- application-specific business logic.

---

## 8. Reparenting Scope

Reparenting is a legitimate capability in richer retained-mode UI toolkits, but it is not automatically required for every application.

For the current chess application, ordinary UI composition usually creates a node directly under the parent that should own it. Typical screens such as menus, settings, rules, opening cards, move history, timers, and dialogs do not inherently require an existing node to move between unrelated parents.

Therefore reparenting is currently a **future capability, not a Phase 1 requirement**.

Potential future use cases include:

- drag-and-drop between containers;
- tab/document transfer;
- docking/workspace movement;
- preserving a complex node while moving it between panels;
- other UI behaviors where destroying and recreating a node would lose meaningful state.

If such a use case becomes a real requirement, the framework should first determine the correct runtime semantics and then expose the smallest API needed for that requirement.

Reparenting must not be introduced merely for API symmetry or because a larger toolkit supports it.

---

## 9. Ownership Scope

The framework uses `std::unique_ptr` as the fundamental ownership mechanism.

The Phase 1 ownership decision is:

```text
add(std::unique_ptr<Node>)
remove(Node&)
```

Live node ownership remains inside the framework. Public ownership-transfer `detach()` is not part of the target runtime contract.

The reason is to minimize the number of client lifetime models. The current runtime already has a live registry, stable `NodeId`, deferred mutation, nested mutation scopes and explicit lifecycle handling. A second client-owned lifetime domain is not justified by a demonstrated application requirement.

Client-held `Node*` references remain non-owning and may become invalid after deferred removal and destruction. `NodeId` provides identity/liveness resolution; it does not provide ownership.

---

## 10. Current Architectural Center

The core architectural problem this framework solves is coordination.

Without a runtime, independent components compete for control over:

```text
render
update
input
callbacks
lifetime
hierarchy
```

The framework centralizes the coordination rules through:

```text
UIManager
    |
    v
NodeTree
    |
    +-- Node ownership/lifetime
    +-- traversal
    +-- mutation safety
    +-- live-node registry
    +-- lifecycle
```

Other subsystems such as layout, input, events, modal handling, and rendering build on that runtime instead of independently controlling the lifetime and traversal of UI components.

This coordination role is the central reason the framework exists.

---

## 11. Design Philosophy

The framework should follow these principles:

1. **Runtime correctness before component breadth.**
   A small set of reliable primitives is more valuable than many loosely integrated controls.

2. **Client extensibility without loss of runtime control.**
   Clients can create custom nodes and participate through callbacks, while the framework retains responsibility for ownership, lifecycle, traversal, and mutation safety.

3. **Minimal sufficient API.**
   Do not add a capability only because a larger UI toolkit has it.

4. **Prefer real requirements over symmetry.**
   The framework should not introduce abstractions solely so that related concepts look symmetrical.

5. **Domain/application logic stays outside the framework.**
   The framework provides infrastructure, not chess/application semantics.

6. **The source code remains authoritative.**
   Documentation records intended and established behavior but must not describe hypothetical behavior as implemented.

---

## 12. Relationship to Large UI Frameworks

Qt, WPF, and similar systems are useful references for established UI concepts, but their feature sets should not be treated as the specification for this project.

Large frameworks serve broad application classes and therefore support many capabilities that may be unnecessary here.

The appropriate question for this project is not:

> "Does Qt/WPF have this feature?"

It is:

> "Does the application class supported by this framework require this capability, and is providing it a responsibility of the framework runtime?"

Existing frameworks are useful as design references and sources of proven patterns, not as feature checklists.

---

## 13. Current Development Strategy

The framework is being stabilized before the chessengine/client integration is used as a verification environment.

The current process is therefore:

```text
1. establish architecture and runtime contracts;
2. stabilize the framework core;
3. complete the six architecture phases;
4. establish the final framework verification path;
5. integrate with the chess client;
6. grow higher-level application features from real requirements.
```

The framework's final scope should be allowed to evolve from actual application needs rather than speculative completeness.

---

## 14. Document Roles

`docs/FRAMEWORK_SCOPE.md`

Defines why the framework exists, what class of applications it targets, what responsibilities belong to the framework, and what capabilities should or should not be introduced.

`docs/ROADMAP.md`

Defines development phases and high-level exit criteria.

`docs/ARCHITECTURE.md`

Describes the architecture that is actually implemented in the main source tree.

`docs/PHASE1_RUNTIME.md`

Records Phase 1 runtime analysis and stabilization work.

`docs/PHASE1_FINAL_DECISIONS.md`

Provides the concise final architecture snapshot for Phase 1 and future development contexts.

`docs/INSTRUCTIONS.md`

Defines the workflow for analyzing and modifying the repository.

When a new development context is opened, `FRAMEWORK_SCOPE.md` should be read early so that implementation decisions are evaluated against the project's actual purpose rather than against assumptions about a generic UI toolkit.
