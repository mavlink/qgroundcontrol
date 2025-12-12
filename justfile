# QGroundControl Development Commands
# Install: cargo install just, brew install just, or apt install just

# Configuration from build-config.json
qt_version := `./tools/setup/read-config.sh qt_version 2>/dev/null || echo "6.10.1"`
qt_dir := env_var_or_default("QT_DIR", home_directory() / "Qt" / qt_version / "gcc_64")
build_type := env_var_or_default("BUILD_TYPE", "Debug")
build_dir := "build"

# Default: show available commands
default:
    @just --list --unsorted

# ─────────────────────────────────────────────────────────────────────────────
# Setup
# ─────────────────────────────────────────────────────────────────────────────

# Install system dependencies (Debian/Ubuntu)
deps:
    @echo "Installing dependencies (requires sudo)..."
    sudo ./tools/setup/install-dependencies-debian.sh

# Initialize git submodules
submodules:
    git submodule update --init --recursive

# ─────────────────────────────────────────────────────────────────────────────
# Build
# ─────────────────────────────────────────────────────────────────────────────

# Configure CMake build
configure: submodules
    {{qt_dir}}/bin/qt-cmake -B {{build_dir}} -G Ninja \
        -DCMAKE_BUILD_TYPE={{build_type}} \
        -DQGC_BUILD_TESTING=ON

# Build the project
build:
    cmake --build {{build_dir}} --config {{build_type}} --parallel

# Configure and build Release
release:
    {{qt_dir}}/bin/qt-cmake -B {{build_dir}} -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DQGC_BUILD_TESTING=OFF
    cmake --build {{build_dir}} --config Release --parallel

# Clean build directory
clean:
    rm -rf {{build_dir}}

# Clean, configure, and build
rebuild: clean configure build

# Full setup: deps, submodules, configure, build
setup: deps submodules configure build

# ─────────────────────────────────────────────────────────────────────────────
# Quality
# ─────────────────────────────────────────────────────────────────────────────

# Run unit tests
test:
    cd {{build_dir}} && ctest --output-on-failure

# Run pre-commit checks
lint:
    pre-commit run --all-files

# Check code formatting (no changes)
format:
    ./tools/format-check.sh --check

# Format code (apply fixes)
format-fix:
    ./tools/format-check.sh

# Run static analysis
analyze:
    ./tools/analyze.sh

# Generate coverage report
coverage:
    ./tools/coverage.sh

# Run lint + test
check: lint test

# ─────────────────────────────────────────────────────────────────────────────
# Run & Deploy
# ─────────────────────────────────────────────────────────────────────────────

# Launch QGroundControl
run:
    ./{{build_dir}}/staging/QGroundControl

# Build documentation
docs:
    npm run docs:build

# Build using Docker (Ubuntu)
docker:
    ./deploy/docker/run-docker-ubuntu.sh

# ─────────────────────────────────────────────────────────────────────────────
# Utilities
# ─────────────────────────────────────────────────────────────────────────────

# Show build configuration
info:
    @echo "Qt version:  {{qt_version}}"
    @echo "Qt dir:      {{qt_dir}}"
    @echo "Build type:  {{build_type}}"
    @echo "Build dir:   {{build_dir}}"

# Update copyright headers
update-headers:
    python3 ./tools/update-headers.py

# Check dependency versions
check-deps:
    ./tools/check-deps.sh

# Clean all caches and build artifacts
distclean: clean
    rm -rf .cache
    rm -rf node_modules
