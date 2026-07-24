param(
    [string]$Port,
    [string]$Env = "m5stack-cardputer"
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptRoot

Set-Location $projectRoot

Write-Host "[PlatformIO Upload] Environment: $Env"
Write-Host "[PlatformIO Upload] Project: $projectRoot"

if ($Port) {
    Write-Host "[PlatformIO Upload] Port: $Port"
    pio run -e $Env -t upload --upload-port $Port
} else {
    pio run -e $Env -t upload
}

exit $LASTEXITCODE
