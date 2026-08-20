# Scroll Architecture

Scroll is a framework-level concern, but the final architecture is intentionally deferred.

## Why Scroll is different

A scrollable component is not merely a visual property such as padding or overflow. Correct scrolling may require coordination between:

```text
content bounds
viewport bounds
clipping
scroll offset
input/wheel routing
hit-testing coordinates
scrollbar presentation
nested scroll containers
```

These concerns can cross the boundary between an individual component and framework infrastructure.

## Current rule

Do not implement a final `Scroll` / `ScrollArea` component until the framework-level scroll contract is decided.

In particular, do not prematurely distribute scroll state across arbitrary `Node` properties or implement independent wheel handling inside each component.

## Expected separation

A future design should distinguish at least:

```text
Scroll state
    offset / range / viewport relationship

Scroll presentation
    viewport / clipping / scrollbar visuals

Scroll input
    wheel/gesture/drag routing

NodeTree/framework services
    coordinate transforms
    hit-testing through clipped/offset content
```

The exact ownership of each responsibility remains a Phase 6+ architecture decision.

## Component implication

A standard `Scroll` / `ScrollArea` component is expected eventually, but its API must be derived from the finalized scroll infrastructure rather than invented first and used to constrain the infrastructure afterward.

Application components such as a chess move list should consume the standard scroll facility; they should not implement their own framework-level scrolling protocol.
