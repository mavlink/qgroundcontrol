<#
.SYNOPSIS
    Download and install GStreamer runtime & devel on Windows.

.DESCRIPTION
    - Must be run as Administrator.
    - Downloads the runtime & dev MSIs from your S3 bucket into $env:TEMP.
    - Installs both with msiexec in passive mode, adding all components.
    - Cleans up the downloaded files.
#>

# Ensure we’re running elevated
if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "This script must be run as Administrator. Relaunching..."
    Start-Process -FilePath pwsh.exe -Verb RunAs -ArgumentList "-NoProfile","-ExecutionPolicy Bypass","-File `"$PSCommandPath`""
    exit
}

# Configuration
$version = "1.22.12"
$baseUrl = "https://gstreamer.freedesktop.org/data/pkg/windows/1.22.12/msvc"
$tempDir = $env:TEMP

$runtimeMsi = Join-Path $tempDir "gstreamer-1.0-msvc-x86_64-$version.msi"
$develMsi   = Join-Path $tempDir "gstreamer-1.0-devel-msvc-x86_64-$version.msi"

# Download
Write-Host "Downloading GStreamer $version runtime..."
Invoke-WebRequest -Uri "$baseUrl/gstreamer-1.0-msvc-x86_64-$version.msi" -OutFile $runtimeMsi

Write-Host "Downloading GStreamer $version devel..."
Invoke-WebRequest -Uri "$baseUrl/gstreamer-1.0-devel-msvc-x86_64-$version.msi" -OutFile $develMsi

# Install
Write-Host "Installing GStreamer runtime..."
Start-Process msiexec.exe -ArgumentList "/i `"$runtimeMsi`" /passive ADDLOCAL=ALL" -Wait

Write-Host "Installing GStreamer devel..."
Start-Process msiexec.exe -ArgumentList "/i `"$develMsi`" /passive ADDLOCAL=ALL" -Wait

# Cleanup
Write-Host "Removing installer files..."
Remove-Item $runtimeMsi, $develMsi -ErrorAction SilentlyContinue

Write-Host "`n✅ GStreamer $version installed!"
Write-Host "You can verify with: gst-launch-1.0 --version"
