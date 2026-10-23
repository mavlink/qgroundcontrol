# QGroundControl Development Commands
# Install (requires just >=1.48 for `set lazy`):
#   python tools/setup/install_python.py dev   (recommended; pulls rust-just into tools/.venv)
#   brew install just / cargo install just
# `apt install just` on Ubuntu ships 1.21 which is too old.

# Evaluate the shell() assignments only in recipes that use them (`info`).
set lazy

host_os := os()
python := "uv run --frozen --project tools --group scripts python"
build_tool := "uv run --frozen --project tools --group build"
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

# Install VS Code workspace defaults without overwriting local settings
vscode:
    {{ python }} ./tools/setup/setup_vscode.py

# ─────────────────────────────────────────────────────────────────────────────
# Build
# ─────────────────────────────────────────────────────────────────────────────

# Configure CMake build
configure:
    {{ build_tool }} python ./tools/configure.py --preset {{ build_preset }} {{ qt_root_arg }}

# Build the project
build:
    {{ build_tool }} cmake --build --preset {{ build_preset }} --parallel {{ jobs }}

# Configure and build Release
release:
    {{ build_tool }} python ./tools/configure.py --preset default-release {{ qt_root_arg }}
    {{ build_tool }} cmake --build --preset default-release --parallel {{ jobs }}

# Clean build directory (forwards to tools/clean.py; pass --cache, --all, --dry-run)
clean *ARGS:
    {{ python }} ./tools/clean.py {{ ARGS }}

# Clean, configure, and build
rebuild: clean configure build

# Full setup: deps, configure, build
setup: deps configure build

# ─────────────────────────────────────────────────────────────────────────────
# Quality
# ─────────────────────────────────────────────────────────────────────────────

# Run application tests (matches CI label filters; override with `LABELS=... EXCLUDE=... JOBS=N just test`)
test labels=env_var_or_default("LABELS", "Unit|Integration") exclude=env_var_or_default("EXCLUDE", "Flaky|Network"):
    {{ build_tool }} ctest --preset default --build-config {{ build_type }} --parallel {{ jobs }} --no-tests=error -L "{{ labels }}" -LE "{{ exclude }}"

# Run pre-commit hooks on all files with prek (pass hook ids and `--hook-stage manual` for manual hooks)
lint *ARGS:
    uv run --frozen --project tools --group precommit prek run --all-files {{ ARGS }}

# Lint English docs with only the docs hooks (every page; pass --changed for branch changes)
lint-docs *ARGS:
    uv run --frozen --project tools --group precommit python ./tools/pre_commit.py --docs {{ ARGS }}

# Check clang-format on lines changed since the default branch (no changes)
format:
    uv run --frozen --project tools --group lint python ./tools/analyze.py --tool clang-format

# Apply clang-format to lines changed since the default branch
format-fix:
    uv run --frozen --project tools --group lint python ./tools/analyze.py --tool clang-format --fix

# Run static analysis
analyze:
    {{ python }} ./tools/analyze.py

# Generate coverage report
coverage:
    {{ build_tool }} python ./tools/coverage_report.py

# Run all Python tooling tests in parallel, in random order (pass pytest options, e.g. --randomly-seed=N)
test-python *ARGS:
    uv run --frozen --project tools --group test pytest -q -n auto tools/tests {{ ARGS }}

# Run lint + Python + application tests
check: lint test-python test

# ─────────────────────────────────────────────────────────────────────────────
# Run & Deploy
# ─────────────────────────────────────────────────────────────────────────────

# Launch QGroundControl
run:
    "{{ app_path }}"

# Build documentation
[working-directory: 'docs']
docs:
    npm run docs:build

# Build using Docker (Ubuntu)
docker:
    python3 deploy/docker/run_docker.py build ubuntu

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

# Clean build, caches, generated files, and node_modules
distclean:
    {{ python }} ./tools/clean.py --all

# Validate schema and cross-field build settings
validate-configs:
    uv run --frozen --project tools --group lint python tools/validate_configs.py

# Print draft release notes for a vX.Y.Z tag (an untagged version drafts the commits since the last release)
release-notes TAG:
    uv run --frozen --project tools --group release python tools/ci/publish/release_notes.py "{{ TAG }}"

# Lint QML with generated module type information (requires a build)
lint-qml-build:
    {{ python }} tools/analyze.py --tool qmllint --qml-build --all --advisory
