#!/usr/bin/env python3
"""
Unified GStreamer build script for QGroundControl.

Builds GStreamer with plugins needed for QGC video streaming across all platforms:
- Linux, macOS, Windows: Meson-based builds
- Android, iOS: Cerbero-based builds

Usage:
    ./build-gstreamer.py --platform linux [OPTIONS]
    ./build-gstreamer.py --platform macos --arch universal [OPTIONS]
    ./build-gstreamer.py --platform android --arch arm64 [OPTIONS]

Options:
    --platform PLATFORM   Target platform (linux, macos, windows, android, ios)
    --arch ARCH           Target architecture (platform-specific, default: native)
    --version VERSION     GStreamer version (default: from build-config.json)
    --type TYPE           Build type: release or debug (default: release)
    --prefix DIR          Install prefix
    --work-dir DIR        Working directory for source
    --qt-prefix DIR       Qt installation path (for qml6 plugin)
    --jobs N              Parallel jobs (default: auto)
    --clean               Clean build directory before building
    --simulator           iOS simulator build (iOS only)
    -h, --help            Show this help message
"""

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional


# ============================================================================
# Shared Utilities
# ============================================================================

class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'

def log_info(msg: str) -> None:
    print(f"{Colors.BLUE}[INFO]{Colors.NC} {msg}")

def log_ok(msg: str) -> None:
    print(f"{Colors.GREEN}[OK]{Colors.NC} {msg}")

def log_warn(msg: str) -> None:
    print(f"{Colors.YELLOW}[WARN]{Colors.NC} {msg}", file=sys.stderr)

def log_error(msg: str) -> None:
    print(f"{Colors.RED}[ERROR]{Colors.NC} {msg}", file=sys.stderr)

def run_cmd(cmd: list, cwd: Optional[Path] = None, check: bool = True,
            env: Optional[dict] = None) -> subprocess.CompletedProcess:
    """Run a command with optional working directory and environment."""
    merged_env = {**os.environ, **(env or {})}
    log_info(f"Running: {' '.join(str(c) for c in cmd)}")
    return subprocess.run(cmd, cwd=cwd, check=check, env=merged_env)

def detect_jobs(override: Optional[int] = None) -> int:
    """Detect number of parallel jobs to use."""
    if override:
        return override
    try:
        return os.cpu_count() or 4
    except Exception:
        return 4

def detect_host_platform() -> str:
    """Detect the host platform."""
    system = platform.system().lower()
    if system == 'darwin':
        return 'macos'
    return system

def detect_host_arch() -> str:
    """Detect the host architecture."""
    machine = platform.machine().lower()
    if machine in ('x86_64', 'amd64'):
        return 'x86_64'
    if machine in ('aarch64', 'arm64'):
        return 'arm64'
    if machine.startswith('arm'):
        return 'armv7'
    return machine

def read_config(key: str, default: str = '') -> str:
    """Read a value from build-config.json."""
    script_dir = Path(__file__).parent
    config_file = script_dir.parent.parent.parent / '.github' / 'build-config.json'

    if not config_file.exists():
        return default

    try:
        with open(config_file) as f:
            config = json.load(f)
        return config.get(key, default)
    except Exception:
        return default

def write_github_output(outputs: dict) -> None:
    """Write outputs for GitHub Actions."""
    github_output = os.environ.get('GITHUB_OUTPUT')
    if github_output:
        with open(github_output, 'a') as f:
            for key, value in outputs.items():
                f.write(f"{key}={value}\n")


# ============================================================================
# Plugin Configurations
# ============================================================================

# Common meson options for all Meson-based platforms
MESON_COMMON = {
    'auto_features': 'disabled',
    'gst-full-libraries': 'video,gl',
    'gpl': 'enabled',
    'libav': 'enabled',
    'orc': 'enabled',
    'qt6': 'enabled',
    'base': 'enabled',
    'good': 'enabled',
    'bad': 'enabled',
    'ugly': 'enabled',
}

# Base plugins (shared across Meson platforms)
PLUGINS_BASE = {
    'gst-plugins-base:app': 'enabled',
    'gst-plugins-base:gl': 'enabled',
    'gst-plugins-base:playback': 'enabled',
    'gst-plugins-base:tcp': 'enabled',
}

