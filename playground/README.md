# B16M ESPHome Firmware

Modulare ESPHome-Firmware fuer den **B16M Industrial Controller** (ESP32-S3, 16 DI, 16 DO, 4 AI, W5500 Ethernet, 4 Sensor-Slots, SSD1306 Display).

Die Konfiguration ist vollstaendig package-template-basiert. Die Struktur trennt jetzt klar zwischen Plattformbasis, wiederverwendbaren Paketen und konkreten Anwendungen.

---

## Dateistruktur

```text
B16M/
|- platform/
|  |- B16M_base.yaml                 # Hardware-Basis des B16M
|  |- assets/                        # benoetigte Plattform-Assets
|  \- components/                    # lokale Custom Components
|- packages/
|  |- networking/                    # WiFi/Ethernet Pakete
|  |- sensors/                       # DHT/Dallas Pakete
|  \- heating/                       # Thermostat- und Heizungsbausteine
|- profiles/                         # importierbare Device-Builder-Profile
|- application/                      # konkrete Setups und Builder-Einstiege
|- shared/                           # gemeinsame Applikationslogik
|- scripts/                          # Build-/Flash-Hilfsskripte
|- build-artifacts/                  # erzeugte Firmware-Artefakte
|- old/                              # archivierte Altdateien
|- README.md
\- secrets.yaml
```

---

## Neues Setup erstellen

1. [B16M/application/setup_template.yaml](B16M/application/setup_template.yaml) kopieren und umbenennen, z. B. `setup_keller.yaml`.
2. `base:`-Package anpassen: Geraetename, Friendly Name und nur die abweichenden Werte setzen.
3. Passendes `network:`-Package aus `packages/networking/` waehlen.
4. Sensor- und Thermostat-Pakete aus `packages/sensors/`, `packages/heating/` und `shared/` einbinden.
5. Optional API-/OTA-Einstellungen lokal anpassen.

---

## ESPHome Device Builder â€“ Zwei Optionen

### **Option A: Flexible Konfiguration (Empfohlen fÃ¼r AnfÃ¤nger)**

Nutzer importiert ein universelles Basis-Profil, editiert dann lokal die YAML und fÃ¼gt Sensoren selbst hinzu.

#### Schritt 1: Basis-Profil wÃ¤hlen und importieren

WÃ¤hle ein Basis-Profil je nach Netzwerk**typ**:

| Profil | Netzwerk | Import-Link |
|--------|----------|-------------|
| **Device â€“ WiFi** | WLAN (DHCP, editierbar auf statisch) | `github://kwittwer/esphome_kfw_lib/B16M/profiles/profile_device_wifi.yaml@main` |
| **Device â€“ Ethernet** | Ethernet W5500 (DHCP, editierbar auf statisch) | `github://kwittwer/esphome_kfw_lib/B16M/profiles/profile_device_ethernet.yaml@main` |

Im ESPHome Dashboard:
1. **Add Device** â†’ **Import**
2. URL einfÃ¼gen
3. **Import** klicken
4. Die Substitutions im Dialog setzen:
   - `device_name` (z.B. `b16m-keller`)
   - `friendly_name` (z.B. `Keller Sensor`)
   - `wifi_ssid` und `wifi_password` (nur WiFi-Profil)

#### Schritt 2: Lokal editieren â€“ Netzwerk anpassen

Im ESPHome Editor die YAML Ã¶ffnen und die Netzwerk-Einstellung anpassen:

**WiFi mit DHCP** (Standard, nichts Ã¤ndern nÃ¶tig):
```yaml
packages:
  network: !include packages/networking/net_wifi.yaml
```

**WiFi mit statischer IP:**
```yaml
packages:
  network: !include packages/networking/net_wifi_static.yaml
  
substitutions:
  static_ip: "192.168.1.100"
  gateway: "192.168.1.1"
  # optional: subnet, dns
```

**Ethernet mit DHCP** (Standard, nichts Ã¤ndern nÃ¶tig):
```yaml
packages:
  network: !include packages/networking/net_ethernet.yaml
```

**Ethernet mit statischer IP:**
```yaml
packages:
  network: !include packages/networking/net_ethernet_static.yaml
  
substitutions:
  static_ip: "192.168.1.100"
  gateway: "192.168.1.1"
```

#### Schritt 3: Sensoren hinzufÃ¼gen

Unter `packages:` neue Sensor-Packages hinzufÃ¼gen. Beispiele:

