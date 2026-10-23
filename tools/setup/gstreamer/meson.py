"""GStreamer builds with Meson for Linux, macOS and Windows."""

from __future__ import annotations

import os
import shutil
import subprocess
from dataclasses import replace

from common.logging import log_info, log_ok
from common.proc import require_tools
from qgc_tools.python_env import executable, sync_groups

from .config import (
    GL_CONFIG,
    MESON_COMMON,
    PLUGINS_BAD,
    PLUGINS_BASE,
    PLUGINS_GOOD,
    PLUGINS_UGLY,
    BuildConfig,
)
from .host import detect_jobs, run_cmd


class MesonBuilder:
    """Build GStreamer using Meson for desktop platforms."""

    def __init__(self, config: BuildConfig):
        self.config = config

    def check_dependencies(self) -> None:
        """Check for required build tools."""
        require_tools("git", "python3", "pkg-config")
        if (
            self.config.platform == "macos"
            and subprocess.run(["xcode-select", "-p"], capture_output=True).returncode != 0
        ):
            raise RuntimeError("Missing required tools: xcode-select")

        log_ok("All dependencies found")

    def ensure_meson(self) -> None:
        """Ensure meson and ninja are available."""
        environment = sync_groups("build")
        os.environ["PATH"] = (
            f"{executable('meson', environment).parent}{os.pathsep}{os.environ['PATH']}"
        )
        if (
            not executable("meson", environment).is_file()
            or not executable("ninja", environment).is_file()
        ):
            raise RuntimeError("Locked build environment is missing meson or ninja")

    def clone_source(self) -> None:
        """Clone GStreamer source if needed."""
        if self.config.clean and self.config.source_dir.exists():
            log_info("Cleaning previous build...")
            shutil.rmtree(self.config.source_dir)

        if not self.config.source_dir.exists():
            log_info(f"Cloning GStreamer {self.config.version}...")
            self.config.work_dir.mkdir(parents=True, exist_ok=True)
            run_cmd(
                [
                    "git",
                    "clone",
                    "--depth",
                    "1",
                    "--branch",
                    self.config.version,
                    "https://github.com/GStreamer/gstreamer.git",
                    str(self.config.source_dir),
                ]
            )
        else:
            log_info(f"Using existing source at {self.config.source_dir}")

    def get_meson_args(self) -> list:
        """Build meson configuration arguments."""
        args = [
            f"--prefix={self.config.prefix}",
            f"--buildtype={self.config.build_type}",
            "--wrap-mode=forcefallback",
            "--strip",
        ]

        # Add common options
        for key, value in MESON_COMMON.items():
            args.append(f"-D{key}={value}")

        # Add base plugins
        for key, value in PLUGINS_BASE.items():
            args.append(f"-D{key}={value}")

        # Add good plugins
        for key, value in PLUGINS_GOOD.items():
            args.append(f"-D{key}={value}")

        # Add bad plugins
        for key, value in PLUGINS_BAD.items():
            args.append(f"-D{key}={value}")

        # Add ugly plugins
        for key, value in PLUGINS_UGLY.items():
            args.append(f"-D{key}={value}")

        # Add platform-specific GL config
        gl_config = GL_CONFIG.get(self.config.platform, {})
        for key, value in gl_config.items():
            args.append(f"-D{key}={value}")

        # Each slice of a macOS universal build targets one arch; the link must match.
        if self.config.platform == "macos" and self.config.arch != "universal":
            arch_flags = f"['-arch', '{self.config.arch}']"
            for lang in ("c", "cpp", "objc", "objcpp"):
                args.append(f"-D{lang}_args={arch_flags}")
                args.append(f"-D{lang}_link_args={arch_flags}")

        return args

    def configure(self) -> None:
        """Configure the build with meson."""
        if (
            self.config.build_dir.exists()
            and not self.config.clean
            and (self.config.build_dir / "build.ninja").exists()
        ):
            log_info("Using existing configuration")
            return

        log_info("Configuring GStreamer...")
        if self.config.build_dir.exists():
            shutil.rmtree(self.config.build_dir)

        args = ["meson", "setup", str(self.config.build_dir), *self.get_meson_args()]

        env = {}
        if self.config.qt_prefix:
            env["PKG_CONFIG_PATH"] = f"{self.config.qt_prefix}/lib/pkgconfig"

        run_cmd(args, cwd=self.config.source_dir, env=env)

    def build(self) -> None:
        """Compile GStreamer."""
        jobs = detect_jobs(self.config.jobs)
        log_info(f"Compiling GStreamer with {jobs} jobs...")
        run_cmd(["meson", "compile", "-C", str(self.config.build_dir), "-j", str(jobs)])

    def install(self) -> None:
        """Install GStreamer."""
        log_info("Installing GStreamer...")
        run_cmd(["meson", "install", "-C", str(self.config.build_dir)])

    def build_universal(self) -> None:
        """Build each macOS arch from one source checkout, then merge them with lipo."""
        log_info("Building universal binary...")
        self.clone_source()

        for arch in ("x86_64", "arm64"):
            builder = MesonBuilder(
                replace(self.config, arch=arch, prefix=self.config.work_dir / f"gst-macos-{arch}")
            )
            builder.configure()
            builder.build()
            builder.install()

        self._lipo_merge()

    def _lipo_merge(self) -> None:
        """Merge architecture-specific builds into universal binary."""
        log_info("Creating universal binaries with lipo...")

        x86_prefix = self.config.work_dir / "gst-macos-x86_64"
        arm_prefix = self.config.work_dir / "gst-macos-arm64"
        uni_prefix = self.config.prefix
        assert uni_prefix is not None  # __post_init__ guarantees this

        # Copy structure from arm64 (arbitrary choice)
        if uni_prefix.exists():
            shutil.rmtree(uni_prefix)
        shutil.copytree(arm_prefix, uni_prefix)

        # Find all dylibs and merge them
        for arm_lib in arm_prefix.rglob("*.dylib"):
            rel_path = arm_lib.relative_to(arm_prefix)
            x86_lib = x86_prefix / rel_path
            uni_lib = uni_prefix / rel_path

            if x86_lib.exists():
                run_cmd(["lipo", "-create", str(x86_lib), str(arm_lib), "-output", str(uni_lib)])

    def run(self) -> None:
        """Execute the full build process."""
        log_info(
            f"Building GStreamer {self.config.version} for {self.config.platform}/{self.config.arch}"
        )

        self.check_dependencies()
        self.ensure_meson()

        if self.config.platform == "macos" and self.config.arch == "universal":
            self.build_universal()
        else:
            self.clone_source()
            self.configure()
            self.build()
            self.install()

        log_ok(f"GStreamer {self.config.version} installed to {self.config.prefix}")
