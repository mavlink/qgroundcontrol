# Qt UI Design

The skill helps to achieve better UX on your Graphical User Interfaces. It enforces a structured, two-phase design workflow, in which it first gates all creation work behind a 7-question intake interview, then applies a comprehensive set of UX laws, typographic scales, motion budgets, accessibility rules, and platform-specific constraints — with a dedicated MCU/embedded mode that overrides desktop defaults with fixed-pixel layouts, bitmap fonts, 48 px touch targets, GPU-free rendering rules, and three-cue safety alarms.
For existing UI code, screen captures of a UI, or images of UI designs from Figma or Claude Design, it switches to an audit mode.

## What it does

Provides a comprehensive rule set that guides AI agents when
producing or working with Graphical User Interfaces (GUI). Covers:

- Gates all design work behind a 7-question intake interview covering platform, screen shape, resolution, design system, content priority, viewing distance, locale, and input methods.
- Applies a curated set of UX laws silently to inform layout and interaction decisions.
- Enforces a modular typographic scale with ratio-based size steps mapped to semantic roles (display, title, body, caption) per target and viewing distance.
- Defines strict motion and animation budgets: GPU-composited properties only (transform/opacity), duration caps of 100–400 ms, and a mandatory reduced-motion fallback path.
- Switches to an embedded/MCU mode that overrides desktop defaults with fixed-pixel layouts, flat solid fills, bitmap fonts, 48–72 px touch targets, hardware button fallbacks, and no GPU-dependent effects.
- Enforces safety-critical embedded rules: three-cue alarms (color + shape + text), confirm-before-actuator, persistent error state indicators, and a defined safe default screen.
- Provides an audit mode that categorises findings as Critical, Warning, or Opportunity across motion, typography, keyboard operability, colour tokens, localisation, and AI latency handling.


## Requirements

- No external dependencies

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |


## Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Full skill instructions |

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
