$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$cliCandidates = @()

$arduinoCliCommand = Get-Command arduino-cli -ErrorAction SilentlyContinue
if ($arduinoCliCommand) {
    $cliCandidates += $arduinoCliCommand.Source
}

$cliCandidates += "C:\Program Files\Arduino CLI\arduino-cli.exe"
$cliCandidates = @($cliCandidates | Where-Object { $_ -and (Test-Path $_) } | Select-Object -Unique)

if (-not $cliCandidates) {
    throw "arduino-cli not found"
}

$arduinoCli = $cliCandidates[0]
$librariesPath = Join-Path -Path $HOME\Documents -ChildPath "Arduino\libraries"
$fqbn = "esp32:esp32:m5stack_cardputer:FlashSize=8M,PartitionScheme=default_8MB"

$arguments = @(
    "compile"
    "--fqbn"
    $fqbn
    "--libraries"
    $librariesPath
    $projectRoot
)

& $arduinoCli @arguments
exit $LASTEXITCODE
