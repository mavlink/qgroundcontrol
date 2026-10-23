"""GStreamer builds with Cerbero for Android and iOS."""

from __future__ import annotations

import shutil
import subprocess
from typing import TYPE_CHECKING, ClassVar

from common.logging import log_info, log_ok
from common.proc import require_tools

from .host import run_cmd

if TYPE_CHECKING:
    from .config import BuildConfig


class CerberoBuilder:
    """Build GStreamer using Cerbero for mobile platforms."""

    # Archs with a config/cross-<platform>[-sim]-<arch>.cbc at the pinned Cerbero version, keyed by
    # (platform, simulator). The universal configs package every arch: Android armv7, arm64, x86 and
    # x86_64; iOS device arm64 plus simulator x86_64; iOS simulator arm64 and x86_64.
    CERBERO_ARCHS: ClassVar[dict[tuple[str, bool], tuple[str, ...]]] = {
        ("android", False): ("arm64", "armv7", "x86", "x86_64", "universal"),
        ("ios", False): ("arm64", "universal"),
        ("ios", True): ("arm64", "x86_64", "universal"),
    }

    def __init__(self, config: BuildConfig):
        self.config = config
        self.cerbero_dir = config.work_dir / "cerbero"

    def check_dependencies(self) -> None:
        """Check for required tools."""
        require_tools("git", "python3")
        if (
            self.config.platform == "ios"
            and subprocess.run(["xcrun", "--show-sdk-path"], capture_output=True).returncode != 0
        ):
            raise RuntimeError("Missing required tools: Xcode/iOS SDK")

        log_ok("All dependencies found")

    def clone_cerbero(self) -> None:
        """Clone Cerbero if needed."""
        if self.config.clean and self.cerbero_dir.exists():
            log_info("Cleaning previous Cerbero...")
            shutil.rmtree(self.cerbero_dir)

        if not self.cerbero_dir.exists():
            log_info(f"Cloning Cerbero for GStreamer {self.config.version}...")
            self.config.work_dir.mkdir(parents=True, exist_ok=True)
            run_cmd(
                [
                    "git",
                    "clone",
                    "--depth",
                    "1",
                    "--branch",
                    self.config.version,
                    "https://github.com/GStreamer/cerbero.git",
                    str(self.cerbero_dir),
                ]
            )
        else:
            log_info(f"Using existing Cerbero at {self.cerbero_dir}")

    def get_config_name(self) -> str:
        """The Cerbero config for the target platform, arch, and simulator flag."""
        plat, arch = self.config.platform, self.config.arch
        simulator = plat == "ios" and self.config.simulator
        if arch not in self.CERBERO_ARCHS.get((plat, simulator), ()):
            raise ValueError(
                f"Unsupported arch {arch} for {plat}{' simulator' if simulator else ''}"
            )
        return f"cross-{plat}{'-sim' if simulator else ''}-{arch.replace('_', '-')}"

    def bootstrap(self, config_name: str) -> None:
        """Bootstrap Cerbero and the target's toolchain (e.g. the Android NDK)."""
        log_info("Bootstrapping Cerbero...")
        cerbero = self.cerbero_dir / "cerbero-uninstalled"
        run_cmd([str(cerbero), "-c", config_name, "bootstrap"], cwd=self.cerbero_dir)

    def package(self, config_name: str) -> None:
        """Build and package gstreamer-1.0 into the install prefix."""
        log_info(f"Building GStreamer with Cerbero config {config_name}...")
        cmd = [str(self.cerbero_dir / "cerbero-uninstalled"), "-c", config_name]
        if self.config.build_type == "debug":
            cmd.extend(["-v", "debug"])
        cmd.extend(["package", "gstreamer-1.0", "-o", str(self.config.prefix)])
        run_cmd(cmd, cwd=self.cerbero_dir)

    def run(self) -> None:
        """Execute the full build process."""
        log_info(
            f"Building GStreamer {self.config.version} for {self.config.platform}/{self.config.arch}"
        )

        config_name = self.get_config_name()
        self.check_dependencies()
        self.clone_cerbero()
        self.bootstrap(config_name)
        self.package(config_name)

        log_ok(f"GStreamer {self.config.version} built for {self.config.platform}")
