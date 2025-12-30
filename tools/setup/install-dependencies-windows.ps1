<#
.SYNOPSIS
    Install GStreamer (x64 only) and the latest Vulkan SDK on Windows.

.DESCRIPTION
    • Must run elevated.
    • Downloads installers to $env:TEMP and removes them afterwards.
    • Sets the required environment variables and updates PATH.
#>

# ────────────────────────────────
# 1) Elevation
# ────────────────────────────────
if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()
    ).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "Restarting with Administrator privileges…"
    Start-Process pwsh.exe -Verb RunAs -ArgumentList "-NoProfile","-ExecutionPolicy Bypass","-File `"$PSCommandPath`""
    exit
}

$ErrorActionPreference = 'Stop'
$tempDir   = $env:TEMP
$arch      = $env:PROCESSOR_ARCHITECTURE     # AMD64 | ARM64 | x86
Write-Host "`n==> Detected architecture: $arch"

# ────────────────────────────────
# 2) GStreamer config (x64 only)
# ────────────────────────────────
# Note: Must Use QtMultimedia VideoReceiver or build GStreamer manually for Arm64
$installGst = $arch -eq 'AMD64'              # treat “x64/AMD64” as the only GS­treamer target
if ($installGst) {
    $gstVersion  = '1.22.12'
    $gstPrefix   = 'C:\gstreamer\1.0\msvc_x86_64'
    $gstBaseUrl  = "https://gstreamer.freedesktop.org/data/pkg/windows/$gstVersion/msvc"
    $gstRuntime  = Join-Path $tempDir 'gstreamer-runtime.msi'
    $gstDevel    = Join-Path $tempDir 'gstreamer-devel.msi'
}

# ────────────────────────────────
# 3) Vulkan config (all arch)
# ────────────────────────────────
$vulkanInstaller  = Join-Path $tempDir 'vulkan-sdk.exe'
$vulkanInstallDir = 'C:\VulkanSDK\latest'
$vulkanUrl        = 'https://sdk.lunarg.com/sdk/download/latest/windows/vulkan-sdk.exe'

# ────────────────────────────────
# 4) Download + install GStreamer
# ────────────────────────────────
if ($installGst) {
    Write-Host "`n==> Downloading GStreamer $gstVersion…"
    Invoke-WebRequest "$gstBaseUrl/gstreamer-1.0-msvc-x86_64-$gstVersion.msi" -OutFile $gstRuntime
    Invoke-WebRequest "$gstBaseUrl/gstreamer-1.0-devel-msvc-x86_64-$gstVersion.msi" -OutFile $gstDevel

    Write-Host "==> Installing GStreamer runtime…"
    Start-Process msiexec.exe -ArgumentList "/i `"$gstRuntime`" /passive ADDLOCAL=ALL TARGETDIR=`"$gstPrefix`"" -Wait
    Write-Host "==> Installing GStreamer devel…"
    Start-Process msiexec.exe -ArgumentList "/i `"$gstDevel`"   /passive ADDLOCAL=ALL TARGETDIR=`"$gstPrefix`"" -Wait

    # Environment
    [Environment]::SetEnvironmentVariable('GSTREAMER_1_0_ROOT_MSVC_X86_64', $gstPrefix, 'Machine')
    [Environment]::SetEnvironmentVariable('GSTREAMER_1_0_ROOT_X86_64',       $gstPrefix, 'Machine')
    $envPath = [Environment]::GetEnvironmentVariable('Path','Machine')
    if ($envPath -notmatch [regex]::Escape("$gstPrefix\bin")) {
        [Environment]::SetEnvironmentVariable('Path', "$envPath;$gstPrefix\bin", 'Machine')
    }
}

# ────────────────────────────────
# 5) Download + install Vulkan SDK
# ────────────────────────────────
Write-Host "`n==> Downloading Vulkan SDK…"
Invoke-WebRequest $vulkanUrl -OutFile $vulkanInstaller

Write-Host "==> Installing Vulkan SDK to $vulkanInstallDir…"
Start-Process $vulkanInstaller `
    -ArgumentList "--root `"$vulkanInstallDir`" --accept-licenses --default-answer --confirm-command install com.lunarg.vulkan.glm com.lunarg.vulkan.volk com.lunarg.vulkan.vma com.lunarg.vulkan.debug" `
    -NoNewWindow -Wait

[Environment]::SetEnvironmentVariable('VULKAN_SDK', $vulkanInstallDir, 'Machine')
$vkBinPaths = @("$vulkanInstallDir\Bin", "$vulkanInstallDir\Bin\%PlatformName%")
$envPath = [Environment]::GetEnvironmentVariable('Path','Machine')
foreach ($p in $vkBinPaths) {
    if ($envPath -notmatch [regex]::Escape($p)) {
        $envPath += ";$p"
    }
}
[Environment]::SetEnvironmentVariable('Path', $envPath, 'Machine')

# ────────────────────────────────
# 6) Cleanup
# ────────────────────────────────
Write-Host "`n==> Cleaning up installers…"
if ($installGst) { Remove-Item $gstRuntime, $gstDevel -ErrorAction SilentlyContinue }
Remove-Item $vulkanInstaller            -ErrorAction SilentlyContinue

# ────────────────────────────────
# 7) Done
# ────────────────────────────────
Write-Host "`n✅ Dependencies installed!"
if ($installGst) { Write-Host "   • Verify GStreamer: gst-launch-1.0 --version" }
Write-Host "   • Verify Vulkan:    vulkaninfo | more"
