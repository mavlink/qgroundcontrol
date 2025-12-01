<#
.SYNOPSIS
    Install GStreamer (x64 only) on Windows.

.DESCRIPTION
    Downloads installers to $env:TEMP and removes them afterwards.
    Sets the required environment variables and updates PATH.
    Supports both local development (requires elevation) and CI environments.
    Reads default GStreamer version from .github/build-config.json.

    Note: Vulkan SDK is NOT required for QGC. GStreamer MSVC packages include
    Vulkan plugin support, and modern GPU drivers provide the Vulkan loader.
    Use -InstallVulkan only if you need validation layers for debugging.

.PARAMETER GStreamerVersion
    GStreamer version to install. Default: from build-config.json

.PARAMETER SkipGStreamer
    Skip GStreamer installation.

.PARAMETER InstallVulkan
    Install Vulkan SDK (optional, for validation layers/debugging only).

.EXAMPLE
    ./install-dependencies-windows.ps1
    ./install-dependencies-windows.ps1 -GStreamerVersion 1.24.0
    ./install-dependencies-windows.ps1 -InstallVulkan
#>

param(
    [string]$GStreamerVersion,
    [switch]$SkipGStreamer,
    [switch]$InstallVulkan
)

# Read default version from build-config.json if not specified
if (-not $GStreamerVersion) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $configPath = Join-Path (Split-Path -Parent (Split-Path -Parent $scriptDir)) ".github/build-config.json"
    if (Test-Path $configPath) {
        $config = Get-Content $configPath | ConvertFrom-Json
        $GStreamerVersion = $config.gstreamer_version
    }
    if (-not $GStreamerVersion) {
        $GStreamerVersion = '1.24.13'  # Fallback default
    }
}

$ErrorActionPreference = 'Stop'
$isCI = $env:GITHUB_ACTIONS -eq 'true' -or $env:CI -eq 'true'

