#include "modbus_tcp_server.h"

namespace esphome {
namespace modbus_tcp_server {

ModbusTCPServer::~ModbusTCPServer() {
#ifdef USE_ESP_IDF
  if (client_socket_ >= 0) {
    close(client_socket_);
  }
  if (server_socket_ >= 0) {
    close(server_socket_);
  }
#endif
}

void ModbusTCPServer::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Modbus TCP Server...");
  
  // Vektoren initialisieren
  holding_registers_.resize(max_registers_, 0);
  input_registers_.resize(max_registers_, 0);
  coils_.resize(max_coils_, false);
  discrete_inputs_.resize(max_coils_, false);

#ifdef USE_ESP_IDF
  // ESP-IDF Socket erstellen
  server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_socket_ < 0) {
    ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
    this->mark_failed();
    return;
  }

  // Socket-Optionen setzen
  int opt = 1;
  setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  
  // Non-blocking setzen
  int flags = fcntl(server_socket_, F_GETFL, 0);
  fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK);

  // Bind
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port_);

  if (bind(server_socket_, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    ESP_LOGE(TAG, "Failed to bind socket: errno %d", errno);
    close(server_socket_);
    server_socket_ = -1;
    this->mark_failed();
    return;
  }

  // Listen
  if (listen(server_socket_, 1) < 0) {
    ESP_LOGE(TAG, "Failed to listen: errno %d", errno);
    close(server_socket_);
    server_socket_ = -1;
    this->mark_failed();
    return;
  }

  ESP_LOGCONFIG(TAG, "Modbus TCP Server started on port %d (ESP-IDF)", port_);

#else
  // Arduino Framework
  server_ = new WiFiServer(port_);
  server_->begin();
  server_->setNoDelay(true);
  ESP_LOGCONFIG(TAG, "Modbus TCP Server started on port %d (Arduino)", port_);
#endif
}

void ModbusTCPServer::dump_config() {
  ESP_LOGCONFIG(TAG, "Modbus TCP Server:");
  ESP_LOGCONFIG(TAG, "  Port: %d", port_);
  ESP_LOGCONFIG(TAG, "  Unit ID: %d", unit_id_);
  ESP_LOGCONFIG(TAG, "  Max Holding Registers: %d", max_registers_);
  ESP_LOGCONFIG(TAG, "  Max Coils: %d", max_coils_);
#ifdef USE_ESP_IDF
  ESP_LOGCONFIG(TAG, "  Framework: ESP-IDF");
#else
  ESP_LOGCONFIG(TAG, "  Framework: Arduino");
#endif
}

void ModbusTCPServer::loop() {
#ifdef USE_ESP_IDF
  // ESP-IDF Implementation
  
  // Auf neue Verbindungen prüfen
  if (client_socket_ < 0) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int new_socket = accept(server_socket_, (struct sockaddr *)&client_addr, &client_len);
    if (new_socket >= 0) {
      client_socket_ = new_socket;
      
      // Non-blocking setzen
      int flags = fcntl(client_socket_, F_GETFL, 0);
      fcntl(client_socket_, F_SETFL, flags | O_NONBLOCK);
      
      // TCP_NODELAY setzen
      int opt = 1;
      setsockopt(client_socket_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
      
      char ip_str[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
      ESP_LOGI(TAG, "New client connected: %s", ip_str);
    }
  }

  // Daten vom Client lesen
  if (client_socket_ >= 0) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_socket_, &read_fds);
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    int activity = select(client_socket_ + 1, &read_fds, NULL, NULL, &timeout);
    
    if (activity > 0 && FD_ISSET(client_socket_, &read_fds)) {
      // Daten verfügbar - prüfen ob genug für MBAP Header
      uint8_t peek_buffer[8];
      int peek_len = recv(client_socket_, peek_buffer, 8, MSG_PEEK);
      
      if (peek_len >= 8) {
        process_request_();
      } else if (peek_len == 0) {
        // Client hat Verbindung geschlossen
        ESP_LOGD(TAG, "Client disconnected");
        close(client_socket_);
        client_socket_ = -1;
      } else if (peek_len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        // Fehler
        ESP_LOGW(TAG, "Socket error: %d", errno);
        close(client_socket_);
        client_socket_ = -1;
      }
    }
  }

