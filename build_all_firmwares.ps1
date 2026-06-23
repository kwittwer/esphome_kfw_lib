# =====================================================
# ESPHome Firmware Build Script fuer nspanel_1 bis 9
# Kompiliert alle Konfigurationen und kopiert die
# Binaries in einen separaten Ordner
# =====================================================

# Einstellungen
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$outputDir = Join-Path $projectRoot "firmware_output"
$logFile = Join-Path $projectRoot "build_log.txt"

# Liste der zu kompilierenden YAML-Dateien.
# Erweiterbar: füge hier neue Einträge hinzu.
$configs = @(
    # "NSPanel\nspanel_1.yaml"
    # "NSPanel\nspanel_2.yaml"
    # "NSPanel\nspanel_3.yaml"
    # "NSPanel\nspanel_4.yaml"
    # "NSPanel\nspanel_5.yaml"
    # "NSPanel\nspanel_6.yaml"
    # "NSPanel\nspanel_7.yaml"
    # "NSPanel\nspanel_8.yaml"
    # "NSPanel\nspanel_9.yaml"
    "Shutter\shelly_2pm_raffstore1.yaml"
    "Shutter\shelly_2pm_raffstore2.yaml"
    "Shutter\shelly_2pm_raffstore3.yaml"
    "Shutter\shelly_2pm_raffstore4.yaml"
)

# Farben für Terminal-Output
$colors = @{
    Info    = "Cyan"
    Success = "Green"
    Warning = "Yellow"
    Error   = "Red"
}

$requiredFirmwareNames = @(
    "firmware.bin"
    "firmware.factory.bin"
    "firmware.ota.bin"
)

function Write-Log {
    param(
        [string]$Message,
        [ValidateSet("Info", "Success", "Warning", "Error")]
        [string]$Type = "Info"
    )
    
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logMessage = "[$timestamp] [$Type] $Message"
    
    Write-Host $logMessage -ForegroundColor $colors[$Type]
    Add-Content -Path $logFile -Value $logMessage
}

# Header
Write-Host ""
Write-Host "====================================================" -foreground Green
Write-Host "   ESPHome Firmware Builder (nspanel_1 - 9)" -foreground Green
Write-Host "====================================================" -foreground Green
Write-Host ""

Write-Log "Script gestartet" "Info"
Write-Log "Projekt-Verzeichnis: $projectRoot" "Info"
Write-Log "Output-Verzeichnis: $outputDir" "Info"

# Cleanup und Create Output Directory
if (Test-Path $logFile) {
    Remove-Item $logFile -Force
}

if (Test-Path $outputDir) {
    Write-Log "Output-Verzeichnis existiert bereits, räume auf..." "Warning"
    Remove-Item $outputDir -Recurse -Force
}

New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
Write-Log "Output-Verzeichnis erstellt" "Success"

# Build-Statistik
$buildStats = @{
    Total   = 0
    Success = 0
    Failed  = 0
    Skipped = 0
}

