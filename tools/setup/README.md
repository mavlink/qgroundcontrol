# QGroundControl Setup Scripts

Build environment setup for QGroundControl development across Linux, macOS, and Windows.

## Overview

This directory contains scripts to install system dependencies, Qt SDK, Python tools, and platform-specific build requirements. All scripts are platform-aware and include cross-platform support via Python.

## Quick Start

```bash
# Install system dependencies (auto-detects platform)
python3 tools/setup/install_dependencies.py

# Or specify platform explicitly
python3 tools/setup/install_dependencies.py --platform debian   # Debian/Ubuntu
python3 tools/setup/install_dependencies.py --platform macos    # macOS
./tools/setup/install-dependencies-windows.ps1                   # Windows (PowerShell)

# Install Python development tools
python tools/setup/install_python.py

# Install Qt SDK
./tools/setup/install_qt.py
```

## Scripts Reference

| Script | Purpose | Platforms |
|--------|---------|-----------|
| **install_dependencies.py** | System packages (cmake, ninja, git, GStreamer, etc.) | Debian/Ubuntu, macOS |
| **install-dependencies-windows.ps1** | Windows system dependencies (Visual Studio, CMake) | Windows |
| **install_python.py** | Python venv + pip dependencies for development | All |
| **install_qt.py** | Qt SDK via aqtinstall (auto-detects platform) | All |
| **read_config.py** | Read build configuration (Python) | All |
| **read-config.ps1** | Read build configuration (PowerShell) | Windows |
| **aqt-settings.ini** | Configuration for aqtinstall (mirrors, paths) | All |

## Installation Guide

### Dependencies (System Packages)

Install system packages required for building QGroundControl:

```bash
# Auto-detect platform (Debian/Ubuntu or macOS)
python3 tools/setup/install_dependencies.py

# Debian/Ubuntu explicitly
sudo python3 tools/setup/install_dependencies.py --platform debian

# macOS explicitly
python3 tools/setup/install_dependencies.py --platform macos

# List all packages by category
python3 tools/setup/install_dependencies.py --list

# Install only Qt dependencies
python3 tools/setup/install_dependencies.py --category qt

# Windows (PowerShell, admin required)
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
.\tools\setup\install-dependencies-windows.ps1
```

### Python Environment

Install Python development tools and create virtual environment:

```bash
# Install CI tools (pre-commit, meson, ninja)
python tools/setup/install_python.py

# Install Qt installation tools
python tools/setup/install_python.py qt

# Install code coverage tools
python tools/setup/install_python.py coverage

# Install all development dependencies
python tools/setup/install_python.py all

# Multiple groups can be combined
python tools/setup/install_python.py ci,coverage
```

**Tip**: Install `uv` for 10-100x faster package installation:
```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

The script automatically uses `uv` if available, falling back to `pip`.

### Qt SDK Installation

Install Qt SDK via aqtinstall (unified cross-platform approach):

```bash
# Auto-detect platform, use defaults from .github/build-config.json
./tools/setup/install_qt.py

# Install specific Qt version
./tools/setup/install_qt.py --version 6.10.1

# Install for Android
./tools/setup/install_qt.py --target android --arch android_arm64_v8a

# Install for iOS (macOS only)
./tools/setup/install_qt.py --target ios

# Install additional Qt tools (e.g., installer framework)
./tools/setup/install_qt.py --tools "tools_ifw"

# Export environment variables as bash
./tools/setup/install_qt.py --export bash

# Export environment variables as PowerShell
./tools/setup/install_qt.py --export powershell
```

**Configuration**: Qt version, path, modules, and architecture are read from `.github/build-config.json`. Defaults can be overridden via environment variables:

```bash
export QT_VERSION=6.10.1
export QT_TARGET=android
export QT_ARCH=android_arm64_v8a
./tools/setup/install_qt.py
```

### Cross-Platform Support

For platform-specific builds (iOS, Android), use install_qt.py with appropriate flags:

```bash
# iOS (requires macOS + Xcode)
./tools/setup/install_qt.py --target ios

# Android (requires Android NDK)
./tools/setup/install_qt.py --target android --arch android_arm64_v8a
```

## GStreamer Plugins

Build GStreamer multimedia plugins (optional, for advanced video codec support):

```bash
cd ./tools/setup/gstreamer

# Auto-detect platform and build
./build-gstreamer.py

# Build for specific platform
./build-gstreamer-linux.sh      # Linux
./build-gstreamer-macos.sh      # macOS
./build-gstreamer-ios.sh        # iOS
./build-gstreamer-android.sh    # Android
./build-gstreamer.py --platform windows  # Windows
```

Results are installed to the Qt SDK location.

## Configuration

### build-config.json

The primary configuration file (`.github/build-config.json`) defines:
- Qt version and modules
- Target platforms (desktop, iOS, Android)
- Architecture settings
- Tool paths

Python scripts read this via `read_config.py`:

```bash
./tools/setup/read_config.py --key qt.version
./tools/setup/read_config.py --key cmake.version
```

PowerShell scripts use `read-config.ps1`:

```powershell
.\tools\setup\read-config.ps1 -Key "qt.version"
```

## CI Integration

These scripts are used in GitHub Actions workflows (`.github/workflows/`):

- **build.yml**: Builds on Linux, macOS, Windows
- **android.yml**: Android APK builds
- **ios.yml**: iOS framework builds

### Example Workflow Usage

```yaml
# In GitHub Actions
- name: Install dependencies (Linux/macOS)
  run: python3 tools/setup/install_dependencies.py

- name: Install Qt
  run: ./tools/setup/install_qt.py --version 6.10.1

- name: Configure build
  run: |
    QT_VERSION=$(python3 ./tools/setup/read_config.py --key qt.version)
    cmake -B build -DCMAKE_PREFIX_PATH="$QT_ROOT_DIR" ...
```

## Troubleshooting

### Qt Installation Fails

1. Check network connectivity (aqtinstall downloads from mirror)
2. Verify mirror in `aqt-settings.ini`
3. Check disk space (Qt SDK ~5GB)
4. Clear cache: `rm -rf ~/.aqtinstall` (Linux/macOS) or `%LOCALAPPDATA%\.aqtinstall` (Windows)

### Python Dependencies Conflict

```bash
# Remove and recreate venv
rm -rf .venv
python tools/setup/install_python.py all
source .venv/bin/activate
```

### macOS Xcode Issues

```bash
# Install Xcode command-line tools
xcode-select --install

# Update to latest Xcode
sudo softwareupdate -i -a
```

### Windows PowerShell Execution Policy

```powershell
# Temporarily allow script execution for this session
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process

# Then run the installer
.\tools\setup\install-dependencies-windows.ps1
```

## Dependencies

| Tool | Installed By | Used For |
|------|--------------|----------|
| **CMake** ≥3.21 | `install_dependencies.py` | Build system |
| **Ninja** ≥1.11 | `install_dependencies.py` | Build backend |
| **Qt 6.x** | `install_qt.py` | GUI framework |
| **Python** ≥3.9 | System or `install_python.py` | Dev tools |
| **Git** | `install_dependencies.py` | Version control |

## See Also

- [QGroundControl Developer Guide](https://dev.qgroundcontrol.com/)
- [Qt Documentation](https://doc.qt.io/qt-6/)
- [CMake Reference](https://cmake.org/cmake/help/latest/)
