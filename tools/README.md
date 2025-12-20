# QGroundControl Tools

This directory contains development tools, scripts, and configuration files for QGroundControl.

## Directory Structure

```
tools/
├── analyze.sh               # Static analysis (clang-tidy, cppcheck)
├── check-deps.sh            # Check for outdated dependencies
├── check-sizes.py           # Check artifact sizes against thresholds
├── clean.sh                 # Clean build artifacts and caches
├── common.sh                # Shared shell functions
├── coverage.sh              # Code coverage reports
├── format-check.sh          # Check/apply clang-format
├── generate-docs.sh         # Generate API docs (Doxygen)
├── param-docs.py            # Generate parameter documentation
├── update-headers.py        # License header management
├── ccache.conf              # ccache configuration
├── coding-style/            # Code style examples
├── debuggers/               # Debugging tools
│   ├── gdb-pretty-printers/ # GDB/LLDB Qt type formatters
│   ├── profile.sh           # Profiling (valgrind, perf)
│   ├── qt6.natvis           # Visual Studio debugger visualizers
│   └── valgrind.supp        # Valgrind suppressions
├── log-analyzer/            # QGC log analysis tools
├── mock-mavlink/            # MAVLink vehicle simulator
├── qtcreator-plugin/        # QtCreator IDE integration tools
├── analyzers/               # Static analysis scripts
├── generators/              # Code generation tools
├── setup/                   # Environment setup scripts
└── translations/            # Translation tools
```

## Quick Start

The repository includes **Makefile** and **justfile** wrappers for common commands:

```bash
# Using make (pre-installed on most systems)
make help        # Show all available commands
make configure   # Configure CMake build
make build       # Build the project
make test        # Run unit tests
make lint        # Run pre-commit checks
make check       # Run lint + test

# Using just (install: cargo install just, brew install just, or apt install just)
just             # Show all available commands
just configure   # Configure CMake build
just build       # Build the project
just setup       # Full setup: deps, submodules, configure, build
```

Both read configuration from `.github/build-config.json` for consistent versioning.

### Direct Script Usage

```bash
# Format changed files
./tools/format-check.sh

# Run static analysis
./tools/analyze.sh

# Clean build
./tools/clean.sh
```

## Development Scripts

### format-check.sh

Check or apply clang-format to source files.

```bash
./tools/format-check.sh                # Format changed files (vs master)
./tools/format-check.sh --check        # Check only (for CI)
./tools/format-check.sh --all          # Format all source files
./tools/format-check.sh src/Vehicle/   # Format specific directory
```

### analyze.sh

Run static analysis on source code.

```bash
./tools/analyze.sh                     # Analyze changed files
./tools/analyze.sh --all               # Analyze all files
./tools/analyze.sh --tool cppcheck     # Use cppcheck instead of clang-tidy
./tools/analyze.sh src/Vehicle/        # Analyze specific directory
```

### clean.sh

Clean build artifacts and caches.

```bash
./tools/clean.sh              # Clean build directory
./tools/clean.sh --all        # Clean everything (build, caches)
./tools/clean.sh --cache      # Clean only caches
./tools/clean.sh --dry-run    # Show what would be removed
```

### coverage.sh

Generate code coverage reports. Wrapper around CMake coverage targets.

```bash
./tools/coverage.sh              # Build with coverage, run tests, generate report
./tools/coverage.sh --report     # Generate report only (after tests)
./tools/coverage.sh --open       # Generate and open in browser
./tools/coverage.sh --clean      # Clean coverage data
```

Requires: `gcovr` (`pip install gcovr`)

**Direct CMake usage:**

```bash
cmake -B build -DQGC_ENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
cmake --build build --target coverage-report  # XML + HTML
cmake --build build --target coverage-html    # HTML only
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

### check-deps.sh

Check for outdated dependencies and submodules.

```bash
./tools/check-deps.sh              # Check all dependencies
./tools/check-deps.sh --submodules # Check git submodules only
./tools/check-deps.sh --qt         # Check Qt version
./tools/check-deps.sh --update     # Update submodules to latest
```

### check-sizes.py

Check artifact sizes against thresholds. Used by CI and locally to catch unexpected size increases.

```bash
./tools/check-sizes.py build/              # Check local build artifacts
./tools/check-sizes.py artifacts/ --json   # Output JSON (for CI)
./tools/check-sizes.py --markdown          # Output markdown table
./tools/check-sizes.py --list-thresholds   # Show configured thresholds
```

Default thresholds: AppImage 200MB, DMG 150MB, EXE 200MB, APK 150MB

### generate-docs.sh

Generate API documentation using Doxygen.

```bash
./tools/generate-docs.sh          # Generate HTML docs
./tools/generate-docs.sh --open   # Generate and open in browser
./tools/generate-docs.sh --pdf    # Generate PDF (requires LaTeX)
./tools/generate-docs.sh --clean  # Clean generated docs
```

Requires: `doxygen`, `graphviz`

### param-docs.py

Generate parameter documentation from FactMetaData JSON files.

```bash
./tools/param-docs.py                    # Generate markdown
./tools/param-docs.py --format html      # Generate HTML
./tools/param-docs.py --format json      # Generate JSON
./tools/param-docs.py --group "Battery"  # Filter by group
./tools/param-docs.py --output params.md # Custom output file
```

## Setup Scripts

Scripts in `setup/` help configure development environments. They read configuration from `.github/build-config.json` for consistent versioning.

| Script | Platform | Description |
|--------|----------|-------------|
| `install-dependencies-debian.sh` | Linux | Install build dependencies via apt |
| `install-dependencies-macos.sh` | macOS | Install dependencies via Homebrew + GStreamer |
| `install-dependencies-windows.ps1` | Windows | Install GStreamer (Vulkan SDK optional) |
| `install-qt-debian.sh` | Linux | Install Qt via aqtinstall |
| `install-qt-macos.sh` | macOS | Install Qt via aqtinstall |
| `install-qt-windows.ps1` | Windows | Install Qt via aqtinstall |
| `build-gstreamer.sh` | Linux | Build GStreamer from source (optional) |
| `read-config.sh` | All | Helper to read `.github/build-config.json` |

### Usage Examples

```bash
# Linux: Install all dependencies
sudo ./tools/setup/install-dependencies-debian.sh
./tools/setup/install-qt-debian.sh

