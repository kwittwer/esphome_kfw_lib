# build-and-version.ps1
# Automatisches Versioning und Build-Script für ESPHome

param(
    [switch]$IncrementCounter = $true,
    [switch]$ResetCounter = $false
)

# Konfiguration
$projectPath = "C:\Users\z004s49s\Documents\B16M_Homecontroller\B16M_Homecontroller"
$yamlFile = Join-Path $projectPath "b16m.yaml"
$buildsFolder = Join-Path $projectPath "Builds"
$binSourcePath = Join-Path $projectPath ".esphome\build\b16m\.pioenvs\b16m\firmware.factory.bin"

# Farben für Output
function Write-ColorOutput($ForegroundColor) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    if ($args) {
        Write-Output $args
    }
    $host.UI.RawUI.ForegroundColor = $fc
}

Write-ColorOutput Cyan "=========================================="
Write-ColorOutput Cyan "  ESPHome Build & Version Manager"
Write-ColorOutput Cyan "=========================================="
Write-Host ""

# Prüfe ob YAML existiert
if (-not (Test-Path $yamlFile)) {
    Write-ColorOutput Red "FEHLER: $yamlFile nicht gefunden!"
    exit 1
}

# Erstelle Builds-Ordner falls nicht vorhanden
if (-not (Test-Path $buildsFolder)) {
    New-Item -ItemType Directory -Path $buildsFolder | Out-Null
    Write-ColorOutput Green "Builds-Ordner erstellt: $buildsFolder"
}

# Lese aktuelle YAML
$yamlContent = Get-Content $yamlFile -Raw

# Extrahiere aktuelle Version
if ($yamlContent -match 'version:\s*["\''"]?(\d{4})\.(\d{2})\.(\d{2})\.(\d+)["\''"]?') {
    $oldYear = $matches[1]
    $oldMonth = $matches[2]
    $oldDay = $matches[3]
    $oldCounter = [int]$matches[4]
    $oldVersion = "$oldYear.$oldMonth.$oldDay.$oldCounter"
    Write-ColorOutput Yellow "Aktuelle Version: $oldVersion"
} else {
    Write-ColorOutput Yellow "Keine Version gefunden, erstelle neue..."
    $oldCounter = 0
    $oldVersion = "keine"
}

# Berechne neue Version
$now = Get-Date
$newYear = $now.ToString("yyyy")
$newMonth = $now.ToString("MM")
$newDay = $now.ToString("dd")

# Prüfe ob Datum sich geändert hat
$dateChanged = ($newYear -ne $oldYear) -or ($newMonth -ne $oldMonth) -or ($newDay -ne $oldDay)

if ($ResetCounter -or $dateChanged) {
    $newCounter = 1
    Write-ColorOutput Cyan "Counter zurückgesetzt (neues Datum oder manuell)"
} elseif ($IncrementCounter) {
    $newCounter = $oldCounter + 1
    Write-ColorOutput Cyan "Counter erhöht: $oldCounter -> $newCounter"
} else {
    $newCounter = $oldCounter
}

$newVersion = "$newYear.$newMonth.$newDay.$newCounter"
Write-ColorOutput Green "Neue Version: $newVersion"
Write-Host ""

