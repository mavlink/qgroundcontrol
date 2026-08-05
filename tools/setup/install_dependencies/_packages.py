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
        # Backing lib for Qt's speechd TextToSpeech plugin so CPackDeb's
        # dpkg-shlibdeps resolves it into a runtime Depends instead of erroring.
        "libspeechd2",
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
        # install verification needs base/good/bad (openh264) + gl (opengl) to match the native set.
        "gstreamer1.0-plugins-base",
        "gstreamer1.0-plugins-good",
        "gstreamer1.0-plugins-bad",
        "gstreamer1.0-gl",
        "libusb-1.0-0-dev",
        "libsdl2-dev",
    ],
}

# Fedora/RHEL (dnf), mirrors DEBIAN_PACKAGES. gstreamer ugly/libav come from RPM
# Fusion; installer uses --skip-unavailable so a missing optional plugin is OK.
FEDORA_PACKAGES: dict[str, list[str]] = {
    "core": [
        "dnf-plugins-core",
        "gnupg2",
        "ca-certificates",
        "appstream",
        "binutils",
        "gcc",
        "gcc-c++",
        "make",
        "ccache",
        "cmake",
        "cppcheck",
        "file",
        "gdb",
        "gettext",
        "git",
        "just",
        "fuse-libs",
        "fuse3",
        "libtool",
        "glibc-langpack-en",
        "mold",
        "nasm",
        "ninja-build",
        "patchelf",
        "pipx",
        "pkgconf-pkg-config",
        "rpm-build",
        "python3",
        "python3-pip",
        "rsync",
        "unzip",
        "valgrind",
        "wget",
        "zsync",
    ],
    "qt": [
        "at-spi2-core-devel",
        "fontconfig-devel",
        "freetype-devel",
        "gtk3-devel",
        "libSM-devel",
        "libX11-devel",
        "libX11-xcb",
        "libxcb-devel",
        "xcb-util-cursor-devel",
        "xcb-util-wm-devel",
        "xcb-util-image-devel",
        "xcb-util-keysyms-devel",
        "xcb-util-renderutil-devel",
        "xcb-util-devel",
        "libXext-devel",
        "libXfixes-devel",
        "libXi-devel",
        "libxkbcommon-devel",
        "libxkbcommon-x11-devel",
        "libXrender-devel",
        "libunwind-devel",
        "mesa-libEGL-devel",
        "mesa-libGL-devel",
    ],
    "gstreamer": [
        "gstreamer1-devel",
        "gstreamer1-plugins-base-devel",
        "gstreamer1-plugins-bad-free-devel",
        "gstreamer1-plugins-base",
        "gstreamer1-plugins-good",
        "gstreamer1-plugins-bad-free",
        "gstreamer1-plugins-ugly",
        "gstreamer1-libav",
        "blas",
        "openblas",
        "python3-gobject",
        "python3-gstreamer1",
    ],
    "sdl": [
        "libusb1-devel",
    ],
    "audio": [
        "pulseaudio-libs-devel",
    ],
    "misc": [
        "vulkan-loader-devel",
        "pipewire-devel",
    ],
}

# Arch (pacman). Arch ships headers inside the runtime package, so there is no
# -dev/-devel split. base-devel (group) covers gcc/make/binutils/libtool/pkgconf.
ARCH_PACKAGES: dict[str, list[str]] = {
    "core": [
        "base-devel",
        "gnupg",
        "ca-certificates",
        "appstream",
        "ccache",
        "cmake",
        "cppcheck",
        "file",
        "gdb",
        "gettext",
        "git",
        "just",
        "fuse2",
        "fuse3",
        "mold",
        "nasm",
        "ninja",
        "patchelf",
        "python-pipx",
        "python",
        "python-pip",
        "rsync",
        "unzip",
        "valgrind",
        "wget",
        "zsync",
    ],
    "qt": [
        "at-spi2-core",
        "fontconfig",
        "freetype2",
        "gtk3",
        "libsm",
        "libx11",
        "libxcb",
        "xcb-util-cursor",
        "xcb-util-wm",
        "xcb-util-image",
        "xcb-util-keysyms",
        "xcb-util-renderutil",
        "xcb-util",
        "libxext",
        "libxfixes",
        "libxi",
        "libxkbcommon",
        "libxkbcommon-x11",
        "libxrender",
        "libunwind",
        "libglvnd",
        "mesa",
    ],
    "gstreamer": [
        "gstreamer",
        "gst-plugins-base",
        "gst-plugins-base-libs",
        "gst-plugins-good",
        "gst-plugins-bad",
        "gst-plugins-ugly",
        "gst-libav",
        "openblas",
        "python-gobject",
        "gst-python",
    ],
    "sdl": [
        "libusb",
    ],
    "audio": [
        "libpulse",
    ],
    "misc": [
        "vulkan-icd-loader",
        "vulkan-headers",
        "pipewire",
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


def _get_categorized_packages(
    table: dict[str, list[str]], category: str | None = None
) -> list[str]:
    """Flatten a category->packages table, skipping cross_/optional categories."""
    if category:
        return list(table.get(category, []))
    packages: list[str] = []
    for cat, pkgs in table.items():
        if cat.startswith("cross_") or cat.endswith("_optional"):
            continue
        packages.extend(pkgs)
    return list(dict.fromkeys(packages))


def get_fedora_packages(category: str | None = None) -> list[str]:
    """Get Fedora/RHEL dnf packages, optionally filtered by category."""
    return _get_categorized_packages(FEDORA_PACKAGES, category)


def get_arch_packages(category: str | None = None) -> list[str]:
    """Get Arch pacman packages, optionally filtered by category."""
    return _get_categorized_packages(ARCH_PACKAGES, category)


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
    "ARCH_PACKAGES",
    "DEBIAN_PACKAGES",
    "FEDORA_PACKAGES",
    "MACOS_PACKAGES",
    "PACKAGE_NAME_RE",
    "PIPX_PACKAGES",
    "get_arch_packages",
    "get_debian_packages",
    "get_fedora_packages",
    "get_macos_packages",
    "validate_extra_packages",
]
