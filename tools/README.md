# QGroundControl Tools

This directory contains development tools, scripts, and configuration files for QGroundControl.

## Directory Structure

```text
tools/
├── analyze.py               # Static analysis and formatting (clang-format, clang-tidy, cppcheck, clazy)
├── build_profile.py         # Summarize Ninja and Clang time-trace build hotspots
├── check_deps.py            # Check for outdated dependencies
├── clean.py                 # Clean build artifacts and caches
├── configure.py             # CMake configuration wrapper
├── coverage.py              # Code coverage reports
├── generate_docs.py         # Generate API docs (Doxygen)
├── pre_commit.py            # Pre-commit hook runner
├── release.py               # Semantic versioning and release automation
├── run_tests.py             # Qt unit test runner
├── configs/                 # Tool configuration files
│   └── ccache.conf          # ccache configuration
├── analyzers/               # Static analysis scripts
│   └── vehicle_null_check.py
├── coding-style/            # Code style examples
├── common/                  # Shared Python utilities
│   ├── patterns.py          # QGC regex patterns
│   └── file_traversal.py    # File discovery
├── debuggers/               # Debugging tools
│   ├── gdb-pretty-printers/ # GDB/LLDB Qt type formatters
│   ├── profile.sh           # Profiling (valgrind, perf)
│   ├── qt6.natvis           # Visual Studio debugger visualizers
│   └── valgrind.supp        # Valgrind suppressions
├── generators/              # Build-time code generation (mavlink enums, config/settings QML)
├── schemas/                 # JSON schemas for editor validation
├── setup/                   # Environment setup scripts
├── simulation/              # Vehicle simulators
│   ├── mock_vehicle.py      # Lightweight MAVLink simulator
│   └── run-arducopter-sitl.sh  # ArduCopter SITL (Docker)
└── translations/            # Translation tools
```

## Quick Start

Common commands are wrapped in a `justfile` (requires `just` >=1.30; install with `python tools/setup/install_python.py dev` to pull `rust-just` into `.venv`, or use `brew install just` / `cargo install just` / `pipx install rust-just`. `apt install just` on Ubuntu ships 1.21 which is too old):

```bash
just             # Show all available commands
just configure   # Configure CMake build
just build       # Build the project
just test        # Run unit tests
just lint        # Run pre-commit checks
just check       # Run lint + test
just setup       # Full setup: deps, submodules, configure, build
```

`just` reads configuration from `.github/build-config.json` for consistent versioning.

### Direct Script Usage

```bash
# Format changed files
./tools/analyze.py --tool clang-format --fix

# Run static analysis
./tools/analyze.py

# Report build hotspots
python3 ./tools/build_profile.py -B build

# Clean build
./tools/clean.py

# Run tools/ Python tests
cd tools && uv run --extra scripts --extra test pytest tests/ -q
```

## Development Scripts

### analyze.py

Run static analysis and formatting on source code.

```bash
./tools/analyze.py                              # Analyze changed files
./tools/analyze.py --all                        # Analyze all files
./tools/analyze.py --tool clang-format --fix    # Format changed files
./tools/analyze.py --tool clang-format --all    # Check formatting (all files)
./tools/analyze.py --tool cppcheck              # Use cppcheck instead of clang-tidy
./tools/analyze.py src/Vehicle/                 # Analyze specific directory
```

### clean.py

Clean build artifacts and caches.

```bash
./tools/clean.py              # Clean build directory
./tools/clean.py --all        # Clean everything (build, caches)
./tools/clean.py --cache      # Clean only caches
./tools/clean.py --dry-run    # Show what would be removed
```

### build_profile.py

Summarize build-time hotspots from Ninja logs and optional Clang time traces.

```bash
python3 ./tools/build_profile.py -B build                 # Report slowest Ninja edges and rebuild churn
python3 ./tools/build_profile.py -B build --limit 25      # Show more rows per section
python3 ./tools/build_profile.py -B build --json          # Machine-readable output
```

