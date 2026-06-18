"""Install system dependencies for QGroundControl development.

Supports Debian/Ubuntu, Fedora/RHEL, Arch, macOS, and Windows. Auto-detects
platform or allows override.

Usage:
    python tools/setup/install_dependencies                 # Auto-detect platform
    python tools/setup/install_dependencies --dry-run       # Show what would be installed
    python tools/setup/install_dependencies --list          # List packages by category
    python tools/setup/install_dependencies --platform debian
"""

from __future__ import annotations

from ._arch import install_arch
from ._cli import list_packages, main, parse_args, print_packages
from ._common import (
    APT_BASE_OPTIONS,
    add_to_path,
    check_apt_package_available,
    detect_platform,
    download_file,
    get_apt_install_command,
    get_apt_update_command,
    get_available_debian_packages,
    get_brew_install_command,
    get_config_value,
    get_dnf_install_command,
    has_command,
    is_ci,
    is_ubuntu,
    run_apt_install_with_retry,
    run_command,
    run_dnf_install_with_retry,
    run_pacman_install_with_retry,
    set_env_var,
)
from ._debian import (
    DEBIAN_PACKAGE_ALTERNATIVES,
    JUST_MIN_VERSION,
    JUST_TARGETS,
    JUST_VERSION,
    _detect_just_version,
    install_debian,
    install_just_debian,
    resolve_package_alternatives,
)
from ._fedora import install_fedora
from ._macos import get_gstreamer_macos_urls, install_macos
from ._packages import (
    ARCH_PACKAGES,
    DEBIAN_PACKAGES,
    FEDORA_PACKAGES,
    MACOS_PACKAGES,
    PACKAGE_NAME_RE,
    PIPX_PACKAGES,
    get_arch_packages,
    get_debian_packages,
    get_fedora_packages,
    get_macos_packages,
    validate_extra_packages,
)
from ._windows import (
    WINDOWS_GSTREAMER_BASE_URL,
    WINDOWS_GSTREAMER_INSTALL_DIR,
    WINDOWS_GSTREAMER_PREFIX,
    WINDOWS_VULKAN_INSTALL_DIR,
    WINDOWS_VULKAN_URL,
    install_windows,
    install_windows_gstreamer,
    install_windows_vulkan,
)

__all__ = [
    "APT_BASE_OPTIONS",
    "ARCH_PACKAGES",
    "DEBIAN_PACKAGES",
    "DEBIAN_PACKAGE_ALTERNATIVES",
    "FEDORA_PACKAGES",
    "JUST_MIN_VERSION",
    "JUST_TARGETS",
    "JUST_VERSION",
    "MACOS_PACKAGES",
    "PACKAGE_NAME_RE",
    "PIPX_PACKAGES",
    "WINDOWS_GSTREAMER_BASE_URL",
    "WINDOWS_GSTREAMER_INSTALL_DIR",
    "WINDOWS_GSTREAMER_PREFIX",
    "WINDOWS_VULKAN_INSTALL_DIR",
    "WINDOWS_VULKAN_URL",
    "_detect_just_version",
    "add_to_path",
    "check_apt_package_available",
    "detect_platform",
    "download_file",
    "get_apt_install_command",
    "get_apt_update_command",
    "get_arch_packages",
    "get_available_debian_packages",
    "get_brew_install_command",
    "get_config_value",
    "get_debian_packages",
    "get_dnf_install_command",
    "get_fedora_packages",
    "get_gstreamer_macos_urls",
    "get_macos_packages",
    "has_command",
    "install_arch",
    "install_debian",
    "install_fedora",
    "install_just_debian",
    "install_macos",
    "install_windows",
    "install_windows_gstreamer",
    "install_windows_vulkan",
    "is_ci",
    "is_ubuntu",
    "list_packages",
    "main",
    "parse_args",
    "print_packages",
    "resolve_package_alternatives",
    "run_apt_install_with_retry",
    "run_command",
    "run_dnf_install_with_retry",
    "run_pacman_install_with_retry",
    "set_env_var",
    "validate_extra_packages",
]
