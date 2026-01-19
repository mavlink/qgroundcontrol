<#
.SYNOPSIS
    Install GStreamer and the latest Vulkan SDK on Windows.

.DESCRIPTION
    - Supports both AMD64 (x64) and ARM64 architectures.
    - AMD64: Downloads MSI installers from QGC S3 bucket.
    - ARM64: Downloads ZIP archive from QGC S3 bucket (built from source).
    - In CI (GitHub Actions): runs without elevation, uses GITHUB_ENV for env vars.
    - Locally: requires elevation, sets machine-level environment variables.
    - Downloads installers to $env:TEMP and removes them afterwards.

.PARAMETER SkipVulkan
    Skip Vulkan SDK installation.

.PARAMETER SkipGStreamer
    Skip GStreamer installation.

.PARAMETER GStreamerVersion
    Override GStreamer version (default: from build-config.json).
#>
param(
    [switch]$SkipVulkan,
    [switch]$SkipGStreamer,
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
# 3) GStreamer config
# ────────────────────────────────
$installGst = -not $SkipGStreamer -and ($arch -eq 'AMD64' -or $arch -eq 'ARM64')
$gstBaseUrl = "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/windows/$gstVersion"

if ($installGst) {
    if ($arch -eq 'AMD64') {
        # x64: Use MSI installers
        $gstInstallDir = 'C:\gstreamer'
        $gstPrefix     = 'C:\gstreamer\1.0\msvc_x86_64'
        $gstRuntime    = Join-Path $tempDir 'gstreamer-runtime.msi'
        $gstDevel      = Join-Path $tempDir 'gstreamer-devel.msi'
        $gstEnvVar     = 'GSTREAMER_1_0_ROOT_MSVC_X86_64'
    } elseif ($arch -eq 'ARM64') {
        # ARM64: Use ZIP archive (built from source)
        $gstInstallDir = 'C:\gstreamer-arm64'
        $gstPrefix     = 'C:\gstreamer-arm64'
        $gstArchive    = Join-Path $tempDir 'gstreamer-arm64.zip'
        $gstEnvVar     = 'GSTREAMER_1_0_ROOT_MSVC_ARM64'
    }
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
    Write-Host "`n==> Installing GStreamer $gstVersion for $arch"

    if ($arch -eq 'AMD64') {
        # AMD64: Download and install MSI packages
        Write-Host "==> Downloading GStreamer MSI installers from $gstBaseUrl"
        Invoke-WebRequest "$gstBaseUrl/gstreamer-1.0-msvc-x86_64-$gstVersion.msi" -OutFile $gstRuntime
        Invoke-WebRequest "$gstBaseUrl/gstreamer-1.0-devel-msvc-x86_64-$gstVersion.msi" -OutFile $gstDevel

        Write-Host "==> Installing GStreamer runtime..."
        Start-Process msiexec.exe -ArgumentList "/i `"$gstRuntime`" /passive INSTALLDIR=`"$gstInstallDir`" ADDLOCAL=ALL" -Wait
        Write-Host "==> Installing GStreamer devel..."
        Start-Process msiexec.exe -ArgumentList "/i `"$gstDevel`" /passive INSTALLDIR=`"$gstInstallDir`" ADDLOCAL=ALL" -Wait

        # Cleanup MSI files
        Remove-Item $gstRuntime, $gstDevel -ErrorAction SilentlyContinue

        # Set legacy environment variable for compatibility
        Set-EnvVar 'GSTREAMER_1_0_ROOT_X86_64' $gstPrefix

    } elseif ($arch -eq 'ARM64') {
        # ARM64: Download and extract ZIP archive
        Write-Host "==> Downloading GStreamer ARM64 archive from $gstBaseUrl"
        $gstZipUrl = "$gstBaseUrl/gstreamer-1.0-msvc-arm64-$gstVersion.zip"

        try {
            Invoke-WebRequest $gstZipUrl -OutFile $gstArchive
        } catch {
            Write-Warning "GStreamer ARM64 not found at $gstZipUrl"
            Write-Warning "ARM64 GStreamer must be built first using: tools/setup/gstreamer/build-gstreamer.py --platform windows"
            Write-Warning "Skipping GStreamer installation."
            $installGst = $false
        }

        if ($installGst) {
            Write-Host "==> Extracting GStreamer to $gstInstallDir..."
            if (Test-Path $gstInstallDir) {
                Remove-Item -Recurse -Force $gstInstallDir
            }
            Expand-Archive -Path $gstArchive -DestinationPath $gstInstallDir -Force

            # Handle nested directory if archive contains root folder
            $nested = Get-ChildItem $gstInstallDir -Directory | Select-Object -First 1
            if ($nested -and (Test-Path "$($nested.FullName)\bin")) {
                # Move contents up one level
                Get-ChildItem $nested.FullName | Move-Item -Destination $gstInstallDir -Force
                Remove-Item $nested.FullName -Force
            }

            # Cleanup ZIP file
            Remove-Item $gstArchive -ErrorAction SilentlyContinue
        }
    }

    if ($installGst) {
        # Set environment variables
        Set-EnvVar $gstEnvVar $gstPrefix
        Add-ToPath "$gstPrefix\bin"
    }
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

    # Cleanup
    Remove-Item $vulkanInstaller -ErrorAction SilentlyContinue
}

# ────────────────────────────────
# 7) Done
# ────────────────────────────────
Write-Host "`nDependencies installed!"
if ($installGst) { Write-Host "  - GStreamer $gstVersion ($arch) at $gstPrefix" }
if ($installVulkan) { Write-Host "  - Vulkan SDK at $vulkanInstallDir" }