# macOS: Install all dependencies
./tools/setup/install-dependencies-macos.sh
./tools/setup/install-qt-macos.sh

# Windows (PowerShell as Admin):
.\tools\setup\install-dependencies-windows.ps1
.\tools\setup\install-qt-windows.ps1

# Build GStreamer from source (Linux, optional)
./tools/setup/build-gstreamer.sh -p /opt/gstreamer
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

# Run via pre-commit
pre-commit run vehicle-null-check --all-files
```

**Detects:**
- `activeVehicle()->method()` without prior null check
- `getParameter()` result used without validation

## Code Generators

Scripts in `generators/` generate boilerplate code from specifications.

### FactGroup Generator

Generate complete FactGroup boilerplate (header, source, JSON metadata).

```bash
# Preview generated files
python -m tools.generators.factgroup.cli \
  --name Example \
  --facts "temp:double:degC,pressure:double:Pa" \
  --dry-run

# Generate with MAVLink handlers
python -m tools.generators.factgroup.cli \
  --name Wind \
  --facts "direction:double:deg,speed:double:m/s" \
  --mavlink "WIND_COV,HIGH_LATENCY2" \
  --output src/Vehicle/FactGroups/
```

**Generates:**
- `VehicleExampleFactGroup.h` - Header with Q_PROPERTY, accessors, members
- `VehicleExampleFactGroup.cc` - Implementation with constructor, handlers
- `ExampleFact.json` - FactMetaData JSON

**Requires:** `pip install jinja2`

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

## Testing Tools

### mock-mavlink/

Simple MAVLink vehicle simulator for testing QGC without hardware.

```bash
# Install dependency
pip install pymavlink

# Run mock vehicle (QGC connects to UDP 14550)
./tools/mock-mavlink/mock_vehicle.py

# Multiple vehicles
./tools/mock-mavlink/mock_vehicle.py --system-id 1 --port 14550 &
./tools/mock-mavlink/mock_vehicle.py --system-id 2 --port 14551 &
```

See [mock-mavlink/README.md](mock-mavlink/README.md) for details.

### log-analyzer/

Analyze QGC application logs and telemetry files.

```bash
# Analyze application log
./tools/log-analyzer/analyze_log.py ~/.local/share/QGroundControl/Logs/QGCConsole.log

# Show only errors
./tools/log-analyzer/analyze_log.py --errors QGCConsole.log

# Analyze MAVLink telemetry log
./tools/log-analyzer/analyze_log.py flight.tlog

# Show statistics
./tools/log-analyzer/analyze_log.py --stats QGCConsole.log

# Filter by component
./tools/log-analyzer/analyze_log.py --component Vehicle QGCConsole.log
```

See [log-analyzer/README.md](log-analyzer/README.md) for details.

## Translation Tools

Scripts in `translations/` manage internationalization.

| Script | Description |
|--------|-------------|
| `qgc-lupdate.sh` | Update Qt translation files (runs lupdate + JSON extractor) |
| `qgc-lupdate-json.py` | Extract translatable strings from JSON files |

```bash
# From repository root:
source tools/translations/qgc-lupdate.sh

# Or run JSON extractor directly:
python3 tools/translations/qgc-lupdate-json.py --verbose
```

See [translations/README.md](translations/README.md) for Crowdin integration.

## Code Quality Tools

### update-headers.py

Updates or validates license headers in source files.

```bash
python3 tools/update-headers.py              # Update headers
python3 tools/update-headers.py --check      # Check only (for CI)
python3 tools/update-headers.py --check src/ # Check specific directory
```

### ccache.conf

Configuration for [ccache](https://ccache.dev/) to speed up rebuilds. CMake automatically uses this when ccache is available.

```bash
# Manual use:
export CCACHE_CONFIGPATH=/path/to/qgroundcontrol/tools/ccache.conf
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
  "qt_modules": "qtcharts qtlocation ...",
  "gstreamer_version": "1.24.12",
  "ndk_version": "r27c",
  ...
}
```

Scripts read from this file to ensure consistent versions across local development and CI.
