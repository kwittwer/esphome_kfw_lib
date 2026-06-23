# flash-latest.ps1
# Flasht den letzten erfolgreichen Build

# param(
#     # [Parameter(Mandatory=$false)]
#     # [ValidateSet("OTA", "USB")]
#     # [string]$Method = "OTA",
    
#     # [Parameter(Mandatory=$false)]
#     # [string]$IPAddress = "",
    
#     # [Parameter(Mandatory=$false)]
#     # [string]$COMPort = ""
# )

# Konfiguration
$projectPath = "C:\Users\z004s49s\Documents\B16M_Homecontroller\B16M_Homecontroller"
# $buildsFolder = Join-Path $projectPath "Builds"
# $latestBin = Join-Path $buildsFolder "b16m_latest.bin"
$yamlFile = Join-Path $projectPath "b16m.yaml"

# Farben für Output
function Write-ColorOutput($ForegroundColor) {
    $fc = $host.UI.RawUI.ForegroundColor
    $host.UI.RawUI.ForegroundColor = $ForegroundColor
    if ($args) {
        Write-Output $args
    }
    $host.UI.RawUI.ForegroundColor = $fc
}

# Write-ColorOutput Cyan "=========================================="
# Write-ColorOutput Cyan "  ESPHome Flash Manager"
# Write-ColorOutput Cyan "=========================================="
# Write-Host ""

# # Prüfe ob latest.bin existiert
# if (-not (Test-Path $latestBin)) {
#     Write-ColorOutput Red "FEHLER: Keine b16m_latest.bin gefunden!"
#     Write-ColorOutput Yellow "Bitte erst einen Build durchfuehren."
#     Write-Host ""
    
#     # Zeige verfügbare Builds
#     $availableBuilds = Get-ChildItem -Path $buildsFolder -Filter "b16m_v*.bin" -ErrorAction SilentlyContinue
#     if ($availableBuilds) {
#         Write-ColorOutput Yellow "Verfuegbare Builds:"
#         $availableBuilds | Sort-Object LastWriteTime -Descending | Select-Object -First 5 | ForEach-Object {
#             $date = $_.LastWriteTime.ToString("yyyy-MM-dd HH:mm")
#             Write-Host "  $($_.Name) - $date"
#         }
#     }
#     exit 1
# }

# # Hole Build-Info
# $binInfo = Get-Item $latestBin
# $binSize = [math]::Round($binInfo.Length / 1MB, 2)
# $binDate = $binInfo.LastWriteTime.ToString("yyyy-MM-dd HH:mm:ss")

# Write-ColorOutput Green "Letzter Build gefunden:"
# Write-ColorOutput White "  Datei:  b16m_latest.bin"
# Write-ColorOutput White "  Datum:  $binDate"
# Write-ColorOutput White "  Groesse: $binSize MB"
# Write-Host ""

$process = Start-Process -FilePath "esphome" -ArgumentList "upload", $yamlFile, -Wait -PassThru

# # Flash-Methode
# if ($Method -eq "OTA") {
#     Write-ColorOutput Cyan "Flash-Methode: OTA (Over-The-Air)"
#     Write-Host ""
    
#     # Hole IP-Adresse aus YAML wenn nicht angegeben
#     # if ([string]::IsNullOrEmpty($IPAddress)) {
#     #     Write-ColorOutput Yellow "Suche IP-Adresse in b16m.yaml..."
        
#     #     # Versuche IP aus YAML zu extrahieren
#     #     $yamlContent = Get-Content $yamlFile -Raw
#     #     if ($yamlContent -match 'use_address:\s*([0-9\.]+)') {
#     #         $IPAddress = $matches[1]
#     #         Write-ColorOutput Green "IP-Adresse gefunden: $IPAddress"
#     #     } else {
#     #         Write-ColorOutput Yellow "Keine IP-Adresse in YAML gefunden."
#     #         $IPAddress = Read-Host "Bitte IP-Adresse eingeben"
#     #     }
#     # }
    
#     Write-Host ""
#     Write-ColorOutput Cyan "Starte OTA Upload "
#     Write-Host ""
    
#     # Führe ESPHome OTA aus

    
#     if ($process.ExitCode -eq 0) {
#         Write-Host ""
#         Write-ColorOutput Green "=========================================="
#         Write-ColorOutput Green "  OTA Upload erfolgreich!"
#         Write-ColorOutput Green "=========================================="
#         Write-ColorOutput Yellow "Geraet wird neu gestartet..."
#     } else {
#         Write-Host ""
#         Write-ColorOutput Red "=========================================="
#         Write-ColorOutput Red "  OTA Upload fehlgeschlagen!"
#         Write-ColorOutput Red "=========================================="
#         Write-ColorOutput Yellow "Exit Code: $($process.ExitCode)"
#         exit 1
#     }
    
# } elseif ($Method -eq "USB") {
#     Write-ColorOutput Cyan "Flash-Methode: USB (Seriell)"
#     Write-Host ""
    
#     # Hole COM-Port wenn nicht angegeben
#     if ([string]::IsNullOrEmpty($COMPort)) {
#         Write-ColorOutput Yellow "Verfuegbare COM-Ports:"
#         $ports = [System.IO.Ports.SerialPort]::getportnames()
#         if ($ports.Count -eq 0) {
#             Write-ColorOutput Red "FEHLER: Keine COM-Ports gefunden!"
#             exit 1
#         }
        
#         $ports | ForEach-Object { Write-Host "  $_" }
#         Write-Host ""
        
#         if ($ports.Count -eq 1) {
#             $COMPort = $ports[0]
#             Write-ColorOutput Green "Verwende einzigen verfuegbaren Port: $COMPort"
#         } else {
#             $COMPort = Read-Host "Bitte COM-Port eingeben (z.B. COM3)"
#         }
#     }
    
#     Write-Host ""
#     Write-ColorOutput Cyan "Starte USB Flash auf $COMPort..."
#     Write-Host ""
    
#     # Führe ESPHome USB Flash aus
#     $process = Start-Process -FilePath "esphome" -ArgumentList "upload", $yamlFile, "--device", $COMPort -NoNewWindow -Wait -PassThru
    
#     if ($process.ExitCode -eq 0) {
#         Write-Host ""
#         Write-ColorOutput Green "=========================================="
#         Write-ColorOutput Green "  USB Flash erfolgreich!"
#         Write-ColorOutput Green "=========================================="
#     } else {
#         Write-Host ""
#         Write-ColorOutput Red "=========================================="
#         Write-ColorOutput Red "  USB Flash fehlgeschlagen!"
#         Write-ColorOutput Red "=========================================="
#         Write-ColorOutput Yellow "Exit Code: $($process.ExitCode)"
#         exit 1
#     }
# }

Write-Host ""