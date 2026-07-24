param(
    [string]$Env = "m5stack-cardputer"
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptRoot

Set-Location $projectRoot

Write-Host "[PlatformIO Build] Environment: $Env"
Write-Host "[PlatformIO Build] Project: $projectRoot"

pio run -e $Env
exit $LASTEXITCODE
