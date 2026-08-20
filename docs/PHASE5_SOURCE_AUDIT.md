# Phase 5 — Source Audit

This is a lightweight audit of source files that need removal, review, or future architectural work. It is not a build-system specification.

## Remove / already obsolete

The following old abstractions no longer belong to the current component architecture:

```text
components/component.hpp
components/paper.hpp
components/paper.cpp
```

They were removed because they belong to the previous component model and/or depend on obsolete `Widget` infrastructure.

The deferred `List` implementation was also removed because the current List contract did not justify a separate component.

## User-owned cleanup

The following legacy files are intentionally left for the user's manual cleanup as previously agreed:

```text
components/label.hpp
components/label.cpp
components/flex_panel.hpp
components/flex_panel.cpp
core/controlnode.hpp
core/controlnode.cpp
```

## Deferred, not obsolete

`Modal` is intentionally retained as a deprecated/inactive implementation reference until Phase 6. It should not be treated as the final component implementation.

`modalmanager` is likewise not removed by this audit because Phase 6 modality work will determine which parts of that infrastructure remain valid.

## Review candidates

`core/primitives.hpp/.cpp` remains useful as a low-level SDL drawing layer. Its role is documented in `PRIMITIVES_ROLE.md`.

`core/text_primitive.hpp/.cpp` remains a useful specialized primitive because text measurement/rendering has intrinsic-size and reusable presentation behavior.

`core/textnode.hpp/.cpp` should be reviewed later against the final primitive/component boundary; it may remain useful as a Node-level text wrapper, but it should not be retained merely because `TextPrimitive` exists.

`core/gridnode.cpp` is a review candidate because Grid is not currently part of the minimal Phase 5 component catalog. Its final status should be decided after checking whether Grid is still required as generic layout infrastructure rather than as an old component abstraction.

## Current source structure target

The intended direction is:

```text
core/
    framework infrastructure
    layout infrastructure
    rendering primitives

components/
    standard UI components

application/
    application-specific composition (outside this framework)
```

The presence of a source file is not itself evidence that the abstraction belongs in the final framework. Every retained file should have a current architectural role.
