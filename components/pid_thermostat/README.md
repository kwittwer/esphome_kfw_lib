# PID Thermostat

Heating/cooling climate component for ESPHome with a shared PWM-controlled valve output.

## Overview

This component implements a PID-based climate controller for ESPHome that can drive a
shared valve output in heating or cooling mode.

It supports:

- Heating and cooling with the same control core
- Shared PWM output control for a valve or actuator
- Fallback temperature and humidity sensors
- Dew-point protection in cooling mode
- Runtime-tunable controller parameters
- Diagnostic sensors and text sensors for troubleshooting

## Runtime-adjustable parameters

The component exposes these parameters as `number` entities at runtime:

- `Kp`
- `Ki`
- `Kd`
- `I Untergrenze`
- `I Obergrenze`
- `Cold Tolerance`
- `Hot Tolerance`
- `Sampling Period`
- `Keep Alive`
- `Min Cycle Duration`
- `Min Off Cycle Duration`
- `PWM Periode`
- `PWM Min`
- `PWM Max`
- `Taupunkt Abstand`
- `Output Safety`
- `Stellgliedtest Ausgang`

## Diagnostics

The component publishes the following diagnostic entities:

- `Reglerausgang`
- `Reglerausgang vor Begrenzung`
- `Reglerinternes Ventil`
- `PID Anteil P`
- `I Speicher`
- `PID Anteil D`
- `PID dt`
- `PID Fehler`
- `Sollwert`
- `Effektiver Sollwert`
- `Taupunkt`
- `Isttemperatur`
- `Istfeuchte`
- `Temperaturquelle`
- `Feuchtequelle`
- `Ausgangsstatus`
- `Modusstatus`

## Control behavior

- In heating mode, the controller uses `effective_target - current_temperature` as the error.
- In cooling mode, the controller uses `current_temperature - effective_target` as the error.
- Cooling mode also raises the effective target to `dew_point + dew_point_offset` when needed.
- `cold_tolerance` and `hot_tolerance` are used as final cut-off thresholds to avoid unnecessary output near the target.

## Configuration notes

Sensor assignments, fallback sensor assignments, sensor timeouts, `valve_switch`,
`valve_control_enabled`, and `debug` remain static YAML configuration.

The component is intended for the `components/pid_thermostat` folder inside this library and
is consumed from ESPHome YAML via `platform: pid_thermostat`.

## Example usage

### 1. Include the repository component

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/kwittwer/esphome_kfw_lib
      ref: main
    components: [pid_thermostat]
```

### 2. Create a device with one thermostat

```yaml
substitutions:
  device_name: test_pid_thermostat
  friendly_name: "Test PID Thermostat"

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}

external_components:
  - source:
      type: git
      url: https://github.com/kwittwer/esphome_kfw_lib
      ref: main
    components: [pid_thermostat]

climate:
  - platform: pid_thermostat
    name: "Room 1"
    id: thermostat1
    device_id: thermostat1_device
    temperature_sensor: room1_temperature
    humidity_sensor: room1_humidity
    fallback_temperature_sensor: room1_fallback_temperature
    fallback_humidity_sensor: room1_fallback_humidity
    temperature_sensor_timeout: 10min
    humidity_sensor_timeout: 10min
    valve_switch: relay_1
    valve_control_enabled: !lambda |-
      return true;
    kp: 5.0
    ki: 0.01
    kd: 500.0
    i_min: -100.0
    i_max: 100.0
    cold_tolerance: 0.3
    hot_tolerance: 0.3
    pwm: 15min
    pwm_min: 0.0
    pwm_max: 100.0
    sampling_period: 0s
    keep_alive: 60s
    min_cycle_duration: 0s
    min_off_cycle_duration: 0s
    output_safety: 5.0
    dew_point_offset: 1.0
```

This creates the climate entity plus the diagnostic and tuning entities in Home Assistant.