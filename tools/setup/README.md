# QGroundControl Setup Scripts

Build environment setup for QGroundControl development across Linux, macOS, and Windows.

## Overview

This directory contains scripts to install system dependencies, Qt SDK, Python tools, and platform-specific build requirements. All scripts are platform-aware and include cross-platform support via Python.

## Quick Start

```bash
# Install system dependencies for your platform
./tools/setup/install-dependencies-debian.sh    # Debian/Ubuntu
./tools/setup/install-dependencies-macos.sh     # macOS
./tools/setup/install-dependencies-windows.ps1  # Windows (PowerShell)

# Install Python development tools
./tools/setup/install-python.sh

# Install Qt SDK
./tools/setup/install-qt.py
```

## Scripts Reference

| Script | Purpose | Platforms |
|--------|---------|-----------|
| **install-dependencies-debian.sh** | System packages (cmake, ninja, git, etc.) | Debian/Ubuntu |
| **install-dependencies-macos.sh** | Homebrew packages (xcode, cmake, ninja) | macOS |
| **install-dependencies-windows.ps1** | Windows system dependencies (Visual Studio, CMake) | Windows |
| **install-python.sh** | Python venv + pip dependencies for development | All |
| **install-qt.py** | Qt SDK via aqtinstall (auto-detects platform) | All |
| **install-qt-debian.sh** | Legacy: Qt installation wrapper for Debian | Debian/Ubuntu |
| **install-qt-ios.sh** | Qt iOS cross-compilation setup | macOS only |
| **install-qt-android.sh** | Qt Android cross-compilation setup | All |
| **read-config.sh** | Read build configuration (bash) | Linux/macOS |
| **read-config.py** | Read build configuration (Python) | All |
| **read-config.ps1** | Read build configuration (PowerShell) | Windows |
| **aqt-settings.ini** | Configuration for aqtinstall (mirrors, paths) | All |

## Installation Guide

### Dependencies (System Packages)

Install system packages required for building QGroundControl:

```bash
# Debian/Ubuntu
./tools/setup/install-dependencies-debian.sh

# macOS
./tools/setup/install-dependencies-macos.sh

# Windows (PowerShell, admin required)
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
.\tools\setup\install-dependencies-windows.ps1
```

### Python Environment

Install Python development tools and create virtual environment:

```bash
# Install CI tools (pre-commit, meson, ninja)
./tools/setup/install-python.sh

# Install Qt installation tools
./tools/setup/install-python.sh qt

# Install code coverage tools
./tools/setup/install-python.sh coverage

# Install all development dependencies
./tools/setup/install-python.sh all

# Multiple groups can be combined
./tools/setup/install-python.sh ci,coverage
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
./tools/setup/install-qt.py

# Install specific Qt version
./tools/setup/install-qt.py --version 6.10.1

# Install for Android
./tools/setup/install-qt.py --target android --arch android_arm64_v8a

# Install for iOS (macOS only)
./tools/setup/install-qt.py --target ios

# Install additional Qt tools (e.g., installer framework)
./tools/setup/install-qt.py --tools "tools_ifw"

# Export environment variables as bash
./tools/setup/install-qt.py --export bash

# Export environment variables as PowerShell
./tools/setup/install-qt.py --export powershell
```

**Configuration**: Qt version, path, modules, and architecture are read from `.github/build-config.json`. Defaults can be overridden via environment variables:

```bash
export QT_VERSION=6.10.1
export QT_TARGET=android
export QT_ARCH=android_arm64_v8a
./tools/setup/install-qt.py
```

### Cross-Platform Support

For platform-specific builds (iOS, Android), use the dedicated wrappers:

```bash
# iOS (requires macOS + Xcode)
./tools/setup/install-qt-ios.sh

# Android (requires Android NDK)
./tools/setup/install-qt-android.sh
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
./build-gstreamer-windows.ps1   # Windows
```

Results are installed to the Qt SDK location.

## Configuration

### build-config.json

The primary configuration file (`.github/build-config.json`) defines:
- Qt version and modules
- Target platforms (desktop, iOS, Android)
- Architecture settings
- Tool paths

Python scripts read this via `read-config.py`:

```bash
./tools/setup/read-config.py --key qt.version
./tools/setup/read-config.py --key cmake.version
```

Bash scripts use `read-config.sh`:

```bash
source ./tools/setup/read-config.sh
echo "$QT_VERSION"
echo "$CMAKE_VERSION"
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

```bash
# In GitHub Actions
- name: Install dependencies
  run: ./tools/setup/install-dependencies-${{ runner.os }}.sh

- name: Install Qt
  run: ./tools/setup/install-qt.py --version 6.10.1

- name: Configure build
  run: |
    source ./tools/setup/read-config.sh
    cmake -B build -DCMAKE_PREFIX_PATH="$QT_PATH" ...
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
./tools/setup/install-python.sh all
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
| **CMake** ≥3.21 | `install-dependencies-*.sh` | Build system |
| **Ninja** ≥1.11 | `install-dependencies-*.sh` | Build backend |
| **Qt 6.x** | `install-qt.py` | GUI framework |
| **Python** ≥3.9 | System or `install-python.sh` | Dev tools |
| **Git** | `install-dependencies-*.sh` | Version control |

## See Also

- [QGroundControl Developer Guide](https://dev.qgroundcontrol.com/)
- [Qt Documentation](https://doc.qt.io/qt-6/)
- [CMake Reference](https://cmake.org/cmake/help/latest/)
