import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, number, sensor, switch, text_sensor
from esphome.const import (
    CONF_ACCURACY_DECIMALS,
    CONF_DISABLED_BY_DEFAULT,
    CONF_ENTITY_CATEGORY,
    CONF_FORCE_UPDATE,
    CONF_ICON,
    CONF_ID,
    CONF_INTERNAL,
    CONF_MQTT_ID,
    CONF_MODE,
    CONF_NAME,
    CONF_ON_PRESS,
    CONF_ON_RAW_VALUE,
    CONF_ON_VALUE,
    CONF_ON_VALUE_RANGE,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_WEB_SERVER,
)
from esphome.core import ID

from . import pid_thermostat_ns

AUTO_LOAD = ["climate"]

PidThermostat = pid_thermostat_ns.class_("PidThermostat", climate.Climate, cg.Component)
PidThermostatNumber = pid_thermostat_ns.class_("PidThermostatNumber", number.Number, cg.Component)
PidThermostatSensor = pid_thermostat_ns.class_("PidThermostatSensor", sensor.Sensor, cg.Component)
PidThermostatTextSensor = pid_thermostat_ns.class_("PidThermostatTextSensor", text_sensor.TextSensor, cg.Component)
NumberKind = pid_thermostat_ns.enum("NumberKind")

CONF_TEMPERATURE_SENSOR = "temperature_sensor"
CONF_HUMIDITY_SENSOR = "humidity_sensor"
CONF_FALLBACK_TEMPERATURE_SENSOR = "fallback_temperature_sensor"
CONF_TEMPERATURE_SENSOR_TIMEOUT = "temperature_sensor_timeout"
CONF_FALLBACK_TEMPERATURE_SENSOR_TIMEOUT = "fallback_temperature_sensor_timeout"
CONF_FALLBACK_HUMIDITY_SENSOR = "fallback_humidity_sensor"
CONF_HUMIDITY_SENSOR_TIMEOUT = "humidity_sensor_timeout"
CONF_FALLBACK_HUMIDITY_SENSOR_TIMEOUT = "fallback_humidity_sensor_timeout"
CONF_VALVE_SWITCH = "valve_switch"
CONF_VALVE_CONTROL_ENABLED = "valve_control_enabled"
CONF_KP = "kp"
CONF_KI = "ki"
CONF_KD = "kd"
CONF_PWM = "pwm"
CONF_PWM_MIN = "pwm_min"
CONF_PWM_MAX = "pwm_max"
CONF_SAMPLING_PERIOD = "sampling_period"
CONF_KEEP_ALIVE = "keep_alive"
CONF_MIN_CYCLE_DURATION = "min_cycle_duration"
CONF_MIN_OFF_CYCLE_DURATION = "min_off_cycle_duration"
CONF_COLD_TOLERANCE = "cold_tolerance"
CONF_HOT_TOLERANCE = "hot_tolerance"
CONF_DEW_POINT_OFFSET = "dew_point_offset"
CONF_OUTPUT_SAFETY = "output_safety"
CONF_DEBUG = "debug"

CONF_KIND = "kind"

NUMBER_KIND_KP = NumberKind.NUMBER_KIND_KP
NUMBER_KIND_KI = NumberKind.NUMBER_KIND_KI
NUMBER_KIND_KD = NumberKind.NUMBER_KIND_KD
NUMBER_KIND_PWM_PERIOD = NumberKind.NUMBER_KIND_PWM_PERIOD
NUMBER_KIND_PWM_MIN = NumberKind.NUMBER_KIND_PWM_MIN
NUMBER_KIND_PWM_MAX = NumberKind.NUMBER_KIND_PWM_MAX
NUMBER_KIND_DEW_POINT_OFFSET = NumberKind.NUMBER_KIND_DEW_POINT_OFFSET

SENSOR_KIND_OUTPUT = "output"
TEXT_SENSOR_KIND_MODE = "mode"
TEXT_SENSOR_KIND_TEMPERATURE_SOURCE = "temperature_source"
TEXT_SENSOR_KIND_HUMIDITY_SOURCE = "humidity_source"


def _number_config(parent_name, parent_id, suffix, id_suffix, kind, unit, initial, min_value, max_value, step):
    return {
        CONF_ID: ID(f"{parent_id.id}_{id_suffix}", is_declaration=True, type=PidThermostatNumber),
        CONF_NAME: f"{parent_name} {suffix}",
        CONF_DISABLED_BY_DEFAULT: False,
        CONF_INTERNAL: False,
        CONF_MQTT_ID: None,
        CONF_ON_VALUE: [],
        CONF_ON_VALUE_RANGE: [],
        CONF_WEB_SERVER: None,
        CONF_ENTITY_CATEGORY: "config",
        CONF_ICON: "mdi:tune",
        CONF_UNIT_OF_MEASUREMENT: unit,
        CONF_KIND: kind,
        CONF_MODE: number.NUMBER_MODES["BOX"],
        "optimistic": True,
        "restore_value": True,
        "initial_value": initial,
        "min_value": min_value,
        "max_value": max_value,
        "step": step,
    }


