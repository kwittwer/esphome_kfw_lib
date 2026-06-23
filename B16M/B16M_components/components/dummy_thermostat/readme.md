Hier ist die vollständig aktualisierte README.md:
# ESPHome Dummy Thermostat Component

Eine flexible ESPHome-Komponente für einen virtuellen Thermostat mit erweiterten Funktionen wie Sensor-Fallback, lokaler Ventilsteuerung, externer Ventilsteuerung und MQTT-Status-Publishing.

## Features

- ✅ **Virtueller Thermostat** ohne physische Heiz-/Kühllogik
- ✅ **Sensor-Fallback** mit automatischem Umschalten bei Timeout
- ✅ **Feuchtigkeitssensor-Unterstützung** mit Fallback
- ✅ **Lokale Ventilsteuerung** mit Hysterese-Logik (nur Heizen)
- ✅ **Externe Ventilsteuerung** via Select, Text Sensor oder Switch (Heizen &amp; Kühlen)
- ✅ **Valve State Switch Timeout** mit automatischer Umschaltung auf lokale Steuerung
- ✅ **Master-Switch** für Ventilsteuerung (Hauptschalter)
- ✅ **Action-Berechnung** basierend auf Ventilstatus und Modus
- ✅ **MQTT-Status-Publishing** (periodisch + bei Änderungen)
- ✅ **Ventil-Ausgabe** via Binary Output oder Switch
- ✅ **Konfigurierbare Deadband/Overrun** Parameter für Heizen und Kühlen
- ✅ **State-Restore** nach Neustart
- ✅ **Lambda-Zugriff** auf alle Status-Informationen

## Konzept

### Ventilsteuerung

Die Komponente unterscheidet zwischen **Ventilstatus** (intern) und **physischem Ventil** (Ausgang):

1. **Ventilstatus** wird bestimmt durch:
   - **Lokale Steuerung** (`use_local_valve_control: true`): Hysterese-Regelung basierend auf Temperatur (**nur Heizen**)
   - **Externe Steuerung** (`use_local_valve_control: false`): Eingabe von Select/Text Sensor/Switch (**Heizen &amp; Kühlen**)

2. **Physisches Ventil** wird gesteuert durch:
   - **Master-Switch** (`valve_control_enabled`): Nur wenn `true`, wird das physische Ventil geschaltet
   - Wenn `false`: Ventil bleibt **immer AUS**, unabhängig vom internen Ventilstatus

3. **Action** (HEATING/COOLING/IDLE) wird berechnet aus:
   - Aktueller Modus (OFF/HEAT/COOL/HEAT_COOL)
   - Ventilstatus (ON/OFF)
   - Temperaturvergleich (bei HEAT_COOL Modus)

### Wichtiger Unterschied: Lokale vs. Externe Steuerung

| Feature | Lokale Steuerung | Externe Steuerung |
|---------|------------------|-------------------|
| **Heizen** | ✅ Ja (Hysterese) | ✅ Ja (extern gesteuert) |
| **Kühlen** | ❌ **Nein** | ✅ Ja (extern gesteuert) |
| **COOL-Modus** | Ventil bleibt AUS | Funktioniert normal |
| **HEAT_COOL-Modus** | Nur Heizen | Heizen &amp; Kühlen |
| **Anwendungsfall** | Einfache Heizung | Komplexe Klimasteuerung |

**Wichtig**: Im lokalen Modus ist Kühlen **deaktiviert**. Für Kühl-Funktionalität muss die externe Steuerung verwendet werden!

### Action-Berechnung

#### Lokaler Modus (nur Heizen)

| Modus | Ventilstatus | Temperatur | Action |
|-------|--------------|------------|--------|
| OFF | - | - | OFF |
| HEAT | OFF | - | IDLE |
| HEAT | ON | - | HEATING |
| COOL | - | - | IDLE (Kühlen deaktiviert) |
| HEAT_COOL | OFF | - | IDLE |
| HEAT_COOL | ON | T < Tsoll | HEATING |
| HEAT_COOL | ON | T ≥ Tsoll | IDLE (Kühlen deaktiviert) |

#### Externer Modus (Heizen &amp; Kühlen)

| Modus | Ventilstatus | Temperatur | Action |
|-------|--------------|------------|--------|
| OFF | - | - | OFF |
| HEAT | OFF | - | IDLE |
| HEAT | ON | - | HEATING |
| COOL | OFF | - | IDLE |
| COOL | ON | - | COOLING |
| HEAT_COOL | OFF | - | IDLE |
| HEAT_COOL | ON | T < Tsoll | HEATING |
| HEAT_COOL | ON | T > Tsoll | COOLING |
| HEAT_COOL | ON | T = Tsoll | IDLE |

## Installation

### 1. Dateien kopieren

Kopiere die folgenden Dateien in dein ESPHome-Projekt:

```text
custom_components/
└── dummy_thermostat/
    ├── __init__.py
    ├── climate.py
    ├── dummy_thermostat.h
    └── dummy_thermostat.cpp

2. YAML-Konfiguration
Basis-Konfiguration (Lokale Steuerung - nur Heizen)
external_components:
  - source:
      type: local
      path: custom_components

climate:
  - platform: dummy_thermostat
    name: "Wohnzimmer Thermostat"
    id: thermostat_wohnzimmer
    sensor: temperature_sensor
    
    # Lokale Steuerung (nur Heizen)
    use_local_valve_control: true
    heating_deadband: 0.5
    heating_overrun: 0.3

Erweiterte Konfiguration mit allen Features
climate:
  - platform: dummy_thermostat
    name: "Wohnzimmer Thermostat"
    id: thermostat_wohnzimmer
    
    # Temperatursensor (erforderlich)
    sensor: temperature_sensor
    
    # Fallback-Sensor bei Ausfall des Hauptsensors
    fallback_sensor: temperature_sensor_backup
    sensor_timeout: 300  # Sekunden bis Fallback aktiviert wird
    
    # Feuchtigkeitssensor (optional)
    humidity_sensor: humidity_sensor
    fallback_humidity_sensor: humidity_sensor_backup
    humidity_sensor_timeout: 300
    
    # Ventilstatus-Eingabe (nur wenn use_local_valve_control: false)
    valve_state_switch: external_valve_request  # Switch
    valve_state_switch_timeout: 300  # Timeout in Sekunden (0 = deaktiviert)
    # ODER
    valve_state_select: external_valve_select   # Select
    # ODER
    valve_state_text_sensor: external_valve_text  # Text Sensor
    
    # MQTT-Status-Publishing
    status_topic: "esphome/thermostat/wohnzimmer/status"
    status_update_interval: 60  # Sekunden (0 = nur bei Änderungen)
    
    # Physisches Ventil (Ausgang)
    valve_output: heating_valve_output  # Binary Output
    # ODER
    valve_switch: heating_valve_switch  # Switch
    
    # Master-Switch für Ventilsteuerung
    valve_control_enabled: true  # oder Lambda
    
    # Steuerungsmodus
    use_local_valve_control: true  # true = lokal (nur Heizen), false = extern (Heizen &amp; Kühlen)
    
    # Hysterese-Parameter für lokale Steuerung (nur Heizen)
    heating_deadband: 0.5  # °C unter Zieltemperatur zum Heizen starten
    heating_overrun: 0.5   # °C über Zieltemperatur zum Heizen stoppen
    
    # Kühl-Parameter (nur für externe Steuerung relevant)
    cooling_deadband: 0.5  # °C über Zieltemperatur zum Kühlen starten
    cooling_overrun: 0.5   # °C unter Zieltemperatur zum Kühlen stoppen

Konfigurationsoptionen
Erforderliche Parameter



Parameter
Typ
Beschreibung



sensor
Sensor-ID
Haupttemperatursensor


Optionale Parameter
Sensor-Fallback



Parameter
Typ
Standard
Beschreibung



fallback_sensor
Sensor-ID
-
Backup-Temperatursensor


sensor_timeout
int
0
Sekunden bis Fallback (0 = deaktiviert)


humidity_sensor
Sensor-ID
-
Hauptfeuchtigkeitssensor


fallback_humidity_sensor
Sensor-ID
-
Backup-Feuchtigkeitssensor


humidity_sensor_timeout
int
0
Sekunden bis Fallback (0 = deaktiviert)


Ventilstatus-Eingabe (nur bei externer Steuerung)



Parameter
Typ
Beschreibung



valve_state_select
Select-ID
Select-Komponente für Ventilstatus (ON/OFF)


valve_state_text_sensor
Text-Sensor-ID
Text-Sensor für Ventilstatus


valve_state_switch
Switch-ID
Switch für Ventilstatus


valve_state_switch_timeout
int
Timeout in Sekunden für Switch-Überwachung (0 = deaktiviert)


Hinweis: Wird nur verwendet wenn use_local_valve_control: false
MQTT-Status



Parameter
Typ
Standard
Beschreibung



status_topic
string
""
MQTT-Topic für Status-Updates


status_update_interval
int
0
Update-Intervall in Sekunden (0 = nur bei Änderungen)


MQTT-Payload-Beispiel:
{
  "mode": "HEAT",
  "action": "HEATING",
  "target_temperature": 21.5,
  "current_temperature": 20.3,
  "current_humidity": 45.2,
  "using_fallback_temperature": false,
  "using_fallback_humidity": false,
  "valve_state": true,
  "valve_control_enabled": true,
  "use_local_valve_control": true,
  "valve_switch_timeout_active": false,
  "heating_deadband": 0.5,
  "heating_overrun": 0.5,
  "cooling_deadband": 0.5,
  "cooling_overrun": 0.5
}

Ventilsteuerung



Parameter
Typ
Standard
Beschreibung



valve_output
Output-ID
-
Binary Output für physisches Ventil


valve_switch
Switch-ID
-
Switch für physisches Ventil


valve_control_enabled
bool/lambda
false
Master-Switch: Nur wenn true wird Ventil geschaltet


use_local_valve_control
bool/lambda
false
true = Lokal (nur Heizen), false = Extern (Heizen &amp; Kühlen)


heating_deadband
float
0.5
°C unter Ziel zum Heizen starten


heating_overrun
float
0.5
°C über Ziel zum Heizen stoppen


cooling_deadband
float
0.5
°C über Ziel zum Kühlen starten (nur extern)


cooling_overrun
float
0.5
°C unter Ziel zum Kühlen stoppen (nur extern)


Valve State Switch Timeout
Die Komponente kann die externe Ventilsteuerung über valve_state_switch überwachen und bei einem Timeout automatisch auf lokale Steuerung umschalten.
Konfiguration
climate:
  - platform: dummy_thermostat
    name: "Thermostat"
    id: thermostat1
    sensor: temp_sensor
    
    # Externe Steuerung mit Timeout-Überwachung
    valve_state_switch: external_valve_request
    valve_state_switch_timeout: 300  # Sekunden (0 = deaktiviert)
    
    # Startet mit externer Steuerung (Heizen + Kühlen)
    use_local_valve_control: false
    
    # Fallback-Parameter (nur Heizen)
    heating_deadband: 0.5
    heating_overrun: 0.3

Verhalten



Situation
Verhalten



Normale Funktion
Externe Steuerung aktiv (Heizen + Kühlen), Timeout-Timer läuft


Timeout erreicht
Automatische Umschaltung auf lokale Steuerung (nur Heizen)


Update empfangen
Timeout-Timer wird zurückgesetzt


Wiederherstellung
Nach halber Timeout-Zeit wird Timeout-Flag gelöscht


Wichtige Hinweise
Bei statischem use_local_valve_control:

Automatische Umschaltung funktioniert
Komponente ändert use_local_valve_control intern auf true
Nach Timeout: Nur noch Heizen möglich (Kühlen deaktiviert)

Bei Lambda-gesteuertem use_local_valve_control:

Automatische Umschaltung nicht möglich
Verwende is_valve_switch_timeout_active() für manuelle Steuerung
Siehe Beispiel unten

Lambda-Methode für Timeout-Status
binary_sensor:
  - platform: template
    name: "Timeout Aktiv"
    lambda: |-
      return id(thermostat1).is_valve_switch_timeout_active();

Log-Ausgaben
[12:34:56][W][dummy_thermostat]: Valve state switch timeout! Switching to local valve control.
[12:34:56][W][dummy_thermostat]:   Last update: 305 ms ago (timeout: 300 sec)
[12:34:56][I][dummy_thermostat]: Automatically switched to local valve control due to timeout.
[12:34:56][W][dummy_thermostat]: WARNING: Cooling modes (COOL/HEAT_COOL) are not supported in local control mode!
[12:34:56][W][dummy_thermostat]:          Only heating will work. Switch to external control for cooling support.

# Bei Lambda-Steuerung:
[12:34:56][W][dummy_thermostat]: Cannot automatically switch to local control - use_local_valve_control is controlled by lambda!
[12:34:56][W][dummy_thermostat]: Please handle timeout externally or use static bool for use_local_valve_control.

# Kühlen im lokalen Modus versucht:
[12:34:56][W][dummy_thermostat]: Local valve: Cooling not supported in local mode - valve OFF

Lambda-Zugriff auf Status-Informationen
Die Komponente bietet öffentliche Methoden für den Zugriff auf Status-Informationen in Lambdas:
Verfügbare Methoden



Methode
Rückgabewert
Beschreibung



is_valve_control_enabled()
bool
Gibt true zurück wenn Hauptschalter aktiv ist


is_using_local_valve_control()
bool
Gibt true zurück wenn lokale Steuerung aktiv ist


get_valve_state()
bool
Gibt aktuellen Ventilstatus zurück (intern)


is_using_fallback_temperature()
bool
Gibt true zurück wenn Fallback-Temperatursensor verwendet wird


is_using_fallback_humidity()
bool
Gibt true zurück wenn Fallback-Feuchtigkeitssensor verwendet wird


is_valve_switch_timeout_active()
bool
Gibt true zurück wenn Valve Switch Timeout aktiv ist


Zugriff auf Standard-Climate-Eigenschaften
Zusätzlich zu den neuen Methoden kannst du auf alle Standard-Climate-Eigenschaften zugreifen:



Eigenschaft
Typ
Beschreibung



current_temperature
float
Aktuelle Temperatur


current_humidity
float
Aktuelle Luftfeuchtigkeit


target_temperature
float
Zieltemperatur


mode
ClimateMode
Aktueller Modus (OFF/HEAT/COOL/HEAT_COOL)


action
ClimateAction
Aktuelle Aktion (OFF/IDLE/HEATING/COOLING)


Lambda-Beispiele
Status in Text Sensor anzeigen
text_sensor:
  - platform: template
    name: "Thermostat Steuerungsmodus"
    update_interval: 10s
    lambda: |-
      if (id(thermostat1).is_using_local_valve_control()) {
        return {"Lokal (nur Heizen)"};
      } else {
        return {"Extern (Heizen + Kühlen)"};
      }

  - platform: template
    name: "Thermostat Hauptschalter Status"
    update_interval: 10s
    lambda: |-
      if (id(thermostat1).is_valve_control_enabled()) {
        return {"Aktiviert"};
      } else {
        return {"Deaktiviert"};
      }

  - platform: template
    name: "Thermostat Vollstatus"
    update_interval: 10s
    lambda: |-
      std::string status = "Hauptschalter: ";
      status += id(thermostat1).is_valve_control_enabled() ? "AN" : "AUS";
      status += " | Steuerung: ";
      if (id(thermostat1).is_using_local_valve_control()) {
        status += "Lokal (nur Heizen)";
      } else {
        status += "Extern (Heizen+Kühlen)";
      }
      status += " | Ventil: ";
      status += id(thermostat1).get_valve_state() ? "AN" : "AUS";
      if (id(thermostat1).is_valve_switch_timeout_active()) {
        status += " | TIMEOUT";
      }
      return status;

Binary Sensor für Status
binary_sensor:
  - platform: template
    name: "Thermostat Hauptschalter Aktiv"
    lambda: |-
      return id(thermostat1).is_valve_control_enabled();

  - platform: template
    name: "Thermostat Lokale Steuerung Aktiv"
    lambda: |-
      return id(thermostat1).is_using_local_valve_control();

  - platform: template
    name: "Thermostat Ventil Anforderung"
    lambda: |-
      return id(thermostat1).get_valve_state();

  - platform: template
    name: "Thermostat Fallback Temperatur"
    lambda: |-
      return id(thermostat1).is_using_fallback_temperature();

  - platform: template
    name: "Thermostat Timeout Aktiv"
    lambda: |-
      return id(thermostat1).is_valve_switch_timeout_active();

Warnung bei Kühl-Versuch im lokalen Modus
text_sensor:
  - platform: template
    name: "Thermostat Modus Warnung"
    update_interval: 5s
    lambda: |-
      if (id(thermostat1).is_using_local_valve_control()) {
        auto mode = id(thermostat1)->mode;
        if (mode == climate::CLIMATE_MODE_COOL) {
          return {"WARNUNG: COOL-Modus im lokalen Modus nicht unterstützt!"};
        } else if (mode == climate::CLIMATE_MODE_HEAT_COOL) {
          return {"INFO: AUTO-Modus - nur Heizen aktiv (Kühlen deaktiviert)"};
        }
      }
      return {"OK"};

LED-Anzeige basierend auf Status
output:
  - platform: gpio
    pin: GPIO2
    id: led_master_switch
    
  - platform: gpio
    pin: GPIO4
    id: led_local_control
    
  - platform: gpio
    pin: GPIO5
    id: led_valve_active

interval:
  - interval: 1s
    then:
      # LED für Hauptschalter
      - if:
          condition:
            lambda: 'return id(thermostat1).is_valve_control_enabled();'
          then:
            - output.turn_on: led_master_switch
          else:
            - output.turn_off: led_master_switch
      
      # LED für lokale Steuerung
      - if:
          condition:
            lambda: 'return id(thermostat1).is_using_local_valve_control();'
          then:
            - output.turn_on: led_local_control
          else:
            - output.turn_off: led_local_control
      
      # LED für Ventilstatus
      - if:
          condition:
            lambda: 'return id(thermostat1).get_valve_state();'
          then:
            - output.turn_on: led_valve_active
          else:
            - output.turn_off: led_valve_active

Anwendungsbeispiele
Beispiel 1: Einfache Heizung (Lokale Steuerung)
globals:
  - id: enable_heating
    type: bool
    initial_value: 'true'

output:
  - platform: gpio
    pin: GPIO26
    id: physical_valve

sensor:
  - platform: dallas
    id: temp_sensor
    address: 0x1234567890ABCDEF

climate:
  - platform: dummy_thermostat
    name: "Heizung"
    id: heating
    sensor: temp_sensor
    
    # Physisches Ventil
    valve_output: physical_valve
    
    # Master-Switch (z.B. über Automation steuerbar)
    valve_control_enabled: !lambda 'return id(enable_heating);'
    
    # Lokale Regelung (nur Heizen)
    use_local_valve_control: true
    heating_deadband: 0.5
    heating_overrun: 0.3
    
    # MQTT Status
    status_topic: "home/heating/status"
    status_update_interval: 60

Verhalten:

Thermostat regelt selbstständig mit Hysterese
Nur Heizen möglich
Wenn enable_heating auf false gesetzt wird, geht Ventil aus
Action zeigt weiterhin an, was der Thermostat möchte

Beispiel 2: Klimaanlage (Externe Steuerung - Heizen &amp; Kühlen)
switch:
  - platform: template
    name: "Ventil Anforderung"
    id: valve_request
    optimistic: true

  - platform: template
    name: "Klima Hauptschalter"
    id: climate_master_switch
    optimistic: true
    initial_state: true

output:
  - platform: gpio
    pin: GPIO26
    id: physical_valve

climate:
  - platform: dummy_thermostat
    name: "Klimaanlage"
    id: climate1
    sensor: temp_sensor
    
    # Ventilstatus von außen (z.B. Home Assistant Automation)
    valve_state_switch: valve_request
    
    # Physisches Ventil
    valve_output: physical_valve
    
    # Master-Switch
    valve_control_enabled: !lambda 'return id(climate_master_switch).state;'
    
    # Externe Steuerung (Heizen + Kühlen)
    use_local_valve_control: false
    
    # Kühl-Parameter werden verwendet
    cooling_deadband: 0.5
    cooling_overrun: 0.3
    
    status_topic: "home/climate/status"

Home Assistant Automation Beispiel:
automation:
  - alias: "Klimaanlage Heizen"
    trigger:
      - platform: numeric_state
        entity_id: sensor.room_temperature
        below: 20
    condition:
      - condition: state
        entity_id: climate.klimaanlage
        state: heat
    action:
      - service: switch.turn_on
        target:
          entity_id: switch.ventil_anforderung

  - alias: "Klimaanlage Kühlen"
    trigger:
      - platform: numeric_state
        entity_id: sensor.room_temperature
        above: 24
    condition:
      - condition: state
        entity_id: climate.klimaanlage
        state: cool
    action:
      - service: switch.turn_on
        target:
          entity_id: switch.ventil_anforderung

Beispiel 3: Umschaltbar zwischen Lokal und Extern
globals:
  - id: use_local_control
    type: bool
    initial_value: 'true'  # Startet im lokalen Modus (nur Heizen)

select:
  - platform: template
    name: "Steuerungsmodus"
    id: control_mode
    options:
      - "Lokal (nur Heizen)"
      - "Extern (Heizen + Kühlen)"
    initial_option: "Lokal (nur Heizen)"
    on_value:
      - lambda: |-
          id(use_local_control) = (x == "Lokal (nur Heizen)");
          ESP_LOGI("climate", "Steuerungsmodus: %s", x.c_str());

  - platform: template
    name: "Ventil Anforderung Extern"
    id: valve_request_select
    options:
      - "OFF"
      - "ON"
    initial_option: "OFF"

climate:
  - platform: dummy_thermostat
    name: "Thermostat"
    id: thermostat1
    sensor: temp_sensor
    
    # Externe Eingabe
    valve_state_select: valve_request_select
    
    # Physisches Ventil
    valve_output: physical_valve
    
    # Immer aktiviert
    valve_control_enabled: true
    
    # Umschaltbar
    use_local_valve_control: !lambda 'return id(use_local_control);'
    
    # Heiz-Parameter (für lokal)
    heating_deadband: 0.5
    heating_overrun: 0.3
    
    # Kühl-Parameter (für extern)
    cooling_deadband: 0.5
    cooling_overrun: 0.3

Beispiel 4: Externe Steuerung mit Timeout-Fallback
switch:
  - platform: template
    name: "Externe Ventil Anforderung"
    id: external_valve_request
    optimistic: true

output:
  - platform: gpio
    pin: GPIO26
    id: physical_valve

climate:
  - platform: dummy_thermostat
    name: "Thermostat"
    id: thermostat1
    sensor: temp_sensor
    
    # Externe Steuerung mit Timeout
    valve_state_switch: external_valve_request
    valve_state_switch_timeout: 300  # 5 Minuten
    
    # Physisches Ventil
    valve_output: physical_valve
    
    # Master-Switch
    valve_control_enabled: true
    
    # Startet mit externer Steuerung (Heizen + Kühlen)
    use_local_valve_control: false
    
    # Hysterese-Parameter für Fallback (nur Heizen)
    heating_deadband: 0.5
    heating_overrun: 0.3
    
    status_topic: "home/thermostat/status"
    status_update_interval: 60

Verhalten:

Normale Steuerung über external_valve_request Switch (Heizen + Kühlen)
Wenn 5 Minuten keine Aktualisierung erfolgt → automatische Umschaltung auf lokale Regelung
Nach Timeout: Nur noch Heizen möglich (Kühlen deaktiviert)
Bei Wiederherstellung der Verbindung → Timeout-Flag wird zurückgesetzt

Beispiel 5: Mit Lambda-Steuerung (manuelle Timeout-Behandlung)
globals:
  - id: use_local_control
    type: bool
    initial_value: 'false'

switch:
  - platform: template
    name: "Externe Ventil Anforderung"
    id: external_valve_request
    optimistic: true

climate:
  - platform: dummy_thermostat
    name: "Thermostat"
    id: thermostat1
    sensor: temp_sensor
    
    # Externe Steuerung mit Timeout
    valve_state_switch: external_valve_request
    valve_state_switch_timeout: 300  # 5 Minuten
    
    valve_output: physical_valve
    valve_control_enabled: true
    
    # Lambda-gesteuert
    use_local_valve_control: !lambda 'return id(use_local_control);'
    
    heating_deadband: 0.5
    heating_overrun: 0.3

# Manuelles Timeout-Handling
interval:
  - interval: 10s
    then:
      - lambda: |-
          // Bei Timeout auf lokal umschalten (nur Heizen)
          if (id(thermostat1).is_valve_switch_timeout_active()) {
            if (!id(use_local_control)) {
              ESP_LOGW("timeout", "Switching to local control due to timeout (heating only)");
              id(use_local_control) = true;
            }
          } else {
            // Bei Wiederherstellung zurück auf extern (Heizen + Kühlen)
            if (id(use_local_control)) {
              ESP_LOGW("timeout", "Switching back to external control (heating + cooling)");
              id(use_local_control) = false;
            }
          }

Beispiel 6: Timeout-Monitoring mit Benachrichtigung
climate:
  - platform: dummy_thermostat
    name: "Thermostat"
    id: thermostat1
    sensor: temp_sensor
    valve_state_switch: external_valve_request
    valve_state_switch_timeout: 180  # 3 Minuten
    valve_output: physical_valve
    valve_control_enabled: true
    use_local_valve_control: false

# Status-Anzeigen
binary_sensor:
  - platform: template
    name: "Thermostat Timeout Aktiv"
    lambda: |-
      return id(thermostat1).is_valve_switch_timeout_active();

text_sensor:
  - platform: template
    name: "Thermostat Steuerungsstatus"
    update_interval: 5s
    lambda: |-
      if (id(thermostat1).is_valve_switch_timeout_active()) {
        return {"TIMEOUT - Lokale Steuerung (nur Heizen)"};
      } else if (id(thermostat1).is_using_local_valve_control()) {
        return {"Lokale Steuerung (nur Heizen)"};
      } else {
        return {"Externe Steuerung (Heizen + Kühlen)"};
      }

# Timeout-Benachrichtigung
binary_sensor:
  - platform: template
    name: "Thermostat Timeout"
    id: timeout_sensor
    lambda: |-
      return id(thermostat1).is_valve_switch_timeout_active();
    on_press:
      - logger.log:
          level: WARN
          format: "WARNUNG: Externe Steuerung Timeout! Umschaltung auf lokale Regelung (nur Heizen)."
      # Optional: Home Assistant Benachrichtigung
      # - homeassistant.service:
      #     service: notify.mobile_app
      #     data:
      #       title: "Thermostat Warnung"
      #       message: "Externe Steuerung ausgefallen - Lokale Regelung aktiv (nur Heizen)"
    on_release:
      - logger.log:
          level: INFO
          format: "INFO: Externe Steuerung wiederhergestellt (Heizen + Kühlen verfügbar)."

Beispiel 7: Multi-Zone Heizung mit Hauptschalter
globals:
  - id: heating_system_enabled
    type: bool
    initial_value: 'true'

sensor:
  - platform: dallas
    id: zone1_temp
    address: 0x1111111111111111
    
  - platform: dallas
    id: zone2_temp
    address: 0x2222222222222222

output:
  - platform: gpio
    pin: GPIO26
    id: zone1_valve
    
  - platform: gpio
    pin: GPIO27
    id: zone2_valve

climate:
  - platform: dummy_thermostat
    name: "Zone 1 Heizung"
    id: zone1_heating
    sensor: zone1_temp
    valve_output: zone1_valve
    valve_control_enabled: !lambda 'return id(heating_system_enabled);'
    use_local_valve_control: true  # Nur Heizen
    heating_deadband: 0.5
    heating_overrun: 0.3
    status_topic: "home/heating/zone1/status"
    status_update_interval: 60
    
  - platform: dummy_thermostat
    name: "Zone 2 Heizung"
    id: zone2_heating
    sensor: zone2_temp
    valve_output: zone2_valve
    valve_control_enabled: !lambda 'return id(heating_system_enabled);'
    use_local_valve_control: true  # Nur Heizen
    heating_deadband: 0.5
    heating_overrun: 0.3
    status_topic: "home/heating/zone2/status"
    status_update_interval: 60

switch:
  - platform: template
    name: "Heizung Hauptschalter"
    id: main_heating_switch
    optimistic: true
    initial_state: true
    on_turn_on:
      - lambda: 'id(heating_system_enabled) = true;'
    on_turn_off:
      - lambda: 'id(heating_system_enabled) = false;'

Beispiel 8: Vollständiges System mit Status-Monitoring
climate:
  - platform: dummy_thermostat
    name: "Thermostat"
    id: thermostat1
    sensor: temp_sensor
    valve_output: physical_valve
    valve_control_enabled: !lambda 'return id(master_switch_global);'
    use_local_valve_control: !lambda 'return id(local_control_global);'

globals:
  - id: master_switch_global
    type: bool
    initial_value: 'true'
  
  - id: local_control_global
    type: bool
    initial_value: 'true'

# Status-Anzeigen
text_sensor:
  - platform: template
    name: "Thermostat Modus"
    update_interval: 5s
    lambda: |-
      std::string status = "";
      
      if (!id(thermostat1).is_valve_control_enabled()) {
        status = "DEAKTIVIERT";
      } else if (id(thermostat1).is_using_local_valve_control()) {
        status = "LOKAL (nur Heizen)";
        if (id(thermostat1).get_valve_state()) {
          status += " - Heizen";
        } else {
          status += " - Idle";
        }
      } else {
        status = "EXTERN (Heizen+Kühlen)";
        if (id(thermostat1).get_valve_state()) {
          auto action = id(thermostat1)->action;
          if (action == climate::CLIMATE_ACTION_HEATING) {
            status += " - Heizen";
          } else if (action == climate::CLIMATE_ACTION_COOLING) {
            status += " - Kühlen";
          } else {
            status += " - Aktiv";
          }
        } else {
          status += " - Idle";
        }
      }
      
      if (id(thermostat1).is_using_fallback_temperature()) {
        status += " [FALLBACK]";
      }
      
      if (id(thermostat1).is_valve_switch_timeout_active()) {
        status += " [TIMEOUT]";
      }
      
      return status;

binary_sensor:
  - platform: template
    name: "Thermostat Aktiv"
    lambda: |-
      return id(thermostat1).is_valve_control_enabled() &amp;&amp; 
             id(thermostat1).get_valve_state();

# Steuerung
switch:
  - platform: template
    name: "Thermostat Hauptschalter"
    lambda: 'return id(master_switch_global);'
    turn_on_action:
      - globals.set:
          id: master_switch_global
          value: 'true'
    turn_off_action:
      - globals.set:
          id: master_switch_global
          value: 'false'

  - platform: template
    name: "Thermostat Lokale Steuerung"
    lambda: 'return id(local_control_global);'
    turn_on_action:
      - globals.set:
          id: local_control_global
          value: 'true'
      - logger.log: "Umgeschaltet auf LOKAL (nur Heizen)"
    turn_off_action:
      - globals.set:
          id: local_control_global
          value: 'false'
      - logger.log: "Umgeschaltet auf EXTERN (Heizen + Kühlen)"

Funktionsweise
Sensor-Fallback

Normalbetrieb: Hauptsensor wird verwendet
Timeout: Nach sensor_timeout Sekunden ohne Update
Umschaltung: Automatischer Wechsel zum Fallback-Sensor
Wiederherstellung: Automatische Rückkehr zum Hauptsensor bei Verfügbarkeit

Lokale Ventilsteuerung (Hysterese - nur Heizen)
Die lokale Ventilsteuerung verwendet eine Hysterese-Logik nur für Heizen:
Temperatur
    ↑
    |
    |     ┌─────────────────  Ziel + Overrun (Heizen STOP)
    |     │   HEATING
    |─────┼─────────────────  Zieltemperatur
    |     │   IDLE
    |     └─────────────────  Ziel - Deadband (Heizen START)
    |
    └─────────────────────→ Zeit

Beispiel (Ziel: 21°C, Deadband: 0.5°C, Overrun: 0.5°C):

Heizen startet bei: 20.5°C
Heizen stoppt bei: 21.5°C
Kühlen: Nicht verfügbar im lokalen Modus

Externe Ventilsteuerung (Heizen &amp; Kühlen)
Im externen Modus kann sowohl geheizt als auch gekühlt werden:
Temperatur
    ↑
    |     ┌─────────────────  Ziel + Cooling Deadband (Kühlen START)
    |     │   COOLING
    |─────┼─────────────────  Zieltemperatur
    |     │   HEATING
    |     └─────────────────  Ziel - Heating Deadband (Heizen START)
    |
    └─────────────────────→ Zeit

Beispiel (Ziel: 22°C):

Heizen startet bei: 21.5°C (22 - 0.5)
Heizen stoppt bei: 22.5°C (22 + 0.5)
Kühlen startet bei: 22.5°C (22 + 0.5)
Kühlen stoppt bei: 21.5°C (22 - 0.5)

Master-Switch (valve_control_enabled)
Der Master-Switch fungiert als Hauptschalter für die physische Ventilsteuerung:



valve_control_enabled
Interner Ventilstatus
Physisches Ventil
Action



false
ON
AUS
HEATING/COOLING (zeigt Anforderung)


false
OFF
AUS
IDLE


true
ON
AN
HEATING/COOLING


true
OFF
AUS
IDLE


Wichtig: Die Action zeigt immer an, was der Thermostat möchte, unabhängig vom Master-Switch.
Steuerungsmodi
Lokale Steuerung (use_local_valve_control: true)

Ventilstatus wird intern durch Hysterese-Logik berechnet
Nur Heizen möglich
Externe Eingaben werden ignoriert
COOL-Modus: Ventil bleibt AUS
HEAT_COOL-Modus: Nur Heizen aktiv
Geeignet für: Einfache Heizungssteuerung

Externe Steuerung (use_local_valve_control: false)

Ventilstatus kommt von Select/Text Sensor/Switch
Heizen UND Kühlen möglich
Lokale Hysterese ist deaktiviert
Alle Modi funktionieren normal
Geeignet für: Komplexe Klimasteuerung, zentrale Steuerung

Valve State Switch Timeout
Bei externer Steuerung über valve_state_switch kann ein Timeout überwacht werden:

Normale Funktion: Timer läuft bei jedem Switch-Update neu
Timeout: Wenn valve_state_switch_timeout Sekunden ohne Update vergehen
Automatische Umschaltung: Bei statischem use_local_valve_control → automatisch auf true
Funktionseinschränkung: Nach Timeout nur noch Heizen möglich (Kühlen deaktiviert)
Lambda-Steuerung: Bei Lambda → Warnung im Log, manuelle Behandlung erforderlich
Wiederherstellung: Bei neuen Updates wird Timeout-Flag nach halber Timeout-Zeit zurückgesetzt

MQTT-Status-Updates
Status-Updates werden gesendet:

Periodisch: Alle status_update_interval Sekunden (wenn > 0)
Bei Änderungen: 
Temperatur ändert sich
Feuchtigkeit ändert sich
Ventilstatus ändert sich
Action ändert sich
Mode ändert sich
Zieltemperatur ändert sich



Hinweis: Bei status_update_interval: 0 werden Updates nur bei Änderungen gesendet.
Verhalten-Matrix
Ventilsteuerung



valve_control_enabled
use_local_valve_control
Ventilstatus-Quelle
Physisches Ventil
Kühlen möglich?



false
-
-
IMMER AUS
❌ Nein


true
true
Lokale Hysterese
Folgt Hysterese
❌ Nein


true
false
Externe Eingabe
Folgt Eingabe
✅ Ja


Action-Berechnung nach Modus
Lokaler Modus (nur Heizen)



Modus
Ventilstatus
Action
Physisches Ventil (wenn enabled)



OFF
-
OFF
AUS


HEAT
OFF
IDLE
AUS


HEAT
ON
HEATING
AN


COOL
-
IDLE
AUS (Kühlen deaktiviert)


HEAT_COOL
OFF
IDLE
AUS


HEAT_COOL
ON (T < Tsoll)
HEATING
AN


HEAT_COOL
ON (T ≥ Tsoll)
IDLE
AUS (Kühlen deaktiviert)


Externer Modus (Heizen &amp; Kühlen)



Modus
Ventilstatus
T vs Tsoll
Action
Physisches Ventil (wenn enabled)



OFF
-
-
OFF
AUS


HEAT
OFF
-
IDLE
AUS


HEAT
ON
-
HEATING
AN


COOL
OFF
-
IDLE
AUS


COOL
ON
-
COOLING
AN


HEAT_COOL
OFF
-
IDLE
AUS


HEAT_COOL
ON
T < Tsoll
HEATING
AN


HEAT_COOL
ON
T > Tsoll
COOLING
AN


HEAT_COOL
ON
T = Tsoll
IDLE
AN


Timeout-Verhalten



Timeout konfiguriert
use_local_valve_control
Timeout aufgetreten
Verhalten



Nein (0)
-
-
Keine Überwachung


Ja
Statisch (bool)
Ja
Automatische Umschaltung auf true (nur Heizen)


Ja
Lambda
Ja
Warnung im Log, manuelle Behandlung


Ja
Beliebig
Nein
Normale Funktion


Debugging
Log-Level erhöhen
logger:
  level: DEBUG
  logs:
    dummy_thermostat: VERBOSE
    mqtt: DEBUG

Typische Log-Ausgaben
[12:34:56][C][dummy_thermostat:123]: Dummy Thermostat:
[12:34:56][C][dummy_thermostat:124]:   Sensor: Temperature Sensor
[12:34:56][C][dummy_thermostat:125]:   Fallback Sensor: Backup Temperature
[12:34:56][C][dummy_thermostat:126]:   Sensor Timeout: 300 seconds
[12:34:56][C][dummy_thermostat:130]:   MQTT Status Topic: esphome/thermostat/status
[12:34:56][C][dummy_thermostat:131]:   MQTT Status: Periodic (60 sec) + on change
[12:34:56][C][dummy_thermostat:136]:   Valve Control Enabled (Master Switch): YES
[12:34:56][C][dummy_thermostat:137]:   Use Local Valve Control: YES (Local Hysteresis - Heating Only)
[12:34:56][C][dummy_thermostat:138]:   Valve State Switch Timeout: 300 seconds
[12:34:56][C][dummy_thermostat:140]:   Heating Deadband: 0.5°C
[12:34:56][C][dummy_thermostat:141]:   Heating Overrun: 0.5°C

# Lokaler Modus - Heizen
[12:35:10][D][dummy_thermostat:089]: Local valve: Start heating (20.3°C < 21.0°C - 0.5°C)
[12:35:10][I][dummy_thermostat:099]: Local valve state changed: ON
[12:35:10][D][dummy_thermostat:156]: Valve output turned ON (control_enabled=YES, valve_state=ON)
[12:35:10][I][dummy_thermostat:234]: Action changed to: HEATING (mode=1, valve=ON, T=20.3°C, Tset=21.0°C)

[12:40:15][D][dummy_thermostat:095]: Local valve: Stop heating (21.6°C >= 21.0°C + 0.5°C)
[12:40:15][I][dummy_thermostat:099]: Local valve state changed: OFF
[12:40:15][D][dummy_thermostat:156]: Valve output turned OFF (control_enabled=YES, valve_state=OFF)
[12:40:15][I][dummy_thermostat:234]: Action changed to: IDLE (mode=1, valve=OFF, T=21.6°C, Tset=21.0°C)

# Lokaler Modus - Kühlen versucht
[12:45:00][W][dummy_thermostat]: WARNING: Cooling modes (COOL/HEAT_COOL) are not supported in local control mode!
[12:45:00][W][dummy_thermostat]:          Only heating will work. Switch to external control for cooling support.
[12:45:01][W][dummy_thermostat]: Local valve: Cooling not supported in local mode - valve OFF

# Timeout-Logs
[12:50:00][W][dummy_thermostat]: Valve state switch timeout! Switching to local valve control.
[12:50:00][W][dummy_thermostat]:   Last update: 305 ms ago (timeout: 300 sec)
[12:50:00][I][dummy_thermostat]: Automatically switched to local valve control due to timeout.
[12:50:00][W][dummy_thermostat]: WARNING: Cooling modes (COOL/HEAT_COOL) are not supported in local control mode!

# Externer Modus - Kühlen funktioniert
[12:55:00][I][dummy_thermostat]: Valve state changed from external input: ON
[12:55:00][I][dummy_thermostat]: Action changed to: COOLING (mode=3, valve=ON, T=25.0°C, Tset=22.0°C)

Debugging-Tipps

Ventil schaltet nicht:

Prüfe valve_control_enabled (muss true sein)
Prüfe Log-Ausgabe: "Valve output turned..."
Prüfe physische Verkabelung


Action ist falsch:

Prüfe Modus (OFF/HEAT/COOL/HEAT_COOL)
Prüfe use_local_valve_control Status
Prüfe Ventilstatus in Logs
Prüfe Temperaturwerte


Kühlen funktioniert nicht:

Lokaler Modus: Kühlen ist deaktiviert → Umschalten auf externen Modus erforderlich
Externer Modus: Prüfe externe Eingabe (Select/Switch/Text Sensor)
Prüfe Log: "Cooling not supported in local mode"


Externe Steuerung funktioniert nicht:

Stelle sicher use_local_valve_control: false
Prüfe ob Input-Komponente korrekt konfiguriert ist
Prüfe Log: "Valve state changed from external input"


Lokale Steuerung funktioniert nicht:

Stelle sicher use_local_valve_control: true
Prüfe Deadband/Overrun Werte
Prüfe Log: "Local valve: Start/Stop heating"
Beachte: Nur Heizen möglich


Lambda-Zugriff funktioniert nicht:

Stelle sicher, dass die Thermostat-ID korrekt ist
Verwende Pointer-Zugriff: id(thermostat_id)->methode() oder id(thermostat_id).methode()
Prüfe Log-Level für eventuelle Fehler


Timeout funktioniert nicht:

Prüfe ob valve_state_switch_timeout > 0
Prüfe ob valve_state_switch konfiguriert ist
Bei Lambda: Prüfe manuelle Timeout-Behandlung
Prüfe Log: "Valve state switch timeout!"
Beachte: Nach Timeout nur noch Heizen möglich


Warnung "Cooling not supported":

Das ist normal im lokalen Modus
Für Kühlen: Umschalten auf externen Modus (use_local_valve_control: false)
Oder: Nur HEAT-Modus verwenden



Bekannte Einschränkungen

Lokaler Modus: Kühlen ist nicht möglich (nur Heizen)
Externer Modus: Heizen UND Kühlen möglich
Bei HEAT_COOL Modus kann nur ein Ventil gesteuert werden (nicht separates Heiz-/Kühlventil)
MQTT erfordert USE_MQTT Kompilier-Flag
Select-Komponente erfordert USE_SELECT Kompilier-Flag
Timeout-Überwachung funktioniert nur mit valve_state_switch, nicht mit Select oder Text Sensor
Automatische Timeout-Umschaltung funktioniert nur bei statischem use_local_valve_control
Nach Timeout-Umschaltung: Nur noch Heizen möglich (Kühlen deaktiviert)

Kompatibilität

ESPHome: 2024.10.0 oder neuer
Plattformen: ESP32, ESP8266
Home Assistant: Alle Versionen mit ESPHome-Integration

Lizenz
MIT License - siehe LICENSE-Datei
Support
Bei Problemen oder Fragen:

Prüfe die Log-Ausgaben mit erhöhtem Log-Level
Stelle sicher, dass alle erforderlichen Komponenten konfiguriert sind
Prüfe die Verhalten-Matrix für deine Konfiguration
Beachte: Lokaler Modus = nur Heizen, Externer Modus = Heizen + Kühlen
Prüfe die MQTT-Verbindung (falls verwendet)
Nutze die Lambda-Methoden für detailliertes Debugging
Bei Timeout-Problemen: Prüfe die Timeout-Konfiguration und Log-Ausgaben
Bei Kühl-Problemen: Prüfe ob use_local_valve_control: false gesetzt ist

Changelog
Version 2.3.0

BREAKING: Kühlen im lokalen Modus deaktiviert
Lokaler Modus (use_local_valve_control: true): Nur Heizen
Externer Modus (use_local_valve_control: false): Heizen UND Kühlen


Hinzugefügt: Warnung im Log bei Kühl-Versuch im lokalen Modus
Verbessert: Klarere Unterscheidung zwischen lokalem und externem Modus
Aktualisiert: README mit detaillierter Erklärung der Modi-Unterschiede

Version 2.2.0

Hinzugefügt: Valve State Switch Timeout-Überwachung
Automatische Umschaltung auf lokale Steuerung bei Timeout
Konfigurierbar über valve_state_switch_timeout
Neue Lambda-Methode: is_valve_switch_timeout_active()


Verbessert: MQTT-Payload enthält jetzt Timeout-Status
Aktualisiert: Schema-Syntax auf climate.climate_schema() (ESPHome 2024.6.0+)
Behoben: millis() Kompilierfehler durch Verwendung von esphome::millis()

Version 2.1.0

Hinzugefügt: Öffentliche Lambda-Methoden für Status-Abfragen
is_valve_control_enabled()
is_using_local_valve_control()
get_valve_state()
is_using_fallback_temperature()
is_using_fallback_humidity()


Erweitert: README mit Lambda-Beispielen
Verbessert: Debugging-Möglichkeiten durch Status-Zugriff

Version 2.0.0

BREAKING: Action wird jetzt aus Ventilstatus berechnet (nicht mehr von außen gesetzt)
Hinzugefügt: valve_control_enabled als Master-Switch
Hinzugefügt: use_local_valve_control für Umschaltung zwischen lokal/extern
Hinzugefügt: Cooling-Parameter (cooling_deadband, cooling_overrun)
Verbessert: MQTT-Updates bei Änderungen + periodisch
Verbessert: Logging mit detaillierten Zustandsinformationen

Version 1.0.0

Initiale Version
Sensor-Fallback-Unterstützung
Lokale Ventilsteuerung mit Hysterese
MQTT-Status-Publishing
State-Restore nach Neustart


Wichtige Hinweise zur Lambda-Verwendung
Zugriff auf neue Methoden
# Beide Varianten funktionieren:
id(thermostat1).is_valve_control_enabled()
id(thermostat1)->is_valve_control_enabled()

Zugriff auf Standard-Climate-Eigenschaften
# Nur Pointer-Zugriff:
id(thermostat1)->current_temperature
id(thermostat1)->current_humidity
id(thermostat1)->target_temperature
id(thermostat1)->mode
id(thermostat1)->action

Wichtig: current_humidity ist eine Standard-Eigenschaft der Climate-Klasse und KEINE neue Methode dieser Komponente!

FAQ - Häufig gestellte Fragen
Q: Warum kann ich im lokalen Modus nicht kühlen?A: Der lokale Modus ist für einfache Heizungssteuerung konzipiert. Für Kühlen verwende den externen Modus (use_local_valve_control: false).
Q: Wie schalte ich zwischen Heizen und Kühlen um?A: Verwende den externen Modus und steuere das Ventil über Home Assistant Automationen oder externe Eingaben.
Q: Was passiert bei einem Timeout?A: Die Komponente schaltet automatisch auf lokalen Modus um (nur Heizen). Kühlen ist dann nicht mehr möglich bis zur Wiederherstellung.
Q: Kann ich beide Modi gleichzeitig nutzen?A: Nein, du musst dich für einen Modus entscheiden. Verwende use_local_valve_control um zwischen den Modi umzuschalten.
Q: Wie erkenne ich in welchem Modus ich bin?A: Nutze die Lambda-Methode is_using_local_valve_control() oder prüfe die MQTT-Status-Nachricht.

Entwickelt mit ❤️ für die ESPHome Community

Die README ist jetzt vollständig aktualisiert mit:
- ✅ Klarer Unterscheidung zwischen lokalem (nur Heizen) und externem Modus (Heizen + Kühlen)
- ✅ Aktualisierte Verhalten-Matrizen
- ✅ Erweiterte Beispiele
- ✅ Neue Log-Ausgaben
- ✅ FAQ-Sektion
- ✅ Changelog mit Version 2.3.0
- ✅ Alle Warnungen und Einschränkungen dokumentiert