For per-translation-unit trace details, configure with `-DQGC_TIME_TRACE=ON`,
rebuild, then run the same report. The script scans the build directory for
Clang `-ftime-trace` JSON files and highlights the slowest traces and events.

### coverage.py

Generate code coverage reports. Wrapper around CMake coverage targets.

```bash
python3 ./tools/coverage.py              # Build with coverage, run tests, generate report
python3 ./tools/coverage.py --report     # Generate report only (after tests)
python3 ./tools/coverage.py --open       # Generate and open in browser
python3 ./tools/coverage.py --clean      # Clean coverage data
```

Requires: `gcovr` (`pip install gcovr`)

**Direct CMake usage:**

```bash
cmake -B build -DQGC_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
cmake --build build --target coverage-report  # XML + HTML from existing data
cmake --build build --target coverage         # Run tests + generate XML + HTML
cmake --build build --target coverage-clean   # Clean .gcda files
```

### debuggers/profile.sh

Profile QGC for performance and memory issues.

```bash
./tools/debuggers/profile.sh                # CPU profiling with perf
./tools/debuggers/profile.sh --memcheck     # Memory leak detection (valgrind)
./tools/debuggers/profile.sh --callgrind    # CPU profiling (valgrind)
./tools/debuggers/profile.sh --massif       # Heap profiling (valgrind)
./tools/debuggers/profile.sh --heaptrack    # Heap profiling (heaptrack)
./tools/debuggers/profile.sh --sanitize     # Build with AddressSanitizer
```

### check_deps.py

Check for outdated dependencies and submodules.

```bash
python3 ./tools/check_deps.py              # Check all dependencies
python3 ./tools/check_deps.py --submodules # Check git submodules only
python3 ./tools/check_deps.py --qt         # Check Qt version
python3 ./tools/check_deps.py --update     # Update submodules to latest
```

### generate_docs.py

Generate API documentation using Doxygen.

```bash
python3 ./tools/generate_docs.py          # Generate HTML docs
python3 ./tools/generate_docs.py --open   # Generate and open in browser
python3 ./tools/generate_docs.py --pdf    # Generate PDF (requires LaTeX)
python3 ./tools/generate_docs.py --clean  # Clean generated docs
```

Requires: `doxygen`, `graphviz`

## Setup Scripts

Scripts in `setup/` help configure development environments. They read configuration from `.github/build-config.json` for consistent versioning.

|Script|Platform|Description|
|------|--------|-----------|
|`install_dependencies --platform debian`|Linux|Install build dependencies via apt|
|`install_dependencies --platform macos`|macOS|Install dependencies via Homebrew + GStreamer|
|`install_dependencies --platform windows`|Windows|Install GStreamer (Vulkan SDK optional)|
|`install_python.py`|All|Install Python tools via uv or pip|
|`build-gstreamer.py`|All|Build GStreamer from source (optional)|
|`download_artifacts.py`|All|Download build artifacts (in `.github/scripts/`)|
|`read_config.py`|All|Read `.github/build-config.json` (Python, cross-platform)|

### Usage Examples

```bash
# Linux: Install all dependencies
python3 ./tools/setup/install_dependencies --platform debian

# macOS: Install all dependencies
python3 ./tools/setup/install_dependencies --platform macos

# Windows (as Admin):
python .\tools\setup\install_dependencies --platform windows

# Install Python tooling (pre-commit, test, coverage, etc.)
python3 ./tools/setup/install_python.py precommit,test,coverage

# Build GStreamer from source (optional — CMake auto-downloads pre-built SDKs)
python3 ./tools/setup/build-gstreamer.py --platform linux --prefix /opt/gstreamer

# Read build config values
python3 ./tools/setup/read_config.py --get qt_version
```

## Static Analyzers

Scripts in `analyzers/` perform QGC-specific static analysis.

### vehicle_null_check.py

Detects unsafe `activeVehicle()` access patterns that could cause null pointer dereferences.

