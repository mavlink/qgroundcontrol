"""Windows installer (MSI for GStreamer, exe for Vulkan SDK)."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

from . import _common as _c

WINDOWS_GSTREAMER_BASE_URL = (
    "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/windows"
)
WINDOWS_GSTREAMER_INSTALL_DIR = "C:\\gstreamer"
WINDOWS_GSTREAMER_PREFIX = "C:\\gstreamer\\1.0\\msvc_x86_64"
WINDOWS_VULKAN_INSTALL_DIR = "C:\\VulkanSDK\\latest"
WINDOWS_VULKAN_URL = "https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe"
WINDOWS_VS_BUILDTOOLS_URL = "https://aka.ms/vs/17/release/vs_BuildTools.exe"
WINDOWS_NSIS_VERSION = "3.11"
WINDOWS_NSIS_URL = (
    f"https://downloads.sourceforge.net/project/nsis/NSIS%203/{WINDOWS_NSIS_VERSION}/"
    f"nsis-{WINDOWS_NSIS_VERSION}-setup.exe"
)


def install_windows_nsis(dry_run: bool = False) -> bool:
    """Install NSIS (makensis) for the Windows installer build step."""
    program_files_x86 = os.environ.get("PROGRAMFILES(X86)", r"C:\Program Files (x86)")
    makensis = Path(program_files_x86) / "NSIS" / "makensis.exe"
    if makensis.exists():
        print(f"NSIS already installed at {makensis}; skipping")
        return True

    print(f"\nInstalling NSIS {WINDOWS_NSIS_VERSION}...")
    with tempfile.TemporaryDirectory() as tmpdir:
        installer = Path(tmpdir) / "nsis-setup.exe"
        if not _c.download_file(WINDOWS_NSIS_URL, installer, dry_run):
            return False
        if not _c.run_command([str(installer), "/S"], dry_run):
            return False

    if not dry_run and not makensis.exists():
        _c.log_error(f"NSIS installer reported success but {makensis} is missing")
        return False
    print("NSIS installed")
    return True


def _verify_msvc(arm64: bool) -> bool:
    """Confirm the requested VC Tools components are present via vswhere."""
    program_files_x86 = os.environ.get("PROGRAMFILES(X86)", r"C:\Program Files (x86)")
    vswhere = Path(program_files_x86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        _c.log_warn(f"vswhere not found at {vswhere}; skipping MSVC verification")
        return True

    components = ["Microsoft.VisualStudio.Component.VC.Tools.x86.x64"]
    if arm64:
        components.append("Microsoft.VisualStudio.Component.VC.Tools.ARM64")
    for component in components:
        result = subprocess.run(
            [
                str(vswhere),
                "-products",
                "*",
                "-requires",
                component,
                "-property",
                "installationPath",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0 or not result.stdout.strip():
            _c.log_error(f"MSVC verification failed: component {component} not installed")
            return False
    return True


def install_windows_msvc(dry_run: bool = False, arm64: bool = False) -> bool:
    """Install Visual Studio 2022 Build Tools with the C++ (VCTools) workload."""
    print("\nInstalling Visual Studio Build Tools (VCTools)...")
    components = ["--add", "Microsoft.VisualStudio.Workload.VCTools", "--includeRecommended"]
    if arm64:
        components += ["--add", "Microsoft.VisualStudio.Component.VC.Tools.ARM64"]

    with tempfile.TemporaryDirectory() as tmpdir:
        installer = Path(tmpdir) / "vs_BuildTools.exe"
        if not _c.download_file(WINDOWS_VS_BUILDTOOLS_URL, installer, dry_run):
            return False
        if not _c.run_command(
            [str(installer), "--quiet", "--wait", "--norestart", "--nocache", *components],
            dry_run,
            ok_returncodes=(0, 3010),
        ):
            return False

    if not dry_run and not _verify_msvc(arm64):
        return False
    print("Visual Studio Build Tools installed")
    return True


def install_windows_gstreamer(version: str, dry_run: bool = False) -> bool:
    """Install GStreamer on Windows (AMD64 only)."""
    arch = os.environ.get("PROCESSOR_ARCHITECTURE", "")
    if arch != "AMD64":
        _c.log_warn(f"Skipping GStreamer: only supported on AMD64 (detected: {arch or 'unknown'})")
        return True

    prefix = WINDOWS_GSTREAMER_PREFIX
    if Path(prefix, "bin", "gst-launch-1.0.exe").exists():
        print(f"GStreamer already installed at {prefix}; skipping download+install")
        _c.set_env_var("GSTREAMER_1_0_ROOT_MSVC_X86_64", prefix)
        _c.set_env_var("GSTREAMER_1_0_ROOT_X86_64", prefix)
        _c.add_to_path(f"{prefix}\\bin")
        return True

    base_url = f"{WINDOWS_GSTREAMER_BASE_URL}/{version}"
    runtime_name = f"gstreamer-1.0-msvc-x86_64-{version}.msi"
    devel_name = f"gstreamer-1.0-devel-msvc-x86_64-{version}.msi"

    print(f"\nInstalling GStreamer {version}...")
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        runtime_msi = tmp / runtime_name
        devel_msi = tmp / devel_name

        download_targets = [
            ("runtime", f"{base_url}/{runtime_name}", runtime_msi),
            ("devel", f"{base_url}/{devel_name}", devel_msi),
        ]
        if dry_run:
            for _, url, path in download_targets:
                if not _c.download_file(url, path, dry_run):
                    return False
        else:
            print("  Downloading GStreamer installers in parallel...")
            with ThreadPoolExecutor(max_workers=2) as executor:
                futures = {
                    executor.submit(_c.download_file, url, path): label
                    for label, url, path in download_targets
                }
                for future in as_completed(futures):
                    label = futures[future]
                    if not future.result():
                        print(
                            f"Failed to download GStreamer {label} installer",
                            file=sys.stderr,
                        )
                        return False

        install_dir = WINDOWS_GSTREAMER_INSTALL_DIR
        for label, msi in [("runtime", runtime_msi), ("devel", devel_msi)]:
            print(f"  Installing GStreamer {label}...")
            if not _c.run_command(
                [
                    "msiexec.exe",
                    "/i",
                    str(msi),
                    "/passive",
                    f"INSTALLDIR={install_dir}",
                    "ADDLOCAL=ALL",
                ],
                dry_run,
            ):
                return False

    prefix = WINDOWS_GSTREAMER_PREFIX
    _c.set_env_var("GSTREAMER_1_0_ROOT_MSVC_X86_64", prefix)
    _c.set_env_var("GSTREAMER_1_0_ROOT_X86_64", prefix)
    _c.add_to_path(f"{prefix}\\bin")
    print(f"GStreamer {version} installed to {prefix}")
    return True


def install_windows_vulkan(dry_run: bool = False) -> bool:
    """Install Vulkan SDK on Windows."""
    print("\nInstalling Vulkan SDK...")
    install_dir = WINDOWS_VULKAN_INSTALL_DIR
    with tempfile.TemporaryDirectory() as tmpdir:
        installer = Path(tmpdir) / "vulkan-sdk.exe"
        if not _c.download_file(WINDOWS_VULKAN_URL, installer, dry_run):
            return False

        if not _c.run_command(
            [
                str(installer),
                "--root",
                install_dir,
                "--accept-licenses",
                "--default-answer",
                "--confirm-command",
                "install",
                "com.lunarg.vulkan.glm",
                "com.lunarg.vulkan.volk",
                "com.lunarg.vulkan.vma",
                "com.lunarg.vulkan.debug",
            ],
            dry_run,
        ):
            return False

    _c.set_env_var("VULKAN_SDK", install_dir)
    _c.add_to_path(f"{install_dir}\\Bin")
    print("Vulkan SDK installed")
    return True


def install_windows(
    dry_run: bool = False,
    gstreamer_version: str | None = None,
    skip_gstreamer: bool = False,
    vulkan: bool = False,
    msvc: bool = False,
    msvc_arm64: bool = False,
    nsis: bool = False,
) -> bool:
    """Install Windows dependencies."""
    _c.log_info("Installing Windows dependencies...")
    arch = os.environ.get("PROCESSOR_ARCHITECTURE", "unknown")
    print(f"Architecture: {arch}")

    if msvc and not install_windows_msvc(dry_run, msvc_arm64):
        return False

    if nsis and not install_windows_nsis(dry_run):
        return False

    if not skip_gstreamer:
        version = gstreamer_version or _c.get_config_value("gstreamer.version.windows")
        if not version:
            print(
                "Error: GStreamer version not found. "
                "Use --gstreamer-version or check build-config.json",
                file=sys.stderr,
            )
            return False
        if not install_windows_gstreamer(version, dry_run):
            return False

    if vulkan and not install_windows_vulkan(dry_run):
        return False

    print("\nWindows dependencies installed!")
    return True


__all__ = [
    "WINDOWS_GSTREAMER_BASE_URL",
    "WINDOWS_GSTREAMER_INSTALL_DIR",
    "WINDOWS_GSTREAMER_PREFIX",
    "WINDOWS_VS_BUILDTOOLS_URL",
    "WINDOWS_VULKAN_INSTALL_DIR",
    "WINDOWS_VULKAN_URL",
    "install_windows",
    "install_windows_gstreamer",
    "install_windows_msvc",
    "install_windows_nsis",
    "install_windows_vulkan",
]