# Good plugins (shared across Meson platforms)
PLUGINS_GOOD = {
    'gst-plugins-good:isomp4': 'enabled',
    'gst-plugins-good:matroska': 'enabled',
    'gst-plugins-good:qt-method': 'auto',
    'gst-plugins-good:qt6': 'enabled',
    'gst-plugins-good:rtp': 'enabled',
    'gst-plugins-good:rtpmanager': 'enabled',
    'gst-plugins-good:rtsp': 'enabled',
    'gst-plugins-good:udp': 'enabled',
}

# Bad plugins (shared across Meson platforms)
PLUGINS_BAD = {
    'gst-plugins-bad:gl': 'enabled',
    'gst-plugins-bad:mpegtsdemux': 'enabled',
    'gst-plugins-bad:rtp': 'enabled',
    'gst-plugins-bad:sdp': 'enabled',
    'gst-plugins-bad:videoparsers': 'enabled',
}

# Ugly plugins (shared across Meson platforms)
PLUGINS_UGLY = {
    'gst-plugins-ugly:x264': 'enabled',
}

# Platform-specific GL configurations
GL_CONFIG = {
    'linux': {
        'gst-plugins-base:gl_api': 'opengl,gles2',
        'gst-plugins-base:gl_platform': 'glx,egl',
        'gst-plugins-base:gl_winsys': 'x11,egl,wayland',
        'gst-plugins-base:x11': 'enabled',
        'gst-plugins-good:qt-egl': 'enabled',
        'gst-plugins-good:qt-wayland': 'enabled',
        'gst-plugins-good:qt-x11': 'enabled',
        'gst-plugins-bad:va': 'enabled',
        'gst-plugins-bad:wayland': 'enabled',
        'gst-plugins-bad:x11': 'enabled',
        'gst-plugins-bad:x265': 'enabled',
        'vaapi': 'enabled',
    },
    'macos': {
        'gst-plugins-base:gl_api': 'opengl,gles2',
        'gst-plugins-base:gl_platform': 'cgl',
        'gst-plugins-base:gl_winsys': 'cocoa',
        'gst-plugins-bad:applemedia': 'enabled',
    },
    'windows': {
        'gst-plugins-base:gl_api': 'opengl',
        'gst-plugins-base:gl_platform': 'wgl,egl',
        'gst-plugins-base:gl_winsys': 'win32,egl',
        'gst-plugins-bad:d3d11': 'enabled',
        'gst-plugins-bad:d3d12': 'enabled',
        'gst-plugins-bad:mediafoundation': 'enabled',
    },
}


# ============================================================================
# Build Configuration
# ============================================================================

@dataclass
class BuildConfig:
    platform: str
    arch: str
    version: str
    build_type: str = 'release'
    prefix: Optional[Path] = None
    work_dir: Path = field(default_factory=lambda: Path('/tmp'))
    qt_prefix: Optional[Path] = None
    jobs: Optional[int] = None
    clean: bool = False
    simulator: bool = False  # iOS only

    def __post_init__(self):
        if not self.prefix:
            self.prefix = self.work_dir / f'gst-{self.platform}-{self.arch}'
        self.source_dir = self.work_dir / 'gstreamer'
        self.build_dir = self.source_dir / 'builddir'

    @property
    def archive_name(self) -> str:
        name = f'gstreamer-1.0-{self.platform}-{self.arch}-{self.version}'
        if self.simulator:
            name += '-simulator'
        return name


# ============================================================================
# Meson Builder (Linux, macOS, Windows)
# ============================================================================

