<#
.SYNOPSIS
    Read build configuration from .github/build-config.json

.DESCRIPTION
    This script reads version variables from the centralized config file.
    Dot-source this script to get the variables, or call with -Get <key> to retrieve a single value.

.PARAMETER Get
    Get a single value by key name

.EXAMPLE
    . .\read-config.ps1              # Import all variables
    .\read-config.ps1 -Get qt_version  # Get single value
#>

param(
    [string]$Get
)

# Find the repo root (location of .github/build-config.json)
function Find-RepoRoot {
    param([string]$StartPath)
    $dir = $StartPath
    while ($dir -and $dir -ne [System.IO.Path]::GetPathRoot($dir)) {
        $configPath = Join-Path $dir ".github\build-config.json"
        if (Test-Path $configPath) {
            return $dir
        }
        $dir = Split-Path $dir -Parent
    }
    return $null
}

# Locate config file
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$localConfig = Join-Path $scriptDir "build-config.json"

if (Test-Path $localConfig) {
    $configFile = $localConfig
} else {
    $repoRoot = Find-RepoRoot $scriptDir
    if ($repoRoot) {
        $configFile = Join-Path $repoRoot ".github\build-config.json"
    } else {
        Write-Error "Could not find build-config.json"
        exit 1
    }
}

# Read JSON config
try {
    $config = Get-Content $configFile -Raw | ConvertFrom-Json
} catch {
    Write-Error "Failed to parse JSON from $configFile : $_"
    exit 1
}

# If -Get specified, return single value
if ($Get) {
    $value = $config.PSObject.Properties[$Get].Value
    if ($null -eq $value) {
        Write-Error "Key '$Get' not found in config"
        exit 1
    }
    Write-Output $value
    exit 0
}

# Export all configuration variables (only if not already set)
if (-not $env:QT_VERSION)                { $env:QT_VERSION = $config.qt_version }
if (-not $env:QT_MODULES)                { $env:QT_MODULES = $config.qt_modules }
if (-not $env:QT_MINIMUM_VERSION)        { $env:QT_MINIMUM_VERSION = $config.qt_minimum_version }
if (-not $env:GSTREAMER_MINIMUM_VERSION) { $env:GSTREAMER_MINIMUM_VERSION = $config.gstreamer_minimum_version }
if (-not $env:GSTREAMER_WINDOWS_VERSION) { $env:GSTREAMER_WINDOWS_VERSION = $config.gstreamer_windows_version }
if (-not $env:GSTREAMER_ANDROID_VERSION) { $env:GSTREAMER_ANDROID_VERSION = $config.gstreamer_android_version }
if (-not $env:NDK_VERSION)               { $env:NDK_VERSION = $config.ndk_version }
if (-not $env:NDK_FULL_VERSION)          { $env:NDK_FULL_VERSION = $config.ndk_full_version }
if (-not $env:JAVA_VERSION)              { $env:JAVA_VERSION = $config.java_version }
if (-not $env:ANDROID_PLATFORM)          { $env:ANDROID_PLATFORM = $config.android_platform }
if (-not $env:ANDROID_MIN_SDK)           { $env:ANDROID_MIN_SDK = $config.android_min_sdk }
if (-not $env:ANDROID_BUILD_TOOLS)       { $env:ANDROID_BUILD_TOOLS = $config.android_build_tools }
if (-not $env:ANDROID_CMDLINE_TOOLS)     { $env:ANDROID_CMDLINE_TOOLS = $config.android_cmdline_tools }
if (-not $env:CMAKE_MINIMUM_VERSION)     { $env:CMAKE_MINIMUM_VERSION = $config.cmake_minimum_version }

# Also set script-scope variables for dot-sourcing
$script:QT_VERSION = $env:QT_VERSION
$script:QT_MODULES = $env:QT_MODULES
$script:QT_MINIMUM_VERSION = $env:QT_MINIMUM_VERSION
$script:GSTREAMER_MINIMUM_VERSION = $env:GSTREAMER_MINIMUM_VERSION
$script:GSTREAMER_WINDOWS_VERSION = $env:GSTREAMER_WINDOWS_VERSION
$script:GSTREAMER_ANDROID_VERSION = $env:GSTREAMER_ANDROID_VERSION
$script:NDK_VERSION = $env:NDK_VERSION
$script:NDK_FULL_VERSION = $env:NDK_FULL_VERSION
$script:JAVA_VERSION = $env:JAVA_VERSION
$script:ANDROID_PLATFORM = $env:ANDROID_PLATFORM
$script:ANDROID_MIN_SDK = $env:ANDROID_MIN_SDK
$script:ANDROID_BUILD_TOOLS = $env:ANDROID_BUILD_TOOLS
$script:ANDROID_CMDLINE_TOOLS = $env:ANDROID_CMDLINE_TOOLS
$script:CMAKE_MINIMUM_VERSION = $env:CMAKE_MINIMUM_VERSION

# Print config if running directly
if ($MyInvocation.InvocationName -ne '.') {
    Write-Host "Build Configuration (from $configFile):"
    Write-Host "  QT_VERSION                = $env:QT_VERSION"
    Write-Host "  QT_MODULES                = $env:QT_MODULES"
    Write-Host "  QT_MINIMUM_VERSION        = $env:QT_MINIMUM_VERSION"
    Write-Host "  GSTREAMER_MINIMUM_VERSION = $env:GSTREAMER_MINIMUM_VERSION"
    Write-Host "  GSTREAMER_WINDOWS_VERSION = $env:GSTREAMER_WINDOWS_VERSION"
    Write-Host "  GSTREAMER_ANDROID_VERSION = $env:GSTREAMER_ANDROID_VERSION"
    Write-Host "  NDK_VERSION               = $env:NDK_VERSION"
    Write-Host "  NDK_FULL_VERSION          = $env:NDK_FULL_VERSION"
    Write-Host "  JAVA_VERSION              = $env:JAVA_VERSION"
    Write-Host "  ANDROID_PLATFORM          = $env:ANDROID_PLATFORM"
    Write-Host "  ANDROID_MIN_SDK           = $env:ANDROID_MIN_SDK"
    Write-Host "  ANDROID_BUILD_TOOLS       = $env:ANDROID_BUILD_TOOLS"
    Write-Host "  ANDROID_CMDLINE_TOOLS     = $env:ANDROID_CMDLINE_TOOLS"
    Write-Host "  CMAKE_MINIMUM_VERSION     = $env:CMAKE_MINIMUM_VERSION"
}