def _sensor_config(parent_name, parent_id):
    return {
        CONF_ID: ID(f"{parent_id.id}_output_pct", is_declaration=True, type=PidThermostatSensor),
        CONF_NAME: f"{parent_name} Reglerausgang",
        CONF_DISABLED_BY_DEFAULT: False,
        CONF_INTERNAL: False,
        CONF_MQTT_ID: None,
        CONF_ON_VALUE: [],
        CONF_ON_VALUE_RANGE: [],
        CONF_WEB_SERVER: None,
        CONF_FORCE_UPDATE: False,
        CONF_ACCURACY_DECIMALS: 1,
        CONF_UNIT_OF_MEASUREMENT: "%",
        CONF_KIND: SENSOR_KIND_OUTPUT,
    }


def _text_sensor_config(parent_name, parent_id, name_suffix, id_suffix, kind):
    return {
        CONF_ID: ID(f"{parent_id.id}_{id_suffix}", is_declaration=True, type=PidThermostatTextSensor),
        CONF_NAME: f"{parent_name} {name_suffix}",
        CONF_DISABLED_BY_DEFAULT: False,
        CONF_INTERNAL: False,
        CONF_MQTT_ID: None,
        CONF_ON_VALUE: [],
        CONF_ON_RAW_VALUE: [],
        CONF_WEB_SERVER: None,
        CONF_KIND: TEXT_SENSOR_KIND_MODE,
        CONF_ICON: "mdi:hvac",
        CONF_ENTITY_CATEGORY: "diagnostic",
    }


