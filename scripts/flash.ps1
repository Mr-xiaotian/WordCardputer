param(
    [string]$Port,
    [switch]$SkipCompile,
    [string]$BuildPath = ".arduino-build"
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptRoot

Set-Location $projectRoot

if (-not $SkipCompile) {
    Write-Host "[Flash] Compiling..."
    & (Join-Path $scriptRoot "build.ps1") -BuildPath $BuildPath
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Write-Host "[Flash] Uploading..."
& (Join-Path $scriptRoot "upload.ps1") -Port $Port -BuildPath $BuildPath
exit $LASTEXITCODE
