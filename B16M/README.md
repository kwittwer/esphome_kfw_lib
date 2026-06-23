# B16M ESPHome Firmware

Modulare ESPHome-Firmware für den **B16M Industrial Controller** (ESP32-S3, 16 DI, 16 DO, 4 AI, W5500 Ethernet, DHT-Sensoren, SSD1306 Display).

Die Konfiguration ist vollständig package-template-basiert. Ein neues Setup-File besteht nur aus den Werten, die vom Standard abweichen.

---

## Dateistruktur

```
B16M/
├── b16m.yaml                    # Referenz-Konfiguration (alle Standardwerte)
├── setup_wohnzimmer.yaml        # Beispiel: echtes Setup mit 4 Thermostaten
├── setup_template.yaml          # Vorlage für neue Setups (hier anfangen!)
├── secrets.yaml                 # WiFi-Passwörter, API-Keys, OTA-Passwörter
│
├── B16M_components/
│   ├── B16M_base.yaml           # Basis-Konfiguration (ESP32, Hardware, I2C, Display)
│   ├── net_wifi.yaml            # Netzwerk: WiFi + DHCP
│   ├── net_wifi_static.yaml     # Netzwerk: WiFi + statische IP
│   ├── net_ethernet.yaml        # Netzwerk: Ethernet W5500 + DHCP
│   ├── net_ethernet_static.yaml # Netzwerk: Ethernet W5500 + statische IP
│   └── thermostat_template.yaml # Thermostat-Instanz (parametrisiert)
│
└── Peripherie/
    └── Thermostate.yaml         # Gemeinsame Thermostat-Basis (Schalter, Master-Sensor)
```

---

## Neues Setup erstellen

1. `setup_template.yaml` kopieren und umbenennen (z.B. `setup_keller.yaml`)
2. `base:`-Package anpassen (Gerätename, Sensor-Namen, nur abweichende Werte)
3. `network:`-Package wählen (siehe unten)
4. Thermostaten konfigurieren
5. `api:` und `ota:` Schlüssel anpassen

---

## Package: `B16M_base.yaml` – Basis-Konfiguration

Enthält ESP32-S3, I2C-Bus, PCF8575 I/O-Expander (16 DI / 16 DO), ADS1115 (4 AI), DHT-Sensoren, Display, Anemometer, Web-Server, RTC.

### Verwendung

```yaml
packages:
  base: !include
    file: B16M_components/B16M_base.yaml
    vars:
      device_name: mein_b16m
      friendly_name: "Mein B16M"
      # Nur abweichende Werte angeben – alles andere nutzt die Standardwerte
```

### Alle Variablen

#### Gerät

| Variable | Standard | Beschreibung |
|---|---|---|
| `device_name` | `b16m` | mDNS-Name, Hostname |
| `friendly_name` | `"B16M Panel"` | Anzeigename in Home Assistant |
| `version` | `"2025.12.10.1"` | Firmware-Version |
| `project_name` | `"Konrad.B16M_Heimsteuerung"` | Projektname |
| `timezone` | `"Europe/Berlin"` | Zeitzone (für RTC und HA-Sync) |
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
| `pcf_in_address` | `"0x24"` | I2C-Adresse PCF8575 Eingänge (DI1–DI16) |
| `pcf_out_address` | `"0x25"` | I2C-Adresse PCF8575 Ausgänge (DO1–DO16) |
| `ads_address` | `"0x48"` | I2C-Adresse ADS1115 (AI1–AI4) |
| `display_address` | `"0x3c"` | I2C-Adresse SSD1306 Display |

#### DHT-Sensoren (T1–T4)

| Variable | Standard | Beschreibung |
|---|---|---|
| `dht1_pin` | `"40"` | GPIO-Pin DHT-Sensor 1 |
| `dht2_pin` | `"15"` | GPIO-Pin DHT-Sensor 2 |
| `dht3_pin` | `"48"` | GPIO-Pin DHT-Sensor 3 |
| `dht4_pin` | `"47"` | GPIO-Pin DHT-Sensor 4 |
| `dht1_update_interval` | `10s` | Update-Intervall DHT 1 |
| `dht2_update_interval` | `1s` | Update-Intervall DHT 2 |
| `dht3_update_interval` | `10s` | Update-Intervall DHT 3 |
| `dht4_update_interval` | `10s` | Update-Intervall DHT 4 |
| `T1_name` | `"Temp1"` | Name für DHT 1 (Sensor-ID und HA-Entität) |
| `T2_name` | `"Temp2"` | Name für DHT 2 |
| `T3_name` | `"Temp3"` | Name für DHT 3 |
| `T4_name` | `"Temp4"` | Name für DHT 4 |

