#pragma once

#include "esphome/core/automation.h"
#include "modbus_tcp_server.h"

namespace esphome {
namespace modbus_tcp_server {

class ModbusWriteRegisterTrigger : public Trigger<uint16_t, uint16_t> {
 public:
  explicit ModbusWriteRegisterTrigger(ModbusTCPServer *parent) {
    parent->add_on_write_register_callback([this](uint16_t address, uint16_t value) {
      this->trigger(address, value);
    });
  }
};

class ModbusWriteCoilTrigger : public Trigger<uint16_t, bool> {
 public:
  explicit ModbusWriteCoilTrigger(ModbusTCPServer *parent) {
    parent->add_on_write_coil_callback([this](uint16_t address, bool value) {
      this->trigger(address, value);
    });
  }
};

}  // namespace modbus_tcp_server
}  // namespace esphome