CONFIG_SCHEMA = climate.climate_schema(PidThermostat).extend(
    {
        cv.Required(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
        cv.Required(CONF_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_FALLBACK_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_TEMPERATURE_SENSOR_TIMEOUT, default="10min"): cv.time_period,
        cv.Optional(CONF_FALLBACK_TEMPERATURE_SENSOR_TIMEOUT, default="10min"): cv.time_period,
        cv.Optional(CONF_FALLBACK_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_HUMIDITY_SENSOR_TIMEOUT, default="0s"): cv.time_period,
        cv.Optional(CONF_FALLBACK_HUMIDITY_SENSOR_TIMEOUT, default="10min"): cv.time_period,
        cv.Optional(CONF_VALVE_SWITCH): cv.use_id(switch.Switch),
        cv.Optional(CONF_VALVE_CONTROL_ENABLED, default=True): cv.templatable(cv.boolean),
        cv.Optional(CONF_KP, default=5.0): cv.float_,
        cv.Optional(CONF_KI, default=0.01): cv.float_,
        cv.Optional(CONF_KD, default=500.0): cv.float_,
        cv.Optional(CONF_PWM, default="15min"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_PWM_MIN, default=0.0): cv.float_range(min=0.0, max=100.0),
        cv.Optional(CONF_PWM_MAX, default=100.0): cv.float_range(min=0.0, max=100.0),
        cv.Optional(CONF_SAMPLING_PERIOD, default="0s"): cv.time_period,
        cv.Optional(CONF_KEEP_ALIVE, default="60s"): cv.time_period,
        cv.Optional(CONF_MIN_CYCLE_DURATION, default="0s"): cv.time_period,
        cv.Optional(CONF_MIN_OFF_CYCLE_DURATION, default="0s"): cv.time_period,
        cv.Optional(CONF_COLD_TOLERANCE, default=0.3): cv.temperature,
        cv.Optional(CONF_HOT_TOLERANCE, default=0.3): cv.temperature,
        cv.Optional(CONF_DEW_POINT_OFFSET, default=1.0): cv.float_range(min=0.0, max=10.0),
        cv.Optional(CONF_OUTPUT_SAFETY, default=5.0): cv.float_range(min=0.0, max=100.0),
        cv.Optional(CONF_DEBUG, default=False): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    sens = await cg.get_variable(config[CONF_TEMPERATURE_SENSOR])
    cg.add(var.set_sensor(sens))

    humidity_sens = await cg.get_variable(config[CONF_HUMIDITY_SENSOR])
    cg.add(var.set_humidity_sensor(humidity_sens))

    if CONF_FALLBACK_TEMPERATURE_SENSOR in config:
        sens = await cg.get_variable(config[CONF_FALLBACK_TEMPERATURE_SENSOR])
        cg.add(var.set_fallback_sensor(sens))

    if CONF_FALLBACK_HUMIDITY_SENSOR in config:
        sens = await cg.get_variable(config[CONF_FALLBACK_HUMIDITY_SENSOR])
        cg.add(var.set_fallback_humidity_sensor(sens))

    if CONF_VALVE_SWITCH in config:
        sw = await cg.get_variable(config[CONF_VALVE_SWITCH])
        cg.add(var.set_valve_switch(sw))

    template_ = await cg.templatable(config[CONF_VALVE_CONTROL_ENABLED], [], cg.bool_)
    cg.add(var.set_valve_control_enabled(template_))

    cg.add(var.set_sensor_timeout(config[CONF_TEMPERATURE_SENSOR_TIMEOUT].total_milliseconds))
    cg.add(var.set_fallback_sensor_timeout(config[CONF_FALLBACK_TEMPERATURE_SENSOR_TIMEOUT].total_milliseconds))
    cg.add(var.set_humidity_sensor_timeout(config[CONF_HUMIDITY_SENSOR_TIMEOUT].total_milliseconds))
    cg.add(var.set_fallback_humidity_sensor_timeout(config[CONF_FALLBACK_HUMIDITY_SENSOR_TIMEOUT].total_milliseconds))
    cg.add(var.set_kp(config[CONF_KP]))
    cg.add(var.set_ki(config[CONF_KI]))
    cg.add(var.set_kd(config[CONF_KD]))
    cg.add(var.set_pwm_period(config[CONF_PWM].total_milliseconds))
    cg.add(var.set_pwm_min(config[CONF_PWM_MIN]))
    cg.add(var.set_pwm_max(config[CONF_PWM_MAX]))
    cg.add(var.set_sampling_period(config[CONF_SAMPLING_PERIOD].total_milliseconds))
    cg.add(var.set_keep_alive(config[CONF_KEEP_ALIVE].total_milliseconds))
    cg.add(var.set_min_cycle_duration(config[CONF_MIN_CYCLE_DURATION].total_milliseconds))
    cg.add(var.set_min_off_cycle_duration(config[CONF_MIN_OFF_CYCLE_DURATION].total_milliseconds))
    cg.add(var.set_cold_tolerance(config[CONF_COLD_TOLERANCE]))
    cg.add(var.set_hot_tolerance(config[CONF_HOT_TOLERANCE]))
    cg.add(var.set_dew_point_offset(config[CONF_DEW_POINT_OFFSET]))
    cg.add(var.set_output_safety(config[CONF_OUTPUT_SAFETY]))
    cg.add(var.set_debug(config[CONF_DEBUG]))

    parent_name = config[CONF_NAME]
    parent_id = config[CONF_ID]

    output_sensor_config = _sensor_config(parent_name, parent_id)
    output_sensor = await sensor.new_sensor(output_sensor_config)
    cg.add(var.set_output_sensor(output_sensor))

    mode_sensor_config = _text_sensor_config(parent_name, parent_id, "Modusstatus", "mode_state", TEXT_SENSOR_KIND_MODE)
    mode_sensor = await text_sensor.new_text_sensor(mode_sensor_config)
    cg.add(var.set_mode_text_sensor(mode_sensor))

    temp_source_sensor_config = _text_sensor_config(
        parent_name, parent_id, "Temperaturquelle", "temperature_source", TEXT_SENSOR_KIND_TEMPERATURE_SOURCE
    )
    temp_source_sensor = await text_sensor.new_text_sensor(temp_source_sensor_config)
    cg.add(var.set_temperature_source_text_sensor(temp_source_sensor))

    humidity_source_sensor_config = _text_sensor_config(
        parent_name, parent_id, "Feuchtequelle", "humidity_source", TEXT_SENSOR_KIND_HUMIDITY_SOURCE
    )
    humidity_source_sensor = await text_sensor.new_text_sensor(humidity_source_sensor_config)
    cg.add(var.set_humidity_source_text_sensor(humidity_source_sensor))

    number_configs = [
        _number_config(parent_name, parent_id, "Kp", "kp", NUMBER_KIND_KP, "", config[CONF_KP], 0, 200, 0.1),
        _number_config(parent_name, parent_id, "Ki", "ki", NUMBER_KIND_KI, "", config[CONF_KI], 0, 1, 0.001),
        _number_config(parent_name, parent_id, "Kd", "kd", NUMBER_KIND_KD, "", config[CONF_KD], 0, 5000, 1),
        _number_config(parent_name, parent_id, "PWM Periode", "pwm_period", NUMBER_KIND_PWM_PERIOD, "s", config[CONF_PWM].total_seconds, 60, 3600, 30),
        _number_config(parent_name, parent_id, "PWM Min", "pwm_min", NUMBER_KIND_PWM_MIN, "%", config[CONF_PWM_MIN], 0, 100, 1),
        _number_config(parent_name, parent_id, "PWM Max", "pwm_max", NUMBER_KIND_PWM_MAX, "%", config[CONF_PWM_MAX], 0, 100, 1),
        _number_config(parent_name, parent_id, "Taupunkt Abstand", "dew_point_offset", NUMBER_KIND_DEW_POINT_OFFSET, "K", config[CONF_DEW_POINT_OFFSET], 0, 10, 0.1),
    ]

    for number_config in number_configs:
        entity = await number.new_number(
            number_config,
            var,
            number_config[CONF_KIND],
            min_value=number_config["min_value"],
            max_value=number_config["max_value"],
            step=number_config["step"],
        )
        cg.add(var.register_number_entity(entity))
