import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import coroutine_with_priority
import esphome.components.binary_sensor as binary_sensor

CONF_ERROR_FLAG = "error_flag"
CONF_WARNING_FLAG = "warning_flag"

status_bin_sensor_ns = cg.esphome_ns.namespace("status_bin_sensor")
StatusBinSensor = status_bin_sensor_ns.class_("StatusBinSensor", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(StatusBinSensor),
        cv.Optional(CONF_ERROR_FLAG): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_WARNING_FLAG): cv.use_id(binary_sensor.BinarySensor),
    }
).extend(cv.COMPONENT_SCHEMA)

@coroutine_with_priority(80.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_ERROR_FLAG in config:
        sens = await cg.get_variable(config[CONF_ERROR_FLAG])
        cg.add(var.set_error_flag(sens))

    if CONF_WARNING_FLAG in config:
        sens = await cg.get_variable(config[CONF_WARNING_FLAG])
        cg.add(var.set_warning_flag(sens))

    cg.add_define("USE_STATUS_BIN_SENSOR")