```bash
# Analyze specific files
python3 tools/analyzers/vehicle_null_check.py src/Vehicle/*.cc

# Analyze entire directory
python3 tools/analyzers/vehicle_null_check.py src/

# JSON output for CI/editor integration
python3 tools/analyzers/vehicle_null_check.py --json src/

# Run via pre-commit
pre-commit run vehicle-null-check --all-files
```

**Detects:**

- `activeVehicle()->method()` without prior null check
- `getParameter()` result used without validation

**Output includes fix suggestions** for each detected issue.

See [analyzers/README.md](analyzers/README.md) for details.

## Shared Utilities

Common utilities in `common/` are used by multiple tools:

- `patterns.py` - QGC-specific regex patterns (Fact, FactGroup, MAVLink)
- `file_traversal.py` - File discovery with proper filtering
- `gh_actions.py` - GitHub API helpers (httpx with `gh` CLI fallback) for workflow runs and artifacts

See [common/README.md](common/README.md) for API documentation.

## Debugging Tools

### debuggers/gdb-pretty-printers/

GDB pretty printers for Qt 6 types. Makes debugging Qt containers and strings readable.

```bash
# In GDB:
source tools/debuggers/gdb-pretty-printers/qt6.py

# Then:
(gdb) print myQString
$1 = "Hello, World!"
```

See [debuggers/gdb-pretty-printers/README.md](debuggers/gdb-pretty-printers/README.md) for setup instructions.

### debuggers/qt6.natvis

Visual Studio debugger visualizers for Qt6 types. Automatically loaded by VS when debugging.

## Simulation Tools

Vehicle simulators for testing QGC without hardware.

### Mock Vehicle (Lightweight)

```bash
pip install pymavlink
./tools/simulation/mock_vehicle.py              # QGC connects to UDP 14550
./tools/simulation/mock_vehicle.py --tcp --port 5760  # TCP mode
```

### ArduCopter SITL (Full Simulation)

```bash
./tools/simulation/run-arducopter-sitl.sh       # Connect to tcp://localhost:5760
./tools/simulation/run-arducopter-sitl.sh --with-latency  # Simulate network lag
```

See [simulation/README.md](simulation/README.md) for details.

## Translation Tools

Scripts in `translations/` manage internationalization.

|Script|Description|
|------|-----------|
|`qgc_lupdate.py`|Update Qt translation files (runs lupdate + JSON extractor + pseudo-loc)|
|`qgc_lupdate_json.py`|Extract translatable strings from JSON files|

```bash
# From repository root:
python3 tools/translations/qgc_lupdate.py

# Or run JSON extractor directly:
python3 tools/translations/qgc_lupdate_json.py
```

See [translations/README.md](translations/README.md) for Crowdin integration.

## Code Quality Tools

### ccache.conf

Configuration for [ccache](https://ccache.dev/) to speed up rebuilds. CMake automatically uses this when ccache is available.

```bash
# Manual use:
export CCACHE_CONFIGPATH=/path/to/qgroundcontrol/tools/configs/ccache.conf
```

### coding-style/

Example files demonstrating QGC coding conventions:

- `CodingStyle.h` - Header file conventions
- `CodingStyle.cc` - Implementation file conventions
- `CodingStyle.qml` - QML file conventions

See [CODING_STYLE.md](../CODING_STYLE.md) for the full style guide.

## VS Code Integration

The repository includes VS Code configuration in `.vscode/`:

- **settings.json** - Editor settings, CMake integration
- **extensions.json** - Recommended extensions
- **tasks.json** - Build, test, format tasks
- **launch.json** - Debug configurations

Open the repository in VS Code and install recommended extensions for the best experience.

## Centralized Configuration

Version numbers and build settings are centralized in `.github/build-config.json`:

```json
{
  "qt_version": "6.10.1",
  "qt_modules": "qtgraphs qtlocation ...",
  "gstreamer_default_version": "1.24.13",
  "ndk_version": "r27c",
  ...
}
```

Scripts read from this file to ensure consistent versions across local development and CI.
