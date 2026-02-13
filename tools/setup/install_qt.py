#!/usr/bin/env python3
"""
Install Qt via aqtinstall - cross-platform unified script.

This script replaces the platform-specific install-qt-debian.sh, install-qt-macos.sh,
install-qt-windows.ps1, install-qt-android.sh, and install-qt-ios.sh scripts
with a single Python implementation.

Usage:
    install_qt.py                           # Auto-detect platform, use defaults from config
    install_qt.py --version 6.10.1          # Specific Qt version
    install_qt.py --target android --android-abis "arm64-v8a armeabi-v7a"  # Android build
    install_qt.py --target ios              # iOS build (macOS only)
    install_qt.py --tools "tools_ifw"       # Install additional Qt tools
    install_qt.py --export bash             # Output env vars as bash exports
    install_qt.py --export powershell       # Output env vars as PowerShell

Environment variables (optional, defaults from build-config.json):
    QT_VERSION, QT_PATH, QT_HOST, QT_TARGET, QT_ARCH, QT_MODULES, QT_TOOLS
    ANDROID_SDK_ROOT, ANDROID_ABIS, JAVA_VERSION, NDK_VERSION, NDK_FULL_VERSION
    ANDROID_PLATFORM, ANDROID_BUILD_TOOLS, ANDROID_CMDLINE_TOOLS
"""

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import urllib.request
from pathlib import Path
from typing import Any, Optional


def find_config_file() -> Optional[Path]:
    """Find build-config.json by searching up from script location."""
    script_dir = Path(__file__).parent.resolve()

    # Check script directory first (for Docker builds)
    local_config = script_dir / "build-config.json"
    if local_config.exists():
        return local_config

    # Search up for .github/build-config.json
    current = script_dir
    while current != current.parent:
        config_path = current / ".github" / "build-config.json"
        if config_path.exists():
            return config_path
        current = current.parent

    return None


def load_config() -> dict[str, Any]:
    """Load configuration from build-config.json."""
    config_file = find_config_file()
    if config_file:
        with open(config_file) as f:
            return json.load(f)
    return {}


def detect_host() -> str:
    """Detect host platform for aqtinstall."""
    system = platform.system().lower()
    if system == "darwin":
        return "mac"
    elif system == "windows":
        return "windows"
    else:
        return "linux"


def get_default_arch(host: str, target: str) -> str:
    """Get default architecture for host/target combination."""
    if target == "desktop":
        if host == "linux":
            if platform.machine() == "aarch64":
                return "linux_gcc_arm64"
            return "linux_gcc_64"
        elif host == "mac":
            return "clang_64"
        elif host == "windows":
            return "win64_msvc2022_64"
    elif target == "android":
        return "android_arm64_v8a"
    elif target == "ios":
        return "ios"
    return "linux_gcc_64"


def get_arch_dir(arch: str) -> str:
    """Map aqtinstall arch to Qt directory name."""
    mapping = {
        "linux_gcc_64": "gcc_64",
        "linux_gcc_arm64": "gcc_arm64",
        "clang_64": "macos",
        "win64_msvc2022_64": "msvc2022_64",
        "win64_msvc2022_arm64": "msvc2022_arm64",
        "android_arm64_v8a": "android_arm64_v8a",
        "android_armv7": "android_armv7",
        "android_x86_64": "android_x86_64",
        "android_x86": "android_x86",
        "ios": "ios",
    }
    return mapping.get(arch, arch)


def get_host_arch_dir(host: str) -> str:
    """Get host architecture directory for cross-compilation."""
    if host == "linux":
        if platform.machine() == "aarch64":
            return "gcc_arm64"
        return "gcc_64"
    elif host == "mac":
        return "macos"
    elif host == "windows":
        return "msvc2022_64"
    return "gcc_64"


def abi_to_qt_arch(abi: str) -> str:
    """Convert Android ABI to aqtinstall architecture name."""
    mapping = {
        "arm64-v8a": "android_arm64_v8a",
        "armeabi-v7a": "android_armv7",
        "x86_64": "android_x86_64",
        "x86": "android_x86",
    }
    return mapping.get(abi, abi)


def find_aqt() -> Optional[str]:
    """Find aqtinstall executable."""
    # Check if aqt is directly available
    aqt = shutil.which("aqt")
    if aqt:
        return aqt

    # Check if we can run via python -m aqt
    try:
        subprocess.run(
            [sys.executable, "-m", "aqt", "--version"],
            capture_output=True,
            check=True,
        )
        return f"{sys.executable} -m aqt"
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass

    return None


