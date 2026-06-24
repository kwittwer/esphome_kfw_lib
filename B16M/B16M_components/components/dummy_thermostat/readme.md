# ESPHome Dummy Thermostat Component

Virtueller ESPHome-Thermostat, der Istwert und Ventilstellung von externen Systemen (Home Assistant, Wärmepumpe) empfängt und bei Verbindungsabbruch automatisch auf lokale Regelung umschaltet.

## Konzept / Datenfluss

```
ESPHome-Knoten A  ──→  HA  ──→  set_current_temperature()  ──→  Thermostat
Wärmepumpe        ──→  HA  ──→  set_valve_state()           ──→  Thermostat
                                                                       │
                                                                  valve_switch /
                                                                  valve_output
```

### Temperaturquelle (Kaskade)

| Priorität | Quelle | Aktivierung |
|-----------|--------|-------------|
| 1 (primär) | Extern via `set_current_temperature()` Service | Solange Updates kommen |
| 2 (Fallback) | Lokaler `fallback_sensor` | Nach `sensor_timeout` ohne externen Update |
| — (Ausfall) | Kein gültiger Wert (NAN) | Nach `fallback_sensor_timeout` ohne Sensor-Update |

### Ventilsteuerung (Kaskade)

| Priorität | Quelle | Aktivierung |
|-----------|--------|-------------|
| 1 (primär) | Extern via `set_valve_state()` Service | Solange Updates kommen |
| 2 (Fallback) | Lokale Hysterese | Nach `valve_update_timeout` oder wenn `use_local_valve_control: true` |

Der physische Ausgang (`valve_switch` / `valve_output`) wird nur geschaltet wenn `valve_control_enabled = true` und `mode != OFF`.

## Unterstützte Modi

| HA-Anzeige | ESPHome-Modus | Lokale Regelung |
|------------|---------------|-----------------|
| Aus | `CLIMATE_MODE_OFF` | Ventil immer AUS |
| Heizen | `CLIMATE_MODE_HEAT` | Hysterese (Heizen) |
| Kühlen | `CLIMATE_MODE_COOL` | Ventil AUS + Warnung |

> **Hinweis:** Im lokalen Fallback-Modus ist Kühlen nicht unterstützt. Bei `COOL`-Modus bleibt das Ventil AUS.

## YAML-Konfiguration

```yaml
climate:
  - platform: dummy_thermostat
    name: "Wohnzimmer"
    id: thermostat1

    # Lokaler Fallback-Temperatursensor (wenn externer Istwert ausbleibt)
    fallback_sensor: my_local_temp_sensor
    sensor_timeout: 600           # Sekunden bis Fallback aktiv wird (Default: 600)
    fallback_sensor_timeout: 600  # Sekunden bis Fallback-Sensor selbst als ausgefallen gilt (Default: 600)

    # Lokaler Fallback-Feuchtigkeitssensor (optional)
    fallback_humidity_sensor: my_local_hum_sensor
    humidity_sensor_timeout: 0    # 0 = deaktiviert (Default: 0)

    # Physischer Ventilausgang (einer von beiden)
    valve_switch: DO1             # ESPHome Switch
    # valve_output: my_output     # oder Binary Output

    # Ventilsteuerung Freigabe (Master-Switch)
    valve_control_enabled: !lambda |-
      return id(heating_master_switch).state && id(hw_master_sw).state;

    # Steuermodus: false = extern (Wärmepumpe), true = lokal (Hysterese)
    use_local_valve_control: !lambda |-
      return id(local_control_switch).state;

    # Timeout für externe Ventilsteuerung (Wärmepumpe)
    valve_update_timeout: 300     # Sekunden ohne set_valve_state() → lokale Übernahme (0 = deaktiviert)

    # Hysterese-Parameter (nur für lokale Heizregelung relevant)
    heating_deadband: 0.5   # Heizen EIN wenn T < Soll - deadband
    heating_overrun: 0.1    # Heizen AUS wenn T >= Soll + overrun
    cooling_deadband: 0.5   # (nicht in lokaler Regelung genutzt)
    cooling_overrun: 0.5

    visual:
      min_temperature: 15.0
      max_temperature: 30.0
      temperature_step:
        target_temperature: 0.5
        current_temperature: 0.1
```

