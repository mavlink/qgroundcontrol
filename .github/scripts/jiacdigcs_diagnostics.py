#!/usr/bin/env python3
"""
JIACDIGCS Build Diagnostics Tool
Generates comprehensive build diagnostics for CI/CD and local builds.
"""

import argparse
import json
import os
import subprocess
import sys
from datetime import datetime
from pathlib import Path


class Colors:
    """ANSI color codes for terminal output."""
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    RESET = '\033[0m'
    BOLD = '\033[1m'


def run_command(cmd, capture=True):
    """Run a command and return output."""
    try:
        if capture:
            result = subprocess.run(
                cmd, shell=True, capture_output=True, text=True, timeout=30
            )
            return result.stdout.strip(), result.returncode
        else:
            return subprocess.run(cmd, shell=True, timeout=30), 0
    except subprocess.TimeoutExpired:
        return "Command timed out", 1
    except Exception as e:
        return str(e), 1


def get_git_info():
    """Extract git information."""
    info = {
        'hash': '',
        'branch': '',
        'version': '',
        'version_string': '',
        'is_dirty': False,
        'commit_date': '',
        'commit_count': 0,
    }
    
    # Hash
    stdout, _ = run_command('git rev-parse --short=8 HEAD')
    info['hash'] = stdout or 'unknown'
    
    # Branch
    stdout, _ = run_command('git rev-parse --abbrev-ref HEAD')
    info['branch'] = stdout or 'unknown'
    
    # Version (from tags)
    stdout, _ = run_command('git describe --tags --always --abbrev=8')
    info['version_string'] = stdout or 'v0.0.0-unknown'
    info['version'] = stdout.lstrip('v') if stdout else '0.0.0-unknown'
    
    # Dirty
    _, code = run_command('git status --porcelain')
    info['is_dirty'] = code != 0
    
    # Commit date
    stdout, _ = run_command('git log -1 --format=%aI')
    info['commit_date'] = stdout or 'unknown'
    
    # Commit count
    stdout, _ = run_command('git rev-list --count HEAD')
    info['commit_count'] = int(stdout) if stdout.isdigit() else 0
    
    return info


def get_build_info(build_dir=None):
    """Extract build information from CMake cache."""
    info = {
        'cmake_version': '',
        'qt_version': '',
        'compiler': '',
        'build_type': '',
        'platform': '',
        'swarm_enabled': True,  # JIACDIGCS always has swarm
        'app_name': 'JIACDIGCS',
        'org_name': 'JIACDIGCS',
    }
    
    # CMake version
    stdout, _ = run_command('cmake --version 2>/dev/null | head -1')
    if stdout:
        info['cmake_version'] = stdout.replace('cmake version ', '')
    
    # Check CMakeCache if available
    if build_dir:
        cache_file = Path(build_dir) / 'CMakeCache.txt'
        if cache_file.exists():
            for line in cache_file.read_text().splitlines():
                if 'CMAKE_BUILD_TYPE:STRING=' in line:
                    info['build_type'] = line.split('=')[1] if '=' in line else ''
                elif 'QT_VERSION:PATH=' in line:
                    info['qt_version'] = line.split('=')[1] if '=' in line else ''
                elif 'CMAKE_C_COMPILER_ID=' in line:
                    info['compiler'] = line.split('=')[1] if '=' in line else ''
    
    # Detect platform
    import platform
    system = platform.system().lower()
    if system == 'windows':
        info['platform'] = 'Windows'
    elif system == 'darwin':
        info['platform'] = 'macOS/iOS'
    elif system == 'linux':
        info['platform'] = 'Linux (Limited)'
    
    return info


def check_dependencies():
    """Check build dependencies."""
    deps = {
        'cmake': False,
        'ninja': False,
        'qt6': False,
        'gstreamer': False,
        'android_sdk': False,
        'java': False,
        'xcode': False,
    }
    
    # CMake
    _, code = run_command('cmake --version')
    deps['cmake'] = code == 0
    
    # Ninja
    _, code = run_command('ninja --version')
    deps['ninja'] = code == 0
    
    # Qt6
    _, code = run_command('qmake6 --version || qmake --version')
    deps['qt6'] = code == 0
    
    # GStreamer
    _, code = run_command('gst-inspect-1.0 --version 2>/dev/null || gst-launch-1.0 --version')
    deps['gstreamer'] = code == 0
    
    # Android SDK
    if os.environ.get('ANDROID_SDK_ROOT') or os.environ.get('ANDROID_HOME'):
        deps['android_sdk'] = True
    
    # Java
    stdout, code = run_command('java -version 2>&1 | head -1')
    deps['java'] = code == 0 and 'java' in stdout.lower()
    
    # Xcode (macOS only)
    if platform.system() == 'Darwin':
        _, code = run_command('xcodebuild -version')
        deps['xcode'] = code == 0
    
    return deps


