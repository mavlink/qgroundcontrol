# Qt C++ Documentation

Generates structured Markdown reference documentation for any
Qt/C++ source file — classes, structs, free-function headers,
utility files, and application entry points like `main.cpp`.

## What it does

Reads C++ header and source files (along with related files such
as `CMakeLists.txt`, `.ui` files, `.qrc` files, and `qmldir`)
and produces developer-friendly Markdown reference docs. The
skill selects the appropriate document structure based on the
file type:

- **Qt classes** with `Q_OBJECT`, signals/slots, and properties
- **Plain C++ classes and structs** with no Qt macros
- **Free-function headers** (utility APIs, helper namespaces)
- **Application entry points** (`main.cpp`) — startup sequence,
  Qt application setup, command-line handling, and top-level
  object wiring

For single files it generates one `.md` file; for folders it
documents every meaningful `.h` and `.cpp` file and creates an
`index.md` linking them all.

## Requirements

- No external dependencies

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |
| **Copilot** | `gh skill install TheQtCompanyRnD/agent-skills qt-cpp-docs` (preview) — or auto-discovered from `.claude/skills/` |
| **Cursor** | Copy `SKILL.md` to `.cursor/rules/qt-cpp-docs/RULE.md` |

## Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Full skill instructions (409 lines) |

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