def install_aqt(host: str) -> bool:
    """Install aqtinstall using pip."""
    print("Installing aqtinstall...")
    try:
        subprocess.run(
            [sys.executable, "-m", "pip", "install", "--upgrade", "aqtinstall"],
            check=True,
        )
        return True
    except subprocess.CalledProcessError:
        return False


def run_aqt(
    args: list[str], config_file: Optional[Path] = None, dry_run: bool = False
) -> bool:
    """Run aqtinstall with the given arguments."""
    aqt = find_aqt()
    if not aqt:
        print("Error: aqtinstall not found. Install with: pip install aqtinstall")
        return False

    cmd = aqt.split() + args
    if config_file and config_file.exists():
        cmd = aqt.split() + ["-c", str(config_file)] + args

    print(f"Running: {' '.join(cmd)}")
    if dry_run:
        return True

    try:
        subprocess.run(cmd, check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error: aqtinstall failed with exit code {e.returncode}")
        return False


def format_env_vars(env: dict[str, str], shell: str) -> str:
    """Format environment variables for the given shell."""
    lines = []
    for key, value in sorted(env.items()):
        if shell in ("bash", "sh", "zsh"):
            lines.append(f'export {key}="{value}"')
        elif shell == "powershell":
            lines.append(f'$env:{key} = "{value}"')
        elif shell == "cmd":
            lines.append(f"set {key}={value}")
        elif shell == "github":
            lines.append(f"{key}={value}")
    return "\n".join(lines)


# =============================================================================
# Android-specific functions
# =============================================================================


def check_java_version(required_version: str) -> bool:
    """Check if required Java version is installed."""
    java = shutil.which("java")
    if not java:
        return False

    try:
        result = subprocess.run(
            ["java", "-version"], capture_output=True, text=True, check=True
        )
        # Java version is on stderr
        output = result.stderr
        # Look for version pattern like '"17.0.1"' or '"17"'
        import re

        match = re.search(r'"(\d+)', output)
        if match:
            current = match.group(1)
            return current == required_version
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    return False


def install_java_linux(version: str, dry_run: bool = False) -> bool:
    """Install Java JDK on Linux."""
    if check_java_version(version):
        print(f"Java {version} already installed")
        return True

    print(f"Installing Java {version}...")

    if dry_run:
        print("  (dry-run) Would install Java")
        return True

    if shutil.which("apt-get"):
        subprocess.run(["sudo", "apt-get", "update"], check=True)
        subprocess.run(
            ["sudo", "apt-get", "install", "-y", f"openjdk-{version}-jdk"], check=True
        )
        return True
    elif shutil.which("dnf"):
        subprocess.run(
            ["sudo", "dnf", "install", "-y", f"java-{version}-openjdk-devel"],
            check=True,
        )
        return True
    else:
        print("Error: No supported package manager (apt-get or dnf)")
        return False


def install_java_macos(version: str, dry_run: bool = False) -> bool:
    """Install Java JDK on macOS via Homebrew."""
    if check_java_version(version):
        print(f"Java {version} already installed")
        return True

    print(f"Installing Java {version} via Homebrew...")

    if dry_run:
        print("  (dry-run) Would install Java via brew")
        return True

    if not shutil.which("brew"):
        print("Error: Homebrew not installed")
        return False

    subprocess.run(["brew", "install", f"openjdk@{version}"], check=True)
    # Create symlink for system Java
    brew_prefix = subprocess.run(
        ["brew", "--prefix"], capture_output=True, text=True, check=True
    ).stdout.strip()
    jdk_path = f"{brew_prefix}/opt/openjdk@{version}/libexec/openjdk.jdk"
    target = f"/Library/Java/JavaVirtualMachines/openjdk-{version}.jdk"
    subprocess.run(["sudo", "ln", "-sfn", jdk_path, target], check=True)
    return True


def get_java_home() -> str:
    """Get JAVA_HOME path."""
    if "JAVA_HOME" in os.environ:
        return os.environ["JAVA_HOME"]

    java = shutil.which("java")
    if java:
        # Resolve symlinks and get parent of bin
        real_java = os.path.realpath(java)
        return str(Path(real_java).parent.parent)

    return ""


def install_android_sdk(
    sdk_root: str,
    platform_version: str,
    build_tools: str,
    cmdline_tools_version: str,
    host: str,
    dry_run: bool = False,
) -> bool:
    """Install Android SDK and command-line tools."""
    sdk_path = Path(sdk_root)
    cmdline_path = sdk_path / "cmdline-tools" / "latest"

    # Download command-line tools if needed
    if not cmdline_path.exists():
        print("Downloading Android command-line tools...")

        if host == "mac":
            tools_os = "mac"
        elif host == "windows":
            tools_os = "win"
        else:
            tools_os = "linux"

        url = f"https://dl.google.com/android/repository/commandlinetools-{tools_os}-{cmdline_tools_version}_latest.zip"

        if dry_run:
            print(f"  (dry-run) Would download: {url}")
        else:
            sdk_path.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(suffix=".zip", delete=False) as tmp:
                print(f"  Downloading from {url}")
                urllib.request.urlretrieve(url, tmp.name)

                # Extract
                import zipfile

                with zipfile.ZipFile(tmp.name, "r") as zf:
                    temp_extract = sdk_path / "cmdline-tools-temp"
                    zf.extractall(temp_extract)

                # Move to correct location
                (sdk_path / "cmdline-tools").mkdir(exist_ok=True)
                (temp_extract / "cmdline-tools").rename(cmdline_path)
                temp_extract.rmdir()
                os.unlink(tmp.name)
    else:
        print("Android command-line tools already installed")

    # Update PATH for sdkmanager
    sdkmanager = cmdline_path / "bin" / "sdkmanager"
    if platform.system() == "Windows":
        sdkmanager = cmdline_path / "bin" / "sdkmanager.bat"

    if not sdkmanager.exists() and not dry_run:
        print(f"Error: sdkmanager not found at {sdkmanager}")
        return False

    # Accept licenses
    print("Accepting Android SDK licenses...")
    if not dry_run:
        try:
            subprocess.run(
                [str(sdkmanager), "--licenses"],
                input=b"y\n" * 20,
                capture_output=True,
                check=False,
            )
        except Exception:
            pass  # License acceptance can fail if already accepted

    # Install SDK components
    print("Installing Android SDK components...")
    components = [
        "platform-tools",
        f"platforms;android-{platform_version}",
        f"build-tools;{build_tools}",
    ]

    if dry_run:
        print(f"  (dry-run) Would install: {', '.join(components)}")
    else:
        subprocess.run(
            [str(sdkmanager), "--install"] + components,
            check=True,
        )

    return True


def install_android_ndk(
    sdk_root: str, ndk_full_version: str, dry_run: bool = False
) -> str:
    """Install Android NDK and return its path."""
    ndk_path = Path(sdk_root) / "ndk" / ndk_full_version

    if ndk_path.exists():
        print(f"NDK already installed at {ndk_path}")
        return str(ndk_path)

    print(f"Installing Android NDK {ndk_full_version}...")
    sdkmanager = Path(sdk_root) / "cmdline-tools" / "latest" / "bin" / "sdkmanager"
    if platform.system() == "Windows":
        sdkmanager = (
            Path(sdk_root) / "cmdline-tools" / "latest" / "bin" / "sdkmanager.bat"
        )

    if dry_run:
        print(f"  (dry-run) Would install ndk;{ndk_full_version}")
    else:
        subprocess.run(
            [str(sdkmanager), "--install", f"ndk;{ndk_full_version}"],
            check=True,
        )

    return str(ndk_path)


# =============================================================================
# iOS-specific functions
# =============================================================================


def check_xcode() -> bool:
    """Check if Xcode command-line tools are installed (macOS only)."""
    if platform.system() != "Darwin":
        return False

    try:
        result = subprocess.run(
            ["xcode-select", "-p"], capture_output=True, text=True, check=True
        )
        print(f"Xcode path: {result.stdout.strip()}")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def get_ios_modules(modules: str) -> str:
    """Filter out modules not available on iOS."""
    excluded = {"qtserialport", "qtscxml"}
    module_list = modules.split()
    filtered = [m for m in module_list if m not in excluded]
    return " ".join(filtered)


# =============================================================================
# Main
# =============================================================================


def main() -> int:
    config = load_config()
    host = detect_host()

    parser = argparse.ArgumentParser(
        description="Install Qt via aqtinstall",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "--version",
        default=os.environ.get("QT_VERSION", config.get("qt_version", "")),
        help="Qt version to install (default: from config)",
    )
    parser.add_argument(
        "--path",
        default=os.environ.get(
            "QT_PATH", "/opt/Qt" if host != "windows" else "C:\\Qt"
        ),
        help="Installation directory (default: /opt/Qt or C:\\Qt)",
    )
    parser.add_argument(
        "--host",
        default=os.environ.get("QT_HOST", host),
        choices=["linux", "mac", "windows"],
        help="Host platform (default: auto-detect)",
    )
    parser.add_argument(
        "--target",
        default=os.environ.get("QT_TARGET", "desktop"),
        choices=["desktop", "android", "ios"],
        help="Target platform (default: desktop)",
    )
    parser.add_argument(
        "--arch",
        default=os.environ.get("QT_ARCH"),
        help="Architecture (default: auto-detect based on host/target)",
    )
    parser.add_argument(
        "--modules",
        default=os.environ.get("QT_MODULES", config.get("qt_modules", "")),
        help="Space-separated Qt modules to install",
    )
    parser.add_argument(
        "--tools",
        default=os.environ.get("QT_TOOLS", ""),
        help="Space-separated Qt tools to install (e.g., tools_ifw)",
    )
    parser.add_argument(
        "--export",
        choices=["bash", "sh", "zsh", "powershell", "cmd", "github"],
        help="Output environment variables for the given shell",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print commands without executing",
    )
    parser.add_argument(
        "--install-aqt",
        action="store_true",
        help="Install aqtinstall if not found",
    )

    # Android-specific options
    android_group = parser.add_argument_group("Android options")
    android_group.add_argument(
        "--android-abis",
        default=os.environ.get("ANDROID_ABIS", "arm64-v8a armeabi-v7a"),
        help="Space-separated Android ABIs (default: arm64-v8a armeabi-v7a)",
    )
    android_group.add_argument(
        "--android-sdk-root",
        default=os.environ.get(
            "ANDROID_SDK_ROOT", str(Path.home() / "Android" / "Sdk")
        ),
        help="Android SDK location (default: ~/Android/Sdk)",
    )
    android_group.add_argument(
        "--install-java",
        action="store_true",
        help="Install Java JDK if needed (Android)",
    )
    android_group.add_argument(
        "--install-android-sdk",
        action="store_true",
        help="Install Android SDK and NDK",
    )
    android_group.add_argument(
        "--java-version",
        default=os.environ.get("JAVA_VERSION", config.get("java_version", "17")),
        help="Java version to install (default: from config)",
    )
    android_group.add_argument(
        "--ndk-version",
        default=os.environ.get(
            "NDK_FULL_VERSION", config.get("ndk_full_version", "")
        ),
        help="Android NDK full version (default: from config)",
    )
    android_group.add_argument(
        "--android-platform",
        default=os.environ.get(
            "ANDROID_PLATFORM", config.get("android_platform", "35")
        ),
        help="Android platform/API level (default: from config)",
    )
    android_group.add_argument(
        "--android-build-tools",
        default=os.environ.get(
            "ANDROID_BUILD_TOOLS", config.get("android_build_tools", "35.0.0")
        ),
        help="Android build-tools version (default: from config)",
    )
    android_group.add_argument(
        "--android-cmdline-tools",
        default=os.environ.get(
            "ANDROID_CMDLINE_TOOLS", config.get("android_cmdline_tools", "13114758")
        ),
        help="Android cmdline-tools version (default: from config)",
    )

    # iOS-specific options
    ios_group = parser.add_argument_group("iOS options")
    ios_group.add_argument(
        "--ios-modules",
        default=os.environ.get("QT_MODULES_IOS", ""),
        help="iOS-specific Qt modules (default: auto-filter from --modules)",
    )

    args = parser.parse_args()

    # Validate required arguments
    if not args.version:
        print("Error: Qt version required. Set QT_VERSION or use --version")
        return 1

    if not args.modules:
        print("Error: Qt modules required. Set QT_MODULES or use --modules")
        return 1

    # iOS validation
    if args.target == "ios" and args.host != "mac":
        print("Error: iOS builds require macOS host")
        return 1

    if args.target == "ios" and not check_xcode():
        print("Error: Xcode command-line tools not installed")
        print("Run: xcode-select --install")
        return 1

    # Set defaults
    if not args.arch:
        args.arch = get_default_arch(args.host, args.target)

    arch_dir = get_arch_dir(args.arch)
    host_arch_dir = get_host_arch_dir(args.host)
    qt_root = Path(args.path) / args.version / arch_dir

    # Print configuration
    print("Qt Installation Configuration:")
    print(f"  Version:   {args.version}")
    print(f"  Path:      {args.path}")
    print(f"  Host:      {args.host}")
    print(f"  Target:    {args.target}")
    print(f"  Arch:      {args.arch}")
    print(f"  Arch Dir:  {arch_dir}")
    print(f"  Modules:   {args.modules}")
    if args.tools:
        print(f"  Tools:     {args.tools}")

    if args.target == "android":
        print(f"  ABIs:      {args.android_abis}")
        print(f"  SDK Root:  {args.android_sdk_root}")
        print(f"  Java:      {args.java_version}")
        print(f"  NDK:       {args.ndk_version}")
        print(f"  Platform:  android-{args.android_platform}")
    elif args.target == "ios":
        ios_modules = args.ios_modules or get_ios_modules(args.modules)
        print(f"  iOS Mods:  {ios_modules}")
    print()

    # Android prerequisites
    ndk_path = ""
    if args.target == "android":
        # Install Java if requested
        if args.install_java:
            print("=== Installing Java ===")
            if args.host == "linux":
                if not install_java_linux(args.java_version, args.dry_run):
                    return 1
            elif args.host == "mac":
                if not install_java_macos(args.java_version, args.dry_run):
                    return 1
            else:
                print("Warning: Java installation not supported on Windows via this script")
            print()

        # Install Android SDK if requested
        if args.install_android_sdk:
            print("=== Installing Android SDK ===")
            if not install_android_sdk(
                args.android_sdk_root,
                args.android_platform,
                args.android_build_tools,
                args.android_cmdline_tools,
                args.host,
                args.dry_run,
            ):
                return 1
            print()

            print("=== Installing Android NDK ===")
            ndk_path = install_android_ndk(
                args.android_sdk_root, args.ndk_version, args.dry_run
            )
            print()

    # Find or install aqtinstall
    if not find_aqt():
        if args.install_aqt:
            if not install_aqt(args.host):
                print("Error: Failed to install aqtinstall")
                return 1
        else:
            print("Error: aqtinstall not found. Use --install-aqt to install it.")
            return 1

    # Find config file
    script_dir = Path(__file__).parent
    aqt_config = script_dir / "aqt-settings.ini"
    if aqt_config.exists():
        print(f"Using aqt config: {aqt_config}")

    # Build aqt arguments
    modules = args.modules.split()

    if args.target == "desktop":
        # Standard desktop installation
        aqt_args = [
            "install-qt",
            args.host,
            args.target,
            args.version,
            args.arch,
            "-O",
            args.path,
            "-m",
            *modules,
            "--autodesktop",
        ]
        if not run_aqt(aqt_args, aqt_config, args.dry_run):
            return 1

    elif args.target == "android":
        # Install host Qt first
        print("=== Installing Qt for host ===")
        host_arch = get_default_arch(args.host, "desktop")
        aqt_args = [
            "install-qt",
            args.host,
            "desktop",
            args.version,
            host_arch,
            "-O",
            args.path,
            "-m",
            *modules,
        ]
        if not run_aqt(aqt_args, aqt_config, args.dry_run):
            return 1

        # Install Qt for each Android ABI
        abis = args.android_abis.split()
        for abi in abis:
            print(f"\n=== Installing Qt for Android ({abi}) ===")
            qt_arch = abi_to_qt_arch(abi)
            aqt_args = [
                "install-qt",
                args.host,
                "android",
                args.version,
                qt_arch,
                "-O",
                args.path,
                "-m",
                *modules,
            ]
            if not run_aqt(aqt_args, aqt_config, args.dry_run):
                print(f"Warning: Failed to install Qt for {abi}")

    elif args.target == "ios":
        # Install host Qt (macOS) first
        print("=== Installing Qt for macOS (host) ===")
        aqt_args = [
            "install-qt",
            "mac",
            "desktop",
            args.version,
            "clang_64",
            "-O",
            args.path,
            "-m",
            *modules,
        ]
        if not run_aqt(aqt_args, aqt_config, args.dry_run):
            return 1

        # Install iOS Qt
        print("\n=== Installing Qt for iOS ===")
        ios_modules = (args.ios_modules or get_ios_modules(args.modules)).split()
        aqt_args = [
            "install-qt",
            "mac",
            "ios",
            args.version,
            "ios",
            "-O",
            args.path,
            "-m",
            *ios_modules,
        ]
        if not run_aqt(aqt_args, aqt_config, args.dry_run):
            return 1

    # Install tools if requested
    if args.tools:
        tools = args.tools.split()
        print(f"\nInstalling Qt tools: {', '.join(tools)}")
        for tool in tools:
            tool_args = ["install-tool", args.host, tool, "-O", args.path]
            if not run_aqt(tool_args, aqt_config, args.dry_run):
                print(f"Warning: Failed to install tool: {tool}")

    # Build environment variables
    qt_root_str = str(qt_root.resolve() if qt_root.exists() else qt_root)
    qt_bin = str((qt_root / "bin").resolve() if qt_root.exists() else qt_root / "bin")
    qt_plugins = str(qt_root / "plugins")
    qt_qml = str(qt_root / "qml")

    if args.target != "desktop":
        qt_host_path = str(Path(args.path) / args.version / host_arch_dir)
    else:
        qt_host_path = qt_root_str

    env_vars = {
        "QT_ROOT_DIR": qt_root_str,
        "QT_HOST_PATH": qt_host_path,
        "QT_PLUGIN_PATH": qt_plugins,
        "QML2_IMPORT_PATH": qt_qml,
    }

    # Android-specific environment variables
    if args.target == "android":
        env_vars["ANDROID_SDK_ROOT"] = args.android_sdk_root
        if ndk_path or args.ndk_version:
            ndk = ndk_path or str(
                Path(args.android_sdk_root) / "ndk" / args.ndk_version
            )
            env_vars["ANDROID_NDK_ROOT"] = ndk
            env_vars["ANDROID_NDK_HOME"] = ndk
            env_vars["ANDROID_NDK"] = ndk
        java_home = get_java_home()
        if java_home:
            env_vars["JAVA_HOME"] = java_home

    # Add tools path if installed
    tools_dir = Path(args.path) / "Tools"
    if tools_dir.exists():
        env_vars["IQTA_TOOLS"] = str(tools_dir)

    # Output environment variables
    if args.export:
        if args.export == "github":
            # Write to GITHUB_OUTPUT and GITHUB_ENV
            github_output = os.environ.get("GITHUB_OUTPUT")
            github_env = os.environ.get("GITHUB_ENV")
            if github_output:
                with open(github_output, "a") as f:
                    f.write(format_env_vars(env_vars, "github") + "\n")
            if github_env:
                with open(github_env, "a") as f:
                    f.write(format_env_vars(env_vars, "github") + "\n")
                    # Also add to PATH
                    qt_bin_path = str(Path(qt_host_path) / "bin")
                    f.write(f"PATH={qt_bin_path}:{os.environ.get('PATH', '')}\n")
        else:
            print("\n# Environment variables (copy to your shell profile):")
            print(format_env_vars(env_vars, args.export))
            if args.export in ("bash", "sh", "zsh"):
                qt_bin_path = f"{qt_host_path}/bin"
                print(f'export PATH="{qt_bin_path}:$PATH"')
            elif args.export == "powershell":
                qt_bin_path = f"{qt_host_path}\\bin"
                print(f'$env:PATH = "{qt_bin_path};$env:PATH"')
    else:
        print("\nEnvironment variables:")
        for key, value in sorted(env_vars.items()):
            print(f"  {key}={value}")
        qt_bin_path = str(Path(qt_host_path) / "bin")
        print(f"  PATH={qt_bin_path}:$PATH")

    # Print additional instructions
    print(f"\nQt {args.version} for {args.target} installed successfully!")

    if args.target == "android":
        print("\nTo build for Android (example for arm64-v8a):")
        print(f"  export QT_ROOT_DIR=\"{args.path}/{args.version}/android_arm64_v8a\"")
        print("  $QT_HOST_PATH/bin/qt-cmake -B build -G Ninja \\")
        print("    -DCMAKE_TOOLCHAIN_FILE=$QT_ROOT_DIR/lib/cmake/Qt6/qt.toolchain.cmake \\")
        print("    -DQT_HOST_PATH=$QT_HOST_PATH \\")
        print("    -DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT \\")
        print("    -DANDROID_NDK_ROOT=$ANDROID_NDK_ROOT \\")
        print("    -DCMAKE_BUILD_TYPE=Release")
    elif args.target == "ios":
        print("\nTo build for iOS:")
        print("  $QT_HOST_PATH/bin/qt-cmake -B build -G Ninja \\")
        print("    -DCMAKE_TOOLCHAIN_FILE=$QT_ROOT_DIR/lib/cmake/Qt6/qt.toolchain.cmake \\")
        print("    -DQT_HOST_PATH=$QT_HOST_PATH \\")
        print("    -DCMAKE_BUILD_TYPE=Release")

    return 0


if __name__ == "__main__":
    sys.exit(main())
