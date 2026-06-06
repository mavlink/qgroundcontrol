# Qt QML Profiler

Profile a 2D QML / Qt Quick application with `qmlprofiler`, parse
the resulting `.qtd` trace, and analyze bottlenecks against the
source code. Does **not** cover Qt Quick 3D.

## What it does

1. **Locates the Qt toolchain** — resolves `qmlprofiler` from
   `CLAUDE.md`, environment (`CMAKE_PREFIX_PATH`, `QTDIR`,
   `Qt6_DIR`), `PATH`, or common install locations on Linux,
   macOS, and Windows.
2. **Builds with QML debugging** (profiling mode only) — adds
   `-DQT_QML_DEBUG` via `CMAKE_CXX_FLAGS` without modifying the
   project's `CMakeLists.txt`.
3. **Runs `qmlprofiler`** (profiling mode only) — writes a
   timestamped trace to `profiler/traces/` and waits for the
   user to exercise and close the app. If this session cannot
   execute the binary (e.g. Claude Desktop without shell
   access, or a sandbox blocking the Qt path), the skill
   instead prints the exact command for the user to run
   manually and resumes once a trace path is supplied.
4. **Parses the trace** — a bundled Python script reads the
   `.qtd` XML and emits a JSON summary covering range events,
   animation frame-time percentiles, memory allocation, and
   pixmap cache usage.
5. **Analyzes hotspots** — maps the top locations to project
   source files and explains each against a QML performance
   anti-pattern catalogue (Binding, Javascript, HandlingSignal,
   Creating, Compiling, SceneGraph/Painting, Memory/Pixmap).
6. **Writes a standalone report** — timestamped Markdown under
   `profiler/reports/` with event summary, animation/frame-time
   table, memory/pixmap summaries, top 30 hotspots, and
   detailed analysis of the top 5.

## Profiling profiles

| Profile | `qmlprofiler --include` |
|---|---|
| `full` *(default)* | *(omit — records everything)* |
| `rendering` | `scenegraph,animations,painting,pixmapcache` |
| `logic` | `javascript,binding,handlingsignal,compiling,creating` |
| `memory` | `memory,creating` |

## Usage

**Profile then analyze** — build, run, and analyze:

```
[--profile <full|rendering|logic|memory>] -- <executable> [app-args...]
```

**Analyze an existing trace** — skip build/run and go straight
to parsing:

```
<path-to-trace.qtd>
```

## How to use

1. Install the skill (see Installation below) and open your QML
   project in your assistant.
2. Invoke the skill with one of the argument forms above —
   either an executable to profile end-to-end, or an existing
   `.qtd` trace to analyze only.
3. When the app launches, exercise it normally and close it
   when done; the trace is written on exit. If your assistant
   cannot execute the binary (e.g. Claude Desktop without a
   shell tool, or a sandbox blocking the Qt path), the skill
   prints the exact command for you to run manually — reply
   with the resulting `.qtd` path to continue.
4. Read the generated report under `profiler/reports/` for
   the top hotspots, frame-time percentiles, memory and
   pixmap summaries, and suggested fixes.

For background on what `qmlprofiler` measures and how each
event type is interpreted, see the upstream Qt Creator
documentation:
[Profiling QML applications](https://doc.qt.io/qtcreator/creator-qml-performance-monitor.html).

## Requirements

- Qt 6 installation containing `bin/qmlprofiler`
- CMake and a C++ toolchain (profiling mode only, to build with
  `-DQT_QML_DEBUG`)
- Python 3 to run the trace parser

## Installation

| Platform | Command |
|----------|---------|
| **Claude Code** | `/plugin marketplace add TheQtCompanyRnD/agent-skills` then `/plugin install qt-development-skills` |
| **Codex CLI** | `npx skills add TheQtCompanyRnD/agent-skills` |

This skill relies on running a Python script and writing report
files, so it targets Tier-1 platforms (Claude Code, Codex CLI)
that consume full skill directories. Condensed single-file
variants are not provided.

## Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Full skill instructions |
| `references/qml-performance-anti-patterns.md` | Event-type-keyed catalogue of QML performance anti-patterns |
| `references/scripts/parse-qmlprofiler-trace.py` | `.qtd` trace parser emitting JSON summary |

## License

LicenseRef-Qt-Commercial OR BSD-3-Clause
