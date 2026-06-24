#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"  // FÜR CallbackManager

#ifdef USE_ESP_IDF
  #include <lwip/sockets.h>
  #include <lwip/netdb.h>
  #include <lwip/err.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <freertos/FreeRTOS.h>
  #include <freertos/task.h>  // FÜR vTaskDelay
#else
  #if defined(USE_ETHERNET)
    #include <Ethernet.h>
    using ModbusTCPServerClient = EthernetClient;
    using ModbusTCPServerSocket = EthernetServer;
  #else
    #ifdef USE_ESP32
      #include <WiFi.h>
    #elif defined(USE_ESP8266)
      #include <ESP8266WiFi.h>
    #endif
    using ModbusTCPServerClient = WiFiClient;
    using ModbusTCPServerSocket = WiFiServer;
  #endif
#endif

namespace esphome {
namespace modbus_tcp_server {

static const char *const TAG = "modbus_tcp_server";

class ModbusTCPServer : public Component {
 public:
  ModbusTCPServer() = default;
  ~ModbusTCPServer();

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  // Konfiguration
  void set_port(uint16_t port) { port_ = port; }
  void set_max_registers(uint16_t max) { max_registers_ = max; }
  void set_max_coils(uint16_t max) { max_coils_ = max; }
  void set_unit_id(uint8_t unit_id) { unit_id_ = unit_id; }
  uint8_t get_unit_id() const { return unit_id_; }

  // Public API für Register/Coils
  void set_holding_register(uint16_t address, uint16_t value);
  uint16_t get_holding_register(uint16_t address);
  void set_input_register(uint16_t address, uint16_t value);
  uint16_t get_input_register(uint16_t address);
  void set_coil(uint16_t address, bool value);
  bool get_coil(uint16_t address);
  void set_discrete_input(uint16_t address, bool value);
  bool get_discrete_input(uint16_t address);

  // Callback für Register-Änderungen
  void add_on_write_register_callback(std::function<void(uint16_t, uint16_t)> &&callback) {
    on_write_register_callbacks_.add(std::move(callback));
  }
  void add_on_write_coil_callback(std::function<void(uint16_t, bool)> &&callback) {
    on_write_coil_callbacks_.add(std::move(callback));
  }

 protected:
  uint16_t port_{502};
  uint16_t max_registers_{100};
  uint16_t max_coils_{100};
  uint8_t unit_id_{1};

#ifdef USE_ESP_IDF
  int server_socket_{-1};
  int client_socket_{-1};
#else
  ModbusTCPServerSocket *server_{nullptr};
  ModbusTCPServerClient client_;
#endif

  std::vector<uint16_t> holding_registers_;
  std::vector<uint16_t> input_registers_;
  std::vector<bool> coils_;
  std::vector<bool> discrete_inputs_;

  uint8_t request_buffer_[260];
  uint8_t response_buffer_[260];

  CallbackManager<void(uint16_t, uint16_t)> on_write_register_callbacks_;
  CallbackManager<void(uint16_t, bool)> on_write_coil_callbacks_;

  void process_request_();
  void handle_read_coils_(uint16_t transaction_id, uint8_t unit_id);
  void handle_read_discrete_inputs_(uint16_t transaction_id, uint8_t unit_id);
  void handle_read_holding_registers_(uint16_t transaction_id, uint8_t unit_id);
  void handle_read_input_registers_(uint16_t transaction_id, uint8_t unit_id);
  void handle_write_single_coil_(uint16_t transaction_id, uint8_t unit_id);
  void handle_write_single_register_(uint16_t transaction_id, uint8_t unit_id);
  void handle_write_multiple_coils_(uint16_t transaction_id, uint8_t unit_id);
  void handle_write_multiple_registers_(uint16_t transaction_id, uint8_t unit_id);
  void send_exception_(uint16_t transaction_id, uint8_t unit_id, uint8_t function_code, uint8_t exception_code);
  
#ifdef USE_ESP_IDF
  int read_bytes_(uint8_t *buffer, size_t length);
  int write_bytes_(const uint8_t *buffer, size_t length);
#endif
};

}  // namespace modbus_tcp_server
}  // namespace esphome