class MesonBuilder:
    """Build GStreamer using Meson for desktop platforms."""

    def __init__(self, config: BuildConfig):
        self.config = config

    def check_dependencies(self) -> None:
        """Check for required build tools."""
        required = ['git', 'python3', 'pkg-config']
        missing = [cmd for cmd in required if not shutil.which(cmd)]

        if self.config.platform == 'macos':
            if subprocess.run(['xcode-select', '-p'], capture_output=True).returncode != 0:
                missing.append('xcode-select')

        if missing:
            log_error(f"Missing required tools: {', '.join(missing)}")
            sys.exit(1)

        log_ok("All dependencies found")

    def ensure_meson(self) -> None:
        """Ensure meson and ninja are available."""
        if not shutil.which('meson'):
            log_info("Installing meson and ninja...")
            run_cmd([sys.executable, '-m', 'pip', 'install', '--user', '--quiet', 'ninja', 'meson'])
            # Add user bin to PATH
            user_bin = Path.home() / '.local' / 'bin'
            os.environ['PATH'] = f"{user_bin}:{os.environ['PATH']}"
        else:
            log_info(f"Using existing meson: {shutil.which('meson')}")

    def clone_source(self) -> None:
        """Clone GStreamer source if needed."""
        if self.config.clean and self.config.source_dir.exists():
            log_info("Cleaning previous build...")
            shutil.rmtree(self.config.source_dir)

        if not self.config.source_dir.exists():
            log_info(f"Cloning GStreamer {self.config.version}...")
            self.config.work_dir.mkdir(parents=True, exist_ok=True)
            run_cmd([
                'git', 'clone', '--depth', '1', '--branch', self.config.version,
                'https://github.com/GStreamer/gstreamer.git',
                str(self.config.source_dir)
            ])
        else:
            log_info(f"Using existing source at {self.config.source_dir}")

    def get_meson_args(self) -> list:
        """Build meson configuration arguments."""
        args = [
            f'--prefix={self.config.prefix}',
            f'--buildtype={self.config.build_type}',
            '--wrap-mode=forcefallback',
            '--strip',
        ]

        # Add common options
        for key, value in MESON_COMMON.items():
            args.append(f'-D{key}={value}')

        # Add base plugins
        for key, value in PLUGINS_BASE.items():
            args.append(f'-D{key}={value}')

        # Add good plugins
        for key, value in PLUGINS_GOOD.items():
            args.append(f'-D{key}={value}')

        # Add bad plugins
        for key, value in PLUGINS_BAD.items():
            args.append(f'-D{key}={value}')

        # Add ugly plugins
        for key, value in PLUGINS_UGLY.items():
            args.append(f'-D{key}={value}')

        # Add platform-specific GL config
        gl_config = GL_CONFIG.get(self.config.platform, {})
        for key, value in gl_config.items():
            args.append(f'-D{key}={value}')

        # Handle macOS universal builds
        if self.config.platform == 'macos' and self.config.arch != 'universal':
            args.append(f'-Dcpp_args=-arch {self.config.arch}')
            args.append(f'-Dc_args=-arch {self.config.arch}')

        return args

    def configure(self) -> None:
        """Configure the build with meson."""
        if self.config.build_dir.exists() and not self.config.clean:
            if (self.config.build_dir / 'build.ninja').exists():
                log_info("Using existing configuration")
                return

        log_info("Configuring GStreamer...")
        if self.config.build_dir.exists():
            shutil.rmtree(self.config.build_dir)

        args = ['meson', 'setup', str(self.config.build_dir)] + self.get_meson_args()

        env = {}
        if self.config.qt_prefix:
            env['PKG_CONFIG_PATH'] = f"{self.config.qt_prefix}/lib/pkgconfig"

        run_cmd(args, cwd=self.config.source_dir, env=env)

    def build(self) -> None:
        """Compile GStreamer."""
        jobs = detect_jobs(self.config.jobs)
        log_info(f"Compiling GStreamer with {jobs} jobs...")
        run_cmd(['meson', 'compile', '-C', str(self.config.build_dir), '-j', str(jobs)])

    def install(self) -> None:
        """Install GStreamer."""
        log_info("Installing GStreamer...")
        run_cmd(['meson', 'install', '-C', str(self.config.build_dir)])

    def build_universal(self) -> None:
        """Build universal binary for macOS."""
        if self.config.platform != 'macos' or self.config.arch != 'universal':
            return

        log_info("Building universal binary...")

        # Build for each architecture
        for arch in ['x86_64', 'arm64']:
            arch_config = BuildConfig(
                platform='macos',
                arch=arch,
                version=self.config.version,
                build_type=self.config.build_type,
                prefix=self.config.work_dir / f'gst-macos-{arch}',
                work_dir=self.config.work_dir,
                qt_prefix=self.config.qt_prefix,
                jobs=self.config.jobs,
                clean=self.config.clean,
            )
            builder = MesonBuilder(arch_config)
            builder.clone_source()
            builder.configure()
            builder.build()
            builder.install()

        # Merge with lipo
        self._lipo_merge()

    def _lipo_merge(self) -> None:
        """Merge architecture-specific builds into universal binary."""
        log_info("Creating universal binaries with lipo...")

        x86_prefix = self.config.work_dir / 'gst-macos-x86_64'
        arm_prefix = self.config.work_dir / 'gst-macos-arm64'
        uni_prefix = self.config.prefix

        uni_prefix.mkdir(parents=True, exist_ok=True)

        # Copy structure from arm64 (arbitrary choice)
        if uni_prefix.exists():
            shutil.rmtree(uni_prefix)
        shutil.copytree(arm_prefix, uni_prefix)

        # Find all dylibs and merge them
        for arm_lib in arm_prefix.rglob('*.dylib'):
            rel_path = arm_lib.relative_to(arm_prefix)
            x86_lib = x86_prefix / rel_path
            uni_lib = uni_prefix / rel_path

            if x86_lib.exists():
                run_cmd(['lipo', '-create', str(x86_lib), str(arm_lib), '-output', str(uni_lib)])

    def run(self) -> None:
        """Execute the full build process."""
        log_info(f"Building GStreamer {self.config.version} for {self.config.platform}/{self.config.arch}")

        self.check_dependencies()
        self.ensure_meson()

        if self.config.platform == 'macos' and self.config.arch == 'universal':
            self.build_universal()
        else:
            self.clone_source()
            self.configure()
            self.build()
            self.install()

        log_ok(f"GStreamer {self.config.version} installed to {self.config.prefix}")

        # Output for CI
        write_github_output({
            'gstreamer_prefix': str(self.config.prefix),
            'gstreamer_version': self.config.version,
            'gstreamer_arch': self.config.arch,
            'archive_name': self.config.archive_name,
        })


