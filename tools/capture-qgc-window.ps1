<#
.SYNOPSIS
    Capture the actual QGroundControl window, not the desktop.

.EXAMPLE
    .\tools\capture-qgc-window.ps1

.EXAMPLE
    .\tools\capture-qgc-window.ps1 -Width 1800 -Height 1100 -StopStartedProcess
#>

[CmdletBinding()]
param(
    [string]$ExePath,

    [string]$OutputPath,

    [int]$Width = 0,

    [int]$Height = 0,

    [int]$WaitSeconds = 12,

    [int]$ReadyTimeoutSeconds = 35,

    [int]$SettleSeconds = 0,

    [switch]$PreserveSize,

    [switch]$Maximize,

    [switch]$KeepStartupDialogs,

    [switch]$StopStartedProcess
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not $ExePath) {
    $ExePath = Join-Path $RepoRoot 'build\qgc-v5.0.8-ui\Debug\QGroundControl.exe'
}
$ExePath = [System.IO.Path]::GetFullPath($ExePath)

if (-not (Test-Path -LiteralPath $ExePath)) {
    throw "QGroundControl.exe was not found: $ExePath"
}

if (-not $OutputPath) {
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
    $OutputPath = Join-Path (Split-Path -Parent $ExePath) "qgc-window-$stamp.png"
}
$OutputPath = [System.IO.Path]::GetFullPath($OutputPath)

Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class QgcWindowCaptureNative {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll")]
    public static extern bool SetProcessDPIAware();

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool BringWindowToTop(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int X, int Y);

    [DllImport("user32.dll")]
    public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("dwmapi.dll")]
    public static extern int DwmGetWindowAttribute(IntPtr hwnd, int dwAttribute, out RECT pvAttribute, int cbAttribute);
}
'@

[QgcWindowCaptureNative]::SetProcessDPIAware() | Out-Null
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

function Get-QgcWindowRect {
    param([Parameter(Mandatory=$true)][IntPtr]$Handle)

    $rect = New-Object QgcWindowCaptureNative+RECT
    $dwmResult = [QgcWindowCaptureNative]::DwmGetWindowAttribute(
        $Handle,
        9, # DWMWA_EXTENDED_FRAME_BOUNDS
        [ref]$rect,
        [Runtime.InteropServices.Marshal]::SizeOf([type][QgcWindowCaptureNative+RECT])
    )

    if ($dwmResult -ne 0 -or (($rect.Right - $rect.Left) -le 0) -or (($rect.Bottom - $rect.Top) -le 0)) {
        [QgcWindowCaptureNative]::GetWindowRect($Handle, [ref]$rect) | Out-Null
    }

    return $rect
}

function Set-QgcWindowTopMost {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Handle,
        [Parameter(Mandatory=$true)][bool]$Enabled
    )

    $insertAfter = if ($Enabled) { [IntPtr](-1) } else { [IntPtr](-2) } # HWND_TOPMOST / HWND_NOTOPMOST
    [QgcWindowCaptureNative]::SetWindowPos(
        $Handle,
        $insertAfter,
        0,
        0,
        0,
        0,
        0x0001 -bor 0x0002 -bor 0x0040 # SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW
    ) | Out-Null
}

function Show-QgcWindowForCapture {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Handle,
        [bool]$Restore = $true
    )

    if ($Restore) {
        [QgcWindowCaptureNative]::ShowWindow($Handle, 9) | Out-Null # SW_RESTORE
    }
    Set-QgcWindowTopMost -Handle $Handle -Enabled $true
    [QgcWindowCaptureNative]::BringWindowToTop($Handle) | Out-Null
    [QgcWindowCaptureNative]::SetForegroundWindow($Handle) | Out-Null
    Start-Sleep -Milliseconds 400
}

function New-QgcWindowBitmap {
    param([Parameter(Mandatory=$true)]$Rect)

    $captureWidth = $Rect.Right - $Rect.Left
    $captureHeight = $Rect.Bottom - $Rect.Top
    if ($captureWidth -le 0 -or $captureHeight -le 0) {
        throw "Invalid QGroundControl window bounds: $($Rect.Left),$($Rect.Top),$($Rect.Right),$($Rect.Bottom)"
    }

    $bitmap = New-Object System.Drawing.Bitmap $captureWidth, $captureHeight
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($Rect.Left, $Rect.Top, 0, 0, [System.Drawing.Size]::new($captureWidth, $captureHeight))
        return $bitmap
    } catch {
        $bitmap.Dispose()
        throw
    } finally {
        $graphics.Dispose()
    }
}