def generate_diagnostics(output_file=None, build_dir=None, verbose=False):
    """Generate comprehensive diagnostics."""
    
    diagnostics = {
        'generated_at': datetime.utcnow().isoformat() + 'Z',
        'product': 'JIACDIGCS',
        'description': 'Professional Multi-UAV Swarm Command and Control Platform',
        'git': get_git_info(),
        'build': get_build_info(build_dir),
        'dependencies': check_dependencies(),
        'environment': dict(os.environ),
    }
    
    # Simplify environment (only relevant vars)
    relevant_env = {
        'PATH': os.environ.get('PATH', ''),
        'QGC_APP_NAME': os.environ.get('QGC_APP_NAME', 'JIACDIGCS'),
        'QT_VERSION': os.environ.get('QT_VERSION', ''),
        'ANDROID_SDK_ROOT': os.environ.get('ANDROID_SDK_ROOT', ''),
        'ANDROID_NDK_ROOT': os.environ.get('ANDROID_NDK_ROOT', ''),
    }
    diagnostics['environment'] = relevant_env
    
    # Calculate summary
    git = diagnostics['git']
    deps = diagnostics['dependencies']
    
    summary = {
        'version': git['version_string'],
        'git_hash': git['hash'],
        'branch': git['branch'],
        'is_dirty': git['is_dirty'],
        'commit_date': git['commit_date'],
        'commit_count': git['commit_count'],
        'app_name': diagnostics['build']['app_name'],
        'platform': diagnostics['build']['platform'],
        'build_type': diagnostics['build']['build_type'] or 'Release',
        'cmake_version': diagnostics['build']['cmake_version'],
        'qt_version': diagnostics['build']['qt_version'] or '6.10.3 (expected)',
        'swarm_enabled': diagnostics['build']['swarm_enabled'],
        'diagnostics_complete': True,
    }
    
    diagnostics['summary'] = summary
    
    # Output
    if output_file:
        with open(output_file, 'w') as f:
            json.dump(diagnostics, f, indent=2)
    
    # Print to console
    print(f"{Colors.BOLD}{Colors.CYAN}")
    print("=" * 60)
    print("       JIACDIGCS Build Diagnostics")
    print("=" * 60)
    print(f"{Colors.RESET}")
    
    print(f"\n{Colors.BOLD}Version Information:{Colors.RESET}")
    print(f"  Version:     {Colors.GREEN}{summary['version']}{Colors.RESET}")
    print(f"  Git Hash:    {summary['git_hash']}")
    print(f"  Branch:      {summary['branch']}")
    print(f"  Commit Date: {summary['commit_date']}")
    print(f"  Dirty:       {summary['is_dirty']}")
    
    print(f"\n{Colors.BOLD}Build Configuration:{Colors.RESET}")
    print(f"  App Name:    {summary['app_name']}")
    print(f"  Platform:    {summary['platform']}")
    print(f"  Build Type:  {summary['build_type']}")
    print(f"  CMake:       {summary['cmake_version'] or 'Not found'}")
    print(f"  Qt:          {summary['qt_version'] or '6.10.3 (expected)'}")
    print(f"  Swarm:       {Colors.GREEN}Enabled{Colors.RESET}")
    
    print(f"\n{Colors.BOLD}Dependency Status:{Colors.RESET}")
    dep_colors = {True: Colors.GREEN, False: Colors.RED}
    dep_symbols = {True: '✓', False: '✗'}
    
    for dep, found in deps.items():
        color = dep_colors[found]
        symbol = dep_symbols[found]
        name = dep.replace('_', ' ').title()
        print(f"  {color}{symbol}{Colors.RESET} {name}")
    
    print(f"\n{Colors.BOLD}Environment Variables:{Colors.RESET}")
    for key, val in relevant_env.items():
        if val:
            display_val = val if len(val) < 50 else val[:50] + '...'
            print(f"  {key}: {display_val}")
    
    print(f"\n{Colors.BOLD}{'=' * 60}{Colors.RESET}")
    print(f"Diagnostics generated at: {diagnostics['generated_at']}")
    print(f"Output saved to: {output_file or 'stdout'}")
    
    if output_file:
        print(f"\n{Colors.GREEN}✓ Diagnostics saved to {output_file}{Colors.RESET}")
    
    return diagnostics


def main():
    parser = argparse.ArgumentParser(
        description='JIACDIGCS Build Diagnostics Tool'
    )
    parser.add_argument(
        '-o', '--output',
        help='Output file path (JSON format)',
        default=None
    )
    parser.add_argument(
        '-b', '--build-dir',
        help='Build directory to analyze',
        default=None
    )
    parser.add_argument(
        '-v', '--verbose',
        help='Verbose output',
        action='store_true'
    )
    
    args = parser.parse_args()
    
    try:
        generate_diagnostics(
            output_file=args.output,
            build_dir=args.build_dir,
            verbose=args.verbose
        )
        return 0
    except Exception as e:
        print(f"{Colors.RED}Error: {e}{Colors.RESET}", file=sys.stderr)
        return 1


if __name__ == '__main__':
    import platform
    sys.exit(main())