**DHT22 auf Slot 1:**
```yaml
packages:
  dht_slot1: !include
    file: packages/sensors/ow_dht_slot.yaml
    vars:
      dht_slot: "40"              # GPIO fÃ¼r Slot 1
      dht_name: "keller_temp"     # Interne ID (kein Leerzeichen)
      dht_display_name: "Keller Temp/Feuchte"
```

**Dallas One-Wire Bus auf Slot 4:**
```yaml
packages:
  dallas_bus: !include packages/sensors/ow_dallas_bus.yaml
  
  dallas_sensor_1: !include
    file: packages/sensors/ow_dallas_sensor.yaml
    vars:
      dallas_id: "dallas_bus"
      dallas_name: "keller_temp"
      dallas_address: "0x2800000000000000"  # Adresse aus Logs kopieren
```

**Mehrere Sensoren (z.B. Slot 1, 2, 3 je DHT; Slot 4 Dallas):**
```yaml
packages:
  dht_slot1: !include
    file: packages/sensors/ow_dht_slot.yaml
    vars:
      dht_slot: "40"
      dht_name: "og_temp"
      dht_display_name: "OG Temp/Feuchte"
      dht_interval: "10s"
  
  dht_slot2: !include
    file: packages/sensors/ow_dht_slot.yaml
    vars:
      dht_slot: "15"
      dht_name: "eg_temp"
      dht_display_name: "EG Temp/Feuchte"
  
  dht_slot3: !include
    file: packages/sensors/ow_dht_slot.yaml
    vars:
      dht_slot: "48"
      dht_name: "keller_temp"
      dht_display_name: "Keller Temp/Feuchte"
  
  dallas_bus: !include packages/sensors/ow_dallas_bus.yaml
  
  dallas_sensor_1: !include
    file: packages/sensors/ow_dallas_sensor.yaml
    vars:
      dallas_id: "dallas_bus"
      dallas_name: "auÃŸen_temp"
      dallas_address: "0x1234567890ABCDEF"
```

#### Schritt 4: GPIO-Zuordnung

| Slot | GPIO | Variable |
|------|------|----------|
| Slot 1 | 40 | `dht_slot: "40"` |
| Slot 2 | 15 | `dht_slot: "15"` |
| Slot 3 | 48 | `dht_slot: "48"` |
| Slot 4 | 47 | Dallas 1-Wire oder DHT |

---

### **Option B: Vordefinierte Profile (fÃ¼r spezifische Sensor-Konfigurationen)**

Wenn deine Sensor-Anordnung exakt einem der folgenden Szenarien entspricht, nutze ein spezialisiertes Profil:

| Profil | Sensoren Slot 1â€“4 | Netzwerk | Import-Link |
|--------|------------------|----------|-------------|
| **A â€“ DHT WiFi** | 4Ã— DHT (Temperatur + Feuchte) | WLAN | `github://kwittwer/esphome_kfw_lib/B16M/profiles/profile_dht_wifi.yaml@main` |
| **A â€“ DHT Ethernet** | 4Ã— DHT | Ethernet W5500 | `github://kwittwer/esphome_kfw_lib/B16M/profiles/profile_dht_ethernet.yaml@main` |
| **B â€“ Dallas WiFi** | Dallas One-Wire Bus (Slot 4) | WLAN | `github://kwittwer/esphome_kfw_lib/B16M/profiles/profile_dallas_wifi.yaml@main` |
| **B â€“ Dallas Ethernet** | Dallas One-Wire Bus (Slot 4) | Ethernet W5500 | `github://kwittwer/esphome_kfw_lib/B16M/profiles/profile_dallas_ethernet.yaml@main` |
| **C â€“ Mix WiFi** | Slot 1â€“3 DHT + Slot 4 Dallas | WLAN | `github://kwittwer/esphome_kfw_lib/B16M/profiles/profile_mix_wifi.yaml@main` |
| **C â€“ Mix Ethernet** | Slot 1â€“3 DHT + Slot 4 Dallas | Ethernet W5500 | `github://kwittwer/esphome_kfw_lib/B16M/profiles/profile_mix_ethernet.yaml@main` |

Dann Schritt 2â€“4 wie oben.

---
### **Option C: Schnelle Konfiguration mit Substitutionen (Statische IP via Dashboard GUI)**

Für Benutzer, die schnell ein Device mit den Standard-Sensoren und **editierbarer statischer IP in der Dashboard GUI** konfigurieren möchten:

#### Ethernet mit Substitutionen

