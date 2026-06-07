# Qt QML Documentation

Generates structured Markdown reference documentation for QML
components and applications from `.qml` source files.

## What it does

Reads QML source files (along with related files such as
`CMakeLists.txt`, `qmldir`, C++ backend headers, and resource
files) and produces developer-friendly Markdown reference docs.
For each QML component, the skill generates a document covering:

- **Component overview** — role in the project, when to use it
- **Project structure and dependencies** — imports, build
  requirements, related types
- **Component hierarchy** — base types and what they provide
- **Properties** — full table with type, default, required flag,
  and description
- **Signals and methods** — signatures, triggers, side effects
- **Inter-component interactions** — bindings, signal consumers,
  shared state
- **Usage example** — minimal instantiation snippet for reusable
  components

For single files it generates one `.md` file; for folders it
documents every `.qml` component and creates an `index.md`
linking them all.

## Requirements

- No external dependencies

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |
| **Copilot** | `gh skill install TheQtCompanyRnD/agent-skills qt-qml-docs` (preview) — or auto-discovered from `.claude/skills/` |
| **Cursor** | Copy `SKILL.md` to `.cursor/rules/qt-qml-docs/RULE.md` |
| **Windsurf** | Copy `platforms/windsurf.md` to `.windsurf/rules/qt-qml-docs.md` |

## Platform variants

The full skill (`SKILL.md`) works on any platform capable of
loading Markdown instructions, including **Claude Code**,
**Codex CLI**, and **GitHub Copilot** (which auto-discovers it
from `.claude/skills/`). For platforms with size constraints,
a condensed variant is available in `platforms/`:

- **windsurf.md** — Compact variant for Windsurf

## Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Full skill instructions (184 lines) |
| `platforms/windsurf.md` | Compact variant for Windsurf |

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
