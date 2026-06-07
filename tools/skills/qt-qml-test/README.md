# Qt QML Test

Generate Qt Quick Test cases (`tst_*.qml`) for QML components
using `TestCase`, `SignalSpy`, `tryCompare`, and the rest of
the Qt Quick Test API. Targets Qt 6.

## What it does

1. **Reads the source QML** — the component(s) the user wants
   tests for. Handles single files and batches; emits one
   `tst_*.qml` per source.
2. **Reads bounded project context** — when the source imports
   a sibling custom component, reads that one file once to
   learn its declared properties and signals. Does not recurse,
   does not read framework files.
3. **Generates a test** using a strict canonical template with
   `Item` wrapper, `Component` block, `TestCase { when:
   windowShown; ... }`, and `createTemporaryObject` lifecycles.
4. **Applies 47 testing rules** consolidated from the canonical
   Qt AI Assistant Qt Quick Test prompt — covering imports,
   structure, properties, signals, mouse and key events,
   conventions, per-control specifics, Window / singleton
   sources, pointer handlers, click-target sizing, and
   Qt Quick 3D source handling.
5. **Writes the test file(s) to disk** — defaults to
   `tests/tst_<ComponentName>.qml`, creating the directory if
   needed, and reports the absolute path(s) created.

## What it covers

- **Property values** — defaults, read/write, computed
  bindings, the `.background` accessor, aliases.
- **Qt Quick Controls** — Button, TextField / TextArea /
  TextEdit / TextInput, Slider, SpinBox, Dial, Dialog (incl.
  FileDialog / FolderDialog / ColorDialog / MessageDialog),
  MenuItem, Image, MouseArea, TapHandler / HoverHandler,
  Accessible signals, NumberAnimation, RegularExpressionValidator.
- **Signal verification** — `SignalSpy` patterns per control,
  multi-instance setup, `tryCompare` against `count`.
- **Multi-document workflows** — emits one test file per
  source QML, 1:1 layout.

## What it does NOT cover

- Building and running the generated tests — use the
  `qt-qml-test-run` companion skill for that. It detects or
  wires up CMake test infrastructure, builds, runs via
  CTest or `qmltestrunner`, and writes a Markdown report.
- Reviewing or refactoring existing tests.
- C++ Qt Test (`QTEST_MAIN`), Squish, Qt Quick 3D, and
  Qt Creator IDE test integration.

## Usage

Open your QML project in your assistant, then ask for tests:

- "Write tests for this QML."
- "Generate Qt Quick Tests for `widgets/MyButton.qml`."
- "Write tests for everything under `app/forms/`."
- "Test that `submitButton` emits the clicked signal."

The skill reads the source(s), applies the rules, and writes
`tst_<ComponentType>.qml` files under `tests/`.

## Requirements

- Qt 6 — the canonical template uses `import QtQuick` and
  `import QtTest` without version numbers, which requires
  Qt 6.
- A QML source file or a directory of them. The skill does not
  scaffold from nothing; it tests existing components.

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |
| **Gemini CLI** | `gemini extensions install https://github.com/TheQtCompanyRnD/agent-skills` |
| **GitHub Copilot** | Use the self-contained variant in `platforms/copilot.prompt.md` |

## Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Full skill instructions: scope, guardrails, template, 47-rule index, references |
| `references/qt-quick-test-rules.md` | Full normative text of all 47 rules with examples and rationale (loaded on first generation) |
| `references/qt-quick-test-template.md` | Canonical template variants (single, nested, focus, multi-instance, dialog, press/move/release, Window, singleton) |
| `references/qt-quick-test-controls.md` | Per-control patterns for interaction and signal testing |
| `references/qt-quick-test-properties.md` | Property testing patterns and exclusions |
| `references/qt-quick-test-pitfalls.md` | Symptom-keyed anti-patterns derived from the negative rules |
| `references/qt-quick-test-project-context.md` | Bounded-read set (source, direct imports, `qmldir`, nearest `CMakeLists.txt`) |
| `references/qt-quick-test-source-import.md` | Source-import resolution: library vs executable backing, module-on-executable refactor |
| `references/qt-quick-test-pre-send-scan.md` | Token list and rewrite procedure for keeping user-facing messages free of skill-internal references |
| `platforms/copilot.prompt.md` | Self-contained variant for GitHub Copilot |

## Companion skills

- `qt-qml-test-run` — builds and runs the generated
  `tst_*.qml` files via CTest or `qmltestrunner`, then writes
  a structured Markdown report. Use it after this skill.
- `qt-qml-profiler` — runs `qmlprofiler` and analyzes
  performance hotspots. Same locate-build-run-parse-report
  architecture as `qt-qml-test-run`.

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