| Variante | Netzwerk | Import-Link |
|----------|----------|-------------|
| **DHCP (Standard)** | Ethernet W5500, DHCP | `github://kwittwer/esphome_kfw_lib/B16M/application/setup_builder_ethernet.yaml@main` |
| **Statische IP (editierbar)** | Ethernet W5500 + statische IP, alle Parameter in Dashboard-GUI | `github://kwittwer/esphome_kfw_lib/B16M/application/setup_builder_ethernet_static.yaml@main` |

#### WiFi mit Substitutionen

| Variante | Netzwerk | Import-Link |
|----------|----------|-------------|
| **DHCP (Standard)** | WLAN, DHCP | `github://kwittwer/esphome_kfw_lib/B16M/application/setup_builder_wifi.yaml@main` |
| **Statische IP (editierbar)** | WLAN + statische IP, alle Parameter in Dashboard-GUI | `github://kwittwer/esphome_kfw_lib/B16M/application/setup_builder_wifi_static.yaml@main` |

#### Nutzung

1. **Wähle die passende Variante** (z.B. `setup_builder_ethernet_static.yaml` für Ethernet + statische IP)
2. **Im ESPHome Dashboard**: **Add Device** → **Import** → URL einfügen
3. **Dashboard zeigt Substitutions-Eingabefelder:**
   - `device_name`: Device-Identifier (z.B. `b16m-keller`)
   - `friendly_name`: Anzeigename (z.B. `Keller Sensor`)
   - Bei statischen Varianten zusätzlich:
     - `eth_static_ip` oder `wifi_static_ip`: Geräte-IP (z.B. `192.168.1.100`)
     - `eth_gateway` oder `wifi_gateway`: Gateway/Router (z.B. `192.168.1.1`)
     - `eth_subnet` oder `wifi_subnet`: Subnet-Mask (z.B. `255.255.255.0`)
     - `eth_dns` oder `wifi_dns`: DNS-Server (z.B. `192.168.1.1`)
   - Bei WiFi-Varianten zusätzlich:
     - `wifi_ssid`: WLAN-Name
     - `wifi_password`: WLAN-Passwort
     - `wifi_ap_password`: Fallback AP Passwort
4. **Alle Werte eingeben** → **Install** → Konfiguration fertig

Diese Variante enthält **bereits 4× DHT-Sensoren** (Slots 1–4) und **Standard-Thermostaten**. Die Konfiguration ist sofort einsatzbereit.

---
## Package: `B16M_base.yaml` â€“ Basis-Konfiguration

Enthaelt ESP32-S3, I2C-Bus, PCF8575 I/O-Expander (16 DI / 16 DO), ADS1115 (4 AI), Display, Anemometer, Web-Server, RTC.
Die 4 Sensor-Slots auf GPIO `40`, `15`, `48`, `47` werden separat ueber eigene Packages eingebunden.

### Verwendung

```yaml
packages:
  base: !include
    file: platform/B16M_base.yaml
    vars:
      device_name: mein_b16m
      friendly_name: "Mein B16M"
      # Nur abweichende Werte angeben â€“ alles andere nutzt die Standardwerte
```

### Alle Variablen

#### GerÃ¤t

| Variable | Standard | Beschreibung |
|---|---|---|
| `device_name` | `b16m` | mDNS-Name, Hostname |
| `friendly_name` | `"B16M Panel"` | Anzeigename in Home Assistant |
| `version` | `"2025.12.10.1"` | Firmware-Version |
| `project_name` | `"Konrad.B16M_Heimsteuerung"` | Projektname |
| `timezone` | `"Europe/Berlin"` | Zeitzone (fÃ¼r RTC und HA-Sync) |
| `rtc_update_interval` | `1h` | Wie oft die RTC neu synchronisiert wird |
| `web_server_port` | `"80"` | Port des eingebauten Web-Servers |

#### I2C Bus

| Variable | Standard | Beschreibung |
|---|---|---|
| `i2c_sda_pin` | `"38"` | SDA-Pin |
| `i2c_scl_pin` | `"39"` | SCL-Pin |
| `i2c_frequency` | `400kHz` | Bus-Frequenz |

#### I/O Expander & Peripherie-Adressen

| Variable | Standard | Beschreibung |
|---|---|---|
| `pcf_in_address` | `"0x24"` | I2C-Adresse PCF8575 EingÃ¤nge (DI1â€“DI16) |
| `pcf_out_address` | `"0x25"` | I2C-Adresse PCF8575 AusgÃ¤nge (DO1â€“DO16) |
| `ads_address` | `"0x48"` | I2C-Adresse ADS1115 (AI1â€“AI4) |
| `display_address` | `"0x3c"` | I2C-Adresse SSD1306 Display |

