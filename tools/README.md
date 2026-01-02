# QGroundControl Tools

Development tools, scripts, and configuration for QGroundControl.

## Quick Start

```bash
# Using make
make help        # Show all available commands
make build       # Build the project
make test        # Run unit tests

# Using just (cargo install just)
just             # Show all available commands
just setup       # Full setup: deps, submodules, configure, build
```

## Python Setup

```bash
./tools/setup/install-python.sh           # Install CI tools
./tools/setup/install-python.sh all       # Install everything
source .venv/bin/activate
```

See `pyproject.toml` for dependency groups: `ci`, `qt`, `coverage`, `dev`, `lsp`.

## Directory Overview

| Directory | Purpose | Details |
|-----------|---------|---------|
| [analyzers/](analyzers/) | Static analysis scripts | `vehicle_null_check.py` |
| [coding-style/](coding-style/) | Code style examples | See also [CODING_STYLE.md](../CODING_STYLE.md) |
| [common/](common/) | Shared Python utilities | `patterns.py`, `file_traversal.py` |
| [configs/](configs/) | Tool configuration files | trufflehog, etc. |
| [debuggers/](debuggers/) | Debugging tools | GDB pretty-printers, Valgrind, natvis |
| [generators/](generators/) | Code generation | FactGroup generator |
| [locators/](locators/) | CLI search tools | Fact/MAVLink lookup |
| [log-analyzer/](log-analyzer/) | Log analysis tools | QGC log parser |
| [lsp/](lsp/) | QGC Language Server | LSP for Facts/MAVLink |
| [qtcreator/](qtcreator/) | Qt Creator integration | Lua plugin, snippets |
| [schemas/](schemas/) | JSON schemas | Editor validation |
| [setup/](setup/) | Environment setup | Platform-specific installers |
| [simulation/](simulation/) | Vehicle simulators | Mock vehicle, SITL scripts |
| [translations/](translations/) | i18n tools | lupdate, Crowdin |

## Root Scripts

| Script | Description |
|--------|-------------|
| `analyze.sh` | Run clang-tidy/cppcheck on source files |
| `check-deps.sh` | Check for outdated dependencies |
| `clean.sh` | Clean build artifacts and caches |
| `common.sh` | Shared shell utilities (sourced by other scripts) |
| `configure.sh` | Configure CMake build with Qt auto-detection |
| `coverage.sh` | Generate code coverage reports |
| `generate-docs.sh` | Generate Doxygen API docs |
| `param-docs.py` | Generate parameter documentation |
| `pre-commit.sh` | Run pre-commit hooks (install with `--install`) |
| `run-tests.sh` | Run unit tests with headless display support |
| `update-headers.py` | License header management |

## Configuration Files

| File | Purpose |
|------|---------|
| `ccache.conf` | ccache settings (auto-used by CMake) |
| `pyproject.toml` | Python dependencies |

## Centralized Config

Version numbers are in `.github/build-config.json`. Scripts use `setup/read-config.sh` to read values.
