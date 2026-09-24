import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, sensor, switch, output, text_sensor, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

CODEOWNERS = ["@your_username"]
AUTO_LOAD = ["climate"]

dummy_thermostat_ns = cg.esphome_ns.namespace("dummy_thermostat")
DummyThermostat = dummy_thermostat_ns.class_(
    "DummyThermostat", climate.Climate, cg.Component
)

# Configuration keys
CONF_FALLBACK_SENSOR = "fallback_sensor"
CONF_SENSOR_TIMEOUT = "sensor_timeout"
CONF_FALLBACK_SENSOR_TIMEOUT = "fallback_sensor_timeout"
CONF_FALLBACK_HUMIDITY_SENSOR = "fallback_humidity_sensor"
CONF_HUMIDITY_SENSOR_TIMEOUT = "humidity_sensor_timeout"
CONF_VALVE_UPDATE_TIMEOUT = "valve_update_timeout"
CONF_VALVE_OUTPUT = "valve_output"
CONF_VALVE_SWITCH = "valve_switch"
CONF_VALVE_CONTROL_ENABLED = "valve_control_enabled"
CONF_USE_LOCAL_VALVE_CONTROL = "use_local_valve_control"
CONF_HEATING_DEADBAND = "heating_deadband"
CONF_HEATING_OVERRUN = "heating_overrun"
CONF_COOLING_DEADBAND = "cooling_deadband"
CONF_COOLING_OVERRUN = "cooling_overrun"
CONF_DIAGNOSTIC_SOURCE_STATUS = "diagnostic_source_status"
CONF_DIAGNOSTIC_FALLBACK_TEMPERATURE_ACTIVE = "diagnostic_fallback_temperature_active"
CONF_DIAGNOSTIC_FALLBACK_HUMIDITY_ACTIVE = "diagnostic_fallback_humidity_active"
CONF_DIAGNOSTIC_LOCAL_CONTROLLER_ACTIVE = "diagnostic_local_controller_active"


def validate_timeout_seconds(value):
    if isinstance(value, int):
        return cv.int_range(min=0)(value)

    if isinstance(value, str):
        normalized = value.strip().lower()
        if normalized in {"0", "0s", "0sec", "0secs", "0second", "0seconds", "0m", "0min", "0h"}:
            return 0
        # Support shorthand like "10m" in addition to ESPHome's "10min".
        if normalized.endswith("m") and not normalized.endswith("ms"):
            normalized = f"{normalized[:-1]}min"
        return int(cv.positive_time_period_seconds(normalized).total_seconds)

    return int(cv.positive_time_period_seconds(value).total_seconds)