#### Logger / Debug

| Variable | Standard | Beschreibung |
|---|---|---|
| `logger_level` | `INFO` | Log-Level: `DEBUG`, `INFO`, `WARNING`, `ERROR` |
| `debug_update_interval` | `10s` | Update-Intervall der Debug-Sensoren (Loop-Zeit, PSRAM, CPU) |

---

## Packages: Netzwerk

Genau **ein** Netzwerk-Package pro Setup einbinden. Das Netzwerk ist **getrennt** von der Basis-Konfiguration, damit man flexibel WiFi oder Ethernet wÃ¤hlen kann.

### Option 1: WiFi + DHCP *(Standard fÃ¼r neue Setups)*

```yaml
packages:
  network: !include
    file: packages/networking/net_wifi.yaml
    vars:
      wifi_ssid: "MeinNetzwerk"
      wifi_password: "MeinPasswort"
      # Alternativ mit secrets.yaml:
      # wifi_ssid: !secret wifi_ssid
      # wifi_password: !secret wifi_password
```

| Variable | Standard | Beschreibung |
|---|---|---|
| `wifi_ssid` | `"Bitte_SSID_setzen"` | WLAN-Netzwerkname |
| `wifi_password` | `"Bitte_Passwort_setzen"` | WLAN-Passwort |
| `wifi_ap_ssid` | `"B16M Fallback AP"` | Name des Fallback-Accesspoints (wenn kein WLAN erreichbar) |
| `wifi_ap_password` | `"b16m12345"` | Passwort des Fallback-APs |

### Option 2: WiFi + statische IP

```yaml
packages:
  network: !include
    file: packages/networking/net_wifi_static.yaml
    vars:
      wifi_ssid: "MeinNetzwerk"
      wifi_password: "MeinPasswort"
      static_ip: "192.168.1.100"
      gateway: "192.168.1.1"
      # subnet und dns sind optional (Standard: 255.255.255.0 / Gateway)
```

| Variable | Standard | Beschreibung |
|---|---|---|
| `wifi_ssid` | `"Bitte_SSID_setzen"` | WLAN-Netzwerkname |
| `wifi_password` | `"Bitte_Passwort_setzen"` | WLAN-Passwort |
| `wifi_ap_ssid` | `"B16M Fallback AP"` | Fallback-AP Name |
| `wifi_ap_password` | `"b16m12345"` | Fallback-AP Passwort |
| `static_ip` | `"192.168.1.100"` | Statische IP-Adresse |
| `gateway` | `"192.168.1.1"` | Standard-Gateway |
| `subnet` | `"255.255.255.0"` | Subnetzmaske |
| `dns` | `"192.168.1.1"` | DNS-Server (Standard: Gateway) |

### Option 3: Ethernet W5500 + DHCP *(Standard B16M-Hardware)*

```yaml
packages:
  network: !include packages/networking/net_ethernet.yaml
  # Pins entsprechen der Standard-B16M-Verdrahtung, keine vars nÃ¶tig
```

Bei abweichender Verdrahtung:

```yaml
packages:
  network: !include
    file: packages/networking/net_ethernet.yaml
    vars:
      eth_clk_pin: GPIO42   # nur abweichende Pins angeben
      eth_cs_pin: GPIO10
```

| Variable | Standard | Beschreibung |
|---|---|---|
| `eth_clk_pin` | `GPIO42` | SPI CLK |
| `eth_mosi_pin` | `GPIO43` | SPI MOSI |
| `eth_miso_pin` | `GPIO44` | SPI MISO |
| `eth_cs_pin` | `GPIO41` | SPI Chip Select |
| `eth_int_pin` | `GPIO2` | W5500 Interrupt |
| `eth_rst_pin` | `GPIO1` | W5500 Reset |

### Option 4: Ethernet W5500 + statische IP

```yaml
packages:
  network: !include
    file: packages/networking/net_ethernet_static.yaml
    vars:
      static_ip: "192.168.1.100"
      gateway: "192.168.1.1"
      # subnet und dns optional
      # eth_*_pin bei abweichender Verdrahtung
```

| Variable | Standard | Beschreibung |
|---|---|---|
| `eth_clk_pin` â€¦ `eth_rst_pin` | (s. oben) | SPI-Pins (nur bei abweichender Verdrahtung) |
| `static_ip` | `"192.168.1.100"` | Statische IP-Adresse |
| `gateway` | `"192.168.1.1"` | Standard-Gateway |
| `subnet` | `"255.255.255.0"` | Subnetzmaske |
| `dns` | `"192.168.1.1"` | DNS-Server |

