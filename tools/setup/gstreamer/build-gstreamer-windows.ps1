<#
.SYNOPSIS
    Build GStreamer from source for QGroundControl on Windows.

.DESCRIPTION
    Builds a minimal GStreamer with only the plugins needed for QGC video streaming:
    RTSP, RTP, UDP, H.264/H.265, MPEG-TS, and Qt6 integration.

    Supports both native builds and cross-compilation for ARM64.

.PARAMETER Version
    GStreamer version tag (default: from build-config.json).

.PARAMETER BuildType
    Build type: Release or Debug (default: Release).

.PARAMETER Arch
    Target architecture: x64 or arm64 (default: auto-detect).

.PARAMETER CrossCompile
    Force cross-compilation from x64 to ARM64.

.PARAMETER Prefix
    Install prefix (default: C:\gstreamer-<arch>).

.PARAMETER WorkDir
    Working directory for source (default: $env:TEMP).

.PARAMETER Jobs
    Parallel jobs (default: auto-detect).

.PARAMETER Clean
    Clean build directory before building.

.PARAMETER QtPrefix
    Path to Qt6 installation for qml6 plugin.

.EXAMPLE
    .\build-gstreamer-windows.ps1
    # Native build with defaults

.EXAMPLE
    .\build-gstreamer-windows.ps1 -Arch arm64 -CrossCompile
    # Cross-compile for ARM64 from x64 host

.EXAMPLE
    .\build-gstreamer-windows.ps1 -Prefix C:\gst -BuildType Debug -Clean
    # Debug build with custom prefix, clean rebuild
#>
param(
    [string]$Version,
    [ValidateSet('Release', 'Debug')]
    [string]$BuildType = 'Release',
    [ValidateSet('x64', 'arm64')]
    [string]$Arch,
    [switch]$CrossCompile,
    [string]$Prefix,
    [string]$WorkDir = $env:TEMP,
    [int]$Jobs = 0,
    [switch]$Clean,
    [string]$QtPrefix
)

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# ────────────────────────────────
# Colors and logging
# ────────────────────────────────
function Write-Info  { param($msg) Write-Host "[INFO] $msg" -ForegroundColor Blue }
function Write-Ok    { param($msg) Write-Host "[OK] $msg" -ForegroundColor Green }
function Write-Warn  { param($msg) Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-Err   { param($msg) Write-Host "[ERROR] $msg" -ForegroundColor Red }

# ────────────────────────────────
# Source build config
# ────────────────────────────────
$configScript = Join-Path (Split-Path $scriptDir -Parent) "read-config.ps1"
if (Test-Path $configScript) {
    . $configScript
} else {
    Write-Err "read-config.ps1 not found"
    exit 1
}

# Resolve version
if (-not $Version) {
    if ($env:GSTREAMER_VERSION) {
        $Version = $env:GSTREAMER_VERSION
    } elseif ($env:GST_VERSION) {
        $Version = $env:GST_VERSION
    } else {
        Write-Err "GStreamer version not specified. Use -Version or set GSTREAMER_VERSION"
        exit 1
    }
}

# ────────────────────────────────
# Detect architecture
# ────────────────────────────────
$hostArch = $env:PROCESSOR_ARCHITECTURE.ToLower()
if ($hostArch -eq 'amd64') { $hostArch = 'x64' }

if (-not $Arch) {
    $Arch = $hostArch
}

$isCrossCompile = $CrossCompile -or ($Arch -eq 'arm64' -and $hostArch -eq 'x64')

if ($isCrossCompile -and $hostArch -ne 'x64') {
    Write-Err "Cross-compilation only supported from x64 host"
    exit 1
}

if ($isCrossCompile -and $Arch -ne 'arm64') {
    Write-Err "Cross-compilation only supported for arm64 target"
    exit 1
}

# ────────────────────────────────
# Set defaults
# ────────────────────────────────
if (-not $Prefix) {
    $Prefix = "C:\gstreamer-$Arch"
}

if ($Jobs -eq 0) {
    $Jobs = $env:NUMBER_OF_PROCESSORS
    if (-not $Jobs) { $Jobs = 4 }
}

$sourceDir = Join-Path $WorkDir "gstreamer"
$buildDir = Join-Path $sourceDir "builddir"
$mesonBuildType = $BuildType.ToLower()

# ────────────────────────────────
# Display configuration
# ────────────────────────────────
Write-Host ""
Write-Info "Building GStreamer $Version for QGroundControl"
Write-Host "  Target arch:     $Arch"
Write-Host "  Host arch:       $hostArch"
Write-Host "  Cross-compile:   $isCrossCompile"
Write-Host "  Build type:      $BuildType"
Write-Host "  Parallel jobs:   $Jobs"
Write-Host "  Source dir:      $sourceDir"
Write-Host "  Install prefix:  $Prefix"
if ($QtPrefix) {
    Write-Host "  Qt prefix:       $QtPrefix"
}
Write-Host ""

# ────────────────────────────────
# Check dependencies
# ────────────────────────────────
Write-Info "Checking dependencies..."

$missing = @()
foreach ($cmd in @('git', 'python')) {
    if (-not (Get-Command $cmd -ErrorAction SilentlyContinue)) {
        $missing += $cmd
    }
}

if ($missing.Count -gt 0) {
    Write-Err "Missing required tools: $($missing -join ', ')"
    Write-Info "Install with: winget install Git.Git Python.Python.3.12"
    exit 1
}

# Check for Visual Studio
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Err "Visual Studio not found. Install Visual Studio 2022 with C++ workload."
    exit 1
}

