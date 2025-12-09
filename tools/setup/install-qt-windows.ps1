<#
.SYNOPSIS
    Install Qt on Windows via aqtinstall.

.DESCRIPTION
    - Uses Python + pip to install helper packages (setuptools, wheel, py7zr, ninja, cmake, aqtinstall).
    - Runs `aqt install-qt` with the same modules list.
    - Adjusts PATH and Qt-related env vars for the current session.
#>

# ————————————————————————————————
# 0) Source build config (required - no fallbacks)
# ————————————————————————————————
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$configScript = Join-Path $scriptDir "read-config.ps1"
if (Test-Path $configScript) {
    . $configScript
} else {
    Write-Error "read-config.ps1 not found"
    exit 1
}

# Verify required variables are set
if (-not $env:QT_VERSION -or -not $env:QT_MODULES) {
    Write-Error "QT_VERSION and QT_MODULES must be set (check build-config.json)"
    exit 1
}

# ————————————————————————————————
# 1) Platform defaults (can be overridden via environment)
# ————————————————————————————————
$QT_VERSION = $env:QT_VERSION
$QT_PATH    = if ($env:QT_PATH)   { $env:QT_PATH }   else { 'C:\Qt' }
$QT_HOST    = if ($env:QT_HOST)   { $env:QT_HOST }   else { 'windows' }
$QT_TARGET  = if ($env:QT_TARGET) { $env:QT_TARGET } else { 'desktop' }
$QT_ARCH    = if ($env:QT_ARCH)   { $env:QT_ARCH }   else { 'win64_msvc2022_64' }
$QT_MODULES = $env:QT_MODULES

Write-Host "Using:"
Write-Host "  QT_VERSION    = $QT_VERSION"
Write-Host "  QT_PATH       = $QT_PATH"
Write-Host "  QT_HOST       = $QT_HOST"
Write-Host "  QT_TARGET     = $QT_TARGET"
Write-Host "  QT_ARCH       = $QT_ARCH"
Write-Host "  QT_MODULES    = $QT_MODULES"

# ————————————————————————————————
# 2) Ensure Python + pip tools
# ————————————————————————————————
if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    Write-Error "Python 3 is not on your PATH. Please install Python 3 first."
    exit 1
}

Write-Host "`nInstalling helper Python packages..."
# The PyPI “ninja” wheels provide the Ninja build system executable :contentReference[oaicite:1]{index=1}
python -m pip install --upgrade pip
python -m pip install setuptools wheel py7zr ninja cmake aqtinstall

# ————————————————————————————————
# 3) Run the aqt Qt installer
# ————————————————————————————————
Write-Host "`nDownloading & installing Qt..."
# Split modules into an array so we can pass them all to -m
$modules = $QT_MODULES -split '\s+'
# Build the argument list for aqt
$aqtArgs = @('install-qt', $QT_HOST, $QT_TARGET, $QT_VERSION, $QT_ARCH, '-O', $QT_PATH, '-m') + $modules

# Invoke either the script entrypoint or via python -m aqt
if (Get-Command aqt -ErrorAction SilentlyContinue) {
    & aqt @aqtArgs
} else {
    & python -m aqt @aqtArgs
}

# ————————————————————————————————
# 4) Update environment for this session
# ————————————————————————————————
$qtRoot = Join-Path $QT_PATH "$QT_VERSION\$QT_ARCH"
$qtBin  = Join-Path $qtRoot 'bin'
$qtLib  = Join-Path $qtRoot 'lib'

# Prepend Qt bin to PATH
$env:PATH = "$qtBin;$env:PATH"
# pkg-config on Windows (if you’re using msys2/Cygwin): point at Qt’s pkgconfig
$env:PKG_CONFIG_PATH = "$qtLib\pkgconfig;$env:PKG_CONFIG_PATH"
# (Optional) Linux-style loader path—ignored by native Windows but useful in WSL
$env:LD_LIBRARY_PATH = "$qtLib;$env:LD_LIBRARY_PATH"
# Qt‑specific
$env:QT_ROOT_DIR      = $qtRoot
$env:QT_PLUGIN_PATH   = Join-Path $qtRoot 'plugins'
$env:QML2_IMPORT_PATH = Join-Path $qtRoot 'qml'

Write-Host "`nEnvironment updated for current session:"
Write-Host "  PATH               = $env:PATH"
Write-Host "  PKG_CONFIG_PATH    = $env:PKG_CONFIG_PATH"
Write-Host "  LD_LIBRARY_PATH    = $env:LD_LIBRARY_PATH"
Write-Host "  QT_ROOT_DIR        = $env:QT_ROOT_DIR"
Write-Host "  QT_PLUGIN_PATH     = $env:QT_PLUGIN_PATH"
Write-Host "  QML2_IMPORT_PATH   = $env:QML2_IMPORT_PATH"

Write-Host "`n✅ Qt $QT_VERSION for $QT_ARCH installed successfully!"
