# QGroundControl Development Commands
# Install (requires just >=1.30 for home_directory()):
#   python tools/setup/install_python.py dev   (recommended; pulls rust-just into .venv)
#   brew install just / cargo install just / pipx install rust-just
# `apt install just` on Ubuntu ships 1.21 which is too old.

# Configuration from build-config.json
qt_version := `python3 ./tools/setup/read_config.py --get qt.version 2>/dev/null || echo "6.11.1"`
cmake_min_version := `python3 ./tools/setup/read_config.py --get build.cmake_minimum_version 2>/dev/null || echo "3.25"`
gstreamer_version := `python3 ./tools/setup/read_config.py --get gstreamer.version.default 2>/dev/null || echo "1.28.4"`
qt_dir := env_var_or_default("QT_DIR", home_directory() / "Qt" / qt_version / "gcc_64")
build_type := env_var_or_default("BUILD_TYPE", "Debug")
build_dir := "build"
# Leave cores free for the user's other work; override with JOBS=N.
jobs := env_var_or_default("JOBS", `python3 -c "import os; print(max(1, (os.cpu_count() or 4)//2))" 2>/dev/null || echo 4`)

# Default: show available commands
default:
    @just --list --unsorted

# ─────────────────────────────────────────────────────────────────────────────
# Setup
# ─────────────────────────────────────────────────────────────────────────────

# Install system dependencies (Debian/Ubuntu)
deps:
    @echo "Installing dependencies (requires sudo)..."
    python3 ./tools/setup/install_dependencies --platform debian

# Initialize git submodules
submodules:
    git submodule update --init --recursive

# ─────────────────────────────────────────────────────────────────────────────
# Build
# ─────────────────────────────────────────────────────────────────────────────

# Configure CMake build
configure: submodules
    python3 ./tools/configure.py -B {{build_dir}} -t {{build_type}} --testing --qt-root {{qt_dir}}

# Build the project
build:
    cmake --build {{build_dir}} --config {{build_type}} --parallel {{jobs}}

# Configure and build Release
release:
    python3 ./tools/configure.py -B {{build_dir}} --release --qt-root {{qt_dir}}
    cmake --build {{build_dir}} --config Release --parallel {{jobs}}

# Clean build directory (forwards to tools/clean.py; pass --cache, --all, --dry-run)
clean *ARGS:
    ./tools/clean.py {{ARGS}}

# Clean, configure, and build
rebuild: clean configure build

# Full setup: deps, submodules, configure, build
setup: deps submodules configure build

# ─────────────────────────────────────────────────────────────────────────────
# Quality
# ─────────────────────────────────────────────────────────────────────────────

# Run unit tests (matches CI label filters; override with `LABELS=... EXCLUDE=... just test`)
test labels=env_var_or_default("LABELS", "Unit|Integration") exclude=env_var_or_default("EXCLUDE", "Flaky|Network"):
    cd {{build_dir}} && ctest --output-on-failure -L "{{labels}}" -LE "{{exclude}}"

# Run pre-commit checks
lint:
    pre-commit run --all-files

# Check code formatting (no changes)
format:
    python3 ./tools/analyze.py --tool clang-format

# Format code (apply fixes)
format-fix:
    python3 ./tools/analyze.py --tool clang-format --fix

# Run static analysis
analyze:
    python3 ./tools/analyze.py

# Generate coverage report
coverage:
    python3 ./tools/coverage.py

# Run lint + test
check: lint test

# ─────────────────────────────────────────────────────────────────────────────
# Run & Deploy
# ─────────────────────────────────────────────────────────────────────────────

# Launch QGroundControl
run:
    ./{{build_dir}}/{{build_type}}/QGroundControl

# Build documentation
docs:
    npm run docs:build

# Build using Docker (Ubuntu)
docker:
    ./deploy/docker/run-docker.sh ubuntu

# ─────────────────────────────────────────────────────────────────────────────
# Utilities
# ─────────────────────────────────────────────────────────────────────────────

# Show build configuration
info:
    @echo "Qt version:  {{qt_version}}"
    @echo "Qt dir:      {{qt_dir}}"
    @echo "CMake min:   {{cmake_min_version}}"
    @echo "GStreamer:   {{gstreamer_version}}"
    @echo "Build type:  {{build_type}}"
    @echo "Build dir:   {{build_dir}}"
    @echo "Jobs:        {{jobs}}"

# Check dependency versions
check-deps:
    python3 ./tools/check_deps.py

# Clean build, caches, and generated files
distclean:
    ./tools/clean.py --all
    rm -rf node_modules