$vsPath = & $vswhere -latest -property installationPath
if (-not $vsPath) {
    Write-Err "Could not find Visual Studio installation"
    exit 1
}

Write-Ok "All dependencies found"

# ────────────────────────────────
# Install Python build tools
# ────────────────────────────────
Write-Info "Installing meson and ninja..."
python -m pip install --quiet --upgrade pip
python -m pip install --quiet meson ninja

# Verify meson is available
$mesonPath = python -c "import shutil; print(shutil.which('meson') or '')" 2>$null
if (-not $mesonPath) {
    # Add Python Scripts to PATH
    $pythonScripts = python -c "import sysconfig; print(sysconfig.get_path('scripts'))"
    $env:PATH = "$pythonScripts;$env:PATH"
}

# ────────────────────────────────
# Set up MSVC environment
# ────────────────────────────────
Write-Info "Setting up MSVC environment..."

$vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path $vcvarsall)) {
    Write-Err "vcvarsall.bat not found at: $vcvarsall"
    exit 1
}

# Determine vcvarsall architecture argument
if ($isCrossCompile) {
    $vcArch = "x64_arm64"
} elseif ($Arch -eq 'arm64') {
    $vcArch = "arm64"
} else {
    $vcArch = "x64"
}

# Capture environment from vcvarsall
$envCmd = "`"$vcvarsall`" $vcArch >nul 2>&1 && set"
$envOutput = cmd /c $envCmd

foreach ($line in $envOutput) {
    if ($line -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}

Write-Ok "MSVC environment configured for $vcArch"

# ────────────────────────────────
# Clean if requested
# ────────────────────────────────
if ($Clean -and (Test-Path $sourceDir)) {
    Write-Info "Cleaning previous build..."
    Remove-Item -Recurse -Force $sourceDir
}

# ────────────────────────────────
# Clone GStreamer
# ────────────────────────────────
if (-not (Test-Path $sourceDir)) {
    Write-Info "Cloning GStreamer $Version..."
    New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null
    git clone --depth 1 --branch $Version https://github.com/GStreamer/gstreamer.git $sourceDir
    if ($LASTEXITCODE -ne 0) {
        Write-Err "Failed to clone GStreamer"
        exit 1
    }
} else {
    Write-Info "Using existing source at $sourceDir"
}

Set-Location $sourceDir

# ────────────────────────────────
# Create cross file for ARM64
# ────────────────────────────────
$crossFile = $null
if ($isCrossCompile) {
    Write-Info "Creating cross-compilation file for ARM64..."

    # Find MSVC paths
    $vcToolsDir = $env:VCToolsInstallDir
    if (-not $vcToolsDir) {
        Write-Err "VCToolsInstallDir not set. MSVC environment not configured correctly."
        exit 1
    }

    $clPath = Join-Path $vcToolsDir "bin\HostX64\ARM64\cl.exe"
    $linkPath = Join-Path $vcToolsDir "bin\HostX64\ARM64\link.exe"
    $libPath = Join-Path $vcToolsDir "bin\HostX64\ARM64\lib.exe"

    if (-not (Test-Path $clPath)) {
        Write-Err "ARM64 cross-compiler not found at: $clPath"
        Write-Info "Install 'MSVC v143 - VS 2022 C++ ARM64 build tools' via Visual Studio Installer"
        exit 1
    }

    # Escape backslashes for meson
    $clPathEsc = $clPath.Replace('\', '/')
    $linkPathEsc = $linkPath.Replace('\', '/')
    $libPathEsc = $libPath.Replace('\', '/')

    $crossFile = Join-Path $sourceDir "cross-arm64.txt"
    @"
[binaries]
c = '$clPathEsc'
cpp = '$clPathEsc'
ar = '$libPathEsc'
strip = '$libPathEsc'
windres = 'rc'

[built-in options]
c_args = []
cpp_args = []

[host_machine]
system = 'windows'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
"@ | Out-File -FilePath $crossFile -Encoding utf8

    Write-Ok "Cross file created: $crossFile"
}

# ────────────────────────────────
# Configure with Meson
# ────────────────────────────────
if (-not (Test-Path (Join-Path $buildDir "build.ninja")) -or $Clean) {
    Write-Info "Configuring GStreamer..."

    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }

    # Build meson arguments
    $mesonArgs = @(
        "setup", $buildDir,
        "--prefix=$Prefix",
        "--buildtype=$mesonBuildType",
        "--wrap-mode=forcefallback",
        "-Dauto_features=disabled",
        "-Dgpl=enabled",
        "-Dlibav=enabled",
        "-Dorc=enabled",
        "-Dbase=enabled",
        "-Dgst-plugins-base:app=enabled",
        "-Dgst-plugins-base:gl=enabled",
        "-Dgst-plugins-base:gl_api=opengl",
        "-Dgst-plugins-base:gl_platform=wgl,egl",
        "-Dgst-plugins-base:gl_winsys=win32,egl",
        "-Dgst-plugins-base:playback=enabled",
        "-Dgst-plugins-base:tcp=enabled",
        "-Dgood=enabled",
        "-Dgst-plugins-good:isomp4=enabled",
        "-Dgst-plugins-good:matroska=enabled",
        "-Dgst-plugins-good:rtp=enabled",
        "-Dgst-plugins-good:rtpmanager=enabled",
        "-Dgst-plugins-good:rtsp=enabled",
        "-Dgst-plugins-good:udp=enabled",
        "-Dbad=enabled",
        "-Dgst-plugins-bad:gl=enabled",
        "-Dgst-plugins-bad:mpegtsdemux=enabled",
        "-Dgst-plugins-bad:rtp=enabled",
        "-Dgst-plugins-bad:sdp=enabled",
        "-Dgst-plugins-bad:videoparsers=enabled",
        "-Dgst-plugins-bad:d3d11=enabled",
        "-Dgst-plugins-bad:d3d12=enabled",
        "-Dgst-plugins-bad:mediafoundation=enabled",
        "-Dugly=enabled"
    )

    # Qt6 support
    if ($QtPrefix) {
        $mesonArgs += "-Dqt6=enabled"
        $mesonArgs += "-Dgst-plugins-good:qt6=enabled"
        $mesonArgs += "-Dgst-plugins-good:qt-method=auto"
        # Add Qt to PKG_CONFIG_PATH
        $qtPkgConfig = Join-Path $QtPrefix "lib\pkgconfig"
        if (Test-Path $qtPkgConfig) {
            $env:PKG_CONFIG_PATH = "$qtPkgConfig;$env:PKG_CONFIG_PATH"
        }
        # Add Qt to CMAKE_PREFIX_PATH for meson to find it
        $env:CMAKE_PREFIX_PATH = "$QtPrefix;$env:CMAKE_PREFIX_PATH"
    } else {
        $mesonArgs += "-Dqt6=disabled"
        $mesonArgs += "-Dgst-plugins-good:qt6=disabled"
    }

    # x264/x265 - may not be available for ARM64
    if ($Arch -eq 'x64') {
        $mesonArgs += "-Dgst-plugins-ugly:x264=enabled"
        $mesonArgs += "-Dgst-plugins-bad:x265=enabled"
    } else {
        # ARM64: disable codecs that may not build
        $mesonArgs += "-Dgst-plugins-ugly:x264=disabled"
        $mesonArgs += "-Dgst-plugins-bad:x265=disabled"
    }

    # Cross-compilation
    if ($crossFile) {
        $mesonArgs += "--cross-file=$crossFile"
    }

    Write-Host "Running: meson $($mesonArgs -join ' ')"
    & meson @mesonArgs

    if ($LASTEXITCODE -ne 0) {
        Write-Err "Meson configuration failed"
        exit 1
    }
} else {
    Write-Info "Using existing configuration"
}

# ────────────────────────────────
# Build
# ────────────────────────────────
Write-Info "Compiling GStreamer (this may take a while)..."
& meson compile -C $buildDir -j $Jobs

if ($LASTEXITCODE -ne 0) {
    Write-Err "Build failed"
    exit 1
}

# ────────────────────────────────
# Install
# ────────────────────────────────
Write-Info "Installing GStreamer to $Prefix..."
& meson install -C $buildDir

if ($LASTEXITCODE -ne 0) {
    Write-Err "Installation failed"
    exit 1
}

# ────────────────────────────────
# Success
# ────────────────────────────────
Write-Host ""
Write-Ok "GStreamer $Version installed successfully!"
Write-Host ""
Write-Host "To use with QGroundControl, set:"
Write-Host "  `$env:GSTREAMER_1_0_ROOT_MSVC_$($Arch.ToUpper()) = '$Prefix'"
Write-Host "  `$env:PATH = '$Prefix\bin;' + `$env:PATH"
Write-Host "  `$env:GST_PLUGIN_PATH = '$Prefix\lib\gstreamer-1.0'"
Write-Host ""

# Output for CI
if ($env:GITHUB_OUTPUT) {
    "gstreamer_prefix=$Prefix" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
    "gstreamer_version=$Version" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
    "gstreamer_arch=$Arch" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
}

if ($env:GITHUB_ENV) {
    "GSTREAMER_1_0_ROOT_MSVC_$($Arch.ToUpper())=$Prefix" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
}
