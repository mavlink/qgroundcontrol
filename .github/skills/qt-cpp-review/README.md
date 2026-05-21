# Qt C++ Code Review

Structured, read-only code review for Qt6 C++ code. Combines a
deterministic Python linter with six parallel deep-analysis agents.

## What it does

1. **Linter** (Phase 1) — A single-pass Python script that checks
   60+ rules covering API naming, memory ownership, thread safety,
   signal/slot patterns, modern C++ with Qt, error handling, and
   deprecated patterns. Runs in under a second with no external
   dependencies beyond Python 3.6+.

2. **Deep analysis** (Phase 2) — Six focused agents run in parallel,
   each covering a specific domain: model contracts, ownership and
   lifecycle, threading, API correctness, error handling, and
   performance. Agents report only findings above 80/100 confidence.

3. **Consolidated report** (Phase 3) — Deduplicated, scored findings
   with file paths, line numbers, traces, and mitigations.

## Requirements

- Python 3.6+ (for the linter script)
- No external Python dependencies

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |
| **Copilot** | `gh skill install TheQtCompanyRnD/agent-skills qt-cpp-review` (preview) — or auto-discovered from `.claude/skills/` |
| **Cursor** | Copy `SKILL.md` to `.cursor/rules/qt-cpp-review/RULE.md` |
| **Windsurf** | Copy `platforms/windsurf.md` to `.windsurf/rules/qt-cpp-review.md` |

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
| `SKILL.md` | Full skill instructions (460 lines) |
| `references/qt-review-checklist.md` | 60+ review rules with IDs |
| `references/qt-framework-checklist.md` | Additional rules for Qt module development |
| `references/qt-deprecated-classes.md` | Deprecated Qt API reference |
| `references/lint-scripts/qt_review_lint.py` | Deterministic Python linter |
| `platforms/windsurf.md` | Compact variant for Windsurf |

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