#else
  // Arduino Framework Implementation
  
  // Auf neue Clients warten
  if (server_->hasClient()) {
    if (!client_ || !client_.connected()) {
      if (client_) {
        ESP_LOGD(TAG, "Client disconnected");
        client_.stop();
      }
      client_ = server_->available();
      ESP_LOGI(TAG, "New client connected: %s", client_.remoteIP().toString().c_str());
    } else {
      WiFiClient temp_client = server_->available();
      ESP_LOGW(TAG, "Connection rejected - already connected");
      temp_client.stop();
    }
  }

  // Anfragen verarbeiten
  if (client_ && client_.connected()) {
    if (client_.available() >= 8) {
      process_request_();
    }
  }
#endif
}

#ifdef USE_ESP_IDF
int ModbusTCPServer::read_bytes_(uint8_t *buffer, size_t length) {
  size_t total_read = 0;
  
  // Timeout setzen (1 Sekunde)
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  setsockopt(client_socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  
  while (total_read < length) {
    int n = recv(client_socket_, buffer + total_read, length - total_read, 0);
    if (n > 0) {
      total_read += n;
    } else if (n == 0) {
      return -1;  // Verbindung geschlossen
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // Timeout
      ESP_LOGW(TAG, "Read timeout");
      return -1;
    } else {
      return -1;  // Fehler
    }
  }
  
  return total_read;
}

int ModbusTCPServer::write_bytes_(const uint8_t *buffer, size_t length) {
  size_t total_written = 0;
  
  // Timeout setzen (1 Sekunde)
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  setsockopt(client_socket_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  
  while (total_written < length) {
    int n = send(client_socket_, buffer + total_written, length - total_written, 0);
    if (n > 0) {
      total_written += n;
    } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      return -1;
    }
  }
  return total_written;
}
#endif

void ModbusTCPServer::process_request_() {
#ifdef USE_ESP_IDF
  // MBAP Header lesen (7 Bytes)
  uint8_t header[7];
  if (read_bytes_(header, 7) < 0) {
    ESP_LOGW(TAG, "Failed to read MBAP header");
    close(client_socket_);
    client_socket_ = -1;
    return;
  }

  uint16_t transaction_id = (header[0] << 8) | header[1];
  uint16_t protocol_id = (header[2] << 8) | header[3];
  uint16_t length = (header[4] << 8) | header[5];
  uint8_t unit_id = header[6];

  // Function Code lesen
  uint8_t function_code;
  if (read_bytes_(&function_code, 1) < 0) {
    ESP_LOGW(TAG, "Failed to read function code");
    close(client_socket_);
    client_socket_ = -1;
    return;
  }

#else
  // Arduino Framework
  uint16_t transaction_id = (client_.read() << 8) | client_.read();
  uint16_t protocol_id = (client_.read() << 8) | client_.read();
  uint16_t length = (client_.read() << 8) | client_.read();
  uint8_t unit_id = client_.read();
  uint8_t function_code = client_.read();
#endif

  // Protokoll-ID prüfen
  if (protocol_id != 0) {
    ESP_LOGW(TAG, "Invalid protocol ID: %d", protocol_id);
    return;
  }

  // Unit ID prüfen
  if (unit_id != 0 && unit_id != unit_id_) {
    ESP_LOGD(TAG, "Unit ID mismatch: got %d, expected %d", unit_id, unit_id_);
    return;
  }

  ESP_LOGD(TAG, "Request: TID=%d, Func=0x%02X, Unit=%d", transaction_id, function_code, unit_id);

  switch (function_code) {
    case 0x01:
      handle_read_coils_(transaction_id, unit_id);
      break;
    case 0x02:
      handle_read_discrete_inputs_(transaction_id, unit_id);
      break;
    case 0x03:
      handle_read_holding_registers_(transaction_id, unit_id);
      break;
    case 0x04:
      handle_read_input_registers_(transaction_id, unit_id);
      break;
    case 0x05:
      handle_write_single_coil_(transaction_id, unit_id);
      break;
    case 0x06:
      handle_write_single_register_(transaction_id, unit_id);
      break;
    case 0x0F:
      handle_write_multiple_coils_(transaction_id, unit_id);
      break;
    case 0x10:
      handle_write_multiple_registers_(transaction_id, unit_id);
      break;
    default:
      ESP_LOGW(TAG, "Unsupported function code: 0x%02X", function_code);
      send_exception_(transaction_id, unit_id, function_code, 0x01);
      break;
  }
}

void ModbusTCPServer::handle_read_holding_registers_(uint16_t transaction_id, uint8_t unit_id) {
#ifdef USE_ESP_IDF
  uint8_t data[4];
  if (read_bytes_(data, 4) < 0) return;
  uint16_t start_address = (data[0] << 8) | data[1];
  uint16_t quantity = (data[2] << 8) | data[3];
#else
  uint16_t start_address = (client_.read() << 8) | client_.read();
  uint16_t quantity = (client_.read() << 8) | client_.read();
#endif

  ESP_LOGD(TAG, "Read Holding Registers: addr=%d, qty=%d", start_address, quantity);

  if (quantity == 0 || quantity > 125) {
    send_exception_(transaction_id, unit_id, 0x03, 0x03);
    return;
  }

  if (start_address + quantity > max_registers_) {
    send_exception_(transaction_id, unit_id, 0x03, 0x02);
    return;
  }

  uint8_t byte_count = quantity * 2;
  int pos = 0;

  response_buffer_[pos++] = transaction_id >> 8;
  response_buffer_[pos++] = transaction_id & 0xFF;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = byte_count + 3;
  response_buffer_[pos++] = unit_id;
  response_buffer_[pos++] = 0x03;
  response_buffer_[pos++] = byte_count;

  for (uint16_t i = 0; i < quantity; i++) {
    uint16_t value = holding_registers_[start_address + i];
    response_buffer_[pos++] = value >> 8;
    response_buffer_[pos++] = value & 0xFF;
  }

#ifdef USE_ESP_IDF
  write_bytes_(response_buffer_, pos);
#else
  client_.write(response_buffer_, pos);
#endif
  
  ESP_LOGD(TAG, "Sent %d bytes", pos);
}

void ModbusTCPServer::handle_read_input_registers_(uint16_t transaction_id, uint8_t unit_id) {
#ifdef USE_ESP_IDF
  uint8_t data[4];
  if (read_bytes_(data, 4) < 0) return;
  uint16_t start_address = (data[0] << 8) | data[1];
  uint16_t quantity = (data[2] << 8) | data[3];
#else
  uint16_t start_address = (client_.read() << 8) | client_.read();
  uint16_t quantity = (client_.read() << 8) | client_.read();
#endif

  ESP_LOGD(TAG, "Read Input Registers: addr=%d, qty=%d", start_address, quantity);

  if (quantity == 0 || quantity > 125) {
    send_exception_(transaction_id, unit_id, 0x04, 0x03);
    return;
  }

  if (start_address + quantity > max_registers_) {
    send_exception_(transaction_id, unit_id, 0x04, 0x02);
    return;
  }

  uint8_t byte_count = quantity * 2;
  int pos = 0;

  response_buffer_[pos++] = transaction_id >> 8;
  response_buffer_[pos++] = transaction_id & 0xFF;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = byte_count + 3;
  response_buffer_[pos++] = unit_id;
  response_buffer_[pos++] = 0x04;
  response_buffer_[pos++] = byte_count;

  for (uint16_t i = 0; i < quantity; i++) {
    uint16_t value = input_registers_[start_address + i];
    response_buffer_[pos++] = value >> 8;
    response_buffer_[pos++] = value & 0xFF;
  }

#ifdef USE_ESP_IDF
  write_bytes_(response_buffer_, pos);
#else
  client_.write(response_buffer_, pos);
#endif
}

void ModbusTCPServer::handle_write_single_register_(uint16_t transaction_id, uint8_t unit_id) {
#ifdef USE_ESP_IDF
  uint8_t data[4];
  if (read_bytes_(data, 4) < 0) return;
  uint16_t address = (data[0] << 8) | data[1];
  uint16_t value = (data[2] << 8) | data[3];
#else
  uint16_t address = (client_.read() << 8) | client_.read();
  uint16_t value = (client_.read() << 8) | client_.read();
#endif

  ESP_LOGD(TAG, "Write Single Register: addr=%d, value=%d", address, value);

  if (address >= max_registers_) {
    send_exception_(transaction_id, unit_id, 0x06, 0x02);
    return;
  }

  holding_registers_[address] = value;
  on_write_register_callbacks_.call(address, value);  // CALLBACK

  int pos = 0;
  response_buffer_[pos++] = transaction_id >> 8;
  response_buffer_[pos++] = transaction_id & 0xFF;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 6;
  response_buffer_[pos++] = unit_id;
  response_buffer_[pos++] = 0x06;
  response_buffer_[pos++] = address >> 8;
  response_buffer_[pos++] = address & 0xFF;
  response_buffer_[pos++] = value >> 8;
  response_buffer_[pos++] = value & 0xFF;

#ifdef USE_ESP_IDF
  write_bytes_(response_buffer_, pos);
#else
  client_.write(response_buffer_, pos);
#endif
}

void ModbusTCPServer::handle_read_coils_(uint16_t transaction_id, uint8_t unit_id) {
#ifdef USE_ESP_IDF
  uint8_t data[4];
  if (read_bytes_(data, 4) < 0) return;
  uint16_t start_address = (data[0] << 8) | data[1];
  uint16_t quantity = (data[2] << 8) | data[3];
#else
  uint16_t start_address = (client_.read() << 8) | client_.read();
  uint16_t quantity = (client_.read() << 8) | client_.read();
#endif

  ESP_LOGD(TAG, "Read Coils: addr=%d, qty=%d", start_address, quantity);

  if (quantity == 0 || quantity > 2000) {
    send_exception_(transaction_id, unit_id, 0x01, 0x03);
    return;
  }

  if (start_address + quantity > max_coils_) {
    send_exception_(transaction_id, unit_id, 0x01, 0x02);
    return;
  }

  uint8_t byte_count = (quantity + 7) / 8;
  int pos = 0;

  response_buffer_[pos++] = transaction_id >> 8;
  response_buffer_[pos++] = transaction_id & 0xFF;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = byte_count + 3;
  response_buffer_[pos++] = unit_id;
  response_buffer_[pos++] = 0x01;
  response_buffer_[pos++] = byte_count;

  for (uint8_t i = 0; i < byte_count; i++) {
    uint8_t byte_value = 0;
    for (uint8_t bit = 0; bit < 8 && (i * 8 + bit) < quantity; bit++) {
      if (coils_[start_address + i * 8 + bit]) {
        byte_value |= (1 << bit);
      }
    }
    response_buffer_[pos++] = byte_value;
  }

#ifdef USE_ESP_IDF
  write_bytes_(response_buffer_, pos);
#else
  client_.write(response_buffer_, pos);
#endif
}

void ModbusTCPServer::handle_read_discrete_inputs_(uint16_t transaction_id, uint8_t unit_id) {
#ifdef USE_ESP_IDF
  uint8_t data[4];
  if (read_bytes_(data, 4) < 0) return;
  uint16_t start_address = (data[0] << 8) | data[1];
  uint16_t quantity = (data[2] << 8) | data[3];
#else
  uint16_t start_address = (client_.read() << 8) | client_.read();
  uint16_t quantity = (client_.read() << 8) | client_.read();
#endif

  ESP_LOGD(TAG, "Read Discrete Inputs: addr=%d, qty=%d", start_address, quantity);

  if (quantity == 0 || quantity > 2000) {
    send_exception_(transaction_id, unit_id, 0x02, 0x03);
    return;
  }

  if (start_address + quantity > max_coils_) {
    send_exception_(transaction_id, unit_id, 0x02, 0x02);
    return;
  }

  uint8_t byte_count = (quantity + 7) / 8;
  int pos = 0;

  response_buffer_[pos++] = transaction_id >> 8;
  response_buffer_[pos++] = transaction_id & 0xFF;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = byte_count + 3;
  response_buffer_[pos++] = unit_id;
  response_buffer_[pos++] = 0x02;
  response_buffer_[pos++] = byte_count;

  for (uint8_t i = 0; i < byte_count; i++) {
    uint8_t byte_value = 0;
    for (uint8_t bit = 0; bit < 8 && (i * 8 + bit) < quantity; bit++) {
      if (discrete_inputs_[start_address + i * 8 + bit]) {
        byte_value |= (1 << bit);
      }
    }
    response_buffer_[pos++] = byte_value;
  }

#ifdef USE_ESP_IDF
  write_bytes_(response_buffer_, pos);
#else
  client_.write(response_buffer_, pos);
#endif
}

void ModbusTCPServer::handle_write_single_coil_(uint16_t transaction_id, uint8_t unit_id) {
#ifdef USE_ESP_IDF
  uint8_t data[4];
  if (read_bytes_(data, 4) < 0) return;
  uint16_t address = (data[0] << 8) | data[1];
  uint16_t value = (data[2] << 8) | data[3];
#else
  uint16_t address = (client_.read() << 8) | client_.read();
  uint16_t value = (client_.read() << 8) | client_.read();
#endif

  ESP_LOGD(TAG, "Write Single Coil: addr=%d, value=0x%04X", address, value);

  if (address >= max_coils_) {
    send_exception_(transaction_id, unit_id, 0x05, 0x02);
    return;
  }

  bool coil_value = (value == 0xFF00);
  coils_[address] = coil_value;
  on_write_coil_callbacks_.call(address, coil_value);  // CALLBACK

  int pos = 0;
  response_buffer_[pos++] = transaction_id >> 8;
  response_buffer_[pos++] = transaction_id & 0xFF;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 6;
  response_buffer_[pos++] = unit_id;
  response_buffer_[pos++] = 0x05;
  response_buffer_[pos++] = address >> 8;
  response_buffer_[pos++] = address & 0xFF;
  response_buffer_[pos++] = value >> 8;
  response_buffer_[pos++] = value & 0xFF;

#ifdef USE_ESP_IDF
  write_bytes_(response_buffer_, pos);
#else
  client_.write(response_buffer_, pos);
#endif
}

void ModbusTCPServer::handle_write_multiple_coils_(uint16_t transaction_id, uint8_t unit_id) {
#ifdef USE_ESP_IDF
  uint8_t header[5];
  if (read_bytes_(header, 5) < 0) return;
  uint16_t start_address = (header[0] << 8) | header[1];
  uint16_t quantity = (header[2] << 8) | header[3];
  uint8_t byte_count = header[4];
  
  uint8_t *coil_data = new uint8_t[byte_count];
  if (read_bytes_(coil_data, byte_count) < 0) {
    delete[] coil_data;
    return;
  }
#else
  uint16_t start_address = (client_.read() << 8) | client_.read();
  uint16_t quantity = (client_.read() << 8) | client_.read();
  uint8_t byte_count = client_.read();
#endif

  ESP_LOGD(TAG, "Write Multiple Coils: addr=%d, qty=%d", start_address, quantity);

  if (start_address + quantity > max_coils_) {
    send_exception_(transaction_id, unit_id, 0x0F, 0x02);
#ifdef USE_ESP_IDF
    delete[] coil_data;
#else
    for (uint8_t i = 0; i < byte_count; i++) client_.read();
#endif
    return;
  }

#ifdef USE_ESP_IDF
  for (uint16_t i = 0; i < quantity; i++) {
    uint8_t byte_index = i / 8;
    uint8_t bit_index = i % 8;
    bool coil_value = (coil_data[byte_index] & (1 << bit_index)) != 0;
    coils_[start_address + i] = coil_value;
    on_write_coil_callbacks_.call(start_address + i, coil_value);  // CALLBACK
  }
  delete[] coil_data;
#else
  for (uint16_t i = 0; i < quantity; i++) {
    if (i % 8 == 0) {
      uint8_t byte_value = client_.read();
      for (uint8_t bit = 0; bit < 8 && i + bit < quantity; bit++) {
        bool coil_value = (byte_value & (1 << bit)) != 0;
        coils_[start_address + i + bit] = coil_value;
        on_write_coil_callbacks_.call(start_address + i + bit, coil_value);  // CALLBACK
      }
      i += 7;  // Loop increment wird noch 1 addieren
    }
  }
#endif

  int pos = 0;
  response_buffer_[pos++] = transaction_id >> 8;
  response_buffer_[pos++] = transaction_id & 0xFF;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 6;
  response_buffer_[pos++] = unit_id;
  response_buffer_[pos++] = 0x0F;
  response_buffer_[pos++] = start_address >> 8;
  response_buffer_[pos++] = start_address & 0xFF;
  response_buffer_[pos++] = quantity >> 8;
  response_buffer_[pos++] = quantity & 0xFF;

#ifdef USE_ESP_IDF
  write_bytes_(response_buffer_, pos);
#else
  client_.write(response_buffer_, pos);
#endif
}

void ModbusTCPServer::handle_write_multiple_registers_(uint16_t transaction_id, uint8_t unit_id) {
#ifdef USE_ESP_IDF
  uint8_t header[5];
  if (read_bytes_(header, 5) < 0) return;
  uint16_t start_address = (header[0] << 8) | header[1];
  uint16_t quantity = (header[2] << 8) | header[3];
  uint8_t byte_count = header[4];
  
  uint8_t *reg_data = new uint8_t[byte_count];
  if (read_bytes_(reg_data, byte_count) < 0) {
    delete[] reg_data;
    return;
  }
#else
  uint16_t start_address = (client_.read() << 8) | client_.read();
  uint16_t quantity = (client_.read() << 8) | client_.read();
  uint8_t byte_count = client_.read();
#endif

  ESP_LOGD(TAG, "Write Multiple Registers: addr=%d, qty=%d", start_address, quantity);

  if (start_address + quantity > max_registers_) {
    send_exception_(transaction_id, unit_id, 0x10, 0x02);
#ifdef USE_ESP_IDF
    delete[] reg_data;
#else
    for (uint8_t i = 0; i < byte_count; i++) client_.read();
#endif
    return;
  }

  for (uint16_t i = 0; i < quantity; i++) {
#ifdef USE_ESP_IDF
    uint16_t value = (reg_data[i*2] << 8) | reg_data[i*2 + 1];
#else
    uint16_t value = (client_.read() << 8) | client_.read();
#endif
    holding_registers_[start_address + i] = value;
    on_write_register_callbacks_.call(start_address + i, value);  // CALLBACK
  }

#ifdef USE_ESP_IDF
  delete[] reg_data;
#endif

  int pos = 0;
  response_buffer_[pos++] = transaction_id >> 8;
  response_buffer_[pos++] = transaction_id & 0xFF;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 6;
  response_buffer_[pos++] = unit_id;
  response_buffer_[pos++] = 0x10;
  response_buffer_[pos++] = start_address >> 8;
  response_buffer_[pos++] = start_address & 0xFF;
  response_buffer_[pos++] = quantity >> 8;
  response_buffer_[pos++] = quantity & 0xFF;

#ifdef USE_ESP_IDF
  write_bytes_(response_buffer_, pos);
#else
  client_.write(response_buffer_, pos);
#endif
}

void ModbusTCPServer::send_exception_(uint16_t transaction_id, uint8_t unit_id, uint8_t function_code, uint8_t exception_code) {
  ESP_LOGW(TAG, "Sending exception: func=0x%02X, code=0x%02X", function_code, exception_code);
  
  int pos = 0;
  response_buffer_[pos++] = transaction_id >> 8;
  response_buffer_[pos++] = transaction_id & 0xFF;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 0;
  response_buffer_[pos++] = 3;
  response_buffer_[pos++] = unit_id;
  response_buffer_[pos++] = function_code | 0x80;
  response_buffer_[pos++] = exception_code;

#ifdef USE_ESP_IDF
  write_bytes_(response_buffer_, pos);
#else
  client_.write(response_buffer_, pos);
#endif
}

// Public API Implementation
void ModbusTCPServer::set_holding_register(uint16_t address, uint16_t value) {
  if (address < max_registers_) {
    holding_registers_[address] = value;
  }
}

uint16_t ModbusTCPServer::get_holding_register(uint16_t address) {
  if (address < max_registers_) {
    return holding_registers_[address];
  }
  return 0;
}

void ModbusTCPServer::set_input_register(uint16_t address, uint16_t value) {
  if (address < max_registers_) {
    input_registers_[address] = value;
  }
}

uint16_t ModbusTCPServer::get_input_register(uint16_t address) {
  if (address < max_registers_) {
    return input_registers_[address];
  }
  return 0;
}

void ModbusTCPServer::set_coil(uint16_t address, bool value) {
  if (address < max_coils_) {
    coils_[address] = value;
  }
}

bool ModbusTCPServer::get_coil(uint16_t address) {
  if (address < max_coils_) {
    return coils_[address];
  }
  return false;
}

void ModbusTCPServer::set_discrete_input(uint16_t address, bool value) {
  if (address < max_coils_) {
    discrete_inputs_[address] = value;
  }
}

bool ModbusTCPServer::get_discrete_input(uint16_t address) {
  if (address < max_coils_) {
    return discrete_inputs_[address];
  }
  return false;
}

}  // namespace modbus_tcp_server
}  // namespace esphome