#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"

#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

#ifdef USE_OUTPUT
#include "esphome/components/output/binary_output.h"
#endif

#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif

namespace esphome {
namespace dummy_thermostat {

class DummyThermostat : public climate::Climate, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_fallback_sensor(sensor::Sensor *sensor) { this->fallback_sensor_ = sensor; }
  void set_sensor_timeout(uint32_t timeout) { this->temp_sensor_timeout_ = timeout; }
  
  void set_fallback_humidity_sensor(sensor::Sensor *sensor) { this->fallback_humidity_sensor_ = sensor; }
  void set_humidity_sensor_timeout(uint32_t timeout) { this->humidity_sensor_timeout_ = timeout; }
  void set_valve_update_timeout(uint32_t timeout) { this->valve_update_timeout_= timeout; }
  
#ifdef USE_OUTPUT
  void set_valve_output(output::BinaryOutput *output) { this->valve_output_ = output; }
#endif

#ifdef USE_SWITCH
  void set_valve_switch(switch_::Switch *sw) { this->valve_switch_ = sw; }
#endif
  
    
  // ENTFERNE DIESE ZEILEN (kommentiere sie aus):
  // void set_valve_control_enabled(bool value) {
  //   this->valve_control_enabled_value_ = value;
  // }

  void set_valve_control_enabled(std::function<bool()> &&f) {
    this->valve_control_enabled_func_ = f;
  }

  // ENTFERNE DIESE ZEILEN (kommentiere sie aus):
  // void set_use_local_valve_control(bool value) {
  //   this->use_local_valve_control_value_ = value;
  // }

  void set_use_local_valve_control(std::function<bool()> &&f) {
    this->use_local_valve_control_func_ = f;
  }
  void set_heating_deadband(float deadband) { this->heating_deadband_ = deadband; }
  void set_heating_overrun(float overrun) { this->heating_overrun_ = overrun; }
  void set_cooling_deadband(float deadband) { this->cooling_deadband_ = deadband; }
  void set_cooling_overrun(float overrun) { this->cooling_overrun_ = overrun; }

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  
  // Public method for manual MQTT publishing
  // void publish_mqtt_status_();
  
  // Public getter methods for use in lambdas
  bool is_valve_control_enabled() { return this->get_valve_control_enabled_(); }
  bool is_using_local_valve_control() { return this->get_use_local_valve_control_(); }
  bool get_valve_state() { return this->valve_state_; }
  bool is_using_fallback_temperature() { return this->using_fallback_temperature_; }
  bool is_using_fallback_humidity() { return this->using_fallback_humidity_; }
  bool is_using_local_controller() { return this->valve_timeout_active_; }
  bool is_valve_switch_timeout_active() { return this->valve_timeout_active_; }
  
  void set_valve_state(bool state);
  void set_current_temperature(float value);
  void set_current_humidity(float value); 

 protected:
  void update_temp_sensor_();
  void update_humidity_sensor_();
  void calculate_action_from_valve_and_mode_();
  void update_valve_output_();
  void calculate_local_valve_state_();
  
  bool get_valve_control_enabled_();
  bool get_use_local_valve_control_();
  bool get_valve_state_from_input_();
  
  sensor::Sensor *fallback_sensor_{nullptr};
  uint32_t temp_sensor_timeout_{0};
  uint32_t last_temp_sensor_update_{0};
  bool using_fallback_temperature_{true};
  
  sensor::Sensor *fallback_humidity_sensor_{nullptr};
  uint32_t humidity_sensor_timeout_{0};
  uint32_t last_humidity_sensor_update_{0};
  bool using_fallback_humidity_{true};
  uint32_t last_loop_update_{0};
  
  uint32_t valve_update_timeout_{0};
  uint32_t last_valve_state_update_{0};
  bool valve_timeout_active_{true};
  
  std::string status_topic_{""};
  uint32_t status_update_interval_{0};
  uint32_t last_status_publish_{0};
  
#ifdef USE_OUTPUT
  output::BinaryOutput *valve_output_{nullptr};
#endif

#ifdef USE_SWITCH
  switch_::Switch *valve_switch_{nullptr};
#endif
  
  // Support both static bool and lambda function
  bool valve_control_enabled_value_{false};
  optional<std::function<bool()>> valve_control_enabled_func_{};
  
  bool use_local_valve_control_value_{false};
  optional<std::function<bool()>> use_local_valve_control_func_{};
  
  float heating_deadband_{0.5f};
  float heating_overrun_{0.5f};
  float cooling_deadband_{0.5f};
  float cooling_overrun_{0.5f};
  
  bool valve_state_{false};
  bool last_valve_state_{false};
};

}  // namespace dummy_thermostat
}  // namespace esphome