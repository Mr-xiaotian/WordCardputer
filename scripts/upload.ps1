param(
    [string]$Port,
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

function Resolve-UploadPort {
    param(
        [string]$PreferredPort
    )

    if ($PreferredPort) {
        return $PreferredPort
    }

    $serialPorts = @(Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue)
    if (-not $serialPorts) {
        throw "No serial port found. Please connect the device or pass -Port COMx."
    }

    $usbPorts = @(
        $serialPorts |
        Where-Object {
            $_.DeviceID -match '^COM\d+$' -and (
                $_.Name -match 'USB' -or
                $_.Description -match 'USB'
            )
        }
    )

    if ($usbPorts.Count -eq 1) {
        return $usbPorts[0].DeviceID
    }

    $com6 = $serialPorts | Where-Object { $_.DeviceID -eq "COM6" } | Select-Object -First 1
    if ($com6) {
        return $com6.DeviceID
    }

    $firstCom = $serialPorts |
        Where-Object { $_.DeviceID -match '^COM\d+$' } |
        Select-Object -First 1
    if ($firstCom) {
        return $firstCom.DeviceID
    }

    throw "No usable COM port found. Please pass -Port COMx."
}

if (-not (Test-Path $buildOutputPath)) {
    throw "Build path not found: $buildOutputPath"
}

$arduinoCli = Resolve-ArduinoCli
$resolvedPort = Resolve-UploadPort -PreferredPort $Port
$fqbn = "m5stack:esp32:m5stack_cardputer:FlashSize=8M,PartitionScheme=default_8MB"

Set-Location $projectRoot

$arguments = @(
    "upload"
    "--verbose"
    "--port"
    $resolvedPort
    "--protocol"
    "serial"
    "--fqbn"
    $fqbn
    "--build-path"
    $buildOutputPath
    (Join-Path $projectRoot "src\WordCardputer")
)

Write-Host "[Upload] Project: $projectRoot"
Write-Host "[Upload] BuildPath: $buildOutputPath"
Write-Host "[Upload] Port: $resolvedPort"
Write-Host "[Upload] FQBN: $fqbn"

& $arduinoCli @arguments
exit $LASTEXITCODE
