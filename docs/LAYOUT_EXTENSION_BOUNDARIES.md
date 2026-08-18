# Layout Extension Boundaries

> **Status:** research result / no implementation decision
> **Date:** 2026-08-18

This document records the current conclusion about where extensibility may need to exist in a framework-owned layout system.

## 1. The central distinction

There are two different reasons a client may need an extension point:

```text
A. New content type
   "I know how to measure this content."

B. New container algorithm
   "I know how to position these children."
```

They should not automatically be represented by one `CustomLayout` mechanism.

The distinction is visible in established systems. SwiftUI gives custom layout containers subview proxies and separate layout values, while WPF and Flutter distinguish child measurement/constraints from container arrangement. citeturn409766search6turn409766search8turn409766search0

## 2. Boundary A — Content measurement

A content-bearing component may need specialized knowledge:

- text shaping;
- line breaking;
- font metrics;
- image intrinsic dimensions;
- RichText runs;
- embedded content;
- domain-specific intrinsic size.

The generic layout engine should not need to understand those implementation details.

A conceptual boundary is:

```text
Framework asks:
    "What size can/desires this content under this proposal?"

Content answers:
    "Width W, height H."
```

The framework continues to own:

- when the question is asked;
- caching/invalidation;
- layout scheduling;
- constraints around the result;
- final placement.

## 3. Boundary B — Custom container layout

A custom container needs a different capability:

```text
Given child measurements and container space,
where should each child be placed?
```

The extension should ideally receive:

- container proposal/constraints;
- child proxies;
- child desired/feasible sizes;
- generic child layout metadata;
- a safe way to assign final geometry.

It should not receive:

- NodeTree ownership operations;
- lifecycle controls;
- mutation queues;
- direct invalidation APIs;
- framework destruction semantics.

SwiftUI's `LayoutSubview` is a useful reference: it exposes `sizeThatFits`, dimensions, layout values and `place`, without making the custom layout responsible for the subview's runtime ownership. citeturn409766search8

## 4. Why these should be independent

Consider RichText.

A RichText component may need custom content measurement but can still use the framework's Grid/Stack/Flex containers.

Conversely, a ChessBoard container may use ordinary Text/Image/Button children but need a custom placement algorithm.

Therefore:

```text
RichText
    needs custom measurement
    does not need custom container layout

ChessBoard
    needs custom container layout
    does not need custom child measurement
```

A single `CustomLayout` interface would unnecessarily couple these cases.

## 5. Default framework behavior

The intended normal path should be:

```text
user sets properties
        ↓
framework invalidates automatically
        ↓
framework asks content for measurement if needed
        ↓
built-in layout strategy allocates geometry
        ↓
framework stores final geometry
```

The ordinary application developer should not have to implement any of these framework phases.

## 6. Advanced extension behavior

An advanced user may opt into one of two extension points.

### Content measurement extension

Conceptually:

```text
CustomContentMeasurement
    proposal -> Size
```

This is suitable for RichText-like content.

### Custom container layout extension

Conceptually:

```text
CustomContainerLayout
    children + measurements + metadata + proposal
        -> child geometry
```

This is suitable for radial, graph, board, timeline or other special containers.

These are research-level concepts, not final C++ interfaces.

## 7. Invalidation remains framework-owned

The strongest requirement from the framework's historical development is:

> Extension authors should not have to call `markLayout` or know the layout queue.

WPF demonstrates a framework-owned property metadata model where a property can declare that changes affect measure or arrange; the property system then schedules invalidation. citeturn409766search1turn409766search9

A C++ framework does not need to reproduce WPF's dependency property system, but it can adopt the same architectural boundary:

```text
property changes
    ↓
framework identifies affected phase
    ↓
framework schedules layout
```

## 8. Container-specific child metadata

Grid/Flex/other containers may need child-specific values:

```text
Grid:
    row
    column
    spans

Flex:
    grow
    shrink
    order
    basis
```

These values do not require specialized Node subclasses.

SwiftUI's `LayoutValueKey` demonstrates a generic way for a layout container to access child-specific layout values without dynamic type discovery. citeturn409766search6turn409766search8

A C++ design may instead use explicit typed layout metadata, a type-erased property store, or framework-owned per-container metadata. The exact mechanism remains open.

## 9. What this means for the "closed layout" goal

The strongest interpretation of "closed layout" is not:

> No user can ever extend layout.

It is:

> The normal framework user cannot fall out of the framework's layout system and should not need to understand its internal mechanics.

Under that definition, a framework may be:

```text
closed by default
+
explicitly extensible at narrow boundaries
```

without contradicting the goal of a closed layout system.

## 10. Research verdict

The historical problems can now be separated more clearly:

```text
Old problem:
custom component knew too much about invalidation/runtime.

Possible fix:
keep custom capability, move invalidation/runtime ownership back into framework.

Old problem:
closed layout engine became coupled to TextEngine.

Possible fix:
introduce generic content measurement boundary.

Old problem:
specialized widget classes caused framework type probing.

Possible fix:
keep stable Node runtime identity and attach layout/content capabilities through explicit framework-owned state or proxies.
```

This does not yet specify the final C++ architecture. It establishes the boundaries the future architecture should try to preserve.
