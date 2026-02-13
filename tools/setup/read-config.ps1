<#
.SYNOPSIS
    Read build configuration from .github/build-config.json

.DESCRIPTION
    Thin delegation script that calls read_config.py for all functionality.
    Dot-source this script to export variables, or call with -Get/-Json to query config.

.PARAMETER Get
    Get a single value by key name

.PARAMETER Json
    Output full config as JSON

.EXAMPLE
    . .\read-config.ps1              # Import all variables
    .\read-config.ps1 -Get qt_version  # Get single value

.NOTES
    See read_config.py for full documentation.
#>

param(
    [string]$Get,
    [switch]$Json
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pythonScript = Join-Path $scriptDir "read_config.py"

# Find Python 3
function Find-Python {
    foreach ($py in @("python3", "python")) {
        try {
            $version = & $py --version 2>&1
            if ($version -match "Python 3") {
                return $py
            }
        } catch {
            # Not found, try next
        }
    }
    return $null
}

$python = Find-Python
if (-not $python) {
    Write-Error "Error: Python 3 is required"
    exit 1
}

# Handle -Get and -Json: delegate to Python
if ($Get) {
    & $python $pythonScript --get $Get
    exit $LASTEXITCODE
}

if ($Json) {
    & $python $pythonScript --json
    exit $LASTEXITCODE
}

# For dot-sourcing: evaluate PowerShell export statements from Python
$exports = & $python $pythonScript --export powershell
foreach ($line in $exports -split "`n") {
    if ($line -match '^\$env:(\w+)\s*=\s*"(.+)"$') {
        $varName = $matches[1]
        $varValue = $matches[2]
        Set-Item -Path "env:$varName" -Value $varValue -Force
    }
}
