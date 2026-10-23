# QGroundControl Tools

Development tools, scripts, and configuration files for QGroundControl. This is the full
reference for every `just` recipe and standalone script; [AGENTS.md](../AGENTS.md) has the
condensed day-to-day workflow.

## Table of Contents

- [Quick Start](#quick-start)
- [Directory Structure](#directory-structure)
- [Just Command Reference](#just-command-reference)
- [Direct Script Usage](#direct-script-usage)
- [Development Scripts](#development-scripts)
- [Setup Scripts](#setup-scripts)
- [Python Environment](#python-environment)
- [Python Script Conventions](#python-script-conventions)
- [Static Analyzers](#static-analyzers)
- [Debugging Tools](#debugging-tools)
- [Other Tool Areas](#other-tool-areas)
- [Code Quality Tools](#code-quality-tools)
- [VS Code Integration](#vs-code-integration)
- [Centralized Configuration](#centralized-configuration)

## Quick Start

Install uv, then the locked `dev` Python profile into `tools/.venv`; the profile includes `just`:

```bash
# Linux/macOS; on Windows install uv from https://docs.astral.sh/uv/getting-started/installation/
python3 tools/setup/install_uv.py
export PATH="$HOME/.local/bin:$PATH"

python3 tools/setup/install_python.py dev   # Windows: python tools/setup/install_python.py dev
. tools/.venv/bin/activate                  # PowerShell: . tools/.venv/Scripts/Activate.ps1
```

A separately installed `just` (`brew install just`, `cargo install just`) must be 1.48 or newer
because the justfile uses `set lazy`; Ubuntu's `apt install just` ships 1.21, which is too old.

Everyday loop:

```bash
just             # List all recipes
just setup       # First-time: deps + configure + build
just build       # Incremental build
just check       # lint + Python tooling tests + application tests — run before declaring done
```

## Directory Structure

```text
tools/
├── _bootstrap.py            # Import bootstrap for entry scripts (tools/ on sys.path, CI debug hooks)
├── analyze.py               # Static analysis and formatting (clang-format, clang-tidy, clazy, qmllint, qmlformat)
├── build_profile.py         # Summarize Ninja and Clang time-trace build hotspots
├── check_deps.py            # Check for outdated dependencies
├── clean.py                 # Clean build artifacts and caches
├── configure.py             # CMake configuration wrapper
├── coverage_report.py       # Code coverage reports
├── moccache_launcher.py     # moccache entry point: the CMake AUTOMOC launcher and the stats/trim CLI
├── pre_commit.py            # Pre-commit runner with CI summaries
├── terrain_tiles.py         # Terrarium elevation tiles from Copernicus GLO-30 (GeoMap)
├── update_px4_metadata.py   # Refresh the bundled PX4 airframe and parameter metadata
├── validate_cmake_presets.py # CMake presets against the locked cmake package's schema (hook)
├── validate_configs.py      # Cross-field checks for build-config.json and link exceptions
├── pyproject.toml, uv.lock  # Locked Python tooling groups
├── analyzers/               # analyze.py backends and QGC-specific analyzers
├── ci/                      # GitHub Actions entrypoints, grouped by CI concern
├── coding-style/            # Code style examples
├── common/                  # Shared Python utilities
├── configs/                 # Tool configuration files
│   ├── ccache.conf          # ccache configuration
│   ├── cliff.toml           # git-cliff release-notes sections (just release-notes)
│   ├── Doxyfile             # Doxygen API docs, run from the repository root
│   ├── editorconfig-checker.json # editorconfig-checker exclusions and relaxed checks
│   ├── gersemirc.yaml       # gersemi CMake formatter trial (manual hook)
│   ├── lychee.toml          # Link checker settings and dated URL exceptions
│   ├── trufflehog-exclude.txt # Paths the trufflehog pre-commit hook skips
│   └── yamlfmt.yaml         # yamlfmt YAML formatter trial (manual hook)
├── debuggers/               # Debugger formatters, profiling, Android memory capture, Valgrind suppressions
├── generators/              # Code generation (MAVLink enums, config/settings QML)
├── moccache/                # Content-addressed cache for Qt moc (AUTOMOC wrapper)
├── qgc_tools/               # Shared policy: Python environment, configure presets, Docker variants
├── setup/                   # Environment setup scripts
├── simulation/              # Vehicle simulators
├── terrain/                 # Terrain tile generation package (see terrain_tiles.py)
├── tests/                   # pytest suite for tools/ (CI script and contract tests in tests/ci/)
└── translations/            # Translation tools (lupdate, JSON extraction, pseudo-localization)
```

## Just Command Reference

All recipes are defined in [`../justfile`](../justfile), grouped by the sections below; run `just`
with no arguments for the same list. Most recipes run Python through
`uv run --frozen --project tools`. `BUILD_TYPE` (default `Debug`), `JOBS` (default: all cores),
and `QT_DIR` override the build recipes; `just info` prints them with the Qt, CMake, and GStreamer
versions from `.github/build-config.json` ([Centralized Configuration](#centralized-configuration)).

### Setup

| Recipe            | Description                                                    |
| ----------------- | -------------------------------------------------------------- |
| `just deps`       | Auto-detect and install system build dependencies              |
| `just vscode`     | Install missing VS Code workspace files from tracked templates |

### Build

| Recipe              | Description                                                                                      |
| ------------------- | ------------------------------------------------------------------------------------------------ |
| `just configure`    | Configure through the matching `default*` CMake preset (Debug by default)                        |
| `just build`        | Build the project (uses all cores by default; override with `JOBS=N`)                            |
| `just release`      | Configure and build in Release mode (testing disabled)                                           |
| `just clean [ARGS]` | Clean the build directory; forwards `ARGS` to `tools/clean.py` (`--cache`, `--all`, `--dry-run`) |
| `just rebuild`      | `clean` + `configure` + `build`                                                                  |
| `just setup`        | Full first-time setup: `deps` + `configure` + `build`                                            |

### Quality

| Recipe                  | Description                                                                                           |
| ----------------------- | ----------------------------------------------------------------------------------------------------- |
| `just test`             | Run application tests via CTest; defaults to labels `Unit`/`Integration`, excluding `Flaky`/`Network` |
| `just test-python`      | Run the Python tooling tests (`tools/tests`, incl. `tools/tests/ci`) in parallel, in random order     |
| `just lint [ARGS]`      | Run non-manual hooks on all files with prek (`prek run --all-files`); `ARGS` select hooks/stages      |
| `just lint-docs [ARGS]` | Lint English docs with only the docs hooks (every page; `--changed` for branch changes)               |
| `just format`           | Check clang-format on lines changed since the default branch (no changes)                             |
| `just format-fix`       | Apply clang-format to lines changed since the default branch                                          |
| `just analyze`          | Run static analysis (`tools/analyze.py`, default tool clang-tidy)                                     |
| `just coverage`         | Build with coverage instrumentation, run tests, generate report                                       |
| `just check`            | `lint` + `test-python` + `test` — run before declaring a task done                                    |

Override `test` label filters via environment variables or positional args:

```bash
just test                              # LABELS=Unit|Integration EXCLUDE=Flaky|Network (defaults)
LABELS="Slow" just test                # override via env var
just test "Slow" "Network"             # override via positional args (labels, exclude)
```

### Run & Deploy

| Recipe        | Description                                                                    |
| ------------- | ------------------------------------------------------------------------------ |
| `just run`    | Launch the built `QGroundControl` binary                                       |
| `just docs`   | Build the VitePress documentation site (`npm run docs:build` in `docs/`)       |
| `just docker` | Build the `ubuntu` Docker variant (`deploy/docker/run_docker.py build ubuntu`) |

### Utilities

| Recipe                   | Description                                                                                           |
| ------------------------ | ----------------------------------------------------------------------------------------------------- |
| `just info`              | Print resolved build configuration (Qt version/dir, CMake min, GStreamer, jobs, ...)                  |
| `just check-deps`        | Check Qt, GStreamer, Python, and build tool versions (`tools/check_deps.py`)                          |
| `just check-gstreamer`   | Check the latest GStreamer patch common to all SDK platforms                                          |
| `just translations`      | Update translation sources (`tools/translations/qgc_lupdate.py`)                                      |
| `just distclean`         | Clean build, caches, generated files, and `node_modules/`                                             |
| `just validate-configs`  | Validate build settings and link exceptions ([Centralized Configuration](#centralized-configuration)) |
| `just release-notes TAG` | Print draft release notes for a `vX.Y.Z` tag ([Release notes](#release-notes))                        |
| `just lint-qml-build`    | Lint all QML against the build's generated type information ([analyze.py](#analyzepy))                |

## Direct Script Usage

Prefer `just` recipes; call the scripts below directly for flags a recipe doesn't expose. Run them
from the repository root with `tools/.venv` activated ([Quick Start](#quick-start)), or prefix the
command with `uv run --frozen --project tools --group dev` instead. The `dev` profile covers every
script here.

Repository configure/build/test/workflow presets are rooted at [`../CMakePresets.json`](../CMakePresets.json).
Use `CMakeUserPresets.json` for machine-local paths or derived presets; it is ignored by Git and
preserved by `just clean`. `tools/configure.py` (behind `just configure`) requires the Qt kit's
`qt-cmake`, found through `--qt-root` (`QT_DIR` for `just`), `QT_ROOT_DIR`, or the newest standard
Qt install for the host architecture. Use `--no-qt-cmake` only when supplying an explicit target
toolchain, as Android CI does. `configure.py` configures into the selected preset's `binaryDir`,
where `cmake --build --preset` and `ctest --preset` look for the tree; `-B` overrides it.

## Development Scripts

### analyze.py

Run static analysis and formatting on source code. Underlies `just format`, `just format-fix`,
`just analyze`, and `just lint-qml-build`.

```bash
python tools/analyze.py                              # Analyze changed files (default tool: clang-tidy)
python tools/analyze.py --all                        # Analyze all files
python tools/analyze.py --tool clang-format --fix    # Format changed lines
python tools/analyze.py --tool clang-format --all    # Check formatting (all files)
python tools/analyze.py src/Vehicle/                 # Analyze specific directory
python tools/analyze.py --tool clang-tidy --tool clazy --cache-dir .cache/analysis-results  # Both, one scan
python tools/analyze.py --tool clang-tidy --all --shard 1 --shard-count 4  # One of four disjoint partitions
python tools/analyze.py --tool clang-tidy --profile-checks src/Vehicle/Vehicle.cc  # Slower per-check profiling
python tools/analyze.py --tool clang-tidy --fix src/Vehicle/  # Apply clang-tidy fixes (local only)
```

Paths are relative to the repository root; absolute paths inside the checkout also work.

Other `--tool` choices: `clazy`, `qmllint`, `qmlformat` (reports what `qmlformat` would change,
never rewrites), `vehicle-null-check`, `qt-translate-noop-check`.
Without paths or `--all`, files changed since the merge base with the default branch (or
`PR_BASE_SHA`; in GitHub's merge checkout of `PR_HEAD_SHA`, its first parent) are analyzed,
including uncommitted and untracked files; without a usable base every file is. For clang-format the
selection narrows to changed lines, matching the pre-commit hook: deletion-only changes are skipped
and new or untracked files are checked in full. `--all` or explicit paths check (or `--fix`) whole
files for intentional reformatting. `--advisory` keeps warnings informational; error-level
diagnostics and tool or compiler invocation errors still fail.

**Compiler analysis** (clang-tidy, Clazy) reads `compile_commands.json` from `--build-dir`
(default `build`) and needs generated headers, not a full build. Configure Ninja with
`CMAKE_EXPORT_COMPILE_COMMANDS=ON`, `CMAKE_DISABLE_PRECOMPILE_HEADERS=ON`,
`CMAKE_AUTOGEN_ORIGIN_DEPENDS=OFF`, `CMAKE_GLOBAL_AUTOGEN_TARGET=ON`, and `QGC_UNITY_BUILD=OFF`,
then build only `cmake --build <dir> --target qgc-analysis-headers` (`autogen` is needed only for
IWYU). Repeating `--tool` shares one file selection and dependency scan; `--cache-dir <dir>`
replays unchanged results and needs `clang-scan-deps`. `tools/setup/build_clazy.py` builds the
Clazy that CI uses. `--tool clang-tidy --fix` applies the selected translation units' fixes with
`clang-apply-replacements` ([analyzers/README.md](analyzers/README.md#clang-tidy-fixes)); CI never
applies fixes. The clang-tidy and Clazy pre-commit hooks run only with `--hook-stage manual`;
fast hooks remain the required PR gate. CI sharding and result caching are described in
[ci-overview.md](../.github/ci-overview.md); output files and profiling in
[analyzers/README.md](analyzers/README.md).

**QML:** `--tool qmllint --qml-build` checks QML against the generated `<build-dir>/qml` type
information after a build. Missing imports, properties, and types are errors; the fast
source-only hook keeps its limited checks. `just lint-qml-build` runs it on every QML file with
`--advisory`.

### pre_commit.py

Runs the [`.pre-commit-config.yaml`](../.pre-commit-config.yaml) hooks with
[prek](https://github.com/j178/prek) and CI-friendly summaries; the pre-commit workflow calls it
with `--ci --changed --results-dir`. A `--changed` run whose docs lint configuration changed also
runs the docs hooks over every English page.

```bash
python tools/pre_commit.py --changed                             # Files changed vs PR_BASE_SHA or the default branch
python tools/pre_commit.py --changed --hook ruff --hook pyright  # Only these hooks (repeatable)
python tools/pre_commit.py --docs --changed                      # English docs with only the docs hooks (just lint-docs)
python tools/pre_commit.py --changed --needs-qt                  # Whether the selection has QML (qmllint needs Qt)
python tools/pre_commit.py --install                             # Install the pre-commit and commit-msg hooks
python tools/pre_commit.py --prepare-cache                       # Install the hook environments CI caches, prune the rest
```

The selected hooks run in one `prek run` call; its output streams to the console and, with
`--results-dir`, into `pre-commit-output.txt`. `--ci` skips the local-only `trufflehog` hook unless
`SKIP` is already set. `--prepare-cache` installs every hook environment except
`CACHE_SKIPPED_HOOKS` (TruffleHog and the Renovate validator) and then runs `prek cache gc`; the
pre-commit workflow runs it on pushes that miss the prek cache, and saves the cache only when the
install succeeded.

The `precommit` group pins prek, which also accepts pre-commit's commands and options
(`prek run <id> --hook-stage manual --files <paths>`, `SKIP=<id>`). The configuration stays
compatible with stock pre-commit, which the group keeps as a fallback runner for one release.
`editorconfig-checker` enforces [`.editorconfig`](../.editorconfig) with the exclusions and relaxed
checks in `configs/editorconfig-checker.json`; `cmake-presets-schema` validates `CMakePresets.json` and
`cmake/presets/*.json` offline against the schema shipped in the locked `cmake` package.
Manual-stage hooks run only on request: `clang-tidy`, `clazy`, the CI validators that download pinned
tools (`renovate-config-validator`, `packer-validate`, `runs-on-lint`, which `pre_commit.py --ci` also
runs on the changed files), and
three formatter trials that report differences without rewriting files: `gersemi` (CMake,
`configs/gersemirc.yaml`), `yamlfmt` (`configs/yamlfmt.yaml`), and `qmlformat` (`analyzers/qmlformat.py`,
`.qmlformat.ini`). For example, `just lint gersemi --hook-stage manual` checks every CMake file.

### clean.py

Clean build artifacts and caches. Underlies `just clean` and `just distclean`.

```bash
python tools/clean.py              # Clean build directory
python tools/clean.py --all        # Clean everything (build, caches, node_modules/, Python bytecode outside venvs)
python tools/clean.py --cache      # Clean only repo-local caches (.cache, incl. ccache and the clangd index, plus a legacy .ccache)
python tools/clean.py --dry-run    # Show what would be removed
```

### build_profile.py

Summarize build-time hotspots from Ninja logs and optional Clang time traces. The parsing and report
code lives in `qgc_tools/build_profile.py`, which `ci/compile/cmake_helper.py` and
`ci/compile/docker_helper.py` also use.

```bash
python tools/build_profile.py -B build                 # Report slowest Ninja edges and rebuild churn
python tools/build_profile.py -B build --limit 25      # Show more rows per section
python tools/build_profile.py -B build --json          # Machine-readable output
python tools/build_profile.py -B build --json-out r.json > r.txt  # Text and JSON from one scan
python tools/build_profile.py -B build --output-dir build/profile  # Ninja-only snapshot
```

For per-translation-unit trace details, configure with `-DQGC_TIME_TRACE=ON`, rebuild, then rerun
the report — it scans the build dir for Clang `-ftime-trace` JSON and highlights the slowest events.

Ordinary CI builds upload Ninja-only snapshots with separate link and generated-code rankings.
Task totals overlap because commands run in parallel; they are not elapsed build time.

### moccache/

Content-addressed cache for Qt's moc, wired in through `AUTOMOC_EXECUTABLE` while the
`QGC_USE_MOCCACHE` CMake option is ON (the default). The generated launcher runs
`moccache_launcher.py`, which imports the `moccache` package so its bytecode is cached in
`tools/moccache/__pycache__`. A moc run loads only `core.py`; `trim.py` (LRU eviction) and `stats.py`
(hit/miss reports) are imported only for their subcommands or a due auto-trim, keeping each cache
hit's startup short. The stats and trim commands run through the launcher too
(`python3 tools/moccache_launcher.py --show-stats ...`).
Misses fall through to the real moc and never fail the build. The
[dev guide Build Caching section](../docs/en/qgc-dev-guide/getting_started/index.md#build-caching)
covers the cache key, environment variables, and stats commands. Tests:
`tools/tests/test_moccache.py`.

### coverage_report.py

Generate code coverage reports through the CMake coverage targets. Underlies `just coverage`.
Configures the `Linux-coverage` preset's `build/Linux-coverage/` directory (override the directory
with `-b`/`--build-dir`). Requires `gcovr` (`coverage` group, included in `dev`).
Coverage compiles pass `-fprofile-update=atomic`, which ccache caches only from 4.10; an older
distro ccache (Ubuntu 24.04 ships 4.9.1) gets no cache hits in coverage builds.
Reports are Cobertura `coverage.xml` (uploaded to Codecov), gcovr's JSON `coverage-summary.json`
(read by the step summary, `verify_coverage_thresholds.py`, and the PR build-results comment), and
HTML `coverage.html`. Reporting fails when the build directory holds no `.gcda` counters, so tests
that wrote no coverage data cannot publish a zero-coverage report.

```bash
python tools/coverage_report.py              # Build with coverage, run tests like CI, generate XML, JSON + HTML
python tools/coverage_report.py --report     # Generate report only from existing data (CI adds --step-summary)
python tools/coverage_report.py --open       # Full run, then open the HTML report
python tools/coverage_report.py --clean      # Delete the coverage build directory
```

**Direct CMake usage:**

```bash
cmake -B build -DQGC_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
cmake --build build --target coverage-report  # XML, JSON summary + HTML from existing data
cmake --build build --target coverage         # Run Unit tests + generate XML, JSON summary + HTML
cmake --build build --target coverage-clean   # Remove coverage data and reports
```

`coverage_report.py` runs CI's two test phases through the `Linux-coverage` test preset, which
excludes `Flaky` and `Network`: Unit tests at one process per CPU, then Integration tests at `-j2`,
because instrumented integration tests time out at full parallelism. The CMake `coverage` target
runs only the Unit tests.

### check_deps.py

Check for outdated dependencies. Underlies `just check-deps`.

```bash
python tools/check_deps.py              # Check all dependencies
python tools/check_deps.py --qt         # Check Qt version
python tools/check_deps.py --gstreamer  # Find latest patch common to all SDK platforms
```

### API documentation (Doxygen)

Run Doxygen from the repository root, as the Doxygen workflow does; HTML goes to `docs/api/html/`.

```bash
doxygen tools/configs/Doxyfile
```

Requires `doxygen`; `graphviz` enables diagrams.

### Release notes

Release tags are created and pushed by hand. `just release-notes vX.Y.Z` prints the notes that
`release.yml` drafts for that tag: git-cliff (`release` group) lists the commits since the previous
`vX.Y.Z` tag reachable from it, grouped by type as configured in `configs/cliff.toml`, as full
Conventional Commits subjects under `## Changes in vX.Y.Z`. For a tag that does not exist yet it
lists the unreleased commits since the last release reachable from `HEAD`.

```bash
just release-notes v5.1.5                                # Notes for an existing tag
python tools/ci/publish/release_notes.py v5.2.0          # Same script; v5.2.0 not tagged yet
```

### PX4 metadata

`px4-metadata.yml` runs `update_px4_metadata.py` daily and opens a pull request when the bundled PX4
airframe (`AirframeFactMetaData.xml`) or parameter (`PX4ParameterFactMetaData.json`) metadata
changes upstream. It retries both downloads from artifacts.px4.io and replaces neither file unless
both are valid: well-formed airframe XML without a DTD, and xz-compressed JSON that lists parameters.

```bash
python3 tools/update_px4_metadata.py
```

## Setup Scripts

Scripts in `setup/` configure development environments and read versions from
`.github/build-config.json`. They need only the standard library, so they also run with the host
`python3` before `tools/.venv` exists. `install_dependencies` is a package
(`tools/setup/install_dependencies/`) run through its `__main__.py`; without `--platform` it
detects the host. `build_gstreamer.py` is the entry for the `setup/gstreamer/` package: `config`
(build settings, Meson options and plugins), `host` (commands, host arch and jobs), `meson` (Linux,
macOS, Windows), `cerbero` (Android, iOS) and `cli` (arguments, CI arch/prefix outputs).

| Script                                    | Platform              | Description                                                                            |
| ----------------------------------------- | --------------------- | -------------------------------------------------------------------------------------- |
| `install_dependencies --platform debian`  | Linux (Debian/Ubuntu) | Install build dependencies via apt                                                     |
| `install_dependencies --platform fedora`  | Linux (Fedora/RHEL)   | Install build dependencies via dnf                                                     |
| `install_dependencies --platform arch`    | Linux (Arch)          | Install build dependencies via pacman                                                  |
| `install_dependencies --platform macos`   | macOS                 | Install dependencies via Homebrew + GStreamer                                          |
| `install_dependencies --platform windows` | Windows               | Install GStreamer (optional `--vulkan`, `--msvc`, `--msvc-arm64`, `--nsis`)            |
| `install_uv.py`                           | Linux, macOS          | Install the uv release named by `required-version` when `uv` is not on `PATH`          |
| `install_python.py`                       | All                   | Install locked Python tool groups via uv ([Python Environment](#python-environment))   |
| `install_qt.py`                           | All                   | Install Qt with [QGC's installer or aqtinstall](#qt-installer) (CI: `plan`/`ensure`)   |
| `setup_vscode.py`                         | All                   | Install missing VS Code workspace files from tracked templates                         |
| `read_config.py`                          | All                   | Read `.github/build-config.json`                                                       |
| `build_gstreamer.py`                      | All                   | Build GStreamer from source (optional)                                                 |
| `build_android_openssl.py`                | Android               | Cross-compile OpenSSL for Android; configure builds only the build type's variant      |
| `build_clazy.py`                          | Linux                 | Build the pinned `clazy-standalone` against the analysis LLVM, as CI does              |

Linux `--category` limits the install to `core`, `qt`, `gstreamer`, `sdl`, `audio` or `misc`.
On Debian, `android` is `core` plus the OpenJDK for `android.java_version` in `build-config.json`;
the Docker Android image uses it, while CI's Android jobs install `core` and get the JDK from
`setup-java`. Debian's `gstreamer_build`, outside the full set, holds what `build_gstreamer.py`
needs for a Linux source build; the Build GStreamer workflow installs it.
`--list [--platform NAME]` lists each platform's packages by category and
`--print-packages [--category NAME]` prints one list on a single line; neither installs anything.
Locally, the Linux installers also sync the locked `build` group (CMake, Ninja, `just`, ...) into
`tools/.venv` and print how to activate it; CI build jobs instead use the `build.cmake_version` and
`build.ninja_version` pins in `build-config.json`, which `build-setup` installs.

On macOS, the GStreamer runtime and development packages come from the official server, falling
back to QGroundControl's public S3 mirror. Both sources are verified against the SHA-256 checksums
pinned in `.github/build-config.json`; installation stops if either package cannot be verified.

```bash
# Linux: Install all dependencies
python3 tools/setup/install_dependencies --platform debian

# Linux: build tools plus Gradle's JDK only, for Android builds
python3 tools/setup/install_dependencies --platform debian --category android

# macOS: Install all dependencies
python3 tools/setup/install_dependencies --platform macos

# Windows (as Admin):
python .\tools\setup\install_dependencies --platform windows

# Install Qt from the configured version and modules
python3 tools/setup/install_qt.py install --from-config --host linux --arch linux_gcc_64

# Build GStreamer from source (optional — CMake auto-downloads pre-built SDKs)
python3 tools/setup/install_dependencies --platform debian --category gstreamer_build
python3 tools/setup/build_gstreamer.py --platform linux --prefix /opt/gstreamer

# Build Clazy like the Code Analysis workflow; then put .cache/clazy/bin on PATH
python3 tools/setup/build_clazy.py --prefix .cache/clazy

# Read build config values
python3 tools/setup/read_config.py --get qt.version
```

### Qt installer

`install_qt.py` installs Qt with one of two tools, chosen by `--installer` or `QGC_QT_INSTALLER`:

- `qgc`, the default: the standard-library package `setup/qt_installer/`. It reads the
  combination's `Updates.xml` from download.qt.io, checks it and every archive against the SHA-256
  sums that download.qt.io publishes (archives may come from any mirror), extracts the archives in parallel with
  `cmake -E tar`, and applies aqtinstall's post-install edits. CMake comes from `PATH`, else from
  the tools environment's `build` group. The Qt root gets `.qgc-qt-manifest.json` (archives,
  checksums, patched files) and, for full installs, the `.qgc-modules` marker that preinstalled-SDK
  reuse reads.
- `aqt`: aqtinstall from the locked `qt` group, which stays as the fallback.

Both produce the same tree apart from install paths in patched text files and pkg-config `prefix=`
lines (aqt leaves them unpatched on `linux_arm64`); `install_qt.py compare` checks this for a
combination. Android built on a Windows host differs on purpose: `qgc` pairs it with the
`msvc2022_64` host Qt, while aqt names a `mingw_64` one that QGC never installs; builds pass
`QT_HOST_PATH` either way. Like aqt, `qgc` reports but does not install addon dependencies missing from
`--modules`, such as `qtquick3d`'s `qtquicktimeline`. The `plan` cache key's digest includes a non-aqt
installer, so neither tool restores the other's tree.

## Python Environment

`install_python.py` installs dependency groups defined in `tools/pyproject.toml`:
`scripts`, `precommit`, `test`, `qt`, `coverage`, `build`, `dev` (default), `lint`, `terrain`,
`simulation`, `release`, and `all`. Groups compose smaller profiles: `dev` includes the build, Qt,
lint, test, and release (git-cliff) tools. `build` adds CMake, Ninja, Meson, `just`, prek (with
pre-commit), and gcovr; `simulation` adds pymavlink for `simulation/mock_vehicle.py`. The Pyright
hook also syncs `terrain` so it can type-check `tools/terrain`. `--list-groups` prints each group's packages. `[tool.uv] required-version` in
`tools/pyproject.toml` is the single uv version: CI, `install_uv.py`, and the Docker image install
exactly that floor, and older uv releases refuse the project (run `uv self update`).

All Python dependencies are resolved in `tools/uv.lock`. Setup requires uv and uses
`uv sync --frozen --inexact` into `tools/.venv`, so repeating it with a smaller group preserves
installed tools. Use `--replace` only to deliberately replace the profile; `--check` validates the
running interpreter's installed versions and Python/platform markers without installing. CMake,
VS Code, and CI use the same environment; CMake creates it with the `scripts` group when it is
missing (`QGC_AUTO_PYTHON_VENV`).

For an image-owned environment, set `QGC_TOOLS_PROJECT` to the directory containing
`pyproject.toml` and `uv.lock`, and `QGC_PYTHON_ENV` to the environment directory.
Docker uses `/opt/qgc-tools` and `/opt/qgc-venv`. Qt installs that fall back to aqtinstall use
the locked `qt` group ([Qt installer](#qt-installer)).

`common/` contains low-level utilities ([module index](common/README.md)); `qgc_tools/` holds
shared policy such as the Python environment, configure-preset selection, Docker variants, and
release-tag checks ([qgc_tools/README.md](qgc_tools/README.md)). Command-line entrypoints are the
top-level `tools/*.py` scripts and those in `setup/`, `ci/`, and `deploy/`. Import-linter enforces
these dependency boundaries: `ci` is the top layer, and `setup` sits beside `analyzers`,
`generators`, `terrain`, and `translations`, which must not import each other.

## Python Script Conventions

New and edited scripts under `tools/` follow one shape, built on the [`common/`](common/README.md)
helpers:

- **Entry**: scripts in subpackages first run the bare statement
  `sys.path.insert(0, str(Path(__file__).resolve().parents[N]))` (N levels below `tools/`);
  top-level scripts already have `tools/` on `sys.path`. Entry scripts then
  `import _bootstrap as _bootstrap`, which installs the CI debug hooks; the redundant alias marks
  the import as intentional. Library modules leave `sys.path` alone
  ([details](common/README.md#bootstrapping-imports)).
- **Main**: define `main(argv: list[str] | None = None) -> int` and end with
  `raise SystemExit(main())` under `if __name__ == "__main__":`, so tests can call `main([...])`.
- **Output**: log locally with `common.logging` (`log_info`, `log_warn`, `log_error`); in CI,
  report problems as `common.gh_actions` annotations (`gh_error`, `gh_warning`) and write step
  outputs and summaries with its `write_github_output` and `write_step_summary`. Build each
  job-summary section with `common.markdown.summary_section` (H3 title, facts table, collapsed
  details) and format numbers with `common.format` so every job reads the same way.
- **Subprocesses and network**: run commands through `common.proc` (`run_captured`, `run_tee`,
  `run_with_retry`), check required executables with its `require_tools` and poll with its
  `wait_until`, download with `common.net`, and call GitHub through `common.gh_actions.gh()`.
- **Files**: locate the checkout with `common.paths` (`REPO_ROOT`, `TOOLS_DIR`) rather than
  `Path(__file__).resolve().parents[N]` outside the entry statement, and read and write JSON with
  `common.io` (`read_json`, `write_json`).

## Static Analyzers

`analyzers/` holds the `analyze.py` backends, whose `clang_format.py` also runs as the
changed-region `clang-format` pre-commit hook and `qmlformat.py` as the manual-stage `qmlformat`
check, and two QGC-specific checks that also run as
pre-commit hooks: `vehicle_null_check.py` detects `activeVehicle()` results dereferenced without a
null check, and `qt_translate_noop_check.py` detects `QT_TRANSLATE_NOOP`/`QT_TR_NOOP` results used
as runtime strings. See [analyzers/README.md](analyzers/README.md) for usage and output formats.

## Debugging Tools

### debuggers/profile.py

Profile QGC for performance and memory issues.

```bash
python tools/debuggers/profile.py                # CPU profiling with perf
python tools/debuggers/profile.py --memcheck     # Memory leak detection (valgrind)
python tools/debuggers/profile.py --callgrind    # CPU profiling (valgrind)
python tools/debuggers/profile.py --massif       # Heap profiling (valgrind)
python tools/debuggers/profile.py --heaptrack    # Heap profiling (heaptrack)
python tools/debuggers/profile.py --sanitize     # Build with QGC_ENABLE_ASAN/UBSAN in build/sanitize/, then run
python tools/debuggers/profile.py --config Debug -- --logging:full  # Forward application arguments
```

### debuggers/android_mem_capture.py

Sample an Android app's memory (`dumpsys meminfo` PSS breakdown plus system `MemAvailable`) over
adb at a fixed interval and write a CSV, for soak testing and before/after comparisons. Handles
process restarts/LMK kills; press Enter during capture to drop a phase marker (POSIX terminals
only). Defaults to the QGC package.

```bash
python tools/debuggers/android_mem_capture.py --label geomap --interval 10   # Capture until Ctrl+C
python tools/debuggers/android_mem_capture.py --label soak --duration 30m    # Timed capture
python tools/debuggers/android_mem_capture.py --compare run-a.csv run-b.csv  # Summarize/compare runs
```

The compare summary reports start/peak/end and a second-half trend (kB/min) for total PSS,
graphics, native heap, and system available memory.

### Debugger Formatters and Suppressions

- `debuggers/gdb-pretty-printers/` — GDB pretty printers for Qt 6 types; see
  [its README](debuggers/gdb-pretty-printers/README.md) for setup.
- `debuggers/qt6.natvis` — Visual Studio debugger visualizers for Qt 6 types. Add it to your
  project's Natvis files, or point `visualizerFile` at it in a VS Code `cppvsdbg` launch
  configuration.
- `debuggers/valgrind.supp` — Valgrind suppressions used by `profile.py` and CTest memcheck.

## Other Tool Areas

These areas have their own READMEs:

- [ci/](ci/README.md) — the GitHub Actions entrypoints, in subpackages such as `planning/`,
  `cache/`, `compile/`, `publish/`, and `reporting/`; workflows run them by path, and their tests
  live in `tests/ci/` (workflow and action contracts in `tests/ci/contracts/`).
- [simulation/](simulation/README.md) — `mock_vehicle.py`, a lightweight MAVLink vehicle on
  UDP 14550, `run_arducopter_sitl.py`, ArduCopter SITL in Docker on `tcp://localhost:5760`, and
  `mock_ntrip_caster.py`, an offline NTRIP caster on `127.0.0.1:2101` for testing RTK corrections,
  and `rc_transmitter.py`, a web page of RC channel sliders for PX4 SITL.
- [translations/](translations/README.md) — `just translations` runs `qgc_lupdate.py` (lupdate,
  the JSON string extractor, and `pseudo_loc.py`); `check_placeholders.py` guards translation
  placeholders.
- [terrain/](terrain/README.md) — `terrain_tiles.py` builds, validates, and uploads the
  terrarium elevation tiles GeoMap renders; it needs the `terrain` group.
- [generators/config_qml/](generators/config_qml/README.md) and
  [generators/settings_qml/](generators/settings_qml/README.md) — JSON schemas for the vehicle
  config and application settings pages CMake generates at configure time.

## Code Quality Tools

### ccache.conf

Configuration for [ccache](https://ccache.dev/). CMake's compiler launcher points ccache at
`tools/configs/ccache.conf` unless `CCACHE_CONFIGPATH` is already set; see the dev guide's
[Build Caching](../docs/en/qgc-dev-guide/getting_started/index.md#build-caching) section.

```bash
# Manual use:
export CCACHE_CONFIGPATH=/path/to/qgroundcontrol/tools/configs/ccache.conf
```

### coding-style/

Worked examples of [CODING_STYLE.md](../CODING_STYLE.md): `CodingStyle.h`, `CodingStyle.cc`, and
`CodingStyle.qml` ([coding-style/README.md](coding-style/README.md)).

## VS Code Integration

`.vscode/` tracks `extensions.json` (recommended extensions) and templates for `settings.json`
(editor and CMake integration), `tasks.json` (build, test, format tasks), and `launch.json` (debug
configurations) as `*.default.json`. `just vscode` copies missing ones without overwriting local
files. Open the repository in VS Code and install the recommended extensions.

## Centralized Configuration

Version numbers and build settings are centralized in `.github/build-config.json`, grouped into
`qt`, `android`, `apple`, `build`, `gstreamer`, and `analysis` objects (schema:
`.github/build-config.schema.json`). Scripts read it so local development and CI use the same
versions. Read a value with its dotted path, e.g. `read_config.py --get qt.version` or
`--get android.ndk_full_version`; exported env vars and CI outputs derive from the path
(`android.ndk_full_version` → `ANDROID_NDK_FULL_VERSION` / `android_ndk_full_version`).

Run `just validate-configs` after changing build versions, schemas, or link exceptions.
It validates required fields, minimum versions, SDK ordering, checksum coverage, and exception expiry.
The pre-commit gate triggers on schema-only edits too. External link exceptions in `configs/lychee.toml`
are exact URLs with a review deadline. Docs builds check English internal links without
exceptions; the all-locale build allows a finite list of existing translation links in
`docs/.vitepress/config.mjs`. CI's documentation checks are described in
[ci-overview.md](../.github/ci-overview.md).
