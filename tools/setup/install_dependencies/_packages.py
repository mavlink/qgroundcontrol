"""Package list constants + selection helpers for all platforms."""

from __future__ import annotations

import re

PACKAGE_NAME_RE = re.compile(r"^[a-zA-Z0-9][a-zA-Z0-9.+-]*$")

DEBIAN_PACKAGES: dict[str, list[str]] = {
    "core": [
        "software-properties-common",
        "gnupg2",
        "ca-certificates",
        "appstream",
        "binutils",
        "build-essential",
        "ccache",
        "cmake",
        "cppcheck",
        "file",
        "gdb",
        "gettext",
        "git",
        "libfuse2",
        "fuse3",
        "libtool",
        "locales",
        "mold",
        "nasm",
        "ninja-build",
        "patchelf",
        "pipx",
        "pkgconf",
        "python3",
        "python3-pip",
        "rsync",
        "unzip",
        "valgrind",
        "wget",
        "zsync",
    ],
    "qt": [
        "libatspi2.0-dev",
        "libfontconfig1-dev",
        "libfreetype-dev",
        "libgtk-3-dev",
        "libsm-dev",
        "libx11-dev",
        "libx11-xcb-dev",
        "libxcb-cursor-dev",
        "libxcb-glx0-dev",
        "libxcb-icccm4-dev",
        "libxcb-image0-dev",
        "libxcb-keysyms1-dev",
        "libxcb-present-dev",
        "libxcb-randr0-dev",
        "libxcb-render-util0-dev",
        "libxcb-render0-dev",
        "libxcb-shape0-dev",
        "libxcb-shm0-dev",
        "libxcb-sync-dev",
        "libxcb-util-dev",
        "libxcb-xfixes0-dev",
        "libxcb-xinerama0-dev",
        "libxcb-xkb-dev",
        "libxcb1-dev",
        "libxext-dev",
        "libxfixes-dev",
        "libxi-dev",
        "libxkbcommon-dev",
        "libxkbcommon-x11-dev",
        "libxrender-dev",
        "libunwind-dev",
        "libegl-dev",
        # Headless GL: xvfb gives ctest a virtual display so the offscreen plugin's
        # GLX path can create a Mesa llvmpipe context (see cmake_helper.py).
        "xvfb",
        "xauth",
        "libgl1-mesa-dri",
        "libglx-mesa0",
    ],
    "gstreamer": [
        "libgstreamer1.0-dev",
        "libgstreamer-plugins-bad1.0-dev",
        "libgstreamer-plugins-base1.0-dev",
        "libgstreamer-plugins-good1.0-dev",
        "libgstreamer-gl1.0-0",
        "gstreamer1.0-plugins-bad",
        "gstreamer1.0-plugins-base",
        "gstreamer1.0-plugins-good",
        "gstreamer1.0-plugins-ugly",
        "gstreamer1.0-plugins-rtp",
        "gstreamer1.0-gl",
        "gstreamer1.0-libav",
        "gstreamer1.0-rtsp",
        "gstreamer1.0-x",
        "libblas3",
        "libopenblas0",
        "python3-gi",
        "python3-gst-1.0",
    ],
    "gstreamer_optional": [
        "gstreamer1.0-qt6",
    ],
    "sdl": [
        "libusb-1.0-0-dev",
    ],
    "audio": [
        "libpulse-dev",
    ],
    "misc": [
        "libvulkan-dev",
        "libpipewire-0.3-dev",
    ],
    # Target-side libraries for the aarch64 cross sysroot. Single source for
    # deploy/docker/install-sysroot-aarch64.sh, which apt-installs these with an
    # :arm64 tag. Excluded from the aggregate (native) set by get_debian_packages.
    "cross_arm64": [
        "libc6",
        "libstdc++6",
        "libgcc-s1",
        "libudev-dev",
        "libinput-dev",
        "libdrm-dev",
        "libgbm-dev",
        "libgl-dev",
        "libgles-dev",
        "libegl-dev",
        "libvulkan-dev",
        "libfontconfig1-dev",
        "libfreetype-dev",
        "libglib2.0-dev",
        "libx11-dev",
        "libx11-xcb-dev",
        "libxext-dev",
        "libxfixes-dev",
        "libxcb1-dev",
        "libxcb-cursor-dev",
        "libxcb-glx0-dev",
        "libxcb-icccm4-dev",
        "libxcb-image0-dev",
        "libxcb-keysyms1-dev",
        "libxcb-randr0-dev",
        "libxcb-render0-dev",
        "libxcb-render-util0-dev",
        "libxcb-shape0-dev",
        "libxcb-shm0-dev",
        "libxcb-sync-dev",
        "libxcb-util-dev",
        "libxcb-xfixes0-dev",
        "libxcb-xinerama0-dev",
        "libxcb-xinput-dev",
        "libxcb-xkb-dev",
        "libxkbcommon-dev",
        "libxkbcommon-x11-dev",
        "libxi-dev",
        "libxrender-dev",
        "libxdamage-dev",
        "libxrandr-dev",
        "libdbus-1-dev",
        "libpulse-dev",
        "libasound2-dev",
        "libssl-dev",
        "libnss3-dev",
        "zlib1g-dev",
        "libicu-dev",
        "libpcre2-dev",
        "libgstreamer1.0-dev",
        "libgstreamer-plugins-base1.0-dev",
        "libgstreamer-plugins-bad1.0-dev",
        # Runtime plugin .so packages for the AppDir (the -dev packages above are link-only);
        # install verification needs playback/tcp (base) and rtsp/rtp/rtpmanager/udp (good).
        "gstreamer1.0-plugins-base",
        "gstreamer1.0-plugins-good",
        "libusb-1.0-0-dev",
        "libsdl2-dev",
    ],
}

MACOS_PACKAGES: list[str] = [
    "cmake",
    "ninja",
    "ccache",
    "git",
    "just",
    "pkgconf",
    "create-dmg",
    "mold",
    "nasm",
]

PIPX_PACKAGES: list[str] = [
    "cmake",
    "ninja",
    "gcovr",
]

def get_debian_packages(
    category: str | None = None,
    include_optional: bool = False,
) -> list[str]:
    """Get Debian packages, optionally filtered by category."""
    if category:
        return list(DEBIAN_PACKAGES.get(category, []))

    packages = []
    for cat, pkgs in DEBIAN_PACKAGES.items():
        if cat.startswith("cross_"):
            continue  # target-arch sysroot libs, not for the native host
        if cat.endswith("_optional") and not include_optional:
            continue
        packages.extend(pkgs)
    return list(dict.fromkeys(packages))  # Remove duplicates, preserve order

def get_macos_packages() -> list[str]:
    """Get macOS Homebrew packages."""
    return list(MACOS_PACKAGES)

def validate_extra_packages(packages: list[str]) -> list[str]:
    """Validate extra package names passed from CI inputs."""
    validated: list[str] = []
    for package in packages:
        if not PACKAGE_NAME_RE.match(package):
            raise ValueError(f"Invalid package name '{package}'")
        validated.append(package)
    return validated

__all__ = [
    "DEBIAN_PACKAGES",
    "MACOS_PACKAGES",
    "PACKAGE_NAME_RE",
    "PIPX_PACKAGES",
    "get_debian_packages",
    "get_macos_packages",
    "validate_extra_packages",
]
