# AGENTS.md

Instructions for AI coding agents (Codex, Claude Code, etc.) working on QGroundControl.

## Quick References

- [CODING_STYLE.md](CODING_STYLE.md) — Naming, formatting, C++20 features, QML style, logging
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) — Architecture patterns (Fact System, Multi-Vehicle, FirmwarePlugin)
- [tools/README.md](tools/README.md) — Development scripts and tooling
- [test/TESTING.md](test/TESTING.md) — Test framework, base classes, CTest labels, MultiSignalSpy, coverage
- [.pre-commit-config.yaml](.pre-commit-config.yaml) — All enforced linters (clang-format, clang-tidy, ruff, pyright, shellcheck, actionlint, zizmor, qmllint, clazy, vehicle-null-check, check-no-qassert, check-no-qtest-ignore-message)

## Review Process

Your output will be reviewed by another AI agent before being accepted. Write code and commit messages that are easy to machine-review: keep changes focused and minimal, use clear naming, and leave explanatory commit messages. Avoid unrelated changes, commented-out code, or ambiguous TODOs.

## Critical Files (Read First!)

1. `src/FactSystem/Fact.h` - Parameter system foundation
2. `src/Vehicle/Vehicle.h` - Core vehicle model
3. `src/FirmwarePlugin/FirmwarePlugin.h` - Firmware abstraction

## Build & Test Commands

- Recommended workflow: [tools/README.md](tools/README.md) Quick Start (`just configure / build / test / lint / check`).
- Testing details (CTest labels, coverage, sanitizers): [test/TESTING.md](test/TESTING.md).
- CI Python script tests: [.github/ci-overview.md](.github/ci-overview.md#tests).

## Golden Rules

See [.github/CONTRIBUTING.md#architecture-patterns](.github/CONTRIBUTING.md#architecture-patterns) for the canonical list (Fact System, Multi-Vehicle null-check, FirmwarePlugin, QML integration) and [CODING_STYLE.md#common-pitfalls](CODING_STYLE.md#common-pitfalls) for the full pitfall list with code examples.

## Code Structure

```
src/
├── Vehicle/          # Vehicle state/comms
├── FactSystem/       # Parameter management
├── FirmwarePlugin/   # PX4/ArduPilot abstraction
├── AutoPilotPlugins/ # Vehicle setup UI
├── MissionManager/   # Mission planning
├── MAVLink/          # Protocol handling
├── QmlControls/      # Reusable QML components
└── Settings/         # Persistent settings
```

## CI Structure

See [.github/ci-overview.md](.github/ci-overview.md) for the workflow/action/script layout and CI conventions (dependencies, shared helpers, bootstrap scripts, build config, GitHub Actions output).

---

**Key Principle**: Match the style of code you're editing. See [CODING_STYLE.md](CODING_STYLE.md) for conventions and [CODING_STYLE.md#examples](CODING_STYLE.md#examples) for canonical Vehicle/Fact/QML snippets.
