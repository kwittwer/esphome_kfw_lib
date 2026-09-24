#pragma once

#include <functional>

#include "esphome/components/climate/climate.h"
#include "esphome/components/number/number.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace pid_thermostat {

enum NumberKind {
  NUMBER_KIND_KP,
  NUMBER_KIND_KI,
  NUMBER_KIND_KD,
  NUMBER_KIND_PWM_PERIOD,
  NUMBER_KIND_PWM_MIN,
  NUMBER_KIND_PWM_MAX,
  NUMBER_KIND_DEW_POINT_OFFSET,
};

class PidThermostat;

class PidThermostatNumber : public number::Number, public Component {
 public:
  PidThermostatNumber(PidThermostat *parent, NumberKind kind) : parent_(parent), kind_(kind) {}

  void setup() override;
  void control(float value) override;
  void dump_config() override;
  void publish_from_parent();

 protected:
  PidThermostat *parent_;
  NumberKind kind_;
};

class PidThermostatSensor : public sensor::Sensor, public Component {
 public:
  void setup() override { this->publish_state(NAN); }
  void dump_config() override;
};

class PidThermostatTextSensor : public text_sensor::TextSensor, public Component {
 public:
  void setup() override { this->publish_state("off"); }
  void dump_config() override;
};

class PidThermostat : public climate::Climate, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_sensor(sensor::Sensor *sensor) { this->sensor_ = sensor; }
  void set_humidity_sensor(sensor::Sensor *sensor) { this->humidity_sensor_ = sensor; }
  void set_fallback_sensor(sensor::Sensor *sensor) { this->fallback_sensor_ = sensor; }
  void set_fallback_humidity_sensor(sensor::Sensor *sensor) { this->fallback_humidity_sensor_ = sensor; }
  void set_valve_switch(switch_::Switch *sw) { this->valve_switch_ = sw; }
  void set_sensor_timeout(uint32_t timeout_ms) { this->sensor_timeout_ms_ = timeout_ms; }
  void set_fallback_sensor_timeout(uint32_t timeout_ms) { this->fallback_sensor_timeout_ms_ = timeout_ms; }
  void set_humidity_sensor_timeout(uint32_t timeout_ms) { this->humidity_sensor_timeout_ms_ = timeout_ms; }
  void set_fallback_humidity_sensor_timeout(uint32_t timeout_ms) { this->fallback_humidity_sensor_timeout_ms_ = timeout_ms; }
  void set_sampling_period(uint32_t period_ms) { this->sampling_period_ms_ = period_ms; }
  void set_keep_alive(uint32_t keep_alive_ms) { this->keep_alive_ms_ = keep_alive_ms; }
  void set_min_cycle_duration(uint32_t duration_ms) { this->min_cycle_duration_ms_ = duration_ms; }
  void set_min_off_cycle_duration(uint32_t duration_ms) { this->min_off_cycle_duration_ms_ = duration_ms; }
  void set_output_safety(float output_safety) { this->output_safety_ = output_safety; }
  void set_cold_tolerance(float cold_tolerance) { this->cold_tolerance_ = cold_tolerance; }
  void set_hot_tolerance(float hot_tolerance) { this->hot_tolerance_ = hot_tolerance; }
  void set_dew_point_offset(float dew_point_offset) { this->dew_point_offset_ = dew_point_offset; }
  void set_debug(bool debug) { this->debug_ = debug; }

  void set_valve_control_enabled(std::function<bool()> &&f) { this->valve_control_enabled_func_ = f; }

  void set_kp(float value);
  void set_ki(float value);
  void set_kd(float value);
  void set_pwm_period(uint32_t period_ms);
  void set_pwm_min(float value);
  void set_pwm_max(float value);
  void set_output_sensor(PidThermostatSensor *sensor) { this->output_sensor_ = sensor; }
  void set_mode_text_sensor(PidThermostatTextSensor *sensor) { this->mode_text_sensor_ = sensor; }
  void set_temperature_source_text_sensor(PidThermostatTextSensor *sensor) { this->temperature_source_text_sensor_ = sensor; }
  void set_humidity_source_text_sensor(PidThermostatTextSensor *sensor) { this->humidity_source_text_sensor_ = sensor; }
  void register_number_entity(PidThermostatNumber *number) { this->number_entities_.push_back(number); }

  float get_kp() const { return this->kp_; }
  float get_ki() const { return this->ki_; }
  float get_kd() const { return this->kd_; }
  float get_pwm_period_seconds() const { return this->pwm_period_ms_ / 1000.0f; }
  float get_pwm_min() const { return this->pwm_min_; }
  float get_pwm_max() const { return this->pwm_max_; }
  float get_dew_point_offset() const { return this->dew_point_offset_; }
  float get_control_output() const { return this->control_output_; }
  float get_pid_p() const { return this->pid_p_; }
  float get_pid_i() const { return this->pid_i_; }
  float get_pid_d() const { return this->pid_d_; }
  float get_pid_error() const { return this->last_error_; }
  float get_pid_dt() const { return this->last_dt_seconds_; }
  bool get_valve_state() const { return this->valve_state_; }
  bool is_using_fallback_temperature() const { return this->using_fallback_temperature_; }
  bool is_using_fallback_humidity() const { return this->using_fallback_humidity_; }

  void set_current_temperature(float value);
  void set_current_humidity(float value);

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

 protected:
  void update_temp_sensor_();
  void update_humidity_sensor_();
  void check_timeouts_();
  void calculate_control_(bool force = false);
  void update_action_();
  void update_valve_output_(bool force = false);
  bool get_valve_control_enabled_();
  bool should_sample_(uint32_t now) const;
  void request_recompute_() { this->pending_recompute_ = true; }
  void publish_child_states_();
  float calculate_dew_point_() const;

  sensor::Sensor *sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  sensor::Sensor *fallback_sensor_{nullptr};
  sensor::Sensor *fallback_humidity_sensor_{nullptr};
  switch_::Switch *valve_switch_{nullptr};
  PidThermostatSensor *output_sensor_{nullptr};
  PidThermostatTextSensor *mode_text_sensor_{nullptr};
  PidThermostatTextSensor *temperature_source_text_sensor_{nullptr};
  PidThermostatTextSensor *humidity_source_text_sensor_{nullptr};
  std::vector<PidThermostatNumber *> number_entities_{};

  optional<std::function<bool()>> valve_control_enabled_func_{};
  bool valve_control_enabled_value_{true};

  uint32_t sensor_timeout_ms_{600000};
  uint32_t fallback_sensor_timeout_ms_{600000};
  uint32_t humidity_sensor_timeout_ms_{0};
  uint32_t fallback_humidity_sensor_timeout_ms_{600000};
  uint32_t sampling_period_ms_{0};
  uint32_t keep_alive_ms_{60000};
  uint32_t min_cycle_duration_ms_{0};
  uint32_t min_off_cycle_duration_ms_{0};
  uint32_t pwm_period_ms_{900000};

  uint32_t last_primary_temp_update_ms_{0};
  uint32_t last_fallback_temp_update_ms_{0};
  uint32_t last_primary_humidity_update_ms_{0};
  uint32_t last_fallback_humidity_update_ms_{0};
  uint32_t last_temp_sensor_update_ms_{0};
  uint32_t last_humidity_sensor_update_ms_{0};
  uint32_t last_compute_ms_{0};
  uint32_t last_keep_alive_ms_{0};
  uint32_t pwm_cycle_start_ms_{0};
  uint32_t last_valve_state_change_ms_{0};

  float kp_{5.0f};
  float ki_{0.01f};
  float kd_{500.0f};
  float pwm_min_{0.0f};
  float pwm_max_{100.0f};
  float cold_tolerance_{0.3f};
  float hot_tolerance_{0.3f};
  float dew_point_offset_{1.0f};
  float output_safety_{5.0f};

  float control_output_{0.0f};
  float pid_p_{0.0f};
  float pid_i_{0.0f};
  float pid_d_{0.0f};
  float integral_{0.0f};
  float last_error_{0.0f};
  float last_dt_seconds_{0.0f};

  bool pending_recompute_{true};
  bool valve_state_{false};
  bool using_fallback_temperature_{false};
  bool using_fallback_humidity_{false};
  bool debug_{false};
};

}  // namespace pid_thermostat
}  // namespace esphome