## Alle YAML-Optionen

| Option | Typ | Default | Beschreibung |
|--------|-----|---------|--------------|
| `fallback_sensor` | Sensor-ID | — | Lokaler Temperatursensor als Fallback |
| `sensor_timeout` | int (s) | 600 | Sekunden ohne externen Update → Fallback-Sensor aktiv |
| `fallback_sensor_timeout` | int (s) | 600 | Sekunden ohne Fallback-Sensor-Update → Temperatur = NAN |
| `fallback_humidity_sensor` | Sensor-ID | — | Lokaler Feuchtigkeitssensor als Fallback |
| `humidity_sensor_timeout` | int (s) | 0 | Sekunden bis Feuchte-Fallback (0 = deaktiviert) |
| `valve_switch` | Switch-ID | — | ESPHome-Switch für physisches Ventil |
| `valve_output` | Output-ID | — | Binary Output für physisches Ventil |
| `valve_control_enabled` | bool/lambda | false | Master-Freigabe: nur wenn true wird das Ventil geschaltet |
| `use_local_valve_control` | bool/lambda | false | true = immer lokale Hysterese, false = extern + Timeout-Fallback |
| `valve_update_timeout` | int (s) | 0 | Sekunden ohne `set_valve_state()` → lokale Übernahme (0 = deaktiviert) |
| `heating_deadband` | float (°C) | 0.5 | Heizen EIN wenn `T < Soll - deadband` |
| `heating_overrun` | float (°C) | 0.5 | Heizen AUS wenn `T >= Soll + overrun` |
| `cooling_deadband` | float (°C) | 0.5 | (für externe Kühlsteuerung, nicht lokal genutzt) |
| `cooling_overrun` | float (°C) | 0.5 | (für externe Kühlsteuerung, nicht lokal genutzt) |

## Services (aus HA aufrufbar)

| Service | Parameter | Beschreibung |
|---------|-----------|--------------|
| `set_valve_state_thermostatN` | `valve: bool` | Externe Ventilstellung setzen (von Wärmepumpe etc.) |
| `set_current_temperature_thermostatN` | `wert: float` | Istwert von externem Knoten setzen |
| `set_current_humidity_thermostatN` | `wert: float` | Istwert Feuchte von externem Knoten setzen |
| `set_display_name_thermostatN` | `name: string` | Anzeigename ändern |

Jeder Service-Aufruf aktualisiert den internen Timestamp — das verhindert das Auslösen des jeweiligen Timeouts.

## Timeout-Verhalten

### Temperatur-Fallback
```
Extern sendet set_current_temperature()
  └── Kein Update für > sensor_timeout (600s)
        → using_fallback_temperature = true
        → Lokaler fallback_sensor übernimmt
        → [W] Temperature sensor timeout! Switching to fallback sensor.

Extern sendet wieder
  → using_fallback_temperature = false
  → [I] Temperature sensor recovered. Switching back to main sensor.

Fallback-Sensor selbst sendet nicht für > fallback_sensor_timeout (600s)
  → current_temperature = NAN
  → Lokale Ventilregelung friert ein (kein gültiger Vergleichswert)
  → [W] Fallback sensor timeout! No valid temperature source available.
```

### Ventil-Fallback
```
Wärmepumpe sendet set_valve_state()
  └── Kein Update für > valve_update_timeout (300s)
        → valve_timeout_active = true
        → Lokale Hysterese übernimmt automatisch
        → [W] Valve update timeout! Switching to local controll.

Wärmepumpe sendet wieder
  → valve_timeout_active = false
  → [I] Valve update timeout recovered. Switching back to main external controll.
```

## Lokale Hysterese-Regelung

Wird aktiv wenn `use_local_valve_control = true` OR `valve_timeout_active = true`.

```
Heizen EIN  wenn:  T < Soll - heating_deadband
Heizen AUS  wenn:  T >= Soll + heating_overrun

Kühlen im lokalen Modus: Ventil bleibt AUS + Warnung im Log
```

Die lokale Berechnung wird maximal alle 500 ms ausgeführt (Throttle).

## Action-Berechnung

