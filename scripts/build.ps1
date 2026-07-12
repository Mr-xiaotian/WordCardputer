param(
    [string]$BuildPath = ".arduino-build"
)

$ErrorActionPreference = "Stop"

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptRoot
$buildOutputPath = if ([System.IO.Path]::IsPathRooted($BuildPath)) {
    $BuildPath
} else {
    Join-Path $projectRoot $BuildPath
}

function Resolve-ArduinoCli {
    $cliCandidates = @()

    $arduinoCliCommand = Get-Command arduino-cli -ErrorAction SilentlyContinue
    if ($arduinoCliCommand) {
        $cliCandidates += $arduinoCliCommand.Source
    }

    $cliCandidates += "C:\Program Files\Arduino CLI\arduino-cli.exe"
    $cliCandidates = @(
        $cliCandidates |
        Where-Object { $_ -and (Test-Path $_) } |
        Select-Object -Unique
    )

    if (-not $cliCandidates) {
        throw "arduino-cli not found"
    }

    return $cliCandidates[0]
}

$arduinoCli = Resolve-ArduinoCli
$librariesPath = Join-Path -Path $HOME\Documents -ChildPath "Arduino\libraries"
$fqbn = "m5stack:esp32:m5stack_cardputer:FlashSize=8M,PartitionScheme=default_8MB"
$coreSqliteLibrary = Join-Path $env:LOCALAPPDATA "Arduino15\packages\m5stack\hardware\esp32\3.2.5\libraries\Sqlite3Esp32"

Set-Location $projectRoot

$arguments = @(
    "compile"
    "--fqbn"
    $fqbn
    "--libraries"
    $librariesPath
    "--build-path"
    $buildOutputPath
)

if (Test-Path $coreSqliteLibrary) {
    $arguments += @("--library", $coreSqliteLibrary)
}

$arguments += $projectRoot

Write-Host "[Build] Project: $projectRoot"
Write-Host "[Build] BuildPath: $buildOutputPath"
Write-Host "[Build] FQBN: $fqbn"

& $arduinoCli @arguments
exit $LASTEXITCODE
