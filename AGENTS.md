# AGENTS.md

Repository-specific instructions for AI coding agents working on SynclairVision's QGroundControl fork.

## Scope and authority

- These instructions apply only to `SynclairVision/qgroundcontrol`.
- Read this file before planning or editing. Then read the closest relevant source, build files, and documentation.
- Follow direct user instructions first. Treat this file as the repository default when the user is silent.
- Do not copy assumptions, commands, or architecture rules from another repository.
- Use only commands and paths verified in this repository. If something is unknown, inspect it or say it is unknown.
- Keep this file current when workflows, tooling, or repository policy changes.

## Required Preflight

Before modifying any repository file:

1. Read [CODING_STYLE.md](CODING_STYLE.md) completely.
2. Read the task-specific references listed in this file.
3. Do not begin editing until those reads are complete.
4. Before the first edit, state in a progress update which instruction files were read.

## Working principles

1. Understand the request and trace the relevant code path before editing.
2. Check the working tree and preserve all existing user or agent changes.
3. Prefer the smallest complete change that fixes the root cause.
4. Match nearby code and the conventions in [CODING_STYLE.md](CODING_STYLE.md).
5. Build or validate in small increments during multi-file work.
6. Review the final diff for correctness, scope, generated files, secrets, and accidental churn.
7. Report exactly what changed, what was validated, and what could not be validated.

Do not hide uncertainty, invent results, or claim a command passed when it was not run successfully.

## Golden Rules (enforced — violations fail CI)

- **Fact System** — ALL vehicle parameters flow through Facts; never create custom parameter storage.
- **Multi-Vehicle** — ALWAYS null-check `activeVehicle()` / `Vehicle*` before dereferencing (`vehicle-null-check`).
- **Firmware Plugin** — use `vehicle->firmwarePlugin()` for firmware-specific behavior, not `if (px4)` branches.
- **QML Integration** — register types with `QML_ELEMENT`/`QML_SINGLETON`/`QML_UNCREATABLE`; expose state via `Q_PROPERTY`.
- **No `Q_ASSERT` in production code** — use defensive checks with early returns (`check-no-qassert`).
- **No `QTest::ignoreMessage`** in tests — use `expectLogMessage`/`ignoreLogMessage` (`check-no-qtest-ignore-message`).
- **No fixed-delay `QTest::qWait(<n>)`** — use `QTRY_*_WITH_TIMEOUT` or `QSignalSpy::wait` (`check-no-fixed-qwait`).
- **Comments** — prefer self-documenting code; comment only non-obvious intent, constraints, or workarounds.

## Repository map

- `src/` - application code
- `src/SynclairVision/` - SynclairVision-specific integrations, including DigiView
- `src/Vehicle/` - vehicle state and communication
- `src/Comms/` - serial, UDP, TCP, and other links
- `src/FactSystem/` - parameters and typed application facts
- `src/FirmwarePlugin/` - PX4 and ArduPilot abstraction
- `src/MissionManager/` - mission planning and transfer
- `src/VideoManager/` - video pipeline
- `src/FlyView/`, `src/PlanView/`, `src/QmlControls/` - QML user interface
- `cmake/`, `CMakeLists.txt` - build configuration
- `tools/`, `justfile` - development and validation tooling
- `docs/` - documentation
- `test/` - existing upstream test code; temporarily out of scope for agents

Useful references:

- [CODING_STYLE.md](CODING_STYLE.md) - C++20, Qt, QML, naming, formatting, and logging
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) - architecture background
- [tools/README.md](tools/README.md) - supported development tooling
- [.github/ci-overview.md](.github/ci-overview.md) - CI structure and commands
- [.pre-commit-config.yaml](.pre-commit-config.yaml) - enforced static checks

## Architecture invariants

- Vehicle parameters must flow through the Fact System. Do not introduce parallel parameter storage.
- Always null-check `activeVehicle()` and any `Vehicle*` before dereferencing.
- Put firmware-specific behavior behind `vehicle->firmwarePlugin()`; avoid scattered PX4/ArduPilot branches.
- Expose C++ state to QML with the appropriate `QML_ELEMENT`, `QML_SINGLETON`, or `QML_UNCREATABLE` macro and `Q_PROPERTY`.
- Preserve QObject ownership, thread affinity, signal/slot lifetime, and shutdown order. Make ownership explicit when it is not obvious.
- Validate external input and shared state at boundaries. For partial state updates, change only intended fields.
- Keep SynclairVision-specific behavior localized under existing extension points where practical. Do not fork upstream behavior unnecessarily.
- Do not use `Q_ASSERT` for recoverable production conditions. Use defensive checks, useful categorized logging, and safe returns.
- Use categorized Qt logging; do not add uncategorized `qDebug()` calls.
- QML must use QGC controls, `ScreenTools` sizing, `QGCPalette` colors, translated user-facing strings, and Qt 6 `Connections` function syntax.

Before changing a subsystem, read its public header, implementation, registration/composition point, and relevant CMake file. For vehicle or parameter work, start with `src/Vehicle/Vehicle.h`, `src/FactSystem/Fact.h`, and `src/FirmwarePlugin/FirmwarePlugin.h` as applicable.

## Code quality

