"""GStreamer builds with Meson for Linux, macOS and Windows."""

from __future__ import annotations

import os
import shutil
import subprocess
from dataclasses import replace
from typing import TYPE_CHECKING

from common.build_config import load_build_config
from common.logging import log_info, log_ok
from common.proc import require_tools
from qgc_tools.python_env import executable, sync_groups

from .config import (
    FFMPEG_FEATURES,
    GL_CONFIG,
    MESON_COMMON,
    PLUGINS_BAD_SUPPORT,
    WRAP_ARGS,
    BuildConfig,
    plugin_options,
)
from .host import detect_jobs, run_cmd

if TYPE_CHECKING:
    from pathlib import Path

MACH_O_64 = bytes.fromhex("cffaedfe")
STATIC_ARCHIVE = b"!<arch>\n"


def _lipo_merges(path: Path) -> bool:
    """Whether *path* is a 64-bit Mach-O file (a library, plugin or program) or a static archive."""
    with path.open("rb") as file:
        head = file.read(len(STATIC_ARCHIVE))
    return head.startswith(MACH_O_64) or head == STATIC_ARCHIVE


class MesonBuilder:
    """Build GStreamer using Meson for desktop platforms."""

    def __init__(self, config: BuildConfig):
        self.config = config

    def check_dependencies(self) -> None:
        """Check for required build tools."""
        if self.config.platform == "windows":
            # GStreamer's build downloads a pkg-config there, and Windows installs Python as python.exe.
            require_tools("git")
        else:
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
            *WRAP_ARGS[self.config.platform],
            "--strip",
        ]

        # Add common options
        for key, value in MESON_COMMON.items():
            args.append(f"-D{key}={value}")

        # The allowlisted plugins (gstreamer.plugins in build-config.json), those a Debug QGC and its tests make
        # streams with, and the support they need
        build_config = load_build_config()
        allowlist = build_config.get("gstreamer", {}).get("plugins", {})
        debug_plugins = build_config.get("gstreamer_plugin_policy", {}).get("mock_server_debug", [])
        for key, value in {
            **plugin_options(self.config.platform, allowlist, debug_plugins),
            **PLUGINS_BAD_SUPPORT,
        }.items():
            args.append(f"-D{key}={value}")

        for feature in FFMPEG_FEATURES:
            args.append(f"-DFFmpeg:{feature}=enabled")
        # FFmpeg's assembly for x86, which most of its decoders' speed comes from; it needs nasm.
        if self.config.arch in ("x86_64", "x64"):
            args.append("-DFFmpeg:x86asm=enabled")

        # Add platform-specific GL config
        gl_config = GL_CONFIG.get(self.config.platform, {})
        for key, value in gl_config.items():
            args.append(f"-D{key}={value}")

        # A run path, so that the prefix's libraries, plugins and programs load each other. Without one, a program
        # built against the prefix takes the libraries it does not link itself, such as libgstaudio, from the system's
        # GStreamer. The library directory is named, where Debian's default would add the multiarch one.
        if self.config.platform == "linux":
            assert self.config.prefix is not None  # __post_init__ guarantees this
            args.append("--libdir=lib")
            for lang in ("c", "cpp"):
                args.append(f"-D{lang}_link_args=['-Wl,-rpath,{self.config.prefix / 'lib'}']")

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
        run_cmd(args, cwd=self.config.source_dir)

    def build(self) -> None:
        """Compile GStreamer."""
        jobs = detect_jobs(self.config.jobs)
        log_info(f"Compiling GStreamer with {jobs} jobs...")
        run_cmd(["meson", "compile", "-C", str(self.config.build_dir), "-j", str(jobs)])

    def install(self, destdir: Path | None = None) -> None:
        """Install GStreamer into the prefix, or into the prefix's path below *destdir*."""
        log_info("Installing GStreamer...")
        args = ["meson", "install", "-C", str(self.config.build_dir)]
        if destdir:
            args += ["--destdir", str(destdir)]
        run_cmd(args)

    def install_pkg_config(self) -> None:
        """Windows: add the pkg-config.exe GStreamer's build downloaded, which QGC's configure runs from the prefix."""
        prefix = self.config.prefix
        assert prefix is not None  # __post_init__ guarantees this
        pkg_config = self.config.source_dir / "subprojects" / "win-pkgconfig" / "pkg-config.exe"
        if not pkg_config.is_file():
            raise RuntimeError(f"GStreamer's build left no pkg-config.exe at {pkg_config}")
        (prefix / "bin").mkdir(parents=True, exist_ok=True)
        shutil.copy2(pkg_config, prefix / "bin" / "pkg-config.exe")

    def build_universal(self) -> None:
        """Build each macOS arch for the prefix from one source checkout, then merge them into it with lipo.

        Both are configured with the prefix, so that the library install names and pkg-config files of the merged
        tree name it, and installed into a staging directory each.
        """
        log_info("Building universal binary...")
        self.clone_source()

        prefix = self.config.prefix
        assert prefix is not None  # __post_init__ guarantees this
        slices = {}
        for arch in ("x86_64", "arm64"):
            stage = self.config.work_dir / f"gst-macos-universal-{arch}"
            if stage.exists():
                shutil.rmtree(stage)
            builder = MesonBuilder(replace(self.config, arch=arch))
            builder.configure()
            builder.build()
            builder.install(destdir=stage)
            slices[arch] = stage / prefix.relative_to(prefix.anchor)
            if not slices[arch].is_dir():
                raise RuntimeError(
                    f"The {arch} build installed nothing for {prefix}: it was configured for another prefix. "
                    "Build again with --clean."
                )

        self._lipo_merge(slices["x86_64"], slices["arm64"])

    def _lipo_merge(self, x86_root: Path, arm_root: Path) -> None:
        """Install the arm64 tree into the prefix, each of its binaries merged with the x86_64 tree's."""
        log_info("Creating universal binaries with lipo...")
        prefix = self.config.prefix
        assert prefix is not None  # __post_init__ guarantees this
        prefix.mkdir(parents=True, exist_ok=True)

        # Sorted, a directory comes before what it holds.
        for source in sorted(arm_root.rglob("*")):
            relative = source.relative_to(arm_root)
            target = prefix / relative
            if source.is_dir() and not source.is_symlink():
                target.mkdir(exist_ok=True)
                continue
            target.unlink(missing_ok=True)
            if source.is_symlink():
                target.symlink_to(os.readlink(source))
            elif _lipo_merges(source) and (x86_root / relative).is_file():
                run_cmd(
                    [
                        "lipo",
                        "-create",
                        str(x86_root / relative),
                        str(source),
                        "-output",
                        str(target),
                    ]
                )
            else:
                shutil.copy2(source, target)

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
            if self.config.platform == "windows":
                self.install_pkg_config()

        log_ok(f"GStreamer {self.config.version} installed to {self.config.prefix}")
