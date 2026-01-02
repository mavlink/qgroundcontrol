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
python tools/setup/install_python.py           # Install CI tools
python tools/setup/install_python.py all       # Install everything
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

### Python Tools (Primary)

| Script | Description |
|--------|-------------|
| `analyze.py` | Run clang-tidy, cppcheck, clazy, qmllint analysis |
| `check_deps.py` | Check/update dependencies and submodules |
| `clean.py` | Clean build artifacts and caches |
| `configure.py` | Configure CMake build with Qt auto-detection |
| `coverage.py` | Generate code coverage reports with gcovr |
| `generate_docs.py` | Generate Doxygen API documentation |
| `lint_fix.py` | Auto-apply pre-commit formatting fixes |
| `param_docs.py` | Generate parameter documentation from JSON |
| `pre_commit.py` | Run pre-commit hooks with formatted output |
| `run_tests.py` | Execute unit tests with headless display support |
| `smoke_test.py` | Smoke tests for tool functionality |
| `update_headers.py` | License header management |
| `debuggers/profile.py` | Multi-tool profiler (perf, valgrind, heaptrack, sanitizers) |

## Configuration Files

| File | Purpose |
|------|---------|
| [configs/ccache.conf](configs/ccache.conf) | ccache settings (auto-used by CMake) |
| `pyproject.toml` | Python dependencies |

## Centralized Config

Version numbers are in `.github/build-config.json`. Scripts use `setup/read_config.py` to read values.
