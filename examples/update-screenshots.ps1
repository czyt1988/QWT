<#
.SYNOPSIS
    Regenerates the documentation screenshots of the Qwt examples.

.DESCRIPTION
    Runs the example executables listed in examples/screenshots-manifest.json with
    the unattended screenshot protocol (examples/common/qwt_example_screenshot.h)
    and writes the PNGs into docs/assets/screenshots{,-3D}.

    The examples must be built before running this script (build.ps1 or
    cmake --build). Windows pop up briefly for each capture - do not move the
    mouse or type while the script runs.

.PARAMETER BuildDir
    CMake build directory containing bin/<example>.exe. Default: <repo>/build

.PARAMETER Only
    Wildcard filter on the example name, e.g. -Only "barchart" or -Only "*3D*".
    Default: run all entries of the manifest.

.PARAMETER Build
    Build the required example targets with CMake before capturing.

.PARAMETER Manifest
    Path to the manifest JSON. Default: <repo>/examples/screenshots-manifest.json

.EXAMPLE
    powershell -File examples/update-screenshots.ps1
    powershell -File examples/update-screenshots.ps1 -Only spectrogram -BuildDir D:\qwt\build
#>
[CmdletBinding()]
param(
    [string]$BuildDir = "",
    [string]$Only = "",
    [switch]$Build,
    [string]$Manifest = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoDir = Split-Path -Parent $scriptDir
if (-not $BuildDir) { $BuildDir = Join-Path $repoDir "build" }
if (-not $Manifest) { $Manifest = Join-Path $scriptDir "screenshots-manifest.json" }
$BuildDir = (Resolve-Path $BuildDir).Path
$assetsDir = Join-Path $repoDir "docs\assets"

if (-not (Test-Path $Manifest)) { throw "Manifest not found: $Manifest" }
if (-not (Test-Path $BuildDir)) { throw "Build directory not found: $BuildDir (build the examples first)" }
$manifestJson = Get-Content -Raw -Encoding UTF8 $Manifest | ConvertFrom-Json
$defaultDelay = 2000
$timeoutPadSec = 20
if ($manifestJson.defaults) {
    if ($manifestJson.defaults.delay) { $defaultDelay = [int]$manifestJson.defaults.delay }
    if ($manifestJson.defaults.timeoutPadSec) { $timeoutPadSec = [int]$manifestJson.defaults.timeoutPadSec }
}

# ---------------------------------------------------------------------------
# Environment: locate Qt binaries and the cmake used for this build tree, so
# that the example executables find their DLLs and -Build works out of the box.
# ---------------------------------------------------------------------------
$cacheFile = Join-Path $BuildDir "CMakeCache.txt"
$cmakeExe = "cmake"
$qtBin = ""
if (Test-Path $cacheFile) {
    foreach ($line in Get-Content $cacheFile) {
        if ($line -match "^CMAKE_COMMAND:INTERNAL=(.+)$") { $cmakeExe = $Matches[1].Trim() }
        if ($line -match "^CMAKE_PREFIX_PATH:PATH=(.+)$") {
            foreach ($p in $Matches[1].Split(';')) {
                if ($p -and (Test-Path (Join-Path $p "bin"))) { $qtBin = Join-Path $p "bin"; break }
            }
        }
    }
}
$binDir = Join-Path $BuildDir "bin"
if (-not (Test-Path $binDir)) { throw "No bin directory in $BuildDir" }
$env:PATH = "$binDir;$env:PATH"
if ($qtBin) { $env:PATH = "$qtBin;$env:PATH" }

# Deterministic output size: force device pixel ratio 1 regardless of the
# desktop scaling, so --size WxH produces a WxH PNG.
$env:QT_SCREEN_SCALE_FACTORS = "1"
$env:QT_SCALE_FACTOR = "1"

function Resolve-ExampleExe([string]$name) {
    $candidates = @(
        (Join-Path $binDir "$name.exe"),
        (Join-Path $binDir "${name}d.exe")
    )
    # multi-config generators (Visual Studio) may use per-config subdirectories
    foreach ($cfg in @("Release", "Debug", "RelWithDebInfo")) {
        $candidates += (Join-Path $binDir "$cfg\$name.exe")
        $candidates += (Join-Path $binDir "$cfg\${name}d.exe")
    }
    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
    return $null
}

function Invoke-BuildTarget([string]$name) {
    Write-Host "  building target '$name' ..." -ForegroundColor DarkGray
    $genArgs = @("--build", $BuildDir, "--target", $name)
    $cacheGen = Select-String -Path $cacheFile -Pattern "^CMAKE_GENERATOR:INTERNAL=(.+)$" -ErrorAction SilentlyContinue
    if ($cacheGen -and $cacheGen.Matches[0].Groups[1].Value -match "Visual Studio|Xcode|Ninja Multi") {
        $genArgs += @("--config", "Release")
    }
    & $cmakeExe @genArgs | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "cmake build failed for target '$name'" }
}

