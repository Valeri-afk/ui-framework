# UI Framework

C++20 UI framework built on SDL3.

The repository contains the current framework source code and
architecture documentation used during development.

## Development

The framework is developed in independent architectural phases.

Current development is in **Phase 2 — Layout** on the `phase2-layout-migration`
branch.

For recovering the development context in a new chat, start with:

- [Phase 2 Handoff / Current Context](docs/PHASE2_HANDOFF.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Development Roadmap](docs/ROADMAP.md)
- [Development Instructions](docs/INSTRUCTIONS.md)

The source code is the authoritative source of truth.

Build/runtime tests are intentionally deferred until the end of Phase 6 by
project decision; Phase 2 work is therefore validated through source
inspection, architectural reasoning and documented numerical cases.
