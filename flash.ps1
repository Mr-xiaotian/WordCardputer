param(
    [string]$Port,
    [switch]$SkipCompile,
    [string]$BuildPath = ".arduino-build"
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

Set-Location $projectRoot

if (-not $SkipCompile) {
    Write-Host "[Flash] Compiling..."
    & (Join-Path $projectRoot "build.ps1") -BuildPath $BuildPath
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Write-Host "[Flash] Uploading..."
& (Join-Path $projectRoot "upload.ps1") -Port $Port -BuildPath $BuildPath
exit $LASTEXITCODE
