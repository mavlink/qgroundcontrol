# QGroundControl Development Commands
# Install (requires just >=1.30 for home_directory()):
#   python tools/setup/install_python.py dev   (recommended; pulls rust-just into .venv)
#   brew install just / cargo install just / pipx install rust-just
# `apt install just` on Ubuntu ships 1.21 which is too old.

host_os := os()
python := if host_os == "windows" { "python" } else { "python3" }
qt_version := shell(python + " ./tools/setup/read_config.py --get qt.version")
cmake_min_version := shell(python + " ./tools/setup/read_config.py --get build.cmake_minimum_version")
gstreamer_version := shell(python + " ./tools/setup/read_config.py --get gstreamer.version.default")
qt_dir := env_var_or_default("QT_DIR", "")
qt_root_arg := if qt_dir == "" { "" } else { "--qt-root \"" + qt_dir + "\"" }
qt_dir_display := if qt_dir == "" { env_var_or_default("QT_ROOT_DIR", "auto-detected") } else { qt_dir }
build_type := env_var_or_default("BUILD_TYPE", "Debug")
build_dir := "build"
build_preset := if build_type == "Debug" { "default" } else if build_type == "Release" { "default-release" } else if build_type == "RelWithDebInfo" { "default-relwithdebinfo" } else { "default-minsizerel" }
# Use all cores by default; override with JOBS=N.
jobs := env_var_or_default("JOBS", num_cpus())
app_path := if host_os == "windows" { build_dir / build_type / "QGroundControl.exe" } else if host_os == "macos" { build_dir / build_type / "QGroundControl.app" / "Contents" / "MacOS" / "QGroundControl" } else { build_dir / build_type / "QGroundControl" }
qgis_plugin_dir := env_var_or_default("QGIS_PLUGIN_DIR", home_directory() / ".local/share/QGIS/QGIS3/profiles/default/python/plugins")
qgis_target_dir := qgis_plugin_dir / "qgc4qgis"


# Default: show available commands
default:
    @just --list --unsorted

# ─────────────────────────────────────────────────────────────────────────────
# Setup
# ─────────────────────────────────────────────────────────────────────────────

# Install system dependencies for the current host
deps:
    @echo "Installing dependencies (requires sudo)..."
    {{ python }} ./tools/setup/install_dependencies

# Initialize git submodules
submodules:
    git submodule update --init --recursive

# Install VS Code workspace defaults without overwriting local settings
vscode:
    {{ python }} ./tools/setup/setup_vscode.py

# ─────────────────────────────────────────────────────────────────────────────
# Build
# ─────────────────────────────────────────────────────────────────────────────

# Configure CMake build
configure: submodules
    {{ python }} ./tools/configure.py --preset {{ build_preset }} -B {{ build_dir }} -t {{ build_type }} {{ qt_root_arg }}

# Build the project
build:
    cmake --build --preset {{ build_preset }} --parallel {{ jobs }}

# Configure and build Release
release:
    {{ python }} ./tools/configure.py --preset default-release -B {{ build_dir }} --release {{ qt_root_arg }}
    cmake --build --preset default-release --parallel {{ jobs }}

# Clean build directory (forwards to tools/clean.py; pass --cache, --all, --dry-run)
clean *ARGS:
    {{ python }} ./tools/clean.py {{ ARGS }}

# Clean, configure, and build
rebuild: clean configure build

# Full setup: deps, submodules, configure, build
setup: deps submodules configure build

# ─────────────────────────────────────────────────────────────────────────────
# Quality
# ─────────────────────────────────────────────────────────────────────────────

# Run application tests (matches CI label filters; override with `LABELS=... EXCLUDE=... JOBS=N just test`)
test labels=env_var_or_default("LABELS", "Unit|Integration") exclude=env_var_or_default("EXCLUDE", "Flaky|Network"):
    ctest --preset default --build-config {{ build_type }} --parallel {{ jobs }} --no-tests=error -L "{{ labels }}" -LE "{{ exclude }}"

# Run pre-commit checks
lint:
    pre-commit run --all-files

# Check code formatting (no changes)
format:
    {{ python }} ./tools/analyze.py --tool clang-format

# Format code (apply fixes)
format-fix:
    {{ python }} ./tools/analyze.py --tool clang-format --fix

# Run static analysis
analyze:
    {{ python }} ./tools/analyze.py

# Generate coverage report
coverage:
    {{ python }} ./tools/coverage.py

# Run lint + test
check: lint test

# ─────────────────────────────────────────────────────────────────────────────
# Run & Deploy
# ─────────────────────────────────────────────────────────────────────────────

# Launch QGroundControl
run:
    "{{ app_path }}"

# Build documentation
docs:
    npm run docs:build

# Build using Docker (Ubuntu)
docker:
    ./deploy/docker/run-docker.sh ubuntu

# ─────────────────────────────────────────────────────────────────────────────
# QGIS Plugin
# ─────────────────────────────────────────────────────────────────────────────

# Install QGIS plugin (symlink qgc4qgis to QGIS default profile)
qgis-install:
    mkdir -p "{{ qgis_plugin_dir }}"
    rm -rf "{{ qgis_target_dir }}"
    ln -s "{{ justfile_directory() / "qgc4qgis" }}" "{{ qgis_target_dir }}"

# Uninstall QGIS plugin
qgis-uninstall:
    rm -rf "{{ qgis_target_dir }}"

# Run QGIS plugin tests
qgis-test:
    pytest -q


# ─────────────────────────────────────────────────────────────────────────────
# Utilities
# ─────────────────────────────────────────────────────────────────────────────

# Show build configuration
info:
    @echo "Qt version:  {{ qt_version }}"
    @echo "Qt dir:      {{ qt_dir_display }}"
    @echo "CMake min:   {{ cmake_min_version }}"
    @echo "GStreamer:   {{ gstreamer_version }}"
    @echo "Build type:  {{ build_type }}"
    @echo "Build dir:   {{ build_dir }}"
    @echo "Jobs:        {{ jobs }}"

# Check dependency versions
check-deps:
    {{ python }} ./tools/check_deps.py

# Check the configured GStreamer minor line for a newer common SDK patch
check-gstreamer:
    {{ python }} ./tools/check_deps.py --gstreamer

# Update translation sources
translations:
    {{ python }} ./tools/translations/qgc_lupdate.py

# Clean build, caches, and generated files
distclean:
    {{ python }} ./tools/clean.py --all
    {{ python }} -c "import shutil; shutil.rmtree('node_modules', ignore_errors=True)"
