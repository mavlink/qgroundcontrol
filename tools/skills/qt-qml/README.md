# Qt QML Coding

QML best practices for writing, reviewing, fixing, and
refactoring QML code. Corrects systematic LLM pre-training
biases around bindings, scoping, modules, JavaScript interop,
and types.

## What it does

Provides a comprehensive rule set that guides AI agents when
producing or working with QML source code. Covers:

- **Imports** — version-free imports (Qt 6), style-specific
  control imports, unnecessary import removal
- **Property bindings** — declarative over imperative, circular
  dependency prevention, binding preservation
- **Layouts** — `anchors` vs `Layout.*` conflicts, sizing
  discipline inside layouts, positioner selection
- **ListView and delegates** — `required property` for roles,
  delegate reuse, state management
- **State management** — `states`/`PropertyChanges` patterns,
  targeted transitions
- **Animations** — off-screen pausing, `Animator` types,
  `Behavior` pitfalls
- **Images** — `sourceSize`, async loading, error handling
- **Accessibility** — roles, names, keyboard navigation
- **Singletons** — `pragma Singleton` + `qmldir` requirements
- **Internationalization** — `qsTr()` wrapping, placeholder
  conventions
- **Performance** — clipping, opacity, `Canvas` avoidance,
  shader and particle management

Also documents non-obvious QML pitfalls (dynamic scope, binding
destruction, `parent` in delegates, etc.).

## Requirements

- No external dependencies

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |
| **Copilot** | `gh skill install TheQtCompanyRnD/agent-skills qt-qml` (preview) — or auto-discovered from `.claude/skills/` |
| **Cursor** | Copy `SKILL.md` to `.cursor/rules/qt-qml/RULE.md` |
| **Windsurf** | Copy `platforms/windsurf.md` to `.windsurf/rules/qt-qml.md` |

## Platform variants

The full skill (`SKILL.md`) is a conceptual rule set that works
on any platform capable of loading Markdown instructions,
including **Claude Code**, **Codex CLI**, and **GitHub Copilot**
(which auto-discovers it from `.claude/skills/`). For platforms
with size constraints, a condensed variant is available in
`platforms/`:

- **windsurf.md** — Compact variant for Windsurf

## Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Full skill instructions (210 lines) |
| `platforms/windsurf.md` | Compact variant for Windsurf |

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
