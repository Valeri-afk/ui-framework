# Scroll Architecture

Scroll is a framework-level behavior/infrastructure concern. A standalone public `Scroll` / `ScrollArea` component is not currently required.

## Current implementation status

The source already contains `ScrollManager` and `UIManager` integration.

Current source-level responsibilities include:

```text
ScrollManager
├── viewport extent
├── content extent
├── offset
├── maximum offset
├── clamping
├── accumulated ancestor offsets
├── wheel routing
└── layout-derived content extent

UIManager
├── owns ScrollManager
├── routes wheel input to ScrollManager
├── applies accumulated scroll coordinate transform
└── synchronizes scroll state during frame preparation
```

The implementation is source-level only until full build and runtime validation are performed.

## Coordinate model

Scroll must not rewrite the original layout positions of descendants.

The intended relationship is:

```text
layout position
      ↓
accumulated scroll offset
      ↓
effective render/input position
```

For nested scroll containers, ancestor offsets accumulate.

The stored layout geometry therefore remains stable while scrolling changes the effective coordinate used by rendering and input.

## Range model

For a scroll container:

```text
maxOffsetX = max(0, content.width  - viewport.width)
maxOffsetY = max(0, content.height - viewport.height)
```

Offsets are clamped to:

```text
0 ≤ offsetX ≤ maxOffsetX
0 ≤ offsetY ≤ maxOffsetY
```

The current implementation also derives the initial viewport/content relationship from layout geometry. Client code may explicitly set scroll viewport/content sizes through the current UIManager scroll API when needed.

## Border and padding

The scroll viewport must use the framework's existing layout box model.

The current implementation derives the usable client size by subtracting positive border and padding from the node's actual size.

Do not introduce a separate Scroll-specific box model.

Conceptually:

```text
actual border box
      ↓ subtract border
padding area
      ↓ account for padding/content origin
scroll viewport/content relationship
```

## Input

Wheel routing is framework-level.

Current flow:

```text
SDL mouse wheel
      ↓
UIManager
      ↓
ScrollManager::handleWheel()
      ↓
hit-test target
      ↓
nearest scrollable ancestor(s)
      ↓
apply available delta
      ↓
clamp
```

The implementation allows remaining wheel delta to propagate to outer scroll containers when an inner scroll container reaches its limit.

If no scroll container consumes the wheel event, normal input processing continues.

## Rendering and clipping

Scroll should reuse the existing NodeTree clipping semantics based on `Overflow::HIDDEN` rather than creating a second clipping architecture.

The current `UIManager` already applies the scroll coordinate transform during the render traversal.

The remaining validation work is to verify that the resulting transform and the existing clipping/hit-test traversal produce correct behavior for nested scroll containers and clipped content.

## Hit testing

Input traversal already runs under the same scroll coordinate transform used by the framework input path.

The next validation target is therefore not a new independent hit-test system, but verification that:

```text
pointer coordinates
      ↓
scroll transform
      ↓
existing NodeTree hit-test
      ↓
viewport clipping
```

produces the expected target for scrolled content.

## Content extent

`ScrollManager::sync()` currently derives content extent from the actual geometry of visible descendants while respecting nested registered scroll containers.

This should be validated against the framework's layout semantics before being considered final.

If a future component requires virtualization or a custom content extent, that requirement should be added deliberately rather than turning `ScrollManager` into an arbitrary measurement engine.

## Scrollbar presentation

Scrollbar visuals are intentionally not part of the current Scroll core.

A scrollbar should only be added after behavior is runtime-validated and a concrete reusable visual contract is established.

A scroll container must remain useful without a visible scrollbar.

## Public component decision

Do not create a standalone `Scroll` / `ScrollArea` component merely because `ScrollManager` exists.

First validate the framework behavior.

Only introduce a public component if repeated application-level usage demonstrates that a component API provides a real reusable abstraction beyond the service/infrastructure already available.

## Remaining work

```text
source integration
    ↓
full build
    ↓
runtime wheel tests
    ↓
render/clipping validation
    ↓
hit-test validation
    ↓
nested scroll validation
    ↓
only then decide on scrollbar/public Scroll component
```