# ============================================================================
# Cerbero Builder (Android, iOS)
# ============================================================================

class CerberoBuilder:
    """Build GStreamer using Cerbero for mobile platforms."""

    CERBERO_CONFIGS = {
        'android': {
            'arm64': 'cross-android-arm64',
            'armv7': 'cross-android-armv7',
            'x86': 'cross-android-x86',
            'x86_64': 'cross-android-x86-64',
        },
        'ios': {
            'arm64': 'cross-ios-arm64',
            'x86_64': 'cross-ios-x86-64',
        },
    }

    def __init__(self, config: BuildConfig):
        self.config = config
        self.cerbero_dir = config.work_dir / 'cerbero'

    def check_dependencies(self) -> None:
        """Check for required tools."""
        required = ['git', 'python3']
        missing = [cmd for cmd in required if not shutil.which(cmd)]

        if self.config.platform == 'ios':
            if subprocess.run(['xcrun', '--show-sdk-path'], capture_output=True).returncode != 0:
                missing.append('Xcode/iOS SDK')

        if missing:
            log_error(f"Missing required tools: {', '.join(missing)}")
            sys.exit(1)

        # Check Python version
        if sys.version_info < (3, 8):
            log_error(f"Python 3.8+ required, found {sys.version_info.major}.{sys.version_info.minor}")
            sys.exit(1)

        log_ok("All dependencies found")

    def clone_cerbero(self) -> None:
        """Clone Cerbero if needed."""
        if self.config.clean and self.cerbero_dir.exists():
            log_info("Cleaning previous Cerbero...")
            shutil.rmtree(self.cerbero_dir)

        if not self.cerbero_dir.exists():
            log_info(f"Cloning Cerbero for GStreamer {self.config.version}...")
            self.config.work_dir.mkdir(parents=True, exist_ok=True)
            run_cmd([
                'git', 'clone', '--depth', '1', '--branch', self.config.version,
                'https://github.com/GStreamer/cerbero.git',
                str(self.cerbero_dir)
            ])
        else:
            log_info(f"Using existing Cerbero at {self.cerbero_dir}")

    def bootstrap(self) -> None:
        """Bootstrap Cerbero."""
        log_info("Bootstrapping Cerbero...")
        cerbero = self.cerbero_dir / 'cerbero-uninstalled'
        run_cmd([str(cerbero), 'bootstrap'], cwd=self.cerbero_dir)

    def get_config_name(self, arch: str) -> str:
        """Get Cerbero config name for platform/arch."""
        configs = self.CERBERO_CONFIGS.get(self.config.platform, {})
        config_name = configs.get(arch)

        if not config_name:
            log_error(f"Unsupported arch {arch} for {self.config.platform}")
            sys.exit(1)

        if self.config.platform == 'ios' and self.config.simulator:
            config_name += '-simulator'

        return config_name

    def build_arch(self, arch: str) -> None:
        """Build for a specific architecture."""
        config_name = self.get_config_name(arch)
        cerbero = self.cerbero_dir / 'cerbero-uninstalled'

        log_info(f"Building GStreamer for {self.config.platform}/{arch}...")

        cmd = [
            str(cerbero), '-c', config_name,
            'package', 'gstreamer-1.0',
            '-o', str(self.config.prefix),
        ]

        if self.config.build_type == 'debug':
            cmd.extend(['--variant', 'debug'])

        run_cmd(cmd, cwd=self.cerbero_dir)

    def build_universal(self, archs: list) -> None:
        """Build for multiple architectures."""
        for arch in archs:
            self.build_arch(arch)

    def run(self) -> None:
        """Execute the full build process."""
        log_info(f"Building GStreamer {self.config.version} for {self.config.platform}/{self.config.arch}")

        self.check_dependencies()
        self.clone_cerbero()
        self.bootstrap()

        if self.config.arch == 'universal':
            if self.config.platform == 'android':
                archs = ['arm64', 'armv7']
            else:  # iOS
                archs = ['arm64']
                if self.config.simulator:
                    archs = ['x86_64', 'arm64']
            self.build_universal(archs)
        else:
            self.build_arch(self.config.arch)

        log_ok(f"GStreamer {self.config.version} built for {self.config.platform}")

        # Output for CI
        write_github_output({
            'gstreamer_prefix': str(self.config.prefix),
            'gstreamer_version': self.config.version,
            'gstreamer_arch': self.config.arch,
            'archive_name': self.config.archive_name,
        })


