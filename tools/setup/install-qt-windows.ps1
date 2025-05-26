<#
.SYNOPSIS
    Install Qt on Windows via aqtinstall.

.DESCRIPTION
    - Uses Python + pip to install helper packages (setuptools, wheel, py7zr, ninja, cmake, aqtinstall).
    - Runs `aqt install-qt` with the same modules list.
    - Adjusts PATH and Qt‑related env vars for the current session.
#>

# ————————————————————————————————
# 1) Defaults (env overrides supported)
# ————————————————————————————————
$QT_VERSION = $env:QT_VERSION      -or '6.8.3'
$QT_PATH    = $env:QT_PATH         -or 'C:\Qt'
$QT_HOST    = $env:QT_HOST         -or 'windows'
$QT_TARGET  = $env:QT_TARGET       -or 'desktop'
# Windows arch must be one of: win64_msvc2017_64, win64_msvc2019_64, win64_mingw81, etc. :contentReference[oaicite:0]{index=0}
$QT_ARCH    = $env:QT_ARCH         -or 'win64_msvc2022_64'
$QT_MODULES = $env:QT_MODULES      -or 'qtcharts qtlocation qtpositioning qtspeech qt5compat qtmultimedia qtserialport qtimageformats qtshadertools qtconnectivity qtquick3d qtsensors'

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