---

## Packages: Sensor-Slots

Die 4 Anschluesse auf GPIO `40`, `15`, `48`, `47` werden nicht in der Base fest verdrahtet.
Stattdessen gilt pro Slot:

- entweder `ow_dht_slot.yaml` fuer genau einen DHT Sensor
- oder `ow_dallas_bus.yaml` fuer einen One-Wire Bus

Auf einem Dallas Bus koennen mehrere Sensoren ueber `ow_dallas_sensor.yaml` angelegt werden. In dieser Struktur kannst du bis zu 16 Dallas Sensoren per wiederholtem Package-Include anbinden.

Die Referenzkonfiguration in `b16m.yaml` verwendet standardmaessig:

- Slot 1 bis 3 als DHT
- Slot 4 auf GPIO `47` als Dallas Bus mit 16 vorbereiteten Dallas Sensor-Instanzen

### Option 1: DHT auf einem Slot

```yaml
packages:
  sensor_slot_1: !include
    file: packages/sensors/ow_dht_slot.yaml
    vars:
      dht_pin: "40"
      dht_name: "Wohnzimmer_Temp"
      dht_id: "T1"
      dht_update_interval: 10s
```

| Variable | Standard | Beschreibung |
|---|---|---|
| `dht_pin` | `"40"` | GPIO-Pin des Slots |
| `dht_name` | `"Temp1"` | Basisname fuer Temperatur-/Humidity-Entitaeten |
| `dht_id` | `"T1"` | ESPHome-ID des DHT Geraets |
| `dht_update_interval` | `10s` | Update-Intervall |

Der Name erzeugt automatisch die Sensor-IDs `"<dht_name>_temperature"` und `"<dht_name>_humidity"`.

### Option 2: Dallas Bus auf einem Slot

```yaml
packages:
  sensor_slot_1: !include
    file: packages/sensors/ow_dallas_bus.yaml
    vars:
      dallas_bus_id: ow_slot_1
      dallas_pin: "40"
```

| Variable | Standard | Beschreibung |
|---|---|---|
| `dallas_bus_id` | `ow_bus_1` | ESPHome-ID des One-Wire Bus |
| `dallas_pin` | `"40"` | GPIO-Pin des Slots |

### Dallas Sensor auf einem Bus

```yaml
packages:
  dallas_sensor_1: !include
    file: packages/sensors/ow_dallas_sensor.yaml
    vars:
      one_wire_id: ow_slot_1
      dallas_address: 0x1234567812345628
      dallas_sensor_name: "Aussen_Temp"
      dallas_sensor_id: outdoor_temp
      dallas_update_interval: 10s
```

| Variable | Standard | Beschreibung |
|---|---|---|
| `one_wire_id` | `ow_bus_1` | Referenz auf den One-Wire Bus |
| `dallas_address` | `0x0000000000000000` | Dallas ROM-Adresse des Sensors |
| `dallas_sensor_name` | `"Dallas_Sensor"` | Anzeigename |
| `dallas_sensor_id` | `dallas_sensor` | ESPHome-ID des Temperatursensors |
| `dallas_update_interval` | `10s` | Update-Intervall |

Die `dallas_address` entspricht der eindeutigen Sensoradresse aus dem ESPHome Log oder einer Discovery-Konfiguration.

---

## Package: `thermostat_template.yaml` â€“ Thermostat-Instanz

Pro Thermostat ein Eintrag unter `packages:`. Bis zu 10 Thermostate mÃ¶glich.

```yaml
packages:
  thermostat1: !include
    file: packages/heating/thermostat_template.yaml
    vars:
      thermostat_id: 1
      thermostat_name: "Wohnzimmer"
      thermostat_fallback_sensor: Schlafzimmer_Temp_temperature
      thermostat_fallback_humidity_sensor: Schlafzimmer_Temp_humidity
      thermostat_valve_switch: DO1
      Thermostat_external_sensor_timeout: "900"
```