# ────────────────────────────────
# 1) Elevation (skip in CI - runners have admin)
# ────────────────────────────────
if (-not $isCI) {
    if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
        ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-Warning "Restarting with Administrator privileges..."
        $args = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "`"$PSCommandPath`"")
        if ($GStreamerVersion) { $args += "-GStreamerVersion", $GStreamerVersion }
        if ($SkipGStreamer.IsPresent) { $args += "-SkipGStreamer" }
        if ($InstallVulkan.IsPresent) { $args += "-InstallVulkan" }
        Start-Process pwsh.exe -Verb RunAs -ArgumentList $args
        exit
    }
}

$tempDir = $env:TEMP
$arch = $env:PROCESSOR_ARCHITECTURE  # AMD64 | ARM64 | x86
Write-Host "`n==> Detected architecture: $arch"
Write-Host "==> Running in CI: $isCI"

# Helper function to set environment variables
function Set-EnvVar {
    param([string]$Name, [string]$Value)
    if ($isCI -and $env:GITHUB_ENV) {
        "$Name=$Value" >> $env:GITHUB_ENV
    } else {
        [Environment]::SetEnvironmentVariable($Name, $Value, 'Machine')
    }
    # Also set for current session
    Set-Item -Path "Env:$Name" -Value $Value
}

function Add-ToPath {
    param([string]$Path)
    if ($isCI -and $env:GITHUB_PATH) {
        $Path >> $env:GITHUB_PATH
    } else {
        $envPath = [Environment]::GetEnvironmentVariable('Path', 'Machine')
        if ($envPath -notmatch [regex]::Escape($Path)) {
            [Environment]::SetEnvironmentVariable('Path', "$envPath;$Path", 'Machine')
        }
    }
    # Also set for current session
    $env:Path = "$env:Path;$Path"
}

# ────────────────────────────────
# 2) GStreamer (x64 only)
# ────────────────────────────────
# Note: GStreamer MSVC packages include Vulkan plugin support.
# ARM64 users must use QtMultimedia VideoReceiver or build GStreamer manually.
$installGst = (-not $SkipGStreamer) -and ($arch -eq 'AMD64')

if ($installGst) {
    $gstPrefix = 'C:\gstreamer\1.0\msvc_x86_64'
    $gstBaseUrl = "https://gstreamer.freedesktop.org/data/pkg/windows/$GStreamerVersion/msvc"
    $gstRuntime = Join-Path $tempDir 'gstreamer-runtime.msi'
    $gstDevel = Join-Path $tempDir 'gstreamer-devel.msi'

    # Helper function to download with retry
    function Download-WithRetry {
        param([string]$Url, [string]$OutFile, [int]$MaxRetries = 3)
        for ($i = 1; $i -le $MaxRetries; $i++) {
            try {
                Write-Host "    Downloading: $Url (attempt $i/$MaxRetries)"
                Invoke-WebRequest $Url -OutFile $OutFile -UseBasicParsing
                return
            } catch {
                if ($i -eq $MaxRetries) { throw }
                Write-Host "    Retry in 5 seconds..."
                Start-Sleep -Seconds 5
            }
        }
    }

    Write-Host "`n==> Downloading GStreamer $GStreamerVersion..."
    Download-WithRetry "$gstBaseUrl/gstreamer-1.0-msvc-x86_64-$GStreamerVersion.msi" $gstRuntime
    Download-WithRetry "$gstBaseUrl/gstreamer-1.0-devel-msvc-x86_64-$GStreamerVersion.msi" $gstDevel

    Write-Host "==> Installing GStreamer runtime..."
    cmd /c start /wait msiexec /package "$gstRuntime" /passive ADDLOCAL=ALL
    Write-Host "==> Installing GStreamer devel..."
    cmd /c start /wait msiexec /package "$gstDevel" /passive ADDLOCAL=ALL

    Set-EnvVar 'GSTREAMER_1_0_ROOT_MSVC_X86_64' $gstPrefix
    Set-EnvVar 'GSTREAMER_1_0_ROOT_X86_64' $gstPrefix
    Add-ToPath "$gstPrefix\bin"

    Remove-Item $gstRuntime, $gstDevel -ErrorAction SilentlyContinue
    Write-Host "==> GStreamer $GStreamerVersion installed (includes Vulkan plugin support)"
}

# ────────────────────────────────
# 3) Vulkan SDK (optional - for validation layers only)
# ────────────────────────────────
if ($InstallVulkan) {
    $vulkanInstaller = Join-Path $tempDir 'vulkan-sdk.exe'
    $vulkanInstallDir = 'C:\VulkanSDK\latest'
    $vulkanUrl = 'https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe'

    Write-Host "`n==> Downloading Vulkan SDK (optional - for validation layers)..."
    Invoke-WebRequest $vulkanUrl -OutFile $vulkanInstaller

    Write-Host "==> Installing Vulkan SDK to $vulkanInstallDir..."
    Start-Process $vulkanInstaller `
        -ArgumentList "--root `"$vulkanInstallDir`" --accept-licenses --default-answer --confirm-command install com.lunarg.vulkan.glm com.lunarg.vulkan.volk com.lunarg.vulkan.vma com.lunarg.vulkan.debug" `
        -NoNewWindow -Wait

    Set-EnvVar 'VULKAN_SDK' $vulkanInstallDir
    Add-ToPath "$vulkanInstallDir\Bin"

    Remove-Item $vulkanInstaller -ErrorAction SilentlyContinue
    Write-Host "==> Vulkan SDK installed"
}

# ────────────────────────────────
# 4) Done
# ────────────────────────────────
Write-Host "`n==> Dependencies installed!"
if ($installGst) { Write-Host "    Verify GStreamer: gst-launch-1.0 --version" }
if ($InstallVulkan) { Write-Host "    Verify Vulkan: vulkaninfo | more" }