CONFIG_SCHEMA = climate.climate_schema(DummyThermostat).extend(
    {
        cv.Optional(CONF_FALLBACK_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SENSOR_TIMEOUT, default=600): validate_timeout_seconds,
        cv.Optional(CONF_FALLBACK_SENSOR_TIMEOUT, default=600): validate_timeout_seconds,
        cv.Optional(CONF_FALLBACK_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_HUMIDITY_SENSOR_TIMEOUT, default=0): validate_timeout_seconds,
        cv.Optional(CONF_VALVE_UPDATE_TIMEOUT, default=0): validate_timeout_seconds,
        cv.Optional(CONF_VALVE_OUTPUT): cv.use_id(output.FloatOutput),
        cv.Optional(CONF_VALVE_SWITCH): cv.use_id(switch.Switch),
        cv.Optional(CONF_VALVE_CONTROL_ENABLED, default=False): cv.templatable(cv.boolean),
        cv.Optional(CONF_USE_LOCAL_VALVE_CONTROL, default=False): cv.templatable(cv.boolean),
        cv.Optional(CONF_HEATING_DEADBAND, default=0.5): cv.temperature,
        cv.Optional(CONF_HEATING_OVERRUN, default=0.5): cv.temperature,
        cv.Optional(CONF_COOLING_DEADBAND, default=0.5): cv.temperature,
        cv.Optional(CONF_COOLING_OVERRUN, default=0.5): cv.temperature,
        cv.Optional(CONF_DIAGNOSTIC_SOURCE_STATUS): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_DIAGNOSTIC_FALLBACK_TEMPERATURE_ACTIVE): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_DIAGNOSTIC_FALLBACK_HUMIDITY_ACTIVE): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_DIAGNOSTIC_LOCAL_CONTROLLER_ACTIVE): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    climate_name = config.get(CONF_NAME, "Dummy Thermostat")
    climate_id_base = "".join(
        c.lower() if c.isalnum() else "_" for c in climate_name
    ).strip("_")
    while "__" in climate_id_base:
        climate_id_base = climate_id_base.replace("__", "_")
    if not climate_id_base:
        climate_id_base = "dummy_thermostat"
    if climate_id_base[0].isdigit():
        climate_id_base = f"n_{climate_id_base}"


    if CONF_FALLBACK_SENSOR in config:
        fallback_sens = await cg.get_variable(config[CONF_FALLBACK_SENSOR])
        cg.add(var.set_fallback_sensor(fallback_sens))

    cg.add(var.set_sensor_timeout(config[CONF_SENSOR_TIMEOUT]))
    cg.add(var.set_fallback_sensor_timeout(config[CONF_FALLBACK_SENSOR_TIMEOUT]))

    if CONF_FALLBACK_HUMIDITY_SENSOR in config:
        fallback_humidity_sens = await cg.get_variable(config[CONF_FALLBACK_HUMIDITY_SENSOR])
        cg.add(var.set_fallback_humidity_sensor(fallback_humidity_sens))

    cg.add(var.set_humidity_sensor_timeout(config[CONF_HUMIDITY_SENSOR_TIMEOUT]))

    cg.add(var.set_valve_update_timeout(config[CONF_VALVE_UPDATE_TIMEOUT]))

    if CONF_VALVE_OUTPUT in config:
        valve_out = await cg.get_variable(config[CONF_VALVE_OUTPUT])
        cg.add(var.set_valve_output(valve_out))

    if CONF_VALVE_SWITCH in config:
        valve_sw = await cg.get_variable(config[CONF_VALVE_SWITCH])
        cg.add(var.set_valve_switch(valve_sw))

    if CONF_VALVE_CONTROL_ENABLED in config:
        template_ = await cg.templatable(config[CONF_VALVE_CONTROL_ENABLED], [], cg.bool_)
        cg.add(var.set_valve_control_enabled(template_))

    if CONF_USE_LOCAL_VALVE_CONTROL in config:
        template_ = await cg.templatable(config[CONF_USE_LOCAL_VALVE_CONTROL], [], cg.bool_)
        cg.add(var.set_use_local_valve_control(template_))

    cg.add(var.set_heating_deadband(config[CONF_HEATING_DEADBAND]))
    cg.add(var.set_heating_overrun(config[CONF_HEATING_OVERRUN]))
    cg.add(var.set_cooling_deadband(config[CONF_COOLING_DEADBAND]))
    cg.add(var.set_cooling_overrun(config[CONF_COOLING_OVERRUN]))

    source_status_config = config.get(CONF_DIAGNOSTIC_SOURCE_STATUS)
    if source_status_config is None:
        source_status_config = text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        )(
            {
                CONF_ID: f"{climate_id_base}_quellenstatus",
                CONF_NAME: f"{climate_name} Quellenstatus",
            }
        )
    diagnostic_status = await text_sensor.new_text_sensor(source_status_config)
    cg.add(var.set_source_status_text_sensor(diagnostic_status))

    fallback_temp_config = config.get(CONF_DIAGNOSTIC_FALLBACK_TEMPERATURE_ACTIVE)
    if fallback_temp_config is None:
        fallback_temp_config = binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        )(
            {
                CONF_ID: f"{climate_id_base}_fallback_temperatur_aktiv",
                CONF_NAME: f"{climate_name} Fallback Temperatur aktiv",
            }
        )
    diagnostic_fallback_temp = await binary_sensor.new_binary_sensor(fallback_temp_config)
    cg.add(var.set_fallback_temperature_binary_sensor(diagnostic_fallback_temp))

    fallback_humidity_config = config.get(CONF_DIAGNOSTIC_FALLBACK_HUMIDITY_ACTIVE)
    if fallback_humidity_config is None:
        fallback_humidity_config = binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        )(
            {
                CONF_ID: f"{climate_id_base}_fallback_feuchte_aktiv",
                CONF_NAME: f"{climate_name} Fallback Feuchte aktiv",
            }
        )
    diagnostic_fallback_humidity = await binary_sensor.new_binary_sensor(
        fallback_humidity_config
    )
    cg.add(var.set_fallback_humidity_binary_sensor(diagnostic_fallback_humidity))

    local_controller_config = config.get(CONF_DIAGNOSTIC_LOCAL_CONTROLLER_ACTIVE)
    if local_controller_config is None:
        local_controller_config = binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        )(
            {
                CONF_ID: f"{climate_id_base}_lokaler_controller_aktiv",
                CONF_NAME: f"{climate_name} Lokaler Controller aktiv",
            }
        )
    diagnostic_local_controller = await binary_sensor.new_binary_sensor(
        local_controller_config
    )
    cg.add(var.set_local_controller_binary_sensor(diagnostic_local_controller))
    