# Qt QML Code Review

Structured, read-only code review for Qt6 QML code. Combines a
deterministic Python linter with six parallel deep-analysis agents.

## What it does

1. **Linter** (Phase 1) — A single-pass Python script that checks
   47+ rules across 13 categories: imports, ordering, bindings,
   layout, loaders, delegates, states, images, performance, style,
   signals, errors, and JavaScript quality. Runs in under a second
   with no external dependencies beyond Python 3.6+.

2. **System qmllint** (Phase 1b, optional) — If `qmllint` is
   available on the system, runs it for type-level checks and
   merges findings with the Python linter output.

3. **Deep analysis** (Phase 2) — Six focused agents run in parallel:
   bindings and properties, layout and anchoring, component loading
   and lifecycle, ListView and delegates, states and transitions,
   performance and code quality. Agents report only findings above
   80/100 confidence.

4. **Consolidated report** (Phase 3) — Deduplicated, scored findings
   with file paths, line numbers, traces, and mitigations.

## Requirements

- Python 3.6+ (for the linter script)
- No external Python dependencies
- Optional: `qmllint` from a Qt 6 installation (for type checks)

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |
| **Copilot** | `gh skill install TheQtCompanyRnD/agent-skills qt-qml-review` (preview) — or auto-discovered from `.claude/skills/` |
| **Cursor** | Copy `SKILL.md` to `.cursor/rules/qt-qml-review/RULE.md` |
| **Windsurf** | Copy `platforms/windsurf.md` to `.windsurf/rules/qt-qml-review.md` |

## Platform variants

The full skill (`SKILL.md`) uses a multi-phase linter + agent
architecture that requires a platform capable of running Python
scripts and launching parallel subagents. This works natively on
**Claude Code** and **Codex CLI**.

**GitHub Copilot** also reads the full skill directory natively
(via `.claude/skills/`, `.github/skills/`, or `~/.copilot/skills/`),
though it cannot run the Python linter or launch parallel
subagents — only the static guidance loads.

For platforms that do not support multi-file skills or script
execution, condensed variants are available in `platforms/`:

- **windsurf.md** — Compact rule summary for Windsurf (under 2K
  chars). Highest-priority rules only.

These variants provide useful review guidance but should be expected
to produce less thorough results than the full skill. The
deterministic linter and parallel agent analysis are only available
on platforms that support the full skill directory.

## Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Full skill instructions (412 lines) |
| `references/qt-qml-review-checklist.md` | 47+ review rules with IDs |
| `references/lint-scripts/qt_qml_lint.py` | Deterministic Python linter |
| `platforms/windsurf.md` | Compact variant for Windsurf |

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
