# Qt QML Test Runner

Build and run Qt Quick Test (`TestCase` / `qmltestrunner`)
tests for a Qt 6 / CMake project, then write a structured
Markdown report. Companion to the `qt-qml-test` skill (which
authors `tst_*.qml` files) — does not duplicate that skill's
generation logic.

## What it does

1. **Locates the Qt toolchain** — resolves `qmltestrunner`
   from `CLAUDE.md`, environment (`CMAKE_PREFIX_PATH`,
   `QTDIR`, `Qt6_DIR`), `PATH`, or common install locations
   on Linux, macOS, and Windows.
2. **Discovers the test target** — uses the path passed in
   `$ARGUMENTS`, or scans the project root for `tst_*.qml`
   files.
3. **Detects existing CMake test wiring** — greps the
   project's CMakeLists.txt files for the canonical
   patterns. Skips Step 5 when wiring is present.
4. **Wires up missing infrastructure** (with `--wire-up`):
   - Writes `tests/CMakeLists.txt` (`QUICK_TEST_MAIN`
     harness) and `tests/main.cpp` — new files only, never
     overwrites.
   - Prints the proposed three-line addition to the root
     `CMakeLists.txt` and applies it after explicit user
     confirmation.
   - Without `--wire-up`, the skill defaults to direct
     `qmltestrunner` invocation when wiring is missing —
     tests run with no file modifications.
5. **Builds** with `cmake -B build -DCMAKE_PREFIX_PATH=…`
   and `cmake --build build`. Surfaces compiler stderr on
   failure; does not run tests against a failed build.
6. **Runs tests** by invoking the built test binary directly
   (for CMake projects with wiring), or `qmltestrunner`
   directly (for the Standalone and Direct paths). Avoids
   `ctest --output-junit`, which reports test counts at
   CTest-target granularity, not per QML test function.
7. **Parses the JUnit XML** with a bundled Python script;
   emits a JSON summary with per-case detail and the 10
   slowest cases.
8. **Writes a Markdown report** under `build/tests/reports/`
   with summary, failed tests, slowest tests, and skipped
   tests.
9. **Prints a console summary** — verdict line, top 3
   failures, path to the report.

## Usage

**Run tests** — discover and run, write a report. Uses
existing CMake test wiring if present; otherwise falls back
to direct `qmltestrunner` (no file modifications):

```
[<path-or-dir>]
```

**Run with auto-wiring** — let the skill add missing CMake
test infrastructure:

```
--wire-up [<path-or-dir>]
```

**Skip the build** — re-run against an existing build (only
useful when iterating on report formatting):

```
--no-build [<path-or-dir>]
```

**Skip writing the Markdown report** — useful in tight
test-fix-test loops where the console summary is enough.
The JUnit XML at Step 7 is still written, so Section 4
(prior-run change detection) still has a baseline for the
next normal run:

```
--no-report [<path-or-dir>]
```

When `<path-or-dir>` is omitted, the skill scans the project
root.

## A note on report accumulation

Every normal run timestamps a fresh
`build/tests/reports/test-report-*.md` and
`build/tests/reports/junit/qmltests-*.xml`. The skill does not
rotate or clean these up — that is intentional, since the
JUnit XMLs feed Section 4's prior-run baseline on subsequent
runs. Over a long-running project, `build/tests/reports/`
grows.

If local accumulation is unwanted, pass `--no-report` on
iterative runs to suppress the Markdown report while still
letting the JUnit XML accumulate (small, infrequently
inspected).

## How to use

1. Install the skill (see Installation below) and open your
   QML project in your assistant.
2. (Optional) Generate tests first with the `qt-qml-test`
   skill. This skill expects `tst_*.qml` files to already
   exist on disk.
3. Invoke this skill with the argument forms above.
4. If the project has no test infrastructure, re-invoke with
   `--wire-up` after reviewing the printed recipe.
5. Read the generated report under `build/tests/reports/` for
   the pass/fail breakdown, failure detail, and slowest cases.

## Requirements

- Qt 6 installation containing `bin/qmltestrunner`.
- A CMake-based project. qmake is not supported.
- CMake ≥ 3.21 and a C++ toolchain (only when building; the
  parser is pure Python).
- Python 3 to run the JUnit XML parser.

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |
| **Gemini CLI** | `gemini extensions install https://github.com/TheQtCompanyRnD/agent-skills` |

This skill builds and runs binaries and writes report files,
so it targets Tier-1 platforms (Claude Code, Codex CLI) that
have shell access. GitHub Copilot and similar in-IDE
assistants without a build environment are not supported.

## Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Full skill instructions with all 10 steps |
| `references/qt-quick-test-cmake.md` | C++ harness CMake recipe, module-on-executable refactor, detection patterns, common failure modes |
| `references/qt-quick-test-report-format.md` | Markdown report format: eight sections, omit conditions, content rules |
| `references/scripts/parse-qmltestrunner-output.py` | JUnit XML parser emitting a JSON summary |

## Companion skills

- `qt-qml-test` — generates `tst_*.qml` files. Use it first
  when no tests exist yet, then this skill to build and run.
- `qt-qml-profiler` — same architecture (locate-build-run-
  parse-report), but for `qmlprofiler` performance traces.

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
