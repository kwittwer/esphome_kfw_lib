#include "modbustcp.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
//#include "asynctcp.h"

namespace esphome {
namespace modbustcp {

static const char *const TAG = "modbus_tcp";

void ModbusTCP::setup() {
     }

void ModbusTCP::dispatch_loopback_data_(const std::vector<uint8_t> &data) {
  for (auto *device : this->devices_) {
    device->on_modbus_data(data);
  }
  this->waiting_for_response = 0;
}

void ModbusTCP::dispatch_loopback_error_(uint8_t function_code, uint8_t exception_code) {
  for (auto *device : this->devices_) {
    device->on_modbus_error(function_code, exception_code);
  }
  this->waiting_for_response = 0;
}

bool ModbusTCP::process_internal_loopback_request_(uint8_t address, uint8_t function_code, uint16_t start_address,
                                                   uint16_t number_of_entities, uint8_t payload_len,
                                                   const uint8_t *payload) {
  if (this->loopback_server_ == nullptr) {
    ESP_LOGE(TAG, "Loopback enabled but no loopback_server_id configured");
    return false;
  }

  if (address != this->loopback_server_->get_unit_id()) {
    this->dispatch_loopback_error_(function_code, static_cast<uint8_t>(ModbusExceptionCode::ILLEGAL_DATA_ADDRESS));
    return true;
  }

  std::vector<uint8_t> response;
  response.reserve(number_of_entities * 2);

  switch (function_code) {
    case uint8_t(ModbusFunctionCode::READ_COILS): {
      const uint16_t byte_count = (number_of_entities + 7U) / 8U;
      response.assign(byte_count, 0);
      for (uint16_t i = 0; i < number_of_entities; i++) {
        if (this->loopback_server_->get_coil(start_address + i)) {
          response[i / 8U] |= uint8_t(1U << (i % 8U));
        }
      }
      this->dispatch_loopback_data_(response);
      return true;
    }
    case uint8_t(ModbusFunctionCode::READ_DISCRETE_INPUTS): {
      const uint16_t byte_count = (number_of_entities + 7U) / 8U;
      response.assign(byte_count, 0);
      for (uint16_t i = 0; i < number_of_entities; i++) {
        if (this->loopback_server_->get_discrete_input(start_address + i)) {
          response[i / 8U] |= uint8_t(1U << (i % 8U));
        }
      }
      this->dispatch_loopback_data_(response);
      return true;
    }
    case uint8_t(ModbusFunctionCode::READ_HOLDING_REGISTERS): {
      for (uint16_t i = 0; i < number_of_entities; i++) {
        const uint16_t value = this->loopback_server_->get_holding_register(start_address + i);
        response.push_back(value >> 8U);
        response.push_back(value & 0xFFU);
      }
      this->dispatch_loopback_data_(response);
      return true;
    }
    case uint8_t(ModbusFunctionCode::READ_INPUT_REGISTERS): {
      for (uint16_t i = 0; i < number_of_entities; i++) {
        const uint16_t value = this->loopback_server_->get_input_register(start_address + i);
        response.push_back(value >> 8U);
        response.push_back(value & 0xFFU);
      }
      this->dispatch_loopback_data_(response);
      return true;
    }
    case uint8_t(ModbusFunctionCode::WRITE_SINGLE_COIL): {
      if (payload == nullptr || payload_len < 2) {
        this->dispatch_loopback_error_(function_code, static_cast<uint8_t>(ModbusExceptionCode::ILLEGAL_DATA_VALUE));
        return true;
      }
      const uint16_t raw = (uint16_t(payload[0]) << 8U) | uint16_t(payload[1]);
      const bool state = raw == 0xFF00;
      this->loopback_server_->set_coil(start_address, state);

      response.push_back(start_address >> 8U);
      response.push_back(start_address & 0xFFU);
      response.push_back(payload[0]);
      response.push_back(payload[1]);
      this->dispatch_loopback_data_(response);
      return true;
    }
    case uint8_t(ModbusFunctionCode::WRITE_SINGLE_REGISTER): {
      if (payload == nullptr || payload_len < 2) {
        this->dispatch_loopback_error_(function_code, static_cast<uint8_t>(ModbusExceptionCode::ILLEGAL_DATA_VALUE));
        return true;
      }
      const uint16_t value = (uint16_t(payload[0]) << 8U) | uint16_t(payload[1]);
      this->loopback_server_->set_holding_register(start_address, value);

      response.push_back(start_address >> 8U);
      response.push_back(start_address & 0xFFU);
      response.push_back(payload[0]);
      response.push_back(payload[1]);
      this->dispatch_loopback_data_(response);
      return true;
    }
    case uint8_t(ModbusFunctionCode::WRITE_MULTIPLE_COILS): {
      if (payload == nullptr) {
        this->dispatch_loopback_error_(function_code, static_cast<uint8_t>(ModbusExceptionCode::ILLEGAL_DATA_VALUE));
        return true;
      }
      for (uint16_t i = 0; i < number_of_entities; i++) {
        const uint8_t byte_index = i / 8U;
        const uint8_t bit_index = i % 8U;
        bool state = false;
        if (byte_index < payload_len) {
          state = (payload[byte_index] & uint8_t(1U << bit_index)) != 0;
        }
        this->loopback_server_->set_coil(start_address + i, state);
      }

      response.push_back(start_address >> 8U);
      response.push_back(start_address & 0xFFU);
      response.push_back(number_of_entities >> 8U);
      response.push_back(number_of_entities & 0xFFU);
      this->dispatch_loopback_data_(response);
      return true;
    }
    case uint8_t(ModbusFunctionCode::WRITE_MULTIPLE_REGISTERS): {
      if (payload == nullptr || payload_len < number_of_entities * 2U) {
        this->dispatch_loopback_error_(function_code, static_cast<uint8_t>(ModbusExceptionCode::ILLEGAL_DATA_VALUE));
        return true;
      }
      for (uint16_t i = 0; i < number_of_entities; i++) {
        const uint16_t offset = i * 2U;
        const uint16_t value = (uint16_t(payload[offset]) << 8U) | uint16_t(payload[offset + 1U]);
        this->loopback_server_->set_holding_register(start_address + i, value);
      }

      response.push_back(start_address >> 8U);
      response.push_back(start_address & 0xFFU);
      response.push_back(number_of_entities >> 8U);
      response.push_back(number_of_entities & 0xFFU);
      this->dispatch_loopback_data_(response);
      return true;
    }
    default:
      this->dispatch_loopback_error_(function_code, static_cast<uint8_t>(ModbusExceptionCode::ILLEGAL_FUNCTION));
      return true;
  }
}

void ModbusTCP::loop() {

  const uint32_t now = App.get_loop_component_start_time();

#if !defined(USE_ESP_IDF)

 while (this->client.available()) {
  
  
  uint8_t byte1[256];
  uint8_t byte = this->client.read(byte1, sizeof(byte1));
  
  std::string res;
    char buf[5];
  size_t len = byte1[8];
  for (size_t i = 9; i < len + 9; i++) {
   sprintf(buf, "%02X", byte1[i]);
   res += buf;
   res += ":"; 
  }
ESP_LOGD(TAG, "<<< %02X%02X %02X%02X %02X%02X %02X %02X %02X %s ",
                      byte1[0], byte1[1], byte1[2], byte1[3], byte1[4], 
                      byte1[5], byte1[6], byte1[7], byte1[8], res.c_str());

if ((byte1[7] & 0x80) == 0x80) {
      ESP_LOGE(TAG,"Error:"); 
    if (byte1[8]  == 0x01) {
      ESP_LOGE(TAG,"Failure Code 0x01 ILLEGAL FUNCTION");
    }
    if (byte1[8]  == 0x02) {
      ESP_LOGE(TAG,"Failure Code 0x02 ILLEGAL DATA ADDRESS");
    }
    if (byte1[8]  == 0x03) {
      ESP_LOGE(TAG,"Failure Code 0x03 ILLEGAL DATA VALUE");
    }
    if (byte1[8]  == 0x04) {
      ESP_LOGE(TAG,"Failure Code 0x04 SERVER FAILURE");
    }
    if (byte1[8]  == 0x05) {
      ESP_LOGE(TAG,"Failure Code 0x05 ACKNOWLEDGE");
    }
    if (byte1[8]  == 0x06) {
      ESP_LOGE(TAG,"Failure Code 0x06 SERVER BUSY");
    }
    return;
  }

 uint8_t bytelen_len = 9;
  std::vector<uint8_t> data(byte1 + bytelen_len, byte1 + bytelen_len + 9);

//  ESP_LOGD(TAG, "Incomming Data %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
//                      data[0], data[1], data[2], data[3], data[4], 
//                      data[5], data[6], data[7], data[8]);
   


 for (auto *device : this->devices_) {
        device->on_modbus_data(data);
     }
 }
#endif
}


void ModbusTCP::dump_config() {
  ESP_LOGCONFIG(TAG, "Modbus_TCP:");
  ESP_LOGCONFIG(TAG,
                "  Client: %s:%d \n"
                "  Send Wait Time: %d ms\n",
                   host_.c_str(), port_, this->send_wait_time_);
  ESP_LOGCONFIG(TAG, "  Internal Loopback: %s", this->internal_loopback_ ? "ENABLED" : "DISABLED");
  if (this->internal_loopback_ && this->loopback_server_ == nullptr) {
    ESP_LOGW(TAG, "  Internal loopback enabled without loopback server");
  }
}
float ModbusTCP::get_setup_priority() const {
  // After UART bus
  return setup_priority::BUS - 1.0f;

  }

void ModbusTCP::send(uint8_t address, uint8_t function_code, uint16_t start_address, uint16_t number_of_entities, uint8_t payload_len, const uint8_t *payload) {
  static const size_t MAX_VALUES = 128;
 const uint32_t now = App.get_loop_component_start_time();
  // Only check max number of registers for standard function codes
  // Some devices use non standard codes like 0x43
  if (number_of_entities > MAX_VALUES && function_code <= 0x10) {
    ESP_LOGE(TAG, "send too many values %d max=%zu", number_of_entities, MAX_VALUES);
    return;
  }

  if (this->internal_loopback_) {
    this->waiting_for_response = address;
    if (this->process_internal_loopback_request_(address, function_code, start_address, number_of_entities, payload_len,
                                                 payload)) {
      last_send_ = millis();
      return;
    }
    this->waiting_for_response = 0;
  }

#if defined(USE_ESP_IDF)
  ESP_LOGE(TAG, "TCP client transport is disabled for ESP-IDF in this build. Use internal_loopback or extend IDF socket client transport.");
  return;
#else
 std::vector<uint8_t> data_send;
 Transaction_Identifier++;
      data_send.push_back(Transaction_Identifier >> 8);
      data_send.push_back(Transaction_Identifier >> 0);
      data_send.push_back(0x00);
      data_send.push_back(0x00);
      data_send.push_back(0x00);
      if (payload != nullptr) { 
        data_send.push_back(0x04 + payload_len);
      }else {
        data_send.push_back(0x06);      // wie viele Bytes kommen
      }
     data_send.push_back(address);
     data_send.push_back(function_code);
     data_send.push_back(start_address >> 8);
     data_send.push_back(start_address >> 0);
    // function nicht 5 oder nicht 6
     if (function_code != ModbusFunctionCode::WRITE_SINGLE_COIL &&
        function_code != ModbusFunctionCode::WRITE_SINGLE_REGISTER) {
      data_send.push_back(number_of_entities >> 8);
      data_send.push_back(number_of_entities >> 0);
    }
  

  if (payload != nullptr) {
    if (function_code == ModbusFunctionCode::WRITE_MULTIPLE_COILS ||
        function_code == ModbusFunctionCode::WRITE_MULTIPLE_REGISTERS) {  // Write multiple
      data_send.push_back(payload_len);                                        // Byte count is required for write
    } else {
      payload_len = 2;  // Write single register or coil
    }
    for (int i = 0; i < payload_len; i++) {
      data_send.push_back(payload[i]);
    }
  }



      /*     
    if (function_code != 0x5 && function_code != 0x6) {
     data_send.push_back(0x06);                          // byte 5
     data_send.push_back(address);
     data_send.push_back(function_code);
     data_send.push_back(start_address >> 8);
     data_send.push_back(start_address >> 0);
     data_send.push_back(number_of_entities >> 8);
     data_send.push_back(number_of_entities >> 0);
      }
    if (function_code == 0x5 && function_code == 0x6) {
     data_send.push_back(payload_len + 0x04);            // byte 5
     data_send.push_back(address);
     data_send.push_back(function_code);
     data_send.push_back(start_address >> 8);
     data_send.push_back(start_address >> 0);
     for (int i = 0; i < payload_len; i++) {
     data_send.push_back(payload[i]);
     }
        
    }
  if (payload != nullptr) {
     if (function_code == 0xF || function_code == 0x10) {  // Write multiple
       data_send.push_back(payload_len);  // Byte count is required for write
    } else {
      payload_len = 2;  // Write single register or coil
    }
    for (int i = 0; i < payload_len; i++) {
     data_send.push_back(payload[i]);
    }

  }
*/
   if (!client.connect(host_.c_str(), port_)) {
      ESP_LOGE(TAG, "Failed to connect to Modbus server %s:%d", host_.c_str(), port_);
      return;
    }
 std::string res1;
 char buf1[5];
 size_t len1 = 11; 
 for (size_t i = 12; i < data_send[5] + 6; i++) {
 sprintf(buf1, "%02X", data_send[i]);
res1 += buf1;
res1 += ":"; 
}
    client.write(reinterpret_cast<const char*>(data_send.data()), data_send.size());
    ESP_LOGD(TAG, ">>> %02X%02X %02X%02X %02X%02X %02X %02X %02X%02X %02X%02X %s",
                   data_send[0], data_send[1],  data_send[2], data_send[3], data_send[4], data_send[5],
                   data_send[6], data_send[7],  data_send[8], data_send[9], data_send[10], data_send[11], res1.c_str());
    

//    delay(200);
  last_send_ = millis();
#endif
}
// Helper function for lambdas
// Send raw command. Except CRC everything must be contained in payload


void ModbusTCP::send_raw(const std::vector<uint8_t> &payload) {
  if (payload.empty()) {
    return;
  }

 // this->write_array(payload);
 // this->write_byte(crc & 0xFF);
 // this->write_byte((crc >> 8) & 0xFF);
//  this->flush();
  waiting_for_response = payload[0];
//  ESP_LOGV(TAG, "Modbus write raw: %s", format_hex_pretty(payload).c_str());
  last_send_ = millis();
}

}  // namespace modbustcp
}  // namespace esphome