> **Wichtig:** `T{n}_name` bestimmt die ESPHome Sensor-IDs.  
> Beispiel: `T2_name: "Schlafzimmer_Temp"` → IDs `Schlafzimmer_Temp_temperature` und `Schlafzimmer_Temp_humidity`.  
> Diese IDs werden in den Thermostat-Fallback-Sensoren referenziert.

#### Logger / Debug

| Variable | Standard | Beschreibung |
|---|---|---|
| `logger_level` | `INFO` | Log-Level: `DEBUG`, `INFO`, `WARNING`, `ERROR` |
| `debug_update_interval` | `10s` | Update-Intervall der Debug-Sensoren (Loop-Zeit, PSRAM, CPU) |

---

## Packages: Netzwerk

Genau **ein** Netzwerk-Package pro Setup einbinden. Das Netzwerk ist **getrennt** von der Basis-Konfiguration, damit man flexibel WiFi oder Ethernet wählen kann.

### Option 1: WiFi + DHCP *(Standard für neue Setups)*

```yaml
packages:
  network: !include
    file: B16M_components/net_wifi.yaml
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
    file: B16M_components/net_wifi_static.yaml
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
  network: !include B16M_components/net_ethernet.yaml
  # Pins entsprechen der Standard-B16M-Verdrahtung, keine vars nötig
```

Bei abweichender Verdrahtung:

```yaml
packages:
  network: !include
    file: B16M_components/net_ethernet.yaml
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
    file: B16M_components/net_ethernet_static.yaml
    vars:
      static_ip: "192.168.1.100"
      gateway: "192.168.1.1"
      # subnet und dns optional
      # eth_*_pin bei abweichender Verdrahtung
```

| Variable | Standard | Beschreibung |
|---|---|---|
| `eth_clk_pin` … `eth_rst_pin` | (s. oben) | SPI-Pins (nur bei abweichender Verdrahtung) |
| `static_ip` | `"192.168.1.100"` | Statische IP-Adresse |
| `gateway` | `"192.168.1.1"` | Standard-Gateway |
| `subnet` | `"255.255.255.0"` | Subnetzmaske |
| `dns` | `"192.168.1.1"` | DNS-Server |

---

## Package: `thermostat_template.yaml` – Thermostat-Instanz

Pro Thermostat ein Eintrag unter `packages:`. Bis zu 10 Thermostate möglich.

```yaml
packages:
  thermostat1: !include
    file: B16M_components/thermostat_template.yaml
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
| `thermostat_id` | Eindeutige Nummer (1–10), keine doppelten |
| `thermostat_name` | Anzeigename in Home Assistant |
| `thermostat_fallback_sensor` | ESPHome Sensor-ID des Fallback-Temperatursensors (aus `T{n}_name + "_temperature"`) |
| `thermostat_fallback_humidity_sensor` | ESPHome Sensor-ID des Fallback-Feuchtigkeitssensors |
| `thermostat_valve_switch` | DO-ID des Ventil-Ausgangs (z.B. `DO1`) |
| `Thermostat_external_sensor_timeout` | Timeout in Sekunden bis zum Fallback-Sensor (Standard: `"900"`) |

> **Fallback-Sensor ID:**  
> Die Sensor-IDs werden aus dem `T{n}_name` in `B16M_base.yaml` gebildet.  
> `T2_name: "Schlafzimmer_Temp"` → `thermostat_fallback_sensor: Schlafzimmer_Temp_temperature`

Außerdem wird `thermostat_base` eingebunden (Heizungsmaster-Schalter, Hardware-Eingang DI1):

```yaml
packages:
  thermostat_base: !include Peripherie/Thermostate.yaml
```

---

## Vollständiges Beispiel: Minimales Setup

```yaml
# setup_keller.yaml

packages:
  base: !include
    file: B16M_components/B16M_base.yaml
    vars:
      device_name: b16m_keller
      friendly_name: "B16M Keller"
      version: "2025.12.10.1"
      T1_name: "Keller_Temp"
      T2_name: "Heizraum_Temp"

  network: !include
    file: B16M_components/net_ethernet_static.yaml
    vars:
      static_ip: "192.168.1.42"
      gateway: "192.168.1.1"

  thermostat_base: !include Peripherie/Thermostate.yaml

  thermostat1: !include
    file: B16M_components/thermostat_template.yaml
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
# Konfiguration prüfen:
esphome config setup_keller.yaml

# Kompilieren:
esphome compile setup_keller.yaml

# OTA-Upload:
esphome run setup_keller.yaml --device OTA
```