function Get-QgcVoidSampleRatio {
    param([Parameter(Mandatory=$true)][System.Drawing.Bitmap]$Bitmap)

    $startX = [Math]::Max(0, [int]($Bitmap.Width * 0.18))
    $startY = [Math]::Max(0, [int]($Bitmap.Height * 0.08))
    $xStep = [Math]::Max(1, [int]($Bitmap.Width / 96))
    $yStep = [Math]::Max(1, [int]($Bitmap.Height / 72))
    $sampled = 0
    $void = 0

    for ($y = $startY; $y -lt $Bitmap.Height; $y += $yStep) {
        for ($x = $startX; $x -lt $Bitmap.Width; $x += $xStep) {
            $pixel = $Bitmap.GetPixel($x, $y)
            $sampled += 1
            if ($pixel.R -le 5 -and $pixel.G -le 5 -and $pixel.B -le 5) {
                $void += 1
            }
        }
    }

    if ($sampled -eq 0) {
        return 1.0
    }

    return $void / $sampled
}

function Find-QgcGreenDialogButton {
    param([Parameter(Mandatory=$true)][System.Drawing.Bitmap]$Bitmap)

    $minX = [int]($Bitmap.Width * 0.25)
    $maxX = [int]($Bitmap.Width * 0.75)
    $minY = [int]($Bitmap.Height * 0.30)
    $maxY = [int]($Bitmap.Height * 0.65)

    $foundMinX = $Bitmap.Width
    $foundMinY = $Bitmap.Height
    $foundMaxX = 0
    $foundMaxY = 0
    $count = 0

    for ($y = $minY; $y -lt $maxY; $y += 2) {
        for ($x = $minX; $x -lt $maxX; $x += 2) {
            $pixel = $Bitmap.GetPixel($x, $y)
            $looksLikeQgcOkGreen =
                $pixel.G -gt 145 -and
                $pixel.R -gt 25 -and $pixel.R -lt 120 -and
                $pixel.B -gt 55 -and $pixel.B -lt 180 -and
                $pixel.G -gt ($pixel.R * 1.6) -and
                $pixel.G -gt ($pixel.B * 1.2)

            $looksLikeQgcOkBlue =
                $pixel.B -gt 170 -and
                $pixel.G -gt 100 -and
                $pixel.R -lt 90 -and
                $pixel.B -gt ($pixel.G * 1.08) -and
                $pixel.G -gt ($pixel.R * 1.7)

            if ($looksLikeQgcOkGreen -or $looksLikeQgcOkBlue) {
                $foundMinX = [Math]::Min($foundMinX, $x)
                $foundMinY = [Math]::Min($foundMinY, $y)
                $foundMaxX = [Math]::Max($foundMaxX, $x)
                $foundMaxY = [Math]::Max($foundMaxY, $y)
                $count += 1
            }
        }
    }

    if ($count -lt 50) {
        return $null
    }

    return [PSCustomObject]@{
        X = [int](($foundMinX + $foundMaxX) / 2)
        Y = [int](($foundMinY + $foundMaxY) / 2)
        Width = $foundMaxX - $foundMinX
        Height = $foundMaxY - $foundMinY
        PixelCount = $count
    }
}

function Invoke-QgcMouseClick {
    param(
        [Parameter(Mandatory=$true)][int]$X,
        [Parameter(Mandatory=$true)][int]$Y
    )

    [QgcWindowCaptureNative]::SetCursorPos($X, $Y) | Out-Null
    Start-Sleep -Milliseconds 100
    [QgcWindowCaptureNative]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero) # MOUSEEVENTF_LEFTDOWN
    Start-Sleep -Milliseconds 80
    [QgcWindowCaptureNative]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero) # MOUSEEVENTF_LEFTUP
}

function Invoke-QgcFallbackDialogOkClick {
    param([Parameter(Mandatory=$true)]$Rect)

    $x = [int]($Rect.Left + (($Rect.Right - $Rect.Left) * 0.655))
    $y = [int]($Rect.Top + (($Rect.Bottom - $Rect.Top) * 0.488))
    Invoke-QgcMouseClick -X $x -Y $y
}

function Wait-QgcPaintedFrame {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Handle,
        [Parameter(Mandatory=$true)][int]$TimeoutSeconds,
        [bool]$Restore = $true
    )

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $lastRect = $null
    $lastVoidRatio = 1.0

    do {
        Show-QgcWindowForCapture -Handle $Handle -Restore:$Restore
        Start-Sleep -Milliseconds 750

        $rect = Get-QgcWindowRect -Handle $Handle
        $lastRect = $rect
        $bitmap = New-QgcWindowBitmap -Rect $rect
        try {
            $lastVoidRatio = Get-QgcVoidSampleRatio -Bitmap $bitmap
        } finally {
            $bitmap.Dispose()
        }

        if ($lastVoidRatio -lt 0.35) {
            return [PSCustomObject]@{
                Rect = $rect
                VoidSampleRatio = $lastVoidRatio
                Ready = $true
            }
        }

        Start-Sleep -Seconds 1
    } while ((Get-Date) -lt $deadline)

    return [PSCustomObject]@{
        Rect = $lastRect
        VoidSampleRatio = $lastVoidRatio
        Ready = $false
    }
}