# ============================================================================
# Main Entry Point
# ============================================================================

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Build GStreamer for QGroundControl',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )

    parser.add_argument('--platform', '-p', required=True,
                        choices=['linux', 'macos', 'windows', 'android', 'ios'],
                        help='Target platform')
    parser.add_argument('--arch', '-a', default='',
                        help='Target architecture (default: native or universal)')
    parser.add_argument('--version', '-v', default='',
                        help='GStreamer version (default: from build-config.json)')
    parser.add_argument('--type', '-t', dest='build_type', default='release',
                        choices=['release', 'debug'],
                        help='Build type (default: release)')
    parser.add_argument('--prefix', default='',
                        help='Install prefix')
    parser.add_argument('--work-dir', '-w', default='/tmp',
                        help='Working directory (default: /tmp)')
    parser.add_argument('--qt-prefix', '-q', default='',
                        help='Qt installation path')
    parser.add_argument('--jobs', '-j', type=int, default=0,
                        help='Parallel jobs (default: auto)')
    parser.add_argument('--clean', '-c', action='store_true',
                        help='Clean build directory')
    parser.add_argument('--simulator', action='store_true',
                        help='iOS simulator build')

    return parser.parse_args()


def get_default_arch(plat: str) -> str:
    """Get default architecture for platform."""
    if plat in ('macos', 'android', 'ios'):
        return 'universal'
    return detect_host_arch()


def main() -> int:
    args = parse_args()

    # Resolve defaults
    version = args.version or read_config('gstreamer_version', '1.24.10')
    arch = args.arch or get_default_arch(args.platform)
    prefix = Path(args.prefix) if args.prefix else None
    work_dir = Path(args.work_dir)
    qt_prefix = Path(args.qt_prefix) if args.qt_prefix else None
    jobs = args.jobs if args.jobs > 0 else None

    config = BuildConfig(
        platform=args.platform,
        arch=arch,
        version=version,
        build_type=args.build_type,
        prefix=prefix,
        work_dir=work_dir,
        qt_prefix=qt_prefix,
        jobs=jobs,
        clean=args.clean,
        simulator=args.simulator,
    )

    # Select builder
    if args.platform in ('linux', 'macos', 'windows'):
        builder = MesonBuilder(config)
    else:
        builder = CerberoBuilder(config)

    try:
        builder.run()
        return 0
    except subprocess.CalledProcessError as e:
        log_error(f"Build failed: {e}")
        return 1
    except KeyboardInterrupt:
        log_warn("Build cancelled")
        return 130


if __name__ == '__main__':
    sys.exit(main())
