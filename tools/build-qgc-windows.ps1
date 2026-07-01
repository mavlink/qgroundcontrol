<#
.SYNOPSIS
    Configure, build, deploy, run, or clean QGroundControl on Windows.

.EXAMPLE
    .\tools\build-qgc-windows.ps1

.EXAMPLE
    .\tools\build-qgc-windows.ps1 -Action all

.EXAMPLE
    .\tools\build-qgc-windows.ps1 -Action run
#>

[CmdletBinding()]
param(
    [ValidateSet('configure', 'build', 'deploy', 'run', 'clean', 'rebuild', 'all')]
    [string]$Action = 'build',

    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Config = 'Debug',

    [string]$QtRoot = $env:QT_ROOT_DIR,

    [string]$QtVersion = $(if ($env:QT_VERSION) { $env:QT_VERSION } else { '6.8.3' }),

    [string[]]$QtInstallRoot = @(
        $(if ($env:QT_PATH) { $env:QT_PATH } else { $null }),
        $(if ($env:QTDIR) { $env:QTDIR } else { $null }),
        $(Join-Path $env:SystemDrive 'Qt'),
        $(Join-Path $env:USERPROFILE 'Qt')
    ),

    [string]$BuildDir,

    [switch]$EnableGStreamer
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot 'build\qgc-v5.0.8-ui'
}

function Resolve-FullPath {
    param([Parameter(Mandatory=$true)][string]$Path)
    return [System.IO.Path]::GetFullPath($Path)
}

function Resolve-QtRoot {
    param([string]$Candidate)

    $candidates = @()
    if (-not [string]::IsNullOrWhiteSpace($Candidate)) {
        $candidates += $Candidate
    }

    foreach ($root in $QtInstallRoot | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique) {
        if (-not (Test-Path -LiteralPath $root)) {
            continue
        }

        $rootToolchain = Join-Path $root 'lib\cmake\Qt6\qt.toolchain.cmake'
        if (Test-Path -LiteralPath $rootToolchain) {
            $candidates += $root
            continue
        }

        $versionDir = Join-Path $root $QtVersion
        if (Test-Path -LiteralPath $versionDir) {
            $candidates += Get-ChildItem -LiteralPath $versionDir -Directory |
                Where-Object { $_.Name -match 'msvc|mingw' } |
                ForEach-Object { $_.FullName }
        }
    }

    foreach ($path in $candidates | Select-Object -Unique) {
        $toolchain = Join-Path $path 'lib\cmake\Qt6\qt.toolchain.cmake'
        if (Test-Path -LiteralPath $toolchain) {
            return (Resolve-FullPath $path)
        }
    }

    throw "Qt $QtVersion toolchain was not found. Set QT_ROOT_DIR or pass -QtRoot."
}

function Resolve-VsDevShell {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (-not [string]::IsNullOrWhiteSpace($installPath)) {
            $candidate = Join-Path $installPath 'Common7\Tools\Launch-VsDevShell.ps1'
            if (Test-Path -LiteralPath $candidate) {
                return $candidate
            }
        }
    }

    $roots = @(
        (Join-Path $env:ProgramFiles 'Microsoft Visual Studio'),
        (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio')
    )

    foreach ($root in $roots) {
        if (-not (Test-Path -LiteralPath $root)) {
            continue
        }

        $candidate = Get-ChildItem -LiteralPath $root -Recurse -Filter Launch-VsDevShell.ps1 -ErrorAction SilentlyContinue |
            Select-Object -First 1 -ExpandProperty FullName
        if ($candidate) {
            return $candidate
        }
    }

    throw 'Visual Studio Developer PowerShell was not found. Install MSVC C++ tools.'
}

function Initialize-BuildEnvironment {
    $script:QtRootResolved = Resolve-QtRoot $QtRoot
    $script:VsDevShell = Resolve-VsDevShell

    $env:QT_ROOT_DIR = $script:QtRootResolved
    $qtBin = Join-Path $script:QtRootResolved 'bin'
    if (-not ($env:Path -split ';' | Where-Object { $_ -ieq $qtBin })) {
        $env:Path = "$qtBin;$env:Path"
    }

    & $script:VsDevShell -Arch amd64 -HostArch amd64
}

function Invoke-Configure {
    Initialize-BuildEnvironment

    $toolchain = Join-Path $script:QtRootResolved 'lib\cmake\Qt6\qt.toolchain.cmake'
    $gstValue = if ($EnableGStreamer) { 'ON' } else { 'OFF' }

    Write-Host "Source:    $RepoRoot"
    Write-Host "Build:     $BuildDir"
    Write-Host "Qt:        $script:QtRootResolved"
    Write-Host "Config:    $Config"
    Write-Host "GStreamer: $gstValue"

    & cmake -S $RepoRoot -B $BuildDir -G 'Ninja Multi-Config' `
        "-DCMAKE_TOOLCHAIN_FILE=$toolchain" `
        "-DCMAKE_CONFIGURATION_TYPES=Debug;Release" `
        "-DQGC_ENABLE_GST_VIDEOSTREAMING=$gstValue"
}

function Invoke-Build {
    Invoke-Configure
    & cmake --build $BuildDir --config $Config --parallel
}

function Invoke-Deploy {
    Initialize-BuildEnvironment

    $exe = Join-Path $BuildDir "$Config\QGroundControl.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "QGroundControl.exe was not found at $exe. Build first."
    }

    $deployMode = if ($Config -eq 'Debug') { '--debug' } else { '--release' }
    & (Join-Path $script:QtRootResolved 'bin\windeployqt.exe') `
        $deployMode `
        --compiler-runtime `
        --qmldir $RepoRoot `
        $exe
}

function Invoke-Run {
    Initialize-BuildEnvironment

    $exe = Join-Path $BuildDir "$Config\QGroundControl.exe"
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "QGroundControl.exe was not found at $exe. Build first."
    }

    Push-Location (Split-Path -Parent $exe)
    try {
        & $exe
    } finally {
        Pop-Location
    }
}

function Invoke-Clean {
    $buildRoot = Resolve-FullPath (Join-Path $RepoRoot 'build')
    $target = Resolve-FullPath $BuildDir

    if (-not $target.StartsWith($buildRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove build directory outside repository build root: $target"
    }

    if (Test-Path -LiteralPath $target) {
        Remove-Item -LiteralPath $target -Recurse -Force
        Write-Host "Removed $target"
    } else {
        Write-Host "Build directory does not exist: $target"
    }
}

switch ($Action) {
    'configure' { Invoke-Configure }
    'build'     { Invoke-Build }
    'deploy'    { Invoke-Deploy }
    'run'       { Invoke-Run }
    'clean'     { Invoke-Clean }
    'rebuild'   { Invoke-Clean; Invoke-Build; Invoke-Deploy }
    'all'       { Invoke-Build; Invoke-Deploy }
}
