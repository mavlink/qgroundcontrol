# Qt Project Skill

Helps AI coding tools set up and evolve Qt 6 projects built with
CMake. Corrects systematic LLM biases — qmake leftovers, the
legacy `qt5_*` macros, raw `.qrc` files for QML — and produces
project layouts that align with the modern Qt 6 CMake API.

## What it does

Activates whenever a Qt 6 project's `CMakeLists.txt` is being
written or edited, or when the user asks to:

- bootstrap a fresh Qt project with the right top-level layout
- add a runnable application binary (`qt_add_executable`)
- add a `add_subdirectory()` subproject
- create a Qt library (`qt_add_library`)
- create or grow a QML module (`qt_add_qml_module`)
- add a Qt plugin (`qt_add_plugin`) and wire it into the project
- add a `.qml` file or a reusable QML UI control to an existing
  module
- organise sources into folders (`src/`, `qml/`, `resources/`,
  `cmake/`)
- add static resources — images, icons, fonts, translations

The skill is opinionated: it always prefers the modern Qt 6
`qt_*` commands and `qt_standard_project_setup()`, never the
qmake `.pro` workflow and never `qt5_*` macros.

## Documentation lookup

When the agent is unsure of a CMake command's exact signature, the
skill instructs it to query a Qt docs MCP tool (e.g. `qt-docs`) if
available, falling back to a web fetch of
[`doc.qt.io/qt-6/cmake-manual.html`](https://doc.qt.io/qt-6/cmake-manual.html)
and per-command reference pages.

## Requirements

- Qt 6.8 or newer (the skill's defaults assume API 6.8, however this is a soft requirement)
- No external script dependencies — the skill is purely prompt-based

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |
| **GitHub Copilot** | `gh skill install TheQtCompanyRnD/agent-skills qt-project` (preview) — or auto-discovered from `.claude/skills/` |
| **Gemini CLI** | `gemini extensions install https://github.com/TheQtCompanyRnD/agent-skills` |

## Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Entry point with hard rules and decision tree |
| `references/simple-project.md` | Simple project, executable, flat structure |
| `references/modular-architecture.md` | Subprojects, libraries, plugins |
| `references/qml-integration.md` | QML modules, files, reusable controls |
| `references/resources.md` | Images, icons, fonts, translations |
| `references/common-mistakes.md` | LLM bias correction checklist |
| `references/configure.md` | Instructions on trying out the project |

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