| Variable | Beschreibung |
|---|---|
| `thermostat_id` | Eindeutige Nummer (1â€“10), keine doppelten |
| `thermostat_name` | Initialer Anzeigename (Default) |
| `thermostat_fallback_sensor` | ESPHome Sensor-ID des Fallback-Temperatursensors (z.B. aus `dht_name + "_temperature"`) |
| `thermostat_fallback_humidity_sensor` | ESPHome Sensor-ID des Fallback-Feuchtigkeitssensors |
| `thermostat_valve_switch` | DO-ID des Ventil-Ausgangs (z.B. `DO1`) |
| `Thermostat_external_sensor_timeout` | Timeout in Sekunden bis zum Fallback-Sensor (Standard: `"900"`) |

### Thermostat-Namen zur Laufzeit aendern (persistent)

Ab Firmware-Stand 2026.6.x erzeugt jede Thermostat-Instanz zusaetzlich ein editierbares Text-Entity:

- `text.thermostat<id>_name` (in Home Assistant und im ESPHome-Webinterface sichtbar)

Verhalten:

- Aenderungen werden persistent gespeichert (`restore_value: true`) und nach Reboot wiederhergestellt.
- Die OLED-Thermostatseite nutzt diesen gespeicherten Laufzeitnamen.
- Leerer Eingabewert faellt automatisch auf `thermostat_name` aus der YAML zurueck.
- Climate-Entity-IDs bleiben stabil und werden nicht zur Laufzeit umbenannt.

Optionaler API-Service pro Instanz:

- `set_display_name_thermostat<thermostat_id>`

> **Fallback-Sensor ID:**  
> Die Sensor-IDs werden aus dem jeweiligen Sensor-Slot-Namen gebildet.  
> `dht_name: "Schlafzimmer_Temp"` -> `thermostat_fallback_sensor: Schlafzimmer_Temp_temperature`

AuÃŸerdem wird `thermostat_base` eingebunden (Heizungsmaster-Schalter, Hardware-Eingang DI1):

```yaml
packages:
  thermostat_base: !include shared/Thermostate.yaml
```

---

## OLED Display Standardseiten

Die aktive Display-Implementierung liegt in [B16M/platform/B16M_base.yaml](B16M/platform/B16M_base.yaml).

Standardmaessig sind folgende Seiten aktiv:

1. Homescreen: Uhrzeit + blinkendes Heartbeat-Icon (1s)
2. Verbindungsseite: Home-Assistant/API-Status + IP-Adresse
3. Infoseite: Firmware-Version + ESPHome-Version + Uptime
4. Inputs: DI1-DI16 als High/Low-Iconmatrix
5. Outputs: DO1-DO16 als High/Low-Iconmatrix
6. Sensorseiten aus Base: AI1, AI2, AI3, AI4, Wind

Zusaetzlich kommen pro Template-Instanz automatisch Seiten dazu:

- Pro Thermostat-Instanz (aus [B16M/packages/heating/thermostat_template.yaml](B16M/packages/heating/thermostat_template.yaml)): Name, Temperatur, Feuchte, Sollwert, Action/Mode, Ventilstatus, Fallbackstatus
- Pro DHT-Slot-Instanz (aus [B16M/packages/sensors/ow_dht_slot.yaml](B16M/packages/sensors/ow_dht_slot.yaml)): Name, Temperatur, Feuchte, Status
- Pro Dallas-Sensor-Instanz (aus [B16M/packages/sensors/ow_dallas_sensor.yaml](B16M/packages/sensors/ow_dallas_sensor.yaml)): Name, Wert, Status

Navigation:

- Auto-Rotation: alle 4 Sekunden naechste Seite
- Manuell: DI16 schaltet sofort zur naechsten Seite
- Heartbeat: Icon blinkt jede Sekunde

---

## VollstÃ¤ndiges Beispiel: Minimales Setup

```yaml
# setup_keller.yaml

packages:
  base: !include
    file: platform/B16M_base.yaml
    vars:
      device_name: b16m_keller
      friendly_name: "B16M Keller"
      version: "2025.12.10.1"

  network: !include
    file: packages/networking/net_ethernet_static.yaml
    vars:
      static_ip: "192.168.1.42"
      gateway: "192.168.1.1"

  sensor_slot_1: !include
    file: packages/sensors/ow_dht_slot.yaml
    vars:
      dht_pin: "40"
      dht_name: "Keller_Temp"
      dht_id: "T1"

  sensor_slot_2: !include
    file: packages/sensors/ow_dht_slot.yaml
    vars:
      dht_pin: "15"
      dht_name: "Heizraum_Temp"
      dht_id: "T2"

  thermostat_base: !include shared/Thermostate.yaml

  thermostat1: !include
    file: packages/heating/thermostat_template.yaml
    vars:
      thermostat_id: 1
      thermostat_name: "Keller"
      thermostat_fallback_sensor: Heizraum_Temp_temperature
      thermostat_fallback_humidity_sensor: Heizraum_Temp_humidity
      thermostat_valve_switch: DO1
      Thermostat_external_sensor_timeout: "900"

api:
  encryption:
    key: "<neuen Key generieren: esphome generate-secrets>"
  reboot_timeout: 6h

ota:
  - platform: esphome
    password: "<OTA-Passwort>"
```