# Hauptschleife
foreach ($config in $configs) {
    $configPath = Join-Path $projectRoot $config
    $configName = [System.IO.Path]::GetFileNameWithoutExtension($config)
    
    $buildStats.Total++
    
    Write-Host ""
    Write-Host "---------------------------------------------------" -foreground Cyan
    Write-Log "Verarbeite: $config [$($buildStats.Total)/$($configs.Count)]" "Info"
    
    # Prüfe ob Config existiert
    if (-not (Test-Path $configPath)) {
        Write-Log "ERROR: Konfiguration nicht gefunden: $configPath" "Error"
        $buildStats.Failed++
        continue
    }
    
    try {
        # Compile
        Write-Log "Starte Compilation..." "Info"
        Write-Host "  -> esphome compile $config" -foreground DarkGray
        
        Push-Location $projectRoot
        try {
            $processOutput = & esphome compile "$config" 2>&1
            $exitCode = $LASTEXITCODE
        }
        finally {
            Pop-Location
        }

        Set-Content -Path "$env:TEMP\esphome_build.log" -Value $processOutput -Force
        
        if ($exitCode -ne 0) {
            Write-Log "Compilation fehlgeschlagen (Exit Code: $exitCode)" "Error"
            $buildStats.Failed++
            
            if ($processOutput) {
                Write-Log "Error Output: $($processOutput -join "`n")" "Warning"
            }
            continue
        }
        
        Write-Log "Compilation erfolgreich" "Success"
        
        # Suche .bin Datei anhand des Build-Ausgabepfads
        $outputText = $processOutput -join "`n"
        $buildDir = $null

        $pathMatch = [regex]::Match($outputText, '(?mi)([A-Za-z]:\\[^"\r\n]+?\.bin)')
        if (-not $pathMatch.Success) {
            $pathMatch = [regex]::Match($outputText, '(?mi)(\.pioenvs[\\/][^"\r\n]+?\.bin)')
        }

        if ($pathMatch.Success) {
            $binPath = $pathMatch.Groups[1].Value -replace '/', '\\'
            if ($binPath.StartsWith('.')) {
                $binPath = Join-Path $projectRoot $binPath
            }
            $buildDir = Split-Path -Parent $binPath
        }

        if (-not $buildDir) {
            $envName = $configName -replace "_", ""
            $configDir = Split-Path -Parent $config
            if (-not $configDir) {
                $buildDir = Join-Path $projectRoot ".pioenvs\$envName"
            }
            else {
                $buildDir = Join-Path $projectRoot $configDir
                $buildDir = Join-Path $buildDir ".pioenvs\$envName"
            }
        }

        $searchDirs = @()
        if ($buildDir -and (Test-Path $buildDir)) {
            $searchDirs += $buildDir
        }

        $configDir = Split-Path -Parent $config
        if ($configDir) {
            $fallbackDir = Join-Path $projectRoot $configDir
            $fallbackDir = Join-Path $fallbackDir ".esphome\build\$configName"
            if ((Test-Path $fallbackDir) -and -not ($searchDirs -contains $fallbackDir)) {
                $searchDirs += $fallbackDir
            }
        }

        $fallbackDir2 = Join-Path $projectRoot ".esphome\build\$configName"
        if ((Test-Path $fallbackDir2) -and -not ($searchDirs -contains $fallbackDir2)) {
            $searchDirs += $fallbackDir2
        }

        # Fallback 3: Suche mit verkürztem Namen (z.B. shelly_2pm_raffstore1 -> raffstore1)
        if ($configDir -and $configName -match '(.+)_(\w+)$') {
            $shortName = $matches[2]
            $fallbackDir3 = Join-Path $projectRoot $configDir
            $fallbackDir3 = Join-Path $fallbackDir3 ".esphome\build\$shortName"
            if ((Test-Path $fallbackDir3) -and -not ($searchDirs -contains $fallbackDir3)) {
                $searchDirs += $fallbackDir3
            }
        }

        $binFiles = @()
        $foundInDir = $null
        foreach ($dir in $searchDirs) {
            Write-Log "Suche Ziel-Binaries in: $dir" "Info"

            $matchesForDir = @()
            foreach ($requiredName in $requiredFirmwareNames) {
                $match = Get-ChildItem -Path $dir -Filter $requiredName -Recurse -File -ErrorAction SilentlyContinue | Select-Object -First 1
                if ($match) {
                    $matchesForDir += $match
                }
            }

            if ($matchesForDir.Count -gt 0) {
                $binFiles = $matchesForDir
                $foundInDir = $dir
                break
            }
        }

        if ($binFiles.Count -eq 0) {
            Write-Log "WARNING: Keine .bin Datei gefunden in $buildDir" "Warning"
            $buildStats.Skipped++
            continue
        }

        $foundNames = @($binFiles | ForEach-Object { $_.Name })
        $missingFirmwareNames = @($requiredFirmwareNames | Where-Object { $_ -notin $foundNames })
        if ($missingFirmwareNames.Count -gt 0) {
            Write-Log "Hinweis: Nicht alle Ziel-Binaries gefunden in $foundInDir. Fehlend: $($missingFirmwareNames -join ', ')" "Warning"
        }
        
        # Kopiere Binaries in einen Geräte-Unterordner und benenne sie mit dem Gerätenamen
        $deviceOutputDir = Join-Path $outputDir $configName
        if (-not (Test-Path $deviceOutputDir)) {
            New-Item -ItemType Directory -Path $deviceOutputDir -Force | Out-Null
        }

        foreach ($binFile in $binFiles) {
            $targetName = "$configName-$($binFile.Name)"
            $targetPath = Join-Path $deviceOutputDir $targetName
            
            Copy-Item -Path $binFile.FullName -Destination $targetPath -Force
            Write-Log "Kopiert: $targetName" "Success"
        }
        
        $buildStats.Success++
        Write-Log "Fertig: $config erfolgreich verarbeitet" "Success"
        
    }
    catch {
        Write-Log "EXCEPTION: $_" "Error"
        $buildStats.Failed++
    }
    finally {
        # Cleanup temp logs
        Remove-Item "$env:TEMP\esphome_build.log" -ErrorAction SilentlyContinue
        Remove-Item "$env:TEMP\esphome_error.log" -ErrorAction SilentlyContinue
    }
}

# Zusammenfassung
Write-Host ""
Write-Host "---------------------------------------------------" -foreground Cyan
Write-Host ""
Write-Host "====================================================" -foreground Green
Write-Host "          BUILD ZUSAMMENFASSUNG" -foreground Green
Write-Host "====================================================" -foreground Green

Write-Log "Gesamt:       $($buildStats.Total)" "Info"
Write-Log "Erfolgreich:  $($buildStats.Success)" "Success"
Write-Log "Fehlgeschlagen: $($buildStats.Failed)" "Error"
Write-Log "Uebersprungen: $($buildStats.Skipped)" "Warning"

if (Test-Path $outputDir) {
    $firmwareCount = @(Get-ChildItem $outputDir -Filter "*.bin" -File -Recurse -ErrorAction SilentlyContinue).Count
    Write-Log "Firmware-Dateien im Output-Ordner: $firmwareCount" "Success"
    Write-Log "" "Info"
    Write-Log "Binaries sind verfügbar unter:" "Info"
    Write-Host "  $outputDir" -foreground Green
    Write-Host ""
    Write-Host "Inhalte:" -foreground Cyan
    Get-ChildItem $outputDir -Filter "*.bin" -File -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
        $sizeInMB = [math]::Round($_.Length / (1024*1024), 2)
        Write-Host "  - $($_.Name) ($sizeInMB MB)" -foreground DarkGray
    }
}

Write-Host ""
if ($buildStats.Failed -gt 0) {
    Write-Log "Script beendet mit FEHLERN (siehe Log: $logFile)" "Error"
    exit 1
}
else {
    Write-Log "Script erfolgreich beendet" "Success"
    exit 0
}
