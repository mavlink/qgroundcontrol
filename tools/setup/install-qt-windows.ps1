<#
.SYNOPSIS
    Install Qt on Windows via aqtinstall.

.DESCRIPTION
    - Reads version/modules from centralized .github/build-config.json
    - Uses Python + pip to install helper packages (setuptools, wheel, py7zr, ninja, cmake, aqtinstall).
    - Runs `aqt install-qt` with the same modules list.
    - Adjusts PATH and Qt‑related env vars for the current session.
#>

# ————————————————————————————————
# Helper: Find repo root and read build-config.json
# ————————————————————————————————
function Get-BuildConfig {
    $scriptDir = Split-Path -Parent $MyInvocation.ScriptName
    $dir = $scriptDir
    while ($dir -and -not (Test-Path (Join-Path $dir ".github/build-config.json"))) {
        $dir = Split-Path -Parent $dir
    }
    if (-not $dir) {
        Write-Warning "Could not find .github/build-config.json, using defaults"
        return $null
    }
    $configPath = Join-Path $dir ".github/build-config.json"
    return Get-Content $configPath | ConvertFrom-Json
}

$config = Get-BuildConfig

# ————————————————————————————————
# 1) Defaults (env overrides supported, fallback to config, then hardcoded defaults)
# ————————————————————————————————
$QT_VERSION = if ($env:QT_VERSION) { $env:QT_VERSION } elseif ($config.qt_version) { $config.qt_version } else { '6.10.1' }
$QT_PATH    = if ($env:QT_PATH) { $env:QT_PATH } else { 'C:\Qt' }
$QT_HOST    = if ($env:QT_HOST) { $env:QT_HOST } else { 'windows' }
$QT_TARGET  = if ($env:QT_TARGET) { $env:QT_TARGET } else { 'desktop' }
# Windows arch must be one of: win64_msvc2017_64, win64_msvc2019_64, win64_mingw81, etc.
$QT_ARCH    = if ($env:QT_ARCH) { $env:QT_ARCH } else { 'win64_msvc2022_64' }
$QT_MODULES = if ($env:QT_MODULES) { $env:QT_MODULES } elseif ($config.qt_modules) { $config.qt_modules } else { 'qtcharts qtlocation qtpositioning qtspeech qt5compat qtmultimedia qtserialport qtimageformats qtshadertools qtconnectivity qtquick3d qtsensors qtscxml' }

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
# The PyPI "ninja" wheel provides the Ninja build system executable
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

Write-Host "`nQt $QT_VERSION for $QT_ARCH installed successfully!"
