<#
.SYNOPSIS
    Install GStreamer (x64 only) and the latest Vulkan SDK on Windows.

.DESCRIPTION
    - In CI (GitHub Actions): runs without elevation, uses GITHUB_ENV for env vars.
    - Locally: requires elevation, sets machine-level environment variables.
    - Downloads installers to $env:TEMP and removes them afterwards.

.PARAMETER SkipVulkan
    Skip Vulkan SDK installation.

.PARAMETER GStreamerVersion
    Override GStreamer version (default: from build-config.json).
#>
param(
    [switch]$SkipVulkan,
    [string]$GStreamerVersion
)

# ────────────────────────────────
# 0) Detect CI environment
# ────────────────────────────────
$isCI = $env:CI -eq 'true' -or $env:GITHUB_ACTIONS -eq 'true'

# ────────────────────────────────
# 1) Source build config
# ────────────────────────────────
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$configScript = Join-Path $scriptDir "read-config.ps1"
if (Test-Path $configScript) {
    . $configScript
} else {
    Write-Error "read-config.ps1 not found"
    exit 1
}

# Use parameter override or environment variable
if ($GStreamerVersion) {
    $gstVersion = $GStreamerVersion
} elseif ($env:GSTREAMER_WINDOWS_VERSION) {
    $gstVersion = $env:GSTREAMER_WINDOWS_VERSION
} else {
    Write-Error "GStreamer version not specified (use -GStreamerVersion or check build-config.json)"
    exit 1
}

# ────────────────────────────────
# 2) Elevation (local only)
# ────────────────────────────────
if (-not $isCI) {
    if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
        ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-Warning "Restarting with Administrator privileges..."
        Start-Process pwsh.exe -Verb RunAs -ArgumentList "-NoProfile","-ExecutionPolicy Bypass","-File `"$PSCommandPath`""
        exit
    }
}

$ErrorActionPreference = 'Stop'
$tempDir   = $env:TEMP
$arch      = $env:PROCESSOR_ARCHITECTURE     # AMD64 | ARM64 | x86
Write-Host "`n==> Detected architecture: $arch"

# ────────────────────────────────
# 3) GStreamer config (x64 only)
# ────────────────────────────────
# Note: Must use QtMultimedia VideoReceiver or build GStreamer manually for Arm64
$installGst = $arch -eq 'AMD64'
if ($installGst) {
    $gstInstallDir = 'C:\gstreamer'  # MSI INSTALLDIR (root)
    $gstPrefix     = 'C:\gstreamer\1.0\msvc_x86_64'  # Actual path after install
    # We now download from S3 because the official GStreamer site is unreliable
    $gstBaseUrl    = "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/windows/$gstVersion"
    $gstRuntime    = Join-Path $tempDir 'gstreamer-runtime.msi'
    $gstDevel      = Join-Path $tempDir 'gstreamer-devel.msi'
}

# ────────────────────────────────
# 4) Vulkan config (all arch, optional)
# ────────────────────────────────
$installVulkan = -not $SkipVulkan
if ($installVulkan) {
    $vulkanInstaller  = Join-Path $tempDir 'vulkan-sdk.exe'
    $vulkanInstallDir = 'C:\VulkanSDK\latest'
    $vulkanUrl        = 'https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe'
}

# Helper function to set environment variables (CI vs local)
function Set-EnvVar {
    param([string]$Name, [string]$Value)
    if ($isCI) {
        # GitHub Actions: append to GITHUB_ENV
        "$Name=$Value" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
        # Also set for current process
        [Environment]::SetEnvironmentVariable($Name, $Value, 'Process')
    } else {
        # Local: set machine-level
        [Environment]::SetEnvironmentVariable($Name, $Value, 'Machine')
    }
}

function Add-ToPath {
    param([string]$PathToAdd)
    if ($isCI) {
        $PathToAdd | Out-File -FilePath $env:GITHUB_PATH -Append -Encoding utf8
    } else {
        $envPath = [Environment]::GetEnvironmentVariable('Path', 'Machine')
        if ($envPath -notmatch [regex]::Escape($PathToAdd)) {
            [Environment]::SetEnvironmentVariable('Path', "$envPath;$PathToAdd", 'Machine')
        }
    }
}

# ────────────────────────────────
# 5) Download + install GStreamer
# ────────────────────────────────
if ($installGst) {
    Write-Host "`n==> Downloading GStreamer version $gstVersion from $gstBaseUrl"
    Invoke-WebRequest "$gstBaseUrl/gstreamer-1.0-msvc-x86_64-$gstVersion.msi" -OutFile $gstRuntime
    Invoke-WebRequest "$gstBaseUrl/gstreamer-1.0-devel-msvc-x86_64-$gstVersion.msi" -OutFile $gstDevel

    Write-Host "==> Installing GStreamer runtime..."
    Start-Process msiexec.exe -ArgumentList "/i `"$gstRuntime`" /passive INSTALLDIR=`"$gstInstallDir`" ADDLOCAL=ALL" -Wait
    Write-Host "==> Installing GStreamer devel..."
    Start-Process msiexec.exe -ArgumentList "/i `"$gstDevel`" /passive INSTALLDIR=`"$gstInstallDir`" ADDLOCAL=ALL" -Wait

    # Environment
    Set-EnvVar 'GSTREAMER_1_0_ROOT_MSVC_X86_64' $gstPrefix
    Set-EnvVar 'GSTREAMER_1_0_ROOT_X86_64' $gstPrefix
    Add-ToPath "$gstPrefix\bin"
}

# ────────────────────────────────
# 6) Download + install Vulkan SDK
# ────────────────────────────────
if ($installVulkan) {
    Write-Host "`n==> Downloading Vulkan SDK..."
    Invoke-WebRequest $vulkanUrl -OutFile $vulkanInstaller

    Write-Host "==> Installing Vulkan SDK to $vulkanInstallDir..."
    Start-Process $vulkanInstaller `
        -ArgumentList "--root `"$vulkanInstallDir`" --accept-licenses --default-answer --confirm-command install com.lunarg.vulkan.glm com.lunarg.vulkan.volk com.lunarg.vulkan.vma com.lunarg.vulkan.debug" `
        -NoNewWindow -Wait

    Set-EnvVar 'VULKAN_SDK' $vulkanInstallDir
    Add-ToPath "$vulkanInstallDir\Bin"
}

# ────────────────────────────────
# 7) Cleanup
# ────────────────────────────────
Write-Host "`n==> Cleaning up installers..."
if ($installGst) { Remove-Item $gstRuntime, $gstDevel -ErrorAction SilentlyContinue }
if ($installVulkan) { Remove-Item $vulkanInstaller -ErrorAction SilentlyContinue }

# ────────────────────────────────
# 8) Done
# ────────────────────────────────
Write-Host "`nDependencies installed!"
if ($installGst) { Write-Host "  - GStreamer $gstVersion (verify: gst-launch-1.0 --version)" }
if ($installVulkan) { Write-Host "  - Vulkan SDK (verify: vulkaninfo)" }
