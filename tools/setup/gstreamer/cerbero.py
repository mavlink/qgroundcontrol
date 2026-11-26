"""GStreamer SDKs for Android and iOS, built with Cerbero and unpacked where QGC's configure takes them."""

from __future__ import annotations

import re
import shutil
import subprocess
from typing import TYPE_CHECKING, ClassVar, cast

from common.io import extract_tar_data
from common.logging import log_info, log_ok
from common.proc import require_tools

from .host import run_cmd

if TYPE_CHECKING:
    from pathlib import Path

    from .config import BuildConfig


class CerberoBuilder:
    """Build the GStreamer SDK with Cerbero and unpack it into the prefix.

    Cerbero's ``package`` command leaves package files, which this unpacks: the prefix is then what
    QGC's configure takes as ``GStreamer_ROOT_DIR``. For Android that is the SDK tree of one arch, or
    of a universal build one tree per arch; for iOS the directory of a ``GStreamer.xcframework``.
    """

    # Archs with a config/cross-android-<arch>.cbc at the pinned Cerbero version. The universal
    # config packages armv7, arm64, x86 and x86_64, each in a tree of its own.
    ANDROID_ARCHS: ClassVar[tuple[str, ...]] = ("arm64", "armv7", "x86", "x86_64", "universal")

    # The Cerbero configs whose frameworks make up the xcframework, keyed by (arch, simulator). The
    # device and universal simulator pair is what the official iOS package holds; Cerbero's own
    # cross-ios-universal mixes a device and a simulator arch in one library, which no xcframework
    # slice can be.
    IOS_CONFIGS: ClassVar[dict[tuple[str, bool], tuple[str, ...]]] = {
        ("universal", False): ("cross-ios-arm64", "cross-ios-sim-universal"),
        ("arm64", False): ("cross-ios-arm64",),
        ("arm64", True): ("cross-ios-sim-arm64",),
        ("x86_64", True): ("cross-ios-sim-x86-64",),
        ("universal", True): ("cross-ios-sim-universal",),
    }

    # Marks a prefix this builder filled, which a later build may replace.
    SDK_STAMP = ".qgc-gstreamer-sdk"

    # Android's development tarball, which has the headers and static libraries: the runtime and debug
    # ones carry a suffix after the version.
    ANDROID_SDK_PACKAGE = re.compile(r"gstreamer-1\.0-android-\w+-[0-9.]+\.tar(\.\w+)?")

    def __init__(self, config: BuildConfig):
        self.config = config
        # Absolute paths: Cerbero's commands run in directories of their own. BuildConfig fills in a
        # prefix that is not given.
        work_dir = config.work_dir.resolve()
        self.prefix = cast("Path", config.prefix).resolve()
        self.cerbero_dir = work_dir / "cerbero"
        self.packages_dir = work_dir / "cerbero-packages"
        self.jobs_config = work_dir / "cerbero-jobs.cbc"

    def check_dependencies(self) -> None:
        """Check for required tools."""
        require_tools("git", "python3")
        if self.config.platform == "ios":
            require_tools("xcodebuild")
            if subprocess.run(["xcrun", "--show-sdk-path"], capture_output=True).returncode != 0:
                raise RuntimeError("Missing required tools: Xcode/iOS SDK")

        log_ok("All dependencies found")

    def check_prefix(self) -> None:
        """Refuse a prefix that holds anything but an SDK of an earlier build, which unpack() replaces."""
        prefix = self.prefix
        if prefix.exists() and any(prefix.iterdir()) and not (prefix / self.SDK_STAMP).exists():
            raise RuntimeError(
                f"{prefix} is not empty and holds no SDK of an earlier build: "
                "remove it or pass another --prefix"
            )

    def clone_cerbero(self) -> None:
        """Clone Cerbero if needed."""
        if self.config.clean and self.cerbero_dir.exists():
            log_info("Cleaning previous Cerbero...")
            shutil.rmtree(self.cerbero_dir)

        if not self.cerbero_dir.exists():
            log_info(f"Cloning Cerbero for GStreamer {self.config.version}...")
            self.cerbero_dir.parent.mkdir(parents=True, exist_ok=True)
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

    def get_config_names(self) -> tuple[str, ...]:
        """The Cerbero configs to build for the target platform, arch, and simulator flag."""
        plat, arch = self.config.platform, self.config.arch
        if plat == "android" and arch in self.ANDROID_ARCHS:
            return (f"cross-android-{arch.replace('_', '-')}",)
        simulator = plat == "ios" and self.config.simulator
        if plat == "ios" and (arch, simulator) in self.IOS_CONFIGS:
            return self.IOS_CONFIGS[arch, simulator]
        raise ValueError(f"Unsupported arch {arch} for {plat}{' simulator' if simulator else ''}")

    def _cerbero(self, config_name: str, *args: str) -> list[str]:
        """A Cerbero command for *config_name* with the build's variants and job limit."""
        cmd = [str(self.cerbero_dir / "cerbero-uninstalled"), "-c", config_name]
        if self.config.jobs:
            cmd.extend(["-c", str(self.jobs_config)])
        # Cerbero builds with debug symbols either way; its optimization variant is what a debug build lacks.
        if self.config.build_type == "debug":
            cmd.extend(["-v", "nooptimization"])
        return [*cmd, *args]

    def write_jobs_config(self) -> None:
        """Limit each recipe's compile jobs with a Cerbero config, as no command-line option does."""
        if self.config.jobs:
            self.jobs_config.write_text(f"num_of_cpus = {self.config.jobs}\n", encoding="utf-8")

    def _jobs_args(self) -> list[str]:
        """How many recipes Cerbero handles at once; it compiles one at a time above four."""
        return ["--jobs", str(self.config.jobs)] if self.config.jobs else []

    def build_package(self, config_name: str) -> Path:
        """Bootstrap and build *config_name*, and return the package Cerbero made of it."""
        log_info(f"Building GStreamer with Cerbero config {config_name}...")
        run_cmd(self._cerbero(config_name, "bootstrap", *self._jobs_args()), cwd=self.cerbero_dir)

        ios = self.config.platform == "ios"
        before = set(self.packages_dir.iterdir())
        run_cmd(
            self._cerbero(
                config_name,
                "package",
                "gstreamer-1.0",
                *self._jobs_args(),
                # The slice of an xcframework; Cerbero's default for iOS is a deprecated installer package.
                *(["--artifact=xcframework"] if ios else []),
                "-o",
                str(self.packages_dir),
            ),
            cwd=self.cerbero_dir,
        )
        made = [
            path
            for path in sorted(set(self.packages_dir.iterdir()) - before)
            if (
                ".xcframework.tar" in path.name
                if ios
                else self.ANDROID_SDK_PACKAGE.fullmatch(path.name)
            )
        ]
        if len(made) != 1:
            names = ", ".join(path.name for path in made) or "none"
            raise RuntimeError(
                f"Expected one package from Cerbero config {config_name}, found: {names}"
            )
        return made[0]

    def create_xcframework(self, frameworks: list[Path]) -> Path:
        """Combine the per-config *frameworks* into one xcframework package, and return it."""
        log_info("Combining the frameworks into GStreamer.xcframework...")
        before = set(self.packages_dir.iterdir())
        sources = [arg for framework in frameworks for arg in ("--source", framework.name)]
        # Cerbero reads the version from the start of the first source: a file name, not a path.
        run_cmd(
            [
                str(self.cerbero_dir / "cerbero-uninstalled"),
                "xcframework",
                "gstreamer-1.0",
                *sources,
            ],
            cwd=self.packages_dir,
        )
        made = [
            path
            for path in sorted(set(self.packages_dir.iterdir()) - before)
            if "-xcframework.tar" in path.name
        ]
        if len(made) != 1:
            names = ", ".join(path.name for path in made) or "none"
            raise RuntimeError(f"Expected one xcframework package from Cerbero, found: {names}")
        return made[0]

    def unpack(self, package: Path) -> None:
        """Replace the prefix with the SDK in *package*."""
        prefix = self.prefix
        log_info(f"Unpacking {package.name} into {prefix}...")
        staging = prefix.with_name(f"{prefix.name}.partial")
        if staging.exists():
            shutil.rmtree(staging)
        staging.mkdir(parents=True)
        extract_tar_data(package, staging)
        self.check_sdk(staging)
        (staging / self.SDK_STAMP).write_text(f"{package.name}\n", encoding="utf-8")
        if prefix.exists():
            shutil.rmtree(prefix)
        staging.rename(prefix)

    def check_sdk(self, root: Path) -> None:
        """Fail unless *root* holds what QGC's configure looks for in GStreamer_ROOT_DIR."""
        if self.config.platform == "ios":
            found = (root / "GStreamer.xcframework" / "Info.plist").is_file()
        else:
            pc = "lib/pkgconfig/gstreamer-1.0.pc"
            found = (root / pc).is_file() or any(root.glob(f"*/{pc}"))
        if not found:
            raise RuntimeError(f"Cerbero's package holds no GStreamer SDK that QGC can use: {root}")

    def run(self) -> None:
        """Execute the full build process."""
        log_info(
            f"Building GStreamer {self.config.version} for {self.config.platform}/{self.config.arch}"
        )

        config_names = self.get_config_names()
        self.check_dependencies()
        self.check_prefix()
        self.clone_cerbero()
        # Packages of an earlier build must not be taken for this one's.
        if self.packages_dir.exists():
            shutil.rmtree(self.packages_dir)
        self.packages_dir.mkdir(parents=True)
        self.write_jobs_config()

        packages = [self.build_package(name) for name in config_names]
        sdk = self.create_xcframework(packages) if self.config.platform == "ios" else packages[0]
        self.unpack(sdk)

        log_ok(f"GStreamer {self.config.version} built for {self.config.platform}")
        log_info(f"Configure QGroundControl with -DGStreamer_ROOT_DIR={self.prefix}")
