# Layout Architecture Case Matrix

> **Status:** comparative analysis / no implementation decision
> **Date:** 2026-08-18
>
> This document runs the candidate layout families through the same set of practical cases. It complements `LAYOUT_MODELS_RESEARCH.md` and is intended to make architectural trade-offs concrete.

## 1. Test cases

Every candidate architecture should be able to explain these cases without hidden client/framework responsibilities:

1. **Text** — content size depends on text, font and available width; wrapping changes height.
2. **Button** — visible content is text-like, but the widget also has padding, border and interaction state.
3. **Stack/Column** — one-dimensional child allocation with spacing and alignment.
4. **Grid** — two-dimensional allocation with fixed/auto/flexible tracks and child placement.
5. **Flex** — one-dimensional layout where remaining space may be distributed or children may shrink/grow.
6. **Custom / RichText** — a capability that the current built-in framework does not fully cover and which may require specialized measurement/layout logic.
7. **Invalidation** — changing text, font, padding, size, child visibility or layout properties causes the correct layout work without client code manually managing the framework queue.

---

## 2. Family A — Fully closed property-driven engine

```text
client properties
      ↓
framework layout engine
      ↓
built-in content + built-in containers
```

### Text

Works well **only if** the framework contains a generic way to obtain the text's content-dependent size without forcing the engine to own all text-specific logic.

If the engine directly understands text shaping/wrapping, its responsibility expands into text infrastructure.

### Button

Straightforward once button content exposes an intrinsic/desired size. Button geometry can remain framework-owned.

### Stack / Grid / Flex

Natural fit. The engine can own all algorithms and all layout properties.

### Custom / RichText

This is the weakness. If the built-in engine does not contain RichText, graph, diagram, or other domain-specific sizing/layout knowledge, the client has no clean escape hatch.

The system becomes safe but closed.

### Invalidation

Best case for client simplicity. Framework knows every property that affects layout and can own invalidation completely.

### Main risk

The framework must continuously expand its built-in capability set or risk becoming a bottleneck.

---

## 3. Family B — Closed engine + generic content measurement

```text
component/content
      ↓
generic intrinsic measurement
      ↓
framework layout engine
      ↓
built-in containers
```

### Text

This is a strong fit. The text component can contain its own text measurement implementation and return generic size information. The layout engine only consumes the result. Text can therefore remain an internal concern of the text-bearing component rather than a public TextEngine service.

Width-dependent text is not a corner case: Qt explicitly supports `heightForWidth` for word-wrapping widgets, Compose provides intrinsic measurements, SwiftUI's Text responds to size proposals, and the historical Label in this repository already measured wrapped text against available width. citeturn494672search1turn494672search0turn494672search8turn118file0

### Button

Good fit. Button's content measurement can remain component-local, while the framework applies padding, min/max and allocation rules.

### Stack / Grid / Flex

Natural fit because these layouts only need generic child measurement results.

### Custom / RichText

Better than Family A if custom content can expose a generic measurement capability without becoming a layout engine itself.

However, a genuinely custom container algorithm is still not covered.

### Invalidation

Can remain entirely framework-owned if content-changing setters are framework-visible and layout-affecting properties automatically invalidate.

### Main risk

The generic content-measurement contract still needs definition. If it becomes too powerful, it can quietly turn into the old client-side Measure/Arrange contract.

---

## 4. Family C — Closed by default + narrow custom-layout escape hatch

```text
normal client
    → properties + built-in layouts

advanced client
    → narrow geometry extension

framework
    → invalidation + scheduling + runtime + lifecycle
```

### Text

Same strong behavior as Family B if content measurement stays generic.

### Button

Same as Family B.

### Stack / Grid / Flex

Built-in and therefore invisible to ordinary custom component authors.

### Custom / RichText

This is the strongest family for the current development state because the framework can remain closed for normal use while advanced users have an escape hatch when built-in coverage is insufficient.

A custom layout extension should ideally receive only generic child proxies / measurements / layout metadata and return geometry. SwiftUI's `LayoutSubview` and `LayoutValueKey` are a useful example of this separation: custom layout can query subview size and custom layout values and place subviews without owning their runtime/lifecycle. citeturn834078search1turn834078search3turn834078search6

### Invalidation

Potentially fully framework-owned. This is the key requirement that distinguishes this model from the historical fully-open Measure/Arrange approach.

### Main risk

The extension API may still become too large if the built-in engine cannot express common cases.

---

## 5. Family D — Fully open Measure / Arrange

```text
custom component
    ↓
measure
arrange
invalidate
framework integration
```

### Text

Very flexible. A text component can define exactly how content is measured.

### Button

Works, but ordinary component authors now know the layout protocol.

### Stack / Grid / Flex

Extremely flexible because every container can implement its own algorithm.

### Custom / RichText

Best expressive power.

### Invalidation

This is where the historical problem appears. Unless the framework introduces additional automatic invalidation semantics, component authors need to know more about dirty state and framework scheduling.

Compose, WPF and Flutter all provide custom layout APIs, but they constrain what custom layout code can do and keep parts of the layout protocol inside dedicated scopes or framework types. This suggests that "custom layout" does not need to imply "custom ownership of the entire layout system." citeturn834078search0turn546959search0turn834078search2

### Main risk

This is too much public responsibility for the default component model in this framework.

---

## 6. Text — deeper comparison

Text is the most useful stress test because its desired height can depend on available width.

Consider:

```text
Text("long paragraph")
width = 200
height = Auto
```

The engine cannot determine final height from text content alone. It needs the relationship:

```text
available width → text measurement → desired height
```

Qt formalizes this with a width-dependent size hint (`heightForWidth`), and Compose has explicit intrinsic measurement APIs. SwiftUI's text responds to size proposals. citeturn494672search1turn494672search3turn494672search0turn494672search2

