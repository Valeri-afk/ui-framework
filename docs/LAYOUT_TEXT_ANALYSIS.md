# Text and Layout Architecture Analysis

> **Status:** research / no implementation decision
> **Date:** 2026-08-18

This document focuses on the historical TextEngine problem and tests whether a framework-owned layout engine can coexist with text/content-specific measurement without mixing responsibilities.

## 1. Current source situation

The current `Node` API still exposes protected virtual:

```cpp
virtual LayoutSize measure(MeasureContext &ctx);
virtual void arrange(ArrangeContext &ctx);
```

This means the current implementation treats measurement and arrangement as capabilities of every Node subclass. fileciteturn121file0

The current measurement context already separates a proposed/available content-box size from the recursive `measureChild` operation. fileciteturn123file0

The current Node implementation also routes layout-affecting mutation through `NodeTree` and queues a layout invalidation after the deferred mutation is applied. fileciteturn124file0

This means the framework already contains useful ingredients for a stronger internal model, even though the public/extension boundary is not yet settled.

## 2. Historical Label evidence

The legacy `Label` implementation is important because it demonstrates the exact problem we are investigating.

The label:

- stores text and font state;
- owns SDL_ttf text objects;
- applies wrapping based on available width;
- measures text through SDL_ttf;
- returns a generic `LayoutSize`;
- draws using its own text state.

Its historical `measure()` receives layout constraints and determines text dimensions using the available width and wrapping configuration. fileciteturn120file0

This shows that text-specific sizing can naturally live near the text component while the output consumed by the surrounding layout is simply a generic size.

The legacy implementation should not be copied because it belongs to the old component architecture, but the responsibility boundary it demonstrates is useful.

## 3. The TextEngine failure revisited

The historical problem can now be described more precisely.

### Bad centralization

```text
LayoutEngine
   ├── knows Text
   ├── knows Font
   ├── knows wrapping
   ├── knows TTF
   └── knows every future content system
```

This makes layout and content measurement coupled.

### Bad client exposure

```text
Text component
   ├── owns text measurement
   ├── implements Measure/Arrange
   ├── manages invalidation
   └── knows framework layout protocol
```

This makes content components coupled to framework internals.

### Better boundary

```text
Text implementation
   └── content-specific measurement
              ↓
        generic size result
              ↓
       framework layout engine
```

The framework orchestrates when measurement happens and how the result participates in parent layout, but it does not need to understand TTF-specific details.

## 4. Why width-dependent text is not a special exception

Text demonstrates a general class of content:

> content whose desired size depends on the proposal/constraints.

Flutter's box model passes min/max constraints downward and expects the child to select a size that satisfies them. citeturn381190search1turn381190search0

SwiftUI's `LayoutSubview` allows a layout to ask a subview for its size under a proposal. citeturn381190search3turn381190search7

Compose's layout model also measures children under parent-supplied constraints and then places them, while enforcing measurement/placement phase boundaries. citeturn381190search2

Therefore width-dependent text is not evidence that layout must know about text internals. It is evidence that layout needs a generic content-measurement query.

## 5. What the generic measurement boundary should know

The minimum conceptual information appears to be:

```text
proposal / constraints
    ↓
content measurement
    ↓
generic Size
```

The measurement side may need to know application/content-specific details such as:

- text content;
- font;
- wrapping;
- image intrinsic dimensions;
- RichText runs;
- content-specific minimum/maximum dimensions.

The layout engine only needs the result and the ability to invoke the measurement operation under controlled constraints.

## 6. What the generic measurement boundary should NOT know

A content measurement implementation should not know:

- which parent container uses it;
- how siblings are distributed;
- the layout queue;
- invalidation roots;
- NodeTree mutation;
- lifecycle ownership;
- renderer traversal order.

Text measurement may depend on a renderer/text backend internally, but this should not turn the layout engine into the owner of that backend.

## 7. Text invalidation

A text component changes state:

```text
setText()
setFont()
setWrapWidth()
```

The framework should classify such state as measurement-affecting and schedule the appropriate layout work.

The measurement implementation itself should remain a geometry query rather than directly scheduling layout.

WPF demonstrates the general property-system pattern of associating property changes with `AffectsMeasure`, `AffectsArrange` and `AffectsRender`. citeturn381190search5

The framework does not need to implement WPF's entire property system to use the same architectural principle.

## 8. Text as a leaf is different from a container

A text-bearing component generally needs:

```text
content measurement
rendering
```

It does not need to implement a general child-arrangement algorithm.

This is an important counterexample to the idea that every component participating in layout needs both Measure and Arrange extension hooks.

A Button can similarly combine:

```text
content measurement
+
padding/border
+
framework-controlled final geometry
```

without becoming a layout container.

## 9. Implication for the current Node API

The current source has:

```cpp
Node
  -> virtual measure()
  -> virtual arrange()
```

That is convenient for implementation but potentially too broad as a public extension model.

The research suggests that a future architecture should consider separating:

```text
Node
   └── generic content measurement capability (when necessary)

Panel/container
   └── layout strategy / arrangement capability
```

rather than requiring every Node subclass to be both.

This is a conceptual direction, not yet a recommendation to remove the existing virtual methods.

## 10. Two kinds of custom content

### Custom intrinsic content

Example:

```text
RichText
Chart
SVG
Markdown
Image with dynamic source
```

The custom requirement is:

```text
proposal → intrinsic/desired size
```

This can coexist with built-in Grid/Stack/Flex.

### Custom container

Example:

```text
ChessBoard
RadialMenu
Graph
Timeline
```

The custom requirement is:

```text
container proposal + child measurements → child geometry
```

These should remain conceptually separate extension points.

## 11. Framework-owned orchestration

Regardless of which extension points exist, the framework should own:

```text
when measurement happens
when arrangement happens
invalidation
scheduling
lifecycle
ownership
constraint normalization
geometry application
```

This is the key requirement derived from the framework's historical failures.

## 12. Important consequence

The historical choice between:

```text
closed LayoutEngine
```

and:

```text
open Measure/Arrange components
```

was unnecessarily binary.

A third possibility is:

```text
closed orchestration
+
internal/general content measurement
+
optional custom container strategy
```

where only the narrow geometric capability crosses the framework boundary.

## 13. Risk: generic measurement can become another God interface

A measurement abstraction must not expand into:

```text
measure
arrange
invalidate
lifecycle
render
input
```

It should answer one question: what size can/should this content occupy under a proposal?

If a content type needs a more complicated protocol than that, the framework should justify the additional abstraction with a concrete use case rather than making every Node implement it.

## 14. Risk: renderer dependency

Text measurement often depends on font/rendering resources. The legacy Label creates an SDL_ttf text engine and text object and uses those resources for measurement and rendering. fileciteturn120file0

This suggests that a future content-measurement capability may need access to a controlled resource/measurement context rather than a raw renderer pointer.

However, it does not imply that the LayoutEngine itself should become renderer-specific.

A possible future split is:

```text
Layout Engine
   ↓
generic ContentMeasureContext
   ↓
Text / Image / other measurement provider
```

The exact context is intentionally not designed yet.

## 15. Current conclusion

The text analysis now supports the following research conclusion:

> A framework-owned layout engine and content-specific text measurement are not inherently incompatible.

The historical failure most likely came from choosing an ownership boundary that forced either:

- the central layout engine to know text internals, or
- the client component to know layout orchestration/invalidation.

A cleaner boundary is possible if the framework treats content measurement as a narrow, generic capability and keeps all scheduling/invalidating/layout orchestration internal.

No implementation decision is made by this document.
