"""Install system dependencies for QGroundControl development.

Supports Debian/Ubuntu, Fedora/RHEL, Arch, macOS, and Windows. Auto-detects
platform or allows override.

Usage:
    python tools/setup/install_dependencies                 # Auto-detect platform
    python tools/setup/install_dependencies --dry-run       # Show what would be installed
    python tools/setup/install_dependencies --list          # List packages by category
    python tools/setup/install_dependencies --platform debian

``__main__`` is the entry point; import helpers from the private submodules.
"""
