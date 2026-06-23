import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome import automation

DEPENDENCIES = ["network"]
CODEOWNERS = ["@yourusername"]

modbus_tcp_server_ns = cg.esphome_ns.namespace("modbus_tcp_server")
ModbusTCPServer = modbus_tcp_server_ns.class_("ModbusTCPServer", cg.Component)

CONF_PORT = "port"
CONF_MAX_REGISTERS = "max_registers"
CONF_MAX_COILS = "max_coils"
CONF_UNIT_ID = "unit_id"
CONF_ON_WRITE_REGISTER = "on_write_register"
CONF_ON_WRITE_COIL = "on_write_coil"

ModbusWriteRegisterTrigger = modbus_tcp_server_ns.class_(
    "ModbusWriteRegisterTrigger", 
    automation.Trigger.template(cg.uint16, cg.uint16)
)

ModbusWriteCoilTrigger = modbus_tcp_server_ns.class_(
    "ModbusWriteCoilTrigger", 
    automation.Trigger.template(cg.uint16, cg.bool_)
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ModbusTCPServer),
        cv.Optional(CONF_PORT, default=502): cv.port,
        cv.Optional(CONF_MAX_REGISTERS, default=100): cv.int_range(min=1, max=65535),
        cv.Optional(CONF_MAX_COILS, default=100): cv.int_range(min=1, max=65535),
        cv.Optional(CONF_UNIT_ID, default=1): cv.int_range(min=0, max=255),
        cv.Optional(CONF_ON_WRITE_REGISTER): automation.validate_automation(
            {
                cv.GenerateID(automation.CONF_TRIGGER_ID): cv.declare_id(ModbusWriteRegisterTrigger),
            }
        ),
        cv.Optional(CONF_ON_WRITE_COIL): automation.validate_automation(
            {
                cv.GenerateID(automation.CONF_TRIGGER_ID): cv.declare_id(ModbusWriteCoilTrigger),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_max_registers(config[CONF_MAX_REGISTERS]))
    cg.add(var.set_max_coils(config[CONF_MAX_COILS]))
    cg.add(var.set_unit_id(config[CONF_UNIT_ID]))

    # Callbacks für Register-Schreibvorgänge
    for conf in config.get(CONF_ON_WRITE_REGISTER, []):
        trigger = cg.new_Pvariable(conf[automation.CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger, [(cg.uint16, "address"), (cg.uint16, "value")], conf
        )

    # Callbacks für Coil-Schreibvorgänge
    for conf in config.get(CONF_ON_WRITE_COIL, []):
        trigger = cg.new_Pvariable(conf[automation.CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger, [(cg.uint16, "address"), (cg.bool_, "value")], conf
        )