function Dismiss-QgcStartupDialogs {
    param(
        [Parameter(Mandatory=$true)][IntPtr]$Handle,
        [bool]$Restore = $true
    )

    $deadline = (Get-Date).AddSeconds(12)
    $dismissed = 0
    $cleanSamples = 0

    while ((Get-Date) -lt $deadline) {
        Show-QgcWindowForCapture -Handle $Handle -Restore:$Restore
        $rect = Get-QgcWindowRect -Handle $Handle
        $bitmap = New-QgcWindowBitmap -Rect $rect
        try {
            $button = Find-QgcGreenDialogButton -Bitmap $bitmap
        } finally {
            $bitmap.Dispose()
        }

        if ($button) {
            Invoke-QgcMouseClick -X ($rect.Left + $button.X) -Y ($rect.Top + $button.Y)
            Invoke-QgcFallbackDialogOkClick -Rect $rect
            Start-Sleep -Milliseconds 150
            [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
            $dismissed += 1
            $cleanSamples = 0
            Start-Sleep -Milliseconds 1500
            continue
        }

        [System.Windows.Forms.SendKeys]::SendWait('{ESC}')
        $cleanSamples += 1
        if ($cleanSamples -ge 2) {
            Start-Sleep -Milliseconds 1200
            Show-QgcWindowForCapture -Handle $Handle -Restore:$Restore
            $verifyRect = Get-QgcWindowRect -Handle $Handle
            $verifyBitmap = New-QgcWindowBitmap -Rect $verifyRect
            try {
                $lateButton = Find-QgcGreenDialogButton -Bitmap $verifyBitmap
            } finally {
                $verifyBitmap.Dispose()
            }

            if (-not $lateButton) {
                break
            }

            Invoke-QgcMouseClick -X ($verifyRect.Left + $lateButton.X) -Y ($verifyRect.Top + $lateButton.Y)
            Invoke-QgcFallbackDialogOkClick -Rect $verifyRect
            Start-Sleep -Milliseconds 150
            [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
            $dismissed += 1
            $cleanSamples = 0
            Start-Sleep -Milliseconds 1500
        }

        Start-Sleep -Milliseconds 900
    }

    return [PSCustomObject]@{
        Dismissed = $dismissed
        CleanSamples = $cleanSamples
    }
}

$process = Get-Process -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and ([System.IO.Path]::GetFullPath($_.Path) -ieq $ExePath) -and $_.MainWindowHandle -ne 0 } |
    Select-Object -First 1

$startedByScript = $false
if (-not $process) {
    $process = Start-Process -FilePath $ExePath -WorkingDirectory (Split-Path -Parent $ExePath) -PassThru
    $startedByScript = $true
}

try {
    $handle = [IntPtr]::Zero
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    while ((Get-Date) -lt $deadline) {
        $process.Refresh()
        if ($process.HasExited) {
            throw "QGroundControl exited early with code $($process.ExitCode)"
        }
        if ($process.MainWindowHandle -ne 0) {
            $handle = $process.MainWindowHandle
            break
        }
        Start-Sleep -Milliseconds 250
    }

    if ($handle -eq [IntPtr]::Zero) {
        throw 'QGroundControl main window was not found.'
    }

    Show-QgcWindowForCapture -Handle $handle

    if ($Maximize) {
        [QgcWindowCaptureNative]::ShowWindow($handle, 3) | Out-Null # SW_MAXIMIZE
        Start-Sleep -Milliseconds 500
    } elseif (-not $PreserveSize) {
        $workArea = [System.Windows.Forms.Screen]::PrimaryScreen.WorkingArea
        $targetWidth = if ($Width -gt 0) { $Width } else { [Math]::Min(1800, $workArea.Width - 80) }
        $targetHeight = if ($Height -gt 0) { $Height } else { [Math]::Min(1100, $workArea.Height - 80) }
        $targetWidth = [Math]::Max(800, [Math]::Min($targetWidth, $workArea.Width - 40))
        $targetHeight = [Math]::Max(600, [Math]::Min($targetHeight, $workArea.Height - 40))
        [QgcWindowCaptureNative]::SetWindowPos(
            $handle,
            [IntPtr]::Zero,
            $workArea.Left + 20,
            $workArea.Top + 20,
            $targetWidth,
            $targetHeight,
            0x0040 # SWP_SHOWWINDOW
        ) | Out-Null
        Start-Sleep -Milliseconds 500
    }

    Show-QgcWindowForCapture -Handle $handle -Restore:(-not $Maximize)
    if ($SettleSeconds -gt 0) {
        Start-Sleep -Seconds $SettleSeconds
    }

    $readyState = Wait-QgcPaintedFrame -Handle $handle -TimeoutSeconds $ReadyTimeoutSeconds -Restore:(-not $Maximize)
    if (-not $readyState.Ready) {
        Write-Warning ("QGroundControl did not reach a fully painted frame before timeout. VoidSampleRatio={0:N3}" -f $readyState.VoidSampleRatio)
    }

    $dialogState = [PSCustomObject]@{ Dismissed = 0; CleanSamples = 0 }
    if (-not $KeepStartupDialogs) {
        $dialogState = Dismiss-QgcStartupDialogs -Handle $handle -Restore:(-not $Maximize)
        Start-Sleep -Seconds 1
    }

    Show-QgcWindowForCapture -Handle $handle -Restore:(-not $Maximize)
    $rect = Get-QgcWindowRect -Handle $handle
    $captureWidth = $rect.Right - $rect.Left
    $captureHeight = $rect.Bottom - $rect.Top
    if ($captureWidth -le 0 -or $captureHeight -le 0) {
        throw "Invalid QGroundControl window bounds: $($rect.Left),$($rect.Top),$($rect.Right),$($rect.Bottom)"
    }

    $screenBounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    if ($rect.Left -lt $screenBounds.Left -or
        $rect.Top -lt $screenBounds.Top -or
        $rect.Right -gt $screenBounds.Right -or
        $rect.Bottom -gt $screenBounds.Bottom) {
        throw "QGroundControl window is not fully inside the primary screen. Bounds=$($rect.Left),$($rect.Top),$($rect.Right),$($rect.Bottom); Screen=$($screenBounds.Left),$($screenBounds.Top),$($screenBounds.Right),$($screenBounds.Bottom). Run without -PreserveSize or reduce -Width/-Height."
    }

    $outputDir = Split-Path -Parent $OutputPath
    if (-not (Test-Path -LiteralPath $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir | Out-Null
    }

    if (-not $KeepStartupDialogs) {
        $finalCleanSamples = 0
        $finalDismissDeadline = (Get-Date).AddSeconds(18)
        while ((Get-Date) -lt $finalDismissDeadline) {
            Show-QgcWindowForCapture -Handle $handle -Restore:(-not $Maximize)
            $finalRect = Get-QgcWindowRect -Handle $handle
            $finalBitmap = New-QgcWindowBitmap -Rect $finalRect
            try {
                $finalButton = Find-QgcGreenDialogButton -Bitmap $finalBitmap
            } finally {
                $finalBitmap.Dispose()
            }

            if (-not $finalButton) {
                [System.Windows.Forms.SendKeys]::SendWait('{ESC}')
                $finalCleanSamples += 1
                if ($finalCleanSamples -ge 2) {
                    break
                }
                Start-Sleep -Milliseconds 900
                continue
            }

            Invoke-QgcMouseClick -X ($finalRect.Left + $finalButton.X) -Y ($finalRect.Top + $finalButton.Y)
            Invoke-QgcFallbackDialogOkClick -Rect $finalRect
            Start-Sleep -Milliseconds 150
            [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
            $dialogState.Dismissed += 1
            $finalCleanSamples = 0
            Start-Sleep -Milliseconds 1700
        }
    }

    Show-QgcWindowForCapture -Handle $handle -Restore:(-not $Maximize)
    $rect = Get-QgcWindowRect -Handle $handle

    $bitmap = New-QgcWindowBitmap -Rect $rect
    try {
        $bitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $bitmap.Dispose()
    }

    [PSCustomObject]@{
        Path = $OutputPath
        Width = $captureWidth
        Height = $captureHeight
        Left = $rect.Left
        Top = $rect.Top
        ProcessId = $process.Id
        StartedByScript = $startedByScript
        Ready = $readyState.Ready
        VoidSampleRatio = ('{0:N3}' -f $readyState.VoidSampleRatio)
        DialogsDismissed = $dialogState.Dismissed
    } | Format-List
} finally {
    if ($handle -ne [IntPtr]::Zero) {
        Set-QgcWindowTopMost -Handle $handle -Enabled $false
    }
    if ($startedByScript -and $StopStartedProcess -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
    }
}
