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
    "NSPanel\nspanel_1.yaml"
    "NSPanel\nspanel_2.yaml"
    "NSPanel\nspanel_3.yaml"
    "NSPanel\nspanel_4.yaml"
    "NSPanel\nspanel_5.yaml"
    "NSPanel\nspanel_6.yaml"
    "NSPanel\nspanel_7.yaml"
    "NSPanel\nspanel_8.yaml"
    "NSPanel\nspanel_9.yaml"
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

        Write-Log "Suche Binaries in: $buildDir" "Info"
        
        $binFiles = @()
        if (Test-Path $buildDir) {
            $binFiles = @(Get-ChildItem -Path $buildDir -Filter "firmware*.bin" -Recurse -ErrorAction SilentlyContinue)
            if ($binFiles.Count -eq 0) {
                $binFiles = @(Get-ChildItem -Path $buildDir -Filter "*.bin" -Recurse -ErrorAction SilentlyContinue)
            }
        }
        
        if ($binFiles.Count -eq 0) {
            # Fallback: Suche in der Konfigurations-Unterordner-Struktur
            $configDir = Split-Path -Parent $config
            if ($configDir) {
                $fallbackDir = Join-Path $projectRoot $configDir
                $fallbackDir = Join-Path $fallbackDir ".esphome\build\$configName"
                if (Test-Path $fallbackDir) {
                    Write-Log "Fallback: Suche Binaries in: $fallbackDir" "Info"
                    $binFiles = @(Get-ChildItem -Path $fallbackDir -Filter "*.bin" -Recurse -ErrorAction SilentlyContinue)
                }
            }
        }
        
        if ($binFiles.Count -eq 0) {
            $fallbackDir2 = Join-Path $projectRoot ".esphome\build\$configName"
            if (Test-Path $fallbackDir2) {
                Write-Log "Fallback: Suche Binaries in: $fallbackDir2" "Info"
                $binFiles = @(Get-ChildItem -Path $fallbackDir2 -Filter "*.bin" -Recurse -ErrorAction SilentlyContinue)
            }
        }
        
        if ($binFiles.Count -eq 0) {
            Write-Log "WARNING: Keine .bin Datei gefunden in $buildDir" "Warning"
            $buildStats.Skipped++
            continue
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
    $firmwareCount = @(Get-ChildItem $outputDir -Filter "*.bin" -ErrorAction SilentlyContinue).Count
    Write-Log "Firmware-Dateien im Output-Ordner: $firmwareCount" "Success"
    Write-Log "" "Info"
    Write-Log "Binaries sind verfügbar unter:" "Info"
    Write-Host "  $outputDir" -foreground Green
    Write-Host ""
    Write-Host "Inhalte:" -foreground Cyan
    Get-ChildItem $outputDir -Filter "*.bin" -ErrorAction SilentlyContinue | ForEach-Object {
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
