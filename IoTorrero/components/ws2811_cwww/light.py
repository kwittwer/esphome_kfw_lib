import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import (
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_CONSTANT_BRIGHTNESS,
    CONF_OUTPUT_ID,
    CONF_REVERSED,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
)
import esphome.final_validate as fv
from esphome.types import ConfigType

from . import (
    CONF_BOUNDARIES,
    CONF_CENTER_POWER_TRANSITION,
    CONF_CENTER_POWER_TRANSITION_OVERLAP,
    CONF_POWER_OFF_TRANSITION_LENGTH,
    CONF_POWER_ON_TRANSITION_LENGTH,
    CONF_SEGMENT_INDEX,
    CONF_WS2811_CWWW_ID,
    WS2811CWWWHub,
    ws2811_cwww_ns,
)

DEPENDENCIES = ["ws2811_cwww"]

WS2811CWWWLightOutput = ws2811_cwww_ns.class_(
    "WS2811CWWWLightOutput", light.AddressableLight
)


def _validate_segment_index(config: ConfigType) -> ConfigType:
    fconf = fv.full_config.get()
    hub_path = fconf.get_path_for_id(config[CONF_WS2811_CWWW_ID])[:-1]
    hub_config = fconf.get_config_for_path(hub_path)
    segment_count = len(hub_config[CONF_BOUNDARIES]) + 1
    if config[CONF_SEGMENT_INDEX] >= segment_count:
        raise cv.Invalid(
            f"segment_index {config[CONF_SEGMENT_INDEX]} is out of range for {segment_count} segments",
            [CONF_SEGMENT_INDEX],
        )
    return config


CONFIG_SCHEMA = cv.All(
    light.ADDRESSABLE_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(WS2811CWWWLightOutput),
            cv.Required(CONF_WS2811_CWWW_ID): cv.use_id(WS2811CWWWHub),
            cv.Required(CONF_SEGMENT_INDEX): cv.positive_int,
            cv.Optional(CONF_REVERSED, default=False): cv.boolean,
            cv.Optional(CONF_COLD_WHITE_COLOR_TEMPERATURE, default="6500 K"): cv.color_temperature,
            cv.Optional(CONF_WARM_WHITE_COLOR_TEMPERATURE, default="2700 K"): cv.color_temperature,
            cv.Optional(CONF_CONSTANT_BRIGHTNESS, default=False): cv.boolean,
            cv.Optional(CONF_CENTER_POWER_TRANSITION, default=True): cv.boolean,
            cv.Optional(CONF_CENTER_POWER_TRANSITION_OVERLAP, default="40%"): cv.percentage,
            cv.Optional(CONF_POWER_ON_TRANSITION_LENGTH, default="1s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_POWER_OFF_TRANSITION_LENGTH, default="1s"): cv.positive_time_period_milliseconds,
        }
    ),
    light.validate_color_temperature_channels,
)

FINAL_VALIDATE_SCHEMA = _validate_segment_index


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_WS2811_CWWW_ID])
    var = cg.new_Pvariable(
        config[CONF_OUTPUT_ID],
        hub,
        config[CONF_SEGMENT_INDEX],
        config[CONF_REVERSED],
    )
    cg.add(hub.register_segment_light(var))
    await cg.register_component(var, config)
    await light.register_light(var, config)

    cg.add(var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE]))
    cg.add(var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE]))
    cg.add(var.set_constant_brightness(config[CONF_CONSTANT_BRIGHTNESS]))
    cg.add(var.set_center_power_transition(config[CONF_CENTER_POWER_TRANSITION]))
    cg.add(var.set_center_power_transition_overlap(config[CONF_CENTER_POWER_TRANSITION_OVERLAP]))
    cg.add(var.set_power_on_transition_length(config[CONF_POWER_ON_TRANSITION_LENGTH]))
    cg.add(var.set_power_off_transition_length(config[CONF_POWER_OFF_TRANSITION_LENGTH]))