---

## Kompilieren & Flashen

```bash
# Konfiguration prÃ¼fen:
esphome config setup_keller.yaml

# Kompilieren:
esphome compile setup_keller.yaml

# OTA-Upload:
esphome run setup_keller.yaml --device OTA
```

---

## Vollstaendige Optionen-Referenz

Dieser Abschnitt dokumentiert alle aktuell verwendeten Konfigurationsoptionen der aktiven Package-Struktur in B16M.

### 1) Base-Package: platform/B16M_base.yaml

Pflicht: keine. Alle Werte haben Defaults.

Konfigurierbare Variablen:

- device_name: b16m
- friendly_name: B16M Panel
- version: 2025.12.10.1
- project_name: Konrad.B16M_Heimsteuerung
- timezone: Europe/Berlin
- rtc_update_interval: 1h
- web_server_port: 80
- i2c_sda_pin: 38
- i2c_scl_pin: 39
- i2c_frequency: 400kHz
- pcf_in_address: 0x24
- pcf_out_address: 0x25
- ads_address: 0x48
- display_address: 0x3c
- logger_level: INFO
- debug_update_interval: 10s

Feste, automatisch aktive Basisfunktionen:

- ESP32-S3 Konfiguration mit ESP-IDF und PSRAM (octal, 80 MHz)
- DS1307 RTC plus Home Assistant Time Sync
- I2C-Bus und Hardware-Komponenten:
  - DI1 bis DI16 ueber PCF8575 Input
  - DO1 bis DO16 ueber PCF8575 Output
  - AI1 bis AI4 ueber ADS1115
  - Wind-Geschwindigkeit (Anemometer) als Pulsecounter
- Netzwerkunabhaengige IP-Textanzeige auf dem Display per network::get_ip_address()
- OLED Display mit Seitenrotation und Heartbeat
- Webserver und Debug-Sensoren

Hinweis:

- Die Sensor-Slots (GPIO 40, 15, 48, 47) werden absichtlich nicht in der Base festgelegt.
- Diese kommen immer ueber ow_dht_slot oder ow_dallas_bus aus dem Top-Level-Setup.

### 2) Netzwerk-Packages

Genau eines davon pro Setup einbinden.

WiFi DHCP (packages/networking/net_wifi.yaml):

- wifi_ssid: Bitte_SSID_setzen
- wifi_password: Bitte_Passwort_setzen
- wifi_ap_ssid: B16M Fallback AP
- wifi_ap_password: b16m12345

WiFi statisch (packages/networking/net_wifi_static.yaml):

- wifi_ssid: Bitte_SSID_setzen
- wifi_password: Bitte_Passwort_setzen
- wifi_ap_ssid: B16M Fallback AP
- wifi_ap_password: b16m12345
- static_ip: 192.168.1.100
- gateway: 192.168.1.1
- subnet: 255.255.255.0
- dns: 192.168.1.1

Ethernet DHCP (packages/networking/net_ethernet.yaml):

- eth_clk_pin: GPIO42
- eth_mosi_pin: GPIO43
- eth_miso_pin: GPIO44
- eth_cs_pin: GPIO41
- eth_int_pin: GPIO2
- eth_rst_pin: GPIO1

Ethernet statisch (packages/networking/net_ethernet_static.yaml):

- eth_clk_pin: GPIO42
- eth_mosi_pin: GPIO43
- eth_miso_pin: GPIO44
- eth_cs_pin: GPIO41
- eth_int_pin: GPIO2
- eth_rst_pin: GPIO1
- static_ip: 192.168.1.100
- gateway: 192.168.1.1
- subnet: 255.255.255.0
- dns: 192.168.1.1

### 3) Sensor-Slot-Packages

Pro physischem Slot genau eine Bus-Variante:

- DHT direkt am Slot: packages/sensors/ow_dht_slot.yaml
- Dallas One-Wire Bus am Slot: packages/sensors/ow_dallas_bus.yaml

DHT Slot Variablen:

- dht_pin: 40
- dht_name: Temp1
- dht_id: T1
- dht_update_interval: 10s