- Use clear names, small focused functions, and comments that explain non-obvious intent or constraints.
- Maintain established public APIs unless the request requires a deliberate change.
- Handle failure paths explicitly. Do not swallow errors or replace them with ambiguous fallbacks.
- Avoid unrelated refactors, speculative abstractions, duplicate logic, dead code, commented-out code, and vague TODOs.
- Do not introduce new dependencies without explaining why existing Qt, C++ standard library, or repository facilities are insufficient.
- Keep headers minimal and use the include ordering defined in [CODING_STYLE.md](CODING_STYLE.md).
- Preserve formatting: 4 spaces, UTF-8, LF line endings, and the repository's 120-column limit.
- For UI changes, consider empty, loading, disconnected, error, and multi-vehicle states.
- For asynchronous or hardware-facing code, reason about cancellation, reconnects, timeouts, stale callbacks, and teardown.

## Agent workflow and collaboration

- Write a short plan for changes spanning multiple files or subsystems.
- Search for existing implementations before creating a new pattern.
- Do not overwrite or revert changes you did not make.
- If multiple agents are used, divide work into non-overlapping areas. One agent owns integration and reviews every contributed diff.
- Do not assume another agent validated your work. The integrating agent is responsible for final verification.
- Stop and ask when requirements conflict, a destructive action is needed, credentials are missing, or the intended behavior cannot be determined safely.
- Never commit secrets, tokens, private keys, local configuration, logs containing credentials, or personal data.

## Build and validation commands

The `justfile` is the command source of truth. Run commands from the repository root.

```bash
just submodules     # initialize recursive submodules
just configure      # configure the Debug build
just build          # incremental build
just release        # configure and build Release
just lint           # run all pre-commit checks
just format         # check C++ formatting without modifying files
just format-fix     # apply C++ formatting when needed
just analyze        # run configured static analysis
just docs           # build documentation
just info           # show resolved build configuration
just check-deps     # verify dependency versions
```

- Use `JOBS=N` to limit build parallelism when necessary.
- `just deps`, `just setup`, and some platform provisioning may use elevated privileges or make broad machine changes. Do not run them unless explicitly requested.
- `just clean`, `just rebuild`, and `just distclean` remove generated state. Inspect the exact target and obtain confirmation when the deletion is not clearly required.
- A full QGroundControl build requires the configured Qt and platform dependencies. If the environment cannot build, run the safest relevant static checks and report the limitation.
- Prefer targeted checks for changed files during iteration, then the appropriate repository-level check before handoff.
- Formatting tools may modify files. Review their diff and keep only changes relevant to the task.

## Temporary test policy

Testing is intentionally not part of the current SynclairVision agent workflow.

- Do not modify, add, delete, or generate files under `test/`.
- Do not create new unit, integration, snapshot, or end-to-end tests anywhere in the repository.
- Do not run `just test`, `ctest`, coverage commands, test targets, or test scripts.
- Do not run `just check`, because it includes `just test`.
- Do not enable or expand test infrastructure as part of another task.
- Do not use missing tests as a reason to broaden the requested change.
- Leave the existing upstream `test/` directory intact.
- This temporary policy overrides test guidance in other repository documents for AI-agent work.
- If a user explicitly requests tests, call out this policy and ask for confirmation before proceeding.

Compensate with careful code review, relevant builds, formatting checks, linting, static analysis, and clear manual verification steps. Never describe a change as tested; state the exact non-test validation performed.

## Definition of done

Keep pull request history intentional and easy to review. Organize related work into coherent commits and
combine fixups or closely related incremental changes before publication. Avoid a trail of tiny work-in-progress,
cleanup, or review-fix commits. Each commit should represent a clear, reviewable change without mixing unrelated work.

Your output will be reviewed by another AI agent before being accepted. Keep changes focused and
minimal, use clear naming, and leave explanatory commit messages. Avoid unrelated changes,
commented-out code, or ambiguous TODOs.

A change is complete when:

1. The requested behavior is implemented with focused, maintainable code.
2. Relevant build, format, lint, static-analysis, or documentation checks pass where the environment supports them.
3. No test commands were run and `test/` is unchanged.
4. The final diff contains no unrelated edits, generated output, debug artifacts, secrets, or accidental dependency changes.
5. User-facing behavior and non-obvious design decisions are documented when needed.
6. The handoff lists changed files, successful validation commands, skipped validation with reasons, and remaining risks.

Do not weaken warnings, linters, or CI configuration merely to make validation pass.

## Generated and dependency content

- Do not edit build directories, CMake-generated files, packaged artifacts, coverage output, caches, or other ignored output.
- Do not edit `node_modules/` or vendored/submodule contents unless the request explicitly targets them.
- Initialize submodules when needed, but do not update their pinned revisions incidentally.
- Do not commit machine-local paths or configuration such as `CMakeUserPresets.json`, `.env` files, or IDE state.

## Commits and review

Use Conventional Commits. Examples:

- `fix(Vehicle): guard disconnected vehicle access`
- `feat(Digiview): expose connection health to QML`
- `docs: clarify agent validation workflow`

Keep commits focused. The final review should prioritize correctness, regressions, lifetime and concurrency issues, architecture violations, unsafe failure handling, and unintended scope. Do not approve work solely because it builds or formats cleanly.
