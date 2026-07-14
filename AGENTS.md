# AGENTS.md

Instructions for AI coding agents (Codex, Claude Code, etc.) working on QGroundControl. This file is
the agent-facing entry point; the linked topic guides remain authoritative for their domains.

## Quick References

- [CODING_STYLE.md](CODING_STYLE.md) — Naming, formatting, C++20 features, QML style, logging
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) — Architecture, contribution, commit, and pull-request workflow
- [tools/README.md](tools/README.md) — Development scripts and tooling
- [test/README.md](test/README.md) — Test framework, base classes, CTest labels, MultiSignalSpy, coverage
- [.github/README.md](.github/README.md) — CI workflow/action/script layout and conventions
- [.pre-commit-config.yaml](.pre-commit-config.yaml) — All enforced linters (clang-format, clang-tidy, ruff, pyright, shellcheck, actionlint, zizmor, qmllint, clazy, vehicle-null-check, check-no-qassert, check-no-qtest-ignore-message)

## Required Rules

Before editing source, read and follow the canonical [coding rules and common
pitfalls](CODING_STYLE.md#common-pitfalls). Before editing tests, also read the test guide's
[logging](test/README.md#expected-log-messages) and [wait/timeout](test/README.md#waittimeout-helpers)
rules. The corresponding pre-commit hooks are listed in [.pre-commit-config.yaml](.pre-commit-config.yaml).

Before changing vehicle parameters, multi-vehicle behavior, or firmware-specific integration, read
the contributor guide's [architecture entry points](.github/CONTRIBUTING.md#architecture-entry-points).

## Build & Test Workflow

Use the canonical [`just` recipes](tools/README.md#just-command-reference). When reproducing CI, use
the commands documented in the [CI overview](.github/README.md#tests) rather than guessing.

- **Build incrementally** — rebuild every few file edits during multi-file C++/Qt work, not just at the end; fix build errors before continuing.
- **Tight test loops** — iterate one test with `ctest -R <name>` (or `--gtest_filter`); for
  source changes, run the full label on the final pass. CI runs
  `ctest --output-on-failure -L Unit`.
- **Focused TDD coverage** — add tests when changed behavior or regression risk warrants them. Write
  the smallest clear set that covers the salient contract; avoid redundant cases and
  implementation-detail assertions.

## Definition of Done

Before considering a change complete, validate the scope you changed:

1. Source and build-system changes compile with `just build`.
2. Relevant lint and format checks pass for the changed files; use `just lint` for the full
   repository gate before submission.
3. Relevant tests pass. For source changes, iterate with `ctest -R <name>` and run the full
   `-L Unit` label on the final pass. For documentation- or CI-only changes, run the applicable
   Markdown, link, schema, and workflow checks instead of unrelated C++ builds and tests.
4. If a commit is requested, follow the contributor guide's [commit-message
   convention](.github/CONTRIBUTING.md#commit-messages).

## Review Conventions

Your output will be reviewed by another AI agent before being accepted. Keep changes focused and
minimal, use clear naming, and avoid unrelated changes, commented-out code, or ambiguous TODOs. If
a commit is requested, use the convention defined in
[CONTRIBUTING.md](.github/CONTRIBUTING.md#commit-messages).

- **Avoid incidental reformatting** — when changing part of an existing file, format only the
  edited lines or region. Do not reformat the entire file unless the user explicitly requests it or
  the file is newly created.

---

**Key Principle**: Match the style of code you're editing. See [CODING_STYLE.md](CODING_STYLE.md) for conventions and [CODING_STYLE.md#examples](CODING_STYLE.md#examples) for canonical Vehicle/Fact/QML snippets.