| Modus | Ventil-State | Action |
|-------|-------------|--------|
| OFF | — | OFF |
| HEAT | OFF | IDLE |
| HEAT | ON | HEATING |
| COOL | OFF | IDLE |
| COOL | ON | COOLING |

> Bei lokalem Fallback und `COOL`-Modus: Ventil wird auf OFF erzwungen → Action = IDLE.

## Lambda-Getter

```cpp
thermostat->is_valve_control_enabled()      // Master-Freigabe aktiv?
thermostat->is_using_local_valve_control()  // use_local_valve_control = true?
thermostat->get_valve_state()               // Interner Ventilstatus ON/OFF
thermostat->is_using_fallback_temperature() // Fallback-Sensor aktiv?
thermostat->is_using_fallback_humidity()    // Feuchte-Fallback aktiv?
thermostat->is_using_local_controller()    // Valve-Timeout-Fallback aktiv?
thermostat->is_valve_switch_timeout_active() // (identisch zu oben)

// Standard Climate-Eigenschaften:
thermostat->current_temperature
thermostat->current_humidity
thermostat->target_temperature
thermostat->mode
thermostat->action
```

## Typische Log-Ausgaben

```
# Boot / Config-Dump
[C][dummy_thermostat]: Dummy Thermostat:
[C][dummy_thermostat]:   Fallback Sensor: Wohnzimmer_Temp
[C][dummy_thermostat]:   Temp Sensor Timeout: 600 seconds
[C][dummy_thermostat]:   Fallback Sensor Timeout: 600 seconds
[C][dummy_thermostat]:   Fallback Humidity Sensor: Wohnzimmer_Hum
[C][dummy_thermostat]:   Humidity Sensor Timeout: 0 seconds
[C][dummy_thermostat]:   Valve Control Enabled (Master Switch): YES
[C][dummy_thermostat]:   Use Local Valve Control: NO (External Input)

# Normalbetrieb
[I][dummy_thermostat]: Current Temp updated by service 21.30
[I][dummy_thermostat]: Valve updated by service true
[I][dummy_thermostat]: Action changed to: HEATING (mode=1, valve=ON, T=20.3°C, Tset=21.0°C)

# Lokale Regelung
[I][dummy_thermostat]: Local valve: Start heating (20.3°C < 21.0°C - 0.5°C)
[I][dummy_thermostat]: Local valve state changed: ON
[D][dummy_thermostat]: Valve switch turned ON (control_enabled=YES, valve_state=ON)

# Fallback-Aktivierung
[W][dummy_thermostat]: Temperature sensor timeout! Switching to fallback sensor.
[W][dummy_thermostat]: Valve update timeout! Switching to local controll.
[W][dummy_thermostat]: Fallback sensor timeout! No valid temperature source available.

# Wiederherstellung
[I][dummy_thermostat]: Temperature sensor recovered. Switching back to main sensor.
[I][dummy_thermostat]: Valve update timeout recovered. Switching back to main external controll.

# Kühlen im lokalen Modus
[W][dummy_thermostat]: Local valve: Cooling not supported in local mode - valve OFF
```

## Beispielkonfiguration (B16M mit Wärmepumpe)

```yaml
climate:
  - platform: dummy_thermostat
    name: "${thermostat_name}"
    id: thermostat${thermostat_id}
    fallback_sensor: ${thermostat_fallback_sensor}
    sensor_timeout: 900
    fallback_sensor_timeout: 600
    fallback_humidity_sensor: ${thermostat_fallback_humidity_sensor}
    humidity_sensor_timeout: 900
    valve_switch: ${thermostat_valve_switch}
    valve_update_timeout: 300
    valve_control_enabled: !lambda |-
      return id(heating_master_switch).state && id(hw_master_sw).state;
    use_local_valve_control: !lambda |-
      return id(local_control_switch).state;
    heating_deadband: 0.5
    heating_overrun: 0.1
```

## Dateistruktur

```
components/
└── dummy_thermostat/
    ├── __init__.py
    ├── climate.py          # ESPHome Codegen / Schema
    ├── dummy_thermostat.h  # C++ Header
    └── dummy_thermostat.cpp
```

## Kompatibilität

- ESPHome: 2026.6.x+
- Plattformen: ESP32, ESP32-S3
- Home Assistant: alle Versionen mit ESPHome-Integration
