# AGENTS.md

Instructions for AI coding agents (Codex, Claude Code, etc.) working on QGroundControl.

## Quick References

- [CODING_STYLE.md](CODING_STYLE.md) — Naming, formatting, C++20 features, QML style, logging
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) — Architecture patterns (Fact System, Multi-Vehicle, FirmwarePlugin)
- [tools/README.md](tools/README.md) — Development scripts, tooling, and every `just` recipe
- [test/README.md](test/README.md) — Test framework, base classes, CTest labels, MultiSignalSpy, sanitizers, coverage
- [cmake/README.md](cmake/README.md) — CMake modules, presets, and test options
- [.github/ci-overview.md](.github/ci-overview.md) — CI workflow/action/script layout and conventions
- [.pre-commit-config.yaml](.pre-commit-config.yaml) — Lint hooks (`just lint` runs every hook not marked `manual`)

## Required Preflight

Before modifying any repository file:

1. Read [CODING_STYLE.md](CODING_STYLE.md) completely.
2. Read the task-specific references listed in this file.
3. Do not begin editing until those reads are complete.
4. Before the first edit, state in a progress update which instruction files were read.

## Golden Rules

These are the non-negotiables. The first four are QGC's core architecture patterns. The pre-commit hook named
in a rule fails CI on a violation. Full list with code examples:
[.github/CONTRIBUTING.md#architecture-patterns](.github/CONTRIBUTING.md#architecture-patterns) and
[CODING_STYLE.md#common-pitfalls](CODING_STYLE.md#common-pitfalls).

- **Fact System** — ALL vehicle parameters flow through Facts; never create custom parameter storage.
- **Multi-Vehicle** — ALWAYS null-check `activeVehicle()` / `Vehicle*` before dereferencing (`vehicle-null-check`
  flags unchecked `activeVehicle()` results).
- **Firmware Plugin** — use `vehicle->firmwarePlugin()` for firmware-specific behavior, not `if (px4)` branches.
- **QML Integration** — register types with `QML_ELEMENT`/`QML_SINGLETON`/`QML_UNCREATABLE`; expose state via `Q_PROPERTY`.
- **No `Q_ASSERT` in production code** — use defensive checks with early returns (`check-no-qassert`).
- **No `QTest::ignoreMessage`** in tests — use `expectLogMessage`/`ignoreLogMessage` (`check-no-qtest-ignore-message`).
- **No fixed-delay `QTest::qWait(<n>)`** — use `QTRY_*_WITH_TIMEOUT` or `QSignalSpy::wait` (`check-no-fixed-qwait`).
- **Comments** — prefer self-documenting code; comment only non-obvious intent, constraints, or workarounds.

## Critical Files (Read First!)

1. `src/FactSystem/Fact.h` — Parameter system foundation
2. `src/Vehicle/Vehicle.h` — Core vehicle model
3. `src/FirmwarePlugin/FirmwarePlugin.h` — Firmware abstraction

## Code Structure

Key modules (`src/` has about 30 subdirectories):

```text
src/
├── Vehicle/          # Vehicle state/comms
├── Comms/            # Link layer (serial, UDP, TCP, Bluetooth)
├── FactSystem/       # Parameter management
├── FirmwarePlugin/   # PX4/ArduPilot abstraction
├── AutoPilotPlugins/ # Vehicle setup UI
├── MissionManager/   # Mission planning
├── MAVLink/          # Protocol handling
├── VideoManager/     # Video pipeline (GStreamer)
├── FlyView/          # In-flight UI
├── PlanView/         # Mission planning UI
├── QmlControls/      # Reusable QML components
└── Settings/         # Persistent settings
```

## Build & Test Commands

The `just` recipes are the canonical workflow — see [tools/README.md](tools/README.md) for the full list. They
configure and build `build/` with the `default*` preset for `BUILD_TYPE` (default `Debug`, tests enabled).

```bash
just configure          # CMake configure through the default preset
just build              # incremental build; uses all cores (override with JOBS=N)
just test               # ctest --preset default, LABELS="Unit|Integration" EXCLUDE="Flaky|Network"
just lint               # prek run --all-files (the pre-commit hooks)
just lint-docs          # only the docs hooks, on docs/en pages (pass --changed for branch changes)
just test-python        # pytest for tools/tests (CI script tests in tools/tests/ci)
just check              # lint + test-python + test
just format-fix         # apply clang-format to changed lines
just info               # print configured versions (Qt, CMake minimum, GStreamer) and build settings
```

- **Build incrementally** — rebuild every few file edits during multi-file C++/Qt work, not just at the end; fix build errors before continuing.
- **Tight test loops** — iterate one test with `ctest --test-dir build -R <name>` or
  `build/Debug/QGroundControl --unittest:<name>` (Linux path); run `just test` on the final pass.
- **Match CI** — Linux CI runs the `Unit` and then the `Integration` label as separate CTest runs, excluding
  `Flaky|Network`, in the `Linux-coverage` and `Linux-sanitizers` (ASan + UBSan; also excludes `NoSanitizer`)
  presets. Reproduce a sanitizer failure with `cmake --workflow --preset Linux-sanitizers` (needs `QT_ROOT_DIR`)
  and coverage with `just coverage`. CI lints only the files changed from the PR base, like
  `uv run --frozen --project tools --group dev python tools/pre_commit.py --changed`.
  See [.github/ci-overview.md](.github/ci-overview.md) for the rest.

## Definition of Done

Before considering a change complete:

1. `just build` succeeds.
2. `just lint` passes.
3. Relevant tests pass: `ctest --test-dir build -R <name>` for the touched area, `just test-python` for Python
   tooling, and `just test` on the final pass.
4. Commit message follows Conventional Commits (below).

## Commit & Review Conventions

Commit messages follow **Conventional Commits** — the type decides the release-notes section
(`tools/configs/cliff.toml` → git-cliff). Use: `feat`, `fix`, `perf`, `revert` (listed in release notes);
`docs`, `style`, `chore`, `refactor`, `test`, `build`, `ci` (omitted unless breaking). Example: `fix(Vehicle): guard null activeVehicle in telemetry handler`.
`prek install` adds a `commit-msg` hook that checks this format; Renovate's `deps` type, `fixup!`/`squash!`,
merge, and revert messages also pass.

Keep pull request history intentional and easy to review. Organize related work into coherent commits and
combine fixups or closely related incremental changes before publication. Avoid a trail of tiny work-in-progress,
cleanup, or review-fix commits. Each commit should represent a clear, reviewable change without mixing unrelated work.

Your output will be reviewed by another AI agent before being accepted. Keep changes focused and
minimal, use clear naming, and leave explanatory commit messages. Avoid unrelated changes,
commented-out code, or ambiguous TODOs.

---

**Key Principle**: Match the style of code you're editing. See [CODING_STYLE.md](CODING_STYLE.md) for conventions and [CODING_STYLE.md#examples](CODING_STYLE.md#examples) for canonical Vehicle/Fact/QML snippets.