This strongly suggests that a layout architecture needs **content measurement as a first-class concept**, even if the main layout engine is closed.

### Important conclusion

The historical failure was probably not:

> closed layout engine + text = impossible.

It was more likely:

> closed layout engine + no clean generic content-measurement boundary = responsibility mixing.

That distinction should influence the Phase 2 design.

---

## 7. Button — compound content without exposing layout to the component author

A Button is a useful middle case because it is not pure text and not a general container.

Ideal flow:

```text
Button
  ├── content measurement
  ├── padding
  ├── border
  └── final allocation
```

The Button should not need to implement `arrange()` merely because its size depends on its text/content.

The framework should be able to say:

```text
content desired size
        ↓
Button adds padding/border
        ↓
Button desired size
        ↓
parent allocates Button rectangle
```

This argues strongly for separating **content measurement** from **container layout algorithm**.

---

## 8. Stack / Column

The Stack problem is comparatively simple:

```text
child desired sizes
        ↓
ordered sequence
        ↓
space distribution
        ↓
child placement
```

A built-in Stack/Row/Column should therefore be a framework-owned policy rather than something every client component implements.

The historical `FlexPanel` demonstrates that users eventually need gaps, main/cross alignment and sometimes grow/shrink behavior, but those concerns are best treated as container policy rather than forcing every child component to implement them. fileciteturn117file0

---

## 9. Grid

Grid introduces parent-specific child metadata:

```text
child:
    row
    column
    rowSpan
    columnSpan
```

The most interesting question is not the algorithm itself; it is how these values become available to the current layout container without forcing the child to become a `GridNode` or forcing the engine to probe concrete child types.

SwiftUI's `LayoutValueKey` is a useful reference here: custom layout values can be attached to a child and then read by the current layout container through a generic proxy. citeturn834078search3turn834078search6

That suggests a potential C++ direction worth researching:

```text
Node
  ├── universal layout properties
  └── container layout metadata

Current Grid strategy
  └── reads Grid-specific metadata
```

The mechanism could be explicit or type-erased; no implementation decision is made here.

---

## 10. Flex

Flex is a stress test of how much algorithmic complexity the framework is willing to own.

Even the core concept is simple:

```text
ideal child sizes
       ↓
free / deficit space
       ↓
grow / shrink / alignment
```

But mature flex systems also need intrinsic sizing, minimum sizes, wrapping, main/cross-axis alignment and edge cases around unbounded constraints. Flutter documents that Row/Column behavior changes with bounded versus unbounded main-axis constraints, and CSS Flexbox defines detailed main- and cross-axis alignment algorithms. citeturn834078search2turn546959search6

### Implication

Do not commit Phase 2 to a "full Flexbox" implementation simply because a historical FlexPanel existed. First identify which flex capabilities the target application actually needs.

---

## 11. Custom / RichText

This is the decisive extensibility test.

Suppose the built-in framework does not yet support RichText. The desired user experience is:

```text
normal components
    → no custom layout required

RichText component
    → can use its own content measurement

truly custom container
    → can provide an advanced layout extension
```

The critical requirement is that the advanced extension must not force the author to understand:

```text
NodeTree ownership
lifecycle
mutation queue
layout invalidation scheduling
```

This is consistent with the direction seen in SwiftUI custom Layout, where the layout receives subview proxies and custom layout values rather than direct runtime objects. citeturn834078search6turn834078search3

---

## 12. Invalidation — comparison result

The matrix gives a strong conclusion:

### Framework-owned invalidation

Should be independent of whether layout is built-in or custom.

Qt's `updateGeometry()` is an example of a widget notifying the framework that its size hint changed, while the layout manager remains responsible for recalculation. Qt also coalesces consecutive updates. citeturn494672search1

### Custom layout does not have to own invalidation

The extension may be called only after the framework has decided a layout pass is necessary.

### Therefore

The historical experience:

```text
custom layout → developer must manually markLayout
```

should be treated as an **API design failure to revisit**, not as a universal law of custom layout architectures.

---

## 13. Strongest architecture boundary suggested by the matrix

The research now points toward four distinct responsibilities:

```text
1. Runtime
   ownership / lifecycle / mutation

2. Content measurement
   text / image / intrinsic content

3. Container layout
   Stack / Grid / Flex / Overlay

4. Layout orchestration
   invalidation / scheduling / constraints / passes
```

They do not necessarily need four separate public APIs or four separate C++ inheritance hierarchies.

In particular:

```text
content measurement
    ≠
container layout
```

is the most important distinction for avoiding a recurrence of the TextEngine problem.

---

## 14. Current research verdict

No final architecture is selected yet.

However, the matrix makes several propositions significantly stronger:

1. **A framework-owned layout engine is viable in C++** and does not require every component to implement layout.
2. **Intrinsic/content measurement must be a first-class boundary** if text and other content-dependent components are to remain framework-friendly.
3. **Measure/Arrange is primarily a protocol, not necessarily a public client API.**
4. **Custom layout is best viewed as an advanced extension path, not the default component model.**
5. **Invalidation can remain framework-owned even when custom geometry is supported.**
6. **Parent-specific child layout metadata does not inherently require `dynamic_cast`.** SwiftUI's layout values demonstrate a generic alternative. citeturn834078search3turn834078search6
7. **One runtime Node does not necessarily imply a God object** if layout/content/runtime state is internally composed rather than exposed as one giant public surface.
8. **The final choice must be tested against text first**, then Button, Stack, Grid, Flex and custom/RichText.

The next research step should therefore focus on the boundary between **content measurement and layout orchestration**, before selecting concrete C++ types such as `LayoutStyle`, `LayoutStrategy`, `LayoutData` or `LayoutPolicy`.
