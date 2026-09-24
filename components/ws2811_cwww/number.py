import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_ID
import esphome.final_validate as fv
from esphome.types import ConfigType

from . import (
    CONF_BOUNDARIES,
    CONF_BOUNDARY_INDEX,
    CONF_TYPE,
    CONF_WS2811_CWWW_ID,
    TYPE_ACTIVE_LEDS,
    TYPE_BOUNDARY,
    WS2811CWWWHub,
    ws2811_cwww_ns,
)

DEPENDENCIES = ["ws2811_cwww"]

WS2811CWWWSegmentBoundaryNumber = ws2811_cwww_ns.class_(
    "WS2811CWWWSegmentBoundaryNumber", number.Number, cg.Component
)


def _get_hub_config(config: ConfigType) -> ConfigType:
    fconf = fv.full_config.get()
    hub_path = fconf.get_path_for_id(config[CONF_WS2811_CWWW_ID])[:-1]
    return fconf.get_config_for_path(hub_path)


def _validate_boundary_index(config: ConfigType) -> ConfigType:
    if config[CONF_TYPE] != TYPE_BOUNDARY:
        return config
    hub_config = _get_hub_config(config)
    if CONF_BOUNDARY_INDEX not in config:
        raise cv.Invalid("boundary_index is required for type 'boundary'", [CONF_BOUNDARY_INDEX])
    if config[CONF_BOUNDARY_INDEX] >= len(hub_config[CONF_BOUNDARIES]):
        raise cv.Invalid(
            f"boundary_index {config[CONF_BOUNDARY_INDEX]} is out of range",
            [CONF_BOUNDARY_INDEX],
        )
    return config


CONFIG_SCHEMA = number.number_schema(
    WS2811CWWWSegmentBoundaryNumber, entity_category="config"
).extend(
    {
        cv.Required(CONF_WS2811_CWWW_ID): cv.use_id(WS2811CWWWHub),
        cv.Optional(CONF_TYPE, default=TYPE_BOUNDARY): cv.one_of(TYPE_BOUNDARY, TYPE_ACTIVE_LEDS, lower=True),
        cv.Optional(CONF_BOUNDARY_INDEX): cv.positive_int,
    }
)

FINAL_VALIDATE_SCHEMA = _validate_boundary_index


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_WS2811_CWWW_ID])

    boundary_index = config.get(CONF_BOUNDARY_INDEX, -1)
    var = cg.new_Pvariable(config[CONF_ID], hub, boundary_index)
    cg.add(hub.register_boundary_number(var))
    await cg.register_component(var, config)
    await number.register_number(var, config, min_value=0, max_value=1, step=1)