# Aktualisiere Version in YAML
if ($yamlContent -match 'version:\s*["\''"]?[\d\.]+["\''"]?') {
    # Version existiert bereits, ersetze sie
    $replacement = 'version: "' + $newVersion + '"'
    $yamlContent = $yamlContent -replace 'version:\s*["\''"]?[\d\.]+["\''"]?', $replacement
} else {
    # Version existiert nicht, füge sie nach "esphome:" ein
    if ($yamlContent -match '(esphome:\s*\r?\n)') {
        $insertion = "esphome:`r`n  version: `"$newVersion`"`r`n"
        $yamlContent = $yamlContent -replace 'esphome:\s*\r?\n', $insertion
    } else {
        Write-ColorOutput Red "FEHLER: Konnte esphome Sektion nicht finden!"
        exit 1
    }
}

# Speichere aktualisierte YAML
$yamlContent | Set-Content $yamlFile -NoNewline
Write-ColorOutput Green "Version in b16m.yaml aktualisiert"
Write-Host ""

# Kompiliere mit ESPHome
Write-ColorOutput Cyan "Starte Kompilierung..."
Write-Host ""

$compileStartTime = Get-Date

# Führe ESPHome aus
$process = Start-Process -FilePath "esphome" -ArgumentList "compile", $yamlFile -NoNewWindow -Wait -PassThru

$compileEndTime = Get-Date
$compileDuration = ($compileEndTime - $compileStartTime).TotalSeconds

Write-Host ""

# Prüfe Kompilierungs-Ergebnis
if ($process.ExitCode -eq 0) {
    Write-ColorOutput Green "=========================================="
    Write-ColorOutput Green "  Kompilierung erfolgreich!"
    Write-ColorOutput Green "=========================================="
    $durationRounded = [math]::Round($compileDuration, 2)
    Write-ColorOutput Yellow "Dauer: $durationRounded Sekunden"
    Write-Host ""
    
    # Prüfe ob .bin Datei existiert
    if (Test-Path $binSourcePath) {
        # Erstelle Dateinamen mit Version
        $binFileName = "b16m_v$newVersion.bin"
        $binDestPath = Join-Path $buildsFolder $binFileName
        
        # Kopiere .bin Datei
        Copy-Item -Path $binSourcePath -Destination $binDestPath -Force
        
        # Hole Dateigröße
        $fileSize = (Get-Item $binDestPath).Length
        $fileSizeMB = [math]::Round($fileSize / 1MB, 2)
        
        Write-ColorOutput Green "Binary kopiert nach:"
        Write-ColorOutput White "  $binDestPath"
        Write-ColorOutput Yellow "  Groesse: $fileSizeMB MB"
        Write-Host ""
        
        # Erstelle auch eine "latest" Kopie
        $latestBinPath = Join-Path $buildsFolder "b16m_latest.bin"
        Copy-Item -Path $binSourcePath -Destination $latestBinPath -Force
        Write-ColorOutput Green "Latest-Version erstellt: b16m_latest.bin"
        
        # Erstelle Build-Info Datei
        $buildInfoPath = Join-Path $buildsFolder "b16m_v$newVersion.txt"
        $buildDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        $esphomeVersion = esphome version 2>&1 | Select-String "Version:" | Out-String
        
        $buildInfo = @"
Build Information
=================
Version:        $newVersion
Build Date:     $buildDate
Build Duration: $durationRounded seconds
File Size:      $fileSizeMB MB
Source:         $yamlFile
Binary:         $binFileName

ESPHome Version: $esphomeVersion
"@
        $buildInfo | Set-Content $buildInfoPath
        Write-ColorOutput Green "Build-Info erstellt: b16m_v$newVersion.txt"
        
        Write-Host ""
        Write-ColorOutput Cyan "=========================================="
        Write-ColorOutput Cyan "  Build abgeschlossen!"
        Write-ColorOutput Cyan "=========================================="
        Write-ColorOutput White "Version:  $newVersion"
        Write-ColorOutput White "Datei:    $binFileName"
        Write-ColorOutput White "Ordner:   $buildsFolder"
        Write-Host ""
        
        # Liste die letzten 5 Builds
        Write-ColorOutput Yellow "Letzte Builds:"
        Get-ChildItem -Path $buildsFolder -Filter "b16m_v*.bin" | 
            Sort-Object LastWriteTime -Descending | 
            Select-Object -First 5 | 
            ForEach-Object {
                $size = [math]::Round($_.Length / 1MB, 2)
                $date = $_.LastWriteTime.ToString("yyyy-MM-dd HH:mm")
                $name = $_.Name.PadRight(30)
                Write-Host "  $name $date  ($size MB)"
            }
        
    } else {
        Write-ColorOutput Red "=========================================="
        Write-ColorOutput Red "FEHLER: Binary-Datei nicht gefunden!"
        Write-ColorOutput Red "=========================================="
        Write-ColorOutput Red "Erwartet: $binSourcePath"
        exit 1
    }
    
} else {
    Write-ColorOutput Red "=========================================="
    Write-ColorOutput Red "  Kompilierung fehlgeschlagen!"
    Write-ColorOutput Red "=========================================="
    Write-ColorOutput Yellow "Exit Code: $($process.ExitCode)"
    Write-Host ""
    
    # Setze Version zurück bei Fehler
    Write-ColorOutput Yellow "Setze Version zurueck auf: $oldVersion"
    if ($oldVersion -ne "keine") {
        $yamlContent = Get-Content $yamlFile -Raw
        $replacement = 'version: "' + $oldVersion + '"'
        $yamlContent = $yamlContent -replace 'version:\s*["\''"]?[\d\.]+["\''"]?', $replacement
        $yamlContent | Set-Content $yamlFile -NoNewline
        Write-ColorOutput Green "Version zurueckgesetzt"
    }
    
    exit 1
}