function Invoke-ExampleProcess([string]$exe, [string[]]$exeArgs, [int]$timeoutMs) {
    $tmpOut = Join-Path $env:TEMP ("qwtshot_" + [guid]::NewGuid().ToString("N") + ".png")
    $fullArgs = @()
    foreach ($a in $exeArgs) { $fullArgs += ('"{0}"' -f $a) }
    $fullArgs += '"--screenshot"'
    $fullArgs += ('"{0}"' -f $tmpOut)

    $proc = Start-Process -FilePath $exe -ArgumentList ($fullArgs -join " ") `
        -WorkingDirectory (Split-Path -Parent $exe) -PassThru -WindowStyle Minimized
    $exited = $proc.WaitForExit($timeoutMs)
    if (-not $exited) {
        try { $proc.Kill() } catch { }
        if (Test-Path $tmpOut) { Remove-Item $tmpOut -Force }
        return @{ ok = $false; error = "timeout after $([int]($timeoutMs / 1000))s" }
    }
    $code = $proc.ExitCode
    if ($code -ne 0 -or -not (Test-Path $tmpOut)) {
        if (Test-Path $tmpOut) { Remove-Item $tmpOut -Force }
        return @{ ok = $false; error = "exit code $code, no output written" }
    }
    return @{ ok = $true; file = $tmpOut }
}

# ---------------------------------------------------------------------------
# Main loop
# ---------------------------------------------------------------------------
$results = @()
$startTime = Get-Date

foreach ($entry in @($manifestJson.examples)) {
    $name = [string]$entry.name
    if ($Only -and ($name -notlike "*$Only*")) { continue }

    Write-Host "=== $name" -ForegroundColor Cyan
    $exe = Resolve-ExampleExe $name
    if (-not $exe -and $Build) {
        try { Invoke-BuildTarget $name } catch { Write-Host "  $_" -ForegroundColor Red }
        $exe = Resolve-ExampleExe $name
    }
    if (-not $exe) {
        Write-Host "  executable not found (build it first, or pass -Build)" -ForegroundColor Red
        $results += [pscustomobject]@{ Example = $name; Output = "-"; Status = "MISSING" }
        continue
    }

    if ($entry.raw) {
        # example with its own capture mode (e.g. colormap)
        $rawArgs = @()
        foreach ($a in @($entry.raw.args)) { $rawArgs += ('"{0}"' -f ($a -replace "\{assets\}", $assetsDir)) }
        $timeoutMs = ($defaultDelay / 1000 + $timeoutPadSec + 30) * 1000
        $proc = Start-Process -FilePath $exe -ArgumentList ($rawArgs -join " ") `
            -WorkingDirectory (Split-Path -Parent $exe) -PassThru -WindowStyle Minimized
        $exited = $proc.WaitForExit([int]$timeoutMs)
        if (-not $exited) { try { $proc.Kill() } catch { } }
        $failReason = ""
        if (-not $exited) { $failReason = "timeout" }
        elseif ($proc.ExitCode -ne 0) { $failReason = "exit code $($proc.ExitCode)" }
        if (-not $failReason) {
            foreach ($out in @($entry.raw.outputs)) {
                $outFile = Join-Path $assetsDir ($out -replace "/", "\")
                if (-not (Test-Path $outFile) -or (Get-Item $outFile).LastWriteTime -lt $startTime) {
                    $failReason = "output not written: $out"; break
                }
            }
        }
        if ($failReason) {
            Write-Host "  FAIL: $failReason" -ForegroundColor Red
            $results += [pscustomobject]@{ Example = $name; Output = "(raw mode)"; Status = "FAIL" }
            continue
        }
        foreach ($mv in @($entry.raw.postMove)) {
            $from = Join-Path $assetsDir ($mv[0] -replace "/", "\")
            $to = Join-Path $assetsDir ($mv[1] -replace "/", "\")
            if (Test-Path $from) { Move-Item -Force $from $to }
        }
        Write-Host "  OK (raw mode)" -ForegroundColor Green
        $results += [pscustomobject]@{ Example = $name; Output = "(raw mode)"; Status = "OK" }
        continue
    }

    foreach ($shot in @($entry.shots)) {
        $delay = $defaultDelay
        if ($shot.delay) { $delay = [int]$shot.delay }
        $exeArgs = @()
        if ($shot.size) { $exeArgs += @("--size", [string]$shot.size) }
        if ($shot.variant) { $exeArgs += @("--variant", [string]$shot.variant) }
        foreach ($a in @($shot.args)) { if ($a) { $exeArgs += [string]$a } }
        $exeArgs += @("--screenshot-delay", "$delay")

        $timeoutMs = ($delay / 1000 + $timeoutPadSec) * 1000
        $r = Invoke-ExampleProcess $exe $exeArgs ([int]$timeoutMs)

        $outRel = [string]$shot.output
        if ($r.ok) {
            $outFile = Join-Path $assetsDir ($outRel -replace "/", "\")
            $outParent = Split-Path -Parent $outFile
            if (-not (Test-Path $outParent)) { New-Item -ItemType Directory -Path $outParent -Force | Out-Null }
            Move-Item -Force $r.file $outFile
            Write-Host "  OK   $outRel" -ForegroundColor Green
            $results += [pscustomobject]@{ Example = $name; Output = $outRel; Status = "OK" }
        } else {
            Write-Host "  FAIL $outRel : $($r.error)" -ForegroundColor Red
            $results += [pscustomobject]@{ Example = $name; Output = $outRel; Status = "FAIL" }
        }
    }
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
$ok = @($results | Where-Object { $_.Status -eq "OK" }).Count
$fail = @($results | Where-Object { $_.Status -eq "FAIL" }).Count
$missing = @($results | Where-Object { $_.Status -eq "MISSING" }).Count

Write-Host ""
Write-Host ("Done in {0:n0}s - OK: {1}, FAIL: {2}, MISSING: {3}" -f ((Get-Date) - $startTime).TotalSeconds, $ok, $fail, $missing)
if ($fail -or $missing) {
    $results | Where-Object { $_.Status -ne "OK" } | Format-Table -AutoSize | Out-String | Write-Host
}
if (-not $Only) {
    Write-Host "Not regenerated (see 'skipped' in the manifest):" -ForegroundColor DarkGray
    foreach ($s in @($manifestJson.skipped)) {
        Write-Host ("  {0} - {1}" -f $s.output, $s.reason) -ForegroundColor DarkGray
    }
}
if ($fail -or $missing) { exit 1 }
exit 0
