param(
    [string]$Port,
    [switch]$SkipCompile,
    [string]$Env = "m5stack-cardputer"
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptRoot

Set-Location $projectRoot

if (-not $SkipCompile) {
    Write-Host "[Flash] Compiling..."
    & (Join-Path $scriptRoot "build.ps1") -Env $Env
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Write-Host "[Flash] Uploading..."
if ($Port) {
    & (Join-Path $scriptRoot "upload.ps1") -Port $Port -Env $Env
} else {
    & (Join-Path $scriptRoot "upload.ps1") -Env $Env
}
exit $LASTEXITCODE