Automatisch erzeugte IDs bei DHT:

- <dht_name>_temperature
- <dht_name>_humidity
- Display-Seite page_sensor_<dht_id>

Dallas Bus Variablen:

- dallas_bus_id: ow_bus_1
- dallas_pin: 40

Dallas Sensor Template (packages/sensors/ow_dallas_sensor.yaml) pro Sensorinstanz:

- one_wire_id: ow_bus_1
- dallas_address: 0x0000000000000000
- dallas_sensor_name: Dallas_Sensor
- dallas_sensor_id: dallas_sensor
- dallas_update_interval: 10s

Automatisch erzeugt bei Dallas Sensor:

- Sensor-ID: <dallas_sensor_id>
- Display-Seite: page_sensor_<dallas_sensor_id>

### 4) Thermostat-Packages

Thermostat-Basis (shared/Thermostate.yaml):

Wird einmalig eingebunden und stellt bereit:

- external_components fuer dummy_thermostat
- local_control_switch
- heating_master_switch
- hw_master_sw (abgeleitet von DI1)

Thermostat-Instanz (packages/heating/thermostat_template.yaml), pro Thermostat einmal:

- thermostat_id: Pflicht, eindeutig
- thermostat_name: Pflicht
- thermostat_fallback_sensor: Pflicht, Sensor-ID
- thermostat_fallback_humidity_sensor: Pflicht, Sensor-ID
- thermostat_valve_switch: Pflicht, z. B. DO1
- Thermostat_external_sensor_timeout: empfohlen, Standard in Beispielen 900

Automatisch erzeugt pro Thermostatinstanz:

- climate id thermostat<thermostat_id>
- text id thermostat<thermostat_id>_name (persistenter Laufzeit-Anzeigename)
- Diagnose binary_sensor t<thermostat_id>_fallback_temp_active
- Display-Seite page_thermostat_<thermostat_id>
- API-Services:
  - set_display_name_thermostat<thermostat_id>
  - set_current_temperature_thermostat<thermostat_id>
  - set_current_humidity_thermostat<thermostat_id>
  - set_valve_state_thermostat<thermostat_id>

### 5) Display-Logik komplett

Aktive Display-Definition ist im Base-Package.

Fixe Basisseiten:

- page_home
- page_connection
- page_info
- page_inputs
- page_outputs
- page_sensor_ai1
- page_sensor_ai2
- page_sensor_ai3
- page_sensor_ai4
- page_sensor_wind

Dynamische Seiten aus Templates:

- DHT: page_sensor_<dht_id>
- Dallas: page_sensor_<dallas_sensor_id>
- Thermostat: page_thermostat_<thermostat_id>

Navigation und Verhalten:

- Auto-Rotation alle 4 Sekunden
- Manueller Seitensprung bei DI16 Press
- Heartbeat-Blinken alle 1 Sekunde

### 6) API und OTA im Top-Level-Setup

Im Setup-File (z. B. b16m.yaml, setup_wohnzimmer.yaml, setup_template.yaml) zu setzen:

- api.encryption.key
- api.reboot_timeout
- ota.platform: esphome
- ota.password

Empfehlung:

- API-Key und OTA-Passwort ueber secrets.yaml verwalten.

### 7) Namens- und ID-Regeln

- thermostat_id darf nicht doppelt vorkommen.
- dht_name sollte eindeutig sein, da daraus Sensor-IDs gebildet werden.
- dallas_sensor_id und dallas_bus_id muessen eindeutig sein.
- thermostat_fallback_sensor muss exakt auf eine existierende Sensor-ID zeigen.
- thermostat_fallback_humidity_sensor muss exakt auf eine existierende Feuchte-ID zeigen.

### 8) Aktueller Standard in b16m.yaml

- Netzwerk: Ethernet DHCP
- Slot 1 bis 3: DHT
- Slot 4: Dallas Bus
- Slot 4 Standard: 16 Dallas Sensorinstanzen
- 4 Thermostat-Instanzen

### 9) Validierung und bekannte Besonderheit

Pruefen:

```bash
esphome config b16m.yaml
esphome config setup_wohnzimmer.yaml
esphome config setup_template.yaml
```

Wichtig:

- ESPHome kann in manchen PowerShell-Pipelines trotz gueltiger Konfiguration einen Exit Code 1 melden, wenn Informationen ueber stderr ausgegeben werden.
- Verlass dich in dem Fall auf die Ausgabezeile INFO Configuration is valid.
