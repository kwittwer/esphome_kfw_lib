#include "pid_thermostat.h"

#include <algorithm>
#include <cmath>

#include "esphome/core/log.h"

namespace esphome {
namespace pid_thermostat {

static const char *const TAG = "pid_thermostat";

void PidThermostatNumber::init_and_restore() {
  if (this->pref_initialized_) {
    return;
  }
  this->pref_initialized_ = true;
  this->pref_ = this->make_entity_preference<float>();

  float restored_value = NAN;
  if (this->pref_.load(&restored_value) && !std::isnan(restored_value)) {
    const bool migrate_legacy_cold_tolerance =
        this->kind_ == NUMBER_KIND_COLD_TOLERANCE && restored_value == 0.3f && this->parent_->get_cold_tolerance() == 5.0f;
    const bool migrate_legacy_hot_tolerance =
        this->kind_ == NUMBER_KIND_HOT_TOLERANCE && restored_value == 0.3f && this->parent_->get_hot_tolerance() == 5.0f;

    if (migrate_legacy_cold_tolerance || migrate_legacy_hot_tolerance) {
      const float migrated_value = this->kind_ == NUMBER_KIND_COLD_TOLERANCE ? this->parent_->get_cold_tolerance()
                                                                              : this->parent_->get_hot_tolerance();
      this->pref_.save(&migrated_value);
      restored_value = migrated_value;
      ESP_LOGI(TAG, "Migrated legacy '%s' default from 0.3 to %.1f", this->get_name().c_str(), migrated_value);
    }

    switch (this->kind_) {
      case NUMBER_KIND_KP:
        this->parent_->set_kp(restored_value);
        break;
      case NUMBER_KIND_KI:
        this->parent_->set_ki(restored_value);
        break;
      case NUMBER_KIND_KD:
        this->parent_->set_kd(restored_value);
        break;
      case NUMBER_KIND_I_MIN:
        this->parent_->set_i_min(restored_value);
        break;
      case NUMBER_KIND_I_MAX:
        this->parent_->set_i_max(restored_value);
        break;
      case NUMBER_KIND_COLD_TOLERANCE:
        this->parent_->set_cold_tolerance(restored_value);
        break;
      case NUMBER_KIND_HOT_TOLERANCE:
        this->parent_->set_hot_tolerance(restored_value);
        break;
      case NUMBER_KIND_SAMPLING_PERIOD:
        this->parent_->set_sampling_period(static_cast<uint32_t>(restored_value * 1000.0f));
        break;
      case NUMBER_KIND_KEEP_ALIVE:
        this->parent_->set_keep_alive(static_cast<uint32_t>(restored_value * 1000.0f));
        break;
      case NUMBER_KIND_MIN_CYCLE_DURATION:
        this->parent_->set_min_cycle_duration(static_cast<uint32_t>(restored_value * 1000.0f));
        break;
      case NUMBER_KIND_MIN_OFF_CYCLE_DURATION:
        this->parent_->set_min_off_cycle_duration(static_cast<uint32_t>(restored_value * 1000.0f));
        break;
      case NUMBER_KIND_PWM_PERIOD:
        this->parent_->set_pwm_period(static_cast<uint32_t>(restored_value * 1000.0f));
        break;
      case NUMBER_KIND_PWM_MIN:
        this->parent_->set_pwm_min(restored_value);
        break;
      case NUMBER_KIND_PWM_MAX:
        this->parent_->set_pwm_max(restored_value);
        break;
      case NUMBER_KIND_DEW_POINT_OFFSET:
        this->parent_->set_dew_point_offset(restored_value);
        break;
      case NUMBER_KIND_OUTPUT_SAFETY:
        this->parent_->set_output_safety(restored_value);
        break;
      case NUMBER_KIND_TEST_OUTPUT:
        this->parent_->set_commissioning_output(restored_value);
        break;
    }
    ESP_LOGD(TAG, "Restored '%s' = %.3f", this->get_name().c_str(), restored_value);
  }

  this->publish_from_parent(true);
}

void PidThermostatNumber::setup() { this->init_and_restore(); }

void PidThermostatNumber::save_value(float value) {
  if (this->pref_initialized_) {
    this->pref_.save(&value);
  }
  this->publish_state(value);
}

void PidThermostatNumber::control(float value) {
  switch (this->kind_) {
    case NUMBER_KIND_KP:
      this->parent_->set_kp(value);
      break;
    case NUMBER_KIND_KI:
      this->parent_->set_ki(value);
      break;
    case NUMBER_KIND_KD:
      this->parent_->set_kd(value);
      break;
    case NUMBER_KIND_I_MIN:
      this->parent_->set_i_min(value);
      break;
    case NUMBER_KIND_I_MAX:
      this->parent_->set_i_max(value);
      break;
    case NUMBER_KIND_COLD_TOLERANCE:
      this->parent_->set_cold_tolerance(value);
      break;
    case NUMBER_KIND_HOT_TOLERANCE:
      this->parent_->set_hot_tolerance(value);
      break;
    case NUMBER_KIND_SAMPLING_PERIOD:
      this->parent_->set_sampling_period(static_cast<uint32_t>(value * 1000.0f));
      break;
    case NUMBER_KIND_KEEP_ALIVE:
      this->parent_->set_keep_alive(static_cast<uint32_t>(value * 1000.0f));
      break;
    case NUMBER_KIND_MIN_CYCLE_DURATION:
      this->parent_->set_min_cycle_duration(static_cast<uint32_t>(value * 1000.0f));
      break;
    case NUMBER_KIND_MIN_OFF_CYCLE_DURATION:
      this->parent_->set_min_off_cycle_duration(static_cast<uint32_t>(value * 1000.0f));
      break;
    case NUMBER_KIND_PWM_PERIOD:
      this->parent_->set_pwm_period(static_cast<uint32_t>(value * 1000.0f));
      break;
    case NUMBER_KIND_PWM_MIN:
      this->parent_->set_pwm_min(value);
      break;
    case NUMBER_KIND_PWM_MAX:
      this->parent_->set_pwm_max(value);
      break;
    case NUMBER_KIND_DEW_POINT_OFFSET:
      this->parent_->set_dew_point_offset(value);
      break;
    case NUMBER_KIND_OUTPUT_SAFETY:
      this->parent_->set_output_safety(value);
      break;
    case NUMBER_KIND_TEST_OUTPUT:
      this->parent_->set_commissioning_output(value);
      break;
  }
  // The parent setters persist the (possibly clamped) value via
  // PidThermostat::save_number_value_(), so no extra save is needed here.
  this->publish_from_parent(true);
}

void PidThermostatNumber::dump_config() {
  LOG_NUMBER("", "PID Thermostat Number", this);
}

void PidThermostatNumber::publish_from_parent(bool force) {
  float value = NAN;
  switch (this->kind_) {
    case NUMBER_KIND_KP:
      value = this->parent_->get_kp();
      break;
    case NUMBER_KIND_KI:
      value = this->parent_->get_ki();
      break;
    case NUMBER_KIND_KD:
      value = this->parent_->get_kd();
      break;
    case NUMBER_KIND_I_MIN:
      value = this->parent_->get_i_min();
      break;
    case NUMBER_KIND_I_MAX:
      value = this->parent_->get_i_max();
      break;
    case NUMBER_KIND_COLD_TOLERANCE:
      value = this->parent_->get_cold_tolerance();
      break;
    case NUMBER_KIND_HOT_TOLERANCE:
      value = this->parent_->get_hot_tolerance();
      break;
    case NUMBER_KIND_SAMPLING_PERIOD:
      value = this->parent_->get_sampling_period_seconds();
      break;
    case NUMBER_KIND_KEEP_ALIVE:
      value = this->parent_->get_keep_alive_seconds();
      break;
    case NUMBER_KIND_MIN_CYCLE_DURATION:
      value = this->parent_->get_min_cycle_duration_seconds();
      break;
    case NUMBER_KIND_MIN_OFF_CYCLE_DURATION:
      value = this->parent_->get_min_off_cycle_duration_seconds();
      break;
    case NUMBER_KIND_PWM_PERIOD:
      value = this->parent_->get_pwm_period_seconds();
      break;
    case NUMBER_KIND_PWM_MIN:
      value = this->parent_->get_pwm_min();
      break;
    case NUMBER_KIND_PWM_MAX:
      value = this->parent_->get_pwm_max();
      break;
    case NUMBER_KIND_DEW_POINT_OFFSET:
      value = this->parent_->get_dew_point_offset();
      break;
    case NUMBER_KIND_OUTPUT_SAFETY:
      value = this->parent_->get_output_safety();
      break;
    case NUMBER_KIND_TEST_OUTPUT:
      value = this->parent_->get_commissioning_output();
      break;
  }

  if (!force &&
      ((std::isnan(this->state) && std::isnan(value)) || (!std::isnan(this->state) && !std::isnan(value) && this->state == value))) {
    return;
  }
  this->publish_state(value);
}

void PidThermostatSensor::dump_config() { LOG_SENSOR("", "PID Thermostat Output Sensor", this); }

void PidThermostatTextSensor::dump_config() { LOG_TEXT_SENSOR("", "PID Thermostat Mode Sensor", this); }

void PidThermostatSwitch::write_state(bool state) {
  this->parent_->set_commissioning_mode(state);
  this->publish_state(state);
}

void PidThermostatSwitch::dump_config() { LOG_SWITCH("", "PID Thermostat Commissioning Switch", this); }

void PidThermostatBinarySensor::dump_config() { LOG_BINARY_SENSOR("", "PID Thermostat Valve Output Sensor", this); }

void PidThermostatButton::press_action() {
  if (this->parent_ != nullptr) {
    this->parent_->reset_controller();
  }
}

void PidThermostat::setup() {
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->target_temperature = 21.0f;
  }

  // Must run before the first control calculation so the stored tuning values
  // replace the defaults that were applied from the YAML config at boot.
  this->restore_number_values_();
  this->restore_integral_state_();

  this->current_temperature = NAN;
  this->current_humidity = NAN;
  this->action = climate::CLIMATE_ACTION_OFF;

  const uint32_t now = millis();
  this->valve_control_enabled_state_ = this->get_valve_control_enabled_();
  this->last_keep_alive_ms_ = now;
  this->last_valve_state_change_ms_ = now;
  this->pwm_cycle_start_ms_ = now;

  if (this->sensor_ != nullptr && !std::isnan(this->sensor_->state))
    this->last_primary_temp_update_ms_ = now;
  if (this->fallback_sensor_ != nullptr && !std::isnan(this->fallback_sensor_->state))
    this->last_fallback_temp_update_ms_ = now;
  if (this->humidity_sensor_ != nullptr && !std::isnan(this->humidity_sensor_->state))
    this->last_primary_humidity_update_ms_ = now;
  if (this->fallback_humidity_sensor_ != nullptr && !std::isnan(this->fallback_humidity_sensor_->state))
    this->last_fallback_humidity_update_ms_ = now;

  if (this->sensor_ != nullptr) {
    this->sensor_->add_on_state_callback([this](float) { this->update_temp_sensor_(); });
  }
  if (this->fallback_sensor_ != nullptr) {
    this->fallback_sensor_->add_on_state_callback([this](float) { this->update_temp_sensor_(); });
  }
  if (this->humidity_sensor_ != nullptr) {
    this->humidity_sensor_->add_on_state_callback([this](float) { this->update_humidity_sensor_(); });
  }
  if (this->fallback_humidity_sensor_ != nullptr) {
    this->fallback_humidity_sensor_->add_on_state_callback([this](float) { this->update_humidity_sensor_(); });
  }

  this->update_temp_sensor_();
  this->update_humidity_sensor_();
  this->calculate_control_(true);
  this->update_action_();
  this->update_valve_output_(true);
    this->publish_valve_output_state_();
  this->publish_child_states_();
  this->publish_state();
}

void PidThermostat::loop() {
  const uint32_t now = millis();
  this->check_timeouts_();

  bool keep_alive_due = false;
  if (this->keep_alive_ms_ > 0 && now - this->last_keep_alive_ms_ >= this->keep_alive_ms_) {
    keep_alive_due = true;
    this->last_keep_alive_ms_ = now;
  }

  const bool pwm_period_due = this->pwm_period_ms_ > 0 && now - this->pwm_cycle_start_ms_ >= this->pwm_period_ms_;
  if (pwm_period_due) {
    this->pwm_cycle_start_ms_ = now;
    this->request_recompute_();
    this->calculate_control_(true);
    this->update_action_();
    this->publish_child_states_();
  } else if (this->should_sample_(now)) {
    // Only an explicitly configured sampling period adds calculation steps
    // between two PWM cycles; sensor updates alone do not trigger a recompute.
    this->calculate_control_();
    this->update_action_();
    this->publish_child_states_();
  }

  this->update_valve_output_(keep_alive_due);
}

void PidThermostat::publish_valve_output_state_() {
  if (this->valve_output_sensor_ == nullptr) {
    return;
  }

  const bool valve_output = this->get_effective_valve_state_();
  if (this->valve_output_sensor_->state != valve_output || !this->valve_output_sensor_->has_state()) {
    this->valve_output_sensor_->publish_state(valve_output);
  }
}

bool PidThermostat::get_effective_valve_state_() const {
  if (this->valve_switch_ != nullptr) {
    return this->valve_switch_->state;
  }
  return this->valve_state_;
}

void PidThermostat::dump_config() {
  ESP_LOGCONFIG(TAG, "PID Thermostat:");
  ESP_LOGCONFIG(TAG, "  Sensor: %s", this->sensor_ ? this->sensor_->get_name().c_str() : "None");
  ESP_LOGCONFIG(TAG, "  Fallback Sensor: %s", this->fallback_sensor_ ? this->fallback_sensor_->get_name().c_str() : "None");
  ESP_LOGCONFIG(TAG, "  Humidity Sensor: %s", this->humidity_sensor_ ? this->humidity_sensor_->get_name().c_str() : "None");
  ESP_LOGCONFIG(TAG, "  Fallback Humidity Sensor: %s",
                this->fallback_humidity_sensor_ ? this->fallback_humidity_sensor_->get_name().c_str() : "None");
  ESP_LOGCONFIG(TAG, "  Valve Switch: %s", this->valve_switch_ ? this->valve_switch_->get_name().c_str() : "None");
  ESP_LOGCONFIG(TAG, "  Kp: %.3f", this->kp_);
  ESP_LOGCONFIG(TAG, "  Ki: %.3f", this->ki_);
  ESP_LOGCONFIG(TAG, "  Kd: %.3f", this->kd_);
  ESP_LOGCONFIG(TAG, "  PWM Period: %.1fs", this->get_pwm_period_seconds());
  ESP_LOGCONFIG(TAG, "  PWM Min/Max: %.1f%% / %.1f%%", this->pwm_min_, this->pwm_max_);
  ESP_LOGCONFIG(TAG, "  Dew Point Offset: %.2fK", this->dew_point_offset_);
  ESP_LOGCONFIG(TAG, "  Sampling Period: %.1fs", this->sampling_period_ms_ / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Sensor Timeout: %.1fs", this->sensor_timeout_ms_ / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Fallback Sensor Timeout: %.1fs", this->fallback_sensor_timeout_ms_ / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Humidity Sensor Timeout: %.1fs", this->humidity_sensor_timeout_ms_ / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Fallback Humidity Timeout: %.1fs", this->fallback_humidity_sensor_timeout_ms_ / 1000.0f);
}

climate::ClimateTraits PidThermostat::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_HUMIDITY);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_COOL,
  });
  traits.set_visual_min_temperature(10.0f);
  traits.set_visual_max_temperature(35.0f);
  traits.set_visual_temperature_step(0.5f);
  return traits;
}

void PidThermostat::control(const climate::ClimateCall &call) {
  const bool mode_changed = call.get_mode().has_value() && *call.get_mode() != this->mode;

  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
  }
  if (call.get_mode().has_value()) {
    this->mode = *call.get_mode();
  }

  if (mode_changed || this->mode == climate::CLIMATE_MODE_OFF) {
    this->reset_controller();
    return;
  }

  if (this->mode == climate::CLIMATE_MODE_OFF) {
    this->integral_ = 0.0f;
    this->control_output_ = 0.0f;
    this->pid_p_ = 0.0f;
    this->pid_i_ = 0.0f;
    this->pid_d_ = 0.0f;
    this->last_error_ = 0.0f;
    this->last_dt_seconds_ = 0.0f;
    this->effective_target_temperature_ = this->target_temperature;
    this->unclamped_output_ = 0.0f;
    this->update_action_();
    this->update_valve_output_(true);
  } else {
    this->request_recompute_();
  }
  this->update_action_();
  this->publish_state();
}

void PidThermostat::set_current_temperature(float value) {
  this->current_temperature = value;
  this->last_temp_sensor_update_ms_ = millis();
  this->request_recompute_();
  this->publish_child_states_();
  this->publish_state();
}

void PidThermostat::set_current_humidity(float value) {
  this->current_humidity = value;
  this->last_humidity_sensor_update_ms_ = millis();
  this->publish_child_states_();
  this->publish_state();
}

void PidThermostat::set_commissioning_mode(bool enabled) {
  if (this->commissioning_mode_ != enabled) {
    this->commissioning_mode_ = enabled;
    this->update_valve_output_(true);
    this->publish_child_states_();
    this->publish_state();
  }
}

void PidThermostat::set_commissioning_output(float value) {
  const float clamped = std::clamp(value, 0.0f, 100.0f);
  if (this->commissioning_output_ != clamped) {
    this->commissioning_output_ = clamped;
    this->save_number_value_(NUMBER_KIND_TEST_OUTPUT, this->commissioning_output_);
    if (this->commissioning_mode_) {
      this->update_valve_output_(true);
    }
    this->publish_child_states_();
    this->publish_state();
  }
}

void PidThermostat::reset_controller() {
  this->integral_ = 0.0f;
  this->pid_p_ = 0.0f;
  this->pid_i_ = 0.0f;
  this->pid_d_ = 0.0f;
  this->control_output_ = 0.0f;
  this->last_dt_seconds_ = 0.0f;
  this->last_compute_ms_ = 0;
  this->pwm_cycle_start_ms_ = millis();

  if (!std::isnan(this->current_temperature) && !std::isnan(this->target_temperature)) {
    const bool heating = this->mode == climate::CLIMATE_MODE_HEAT;
    float effective_target = this->target_temperature;
    if (this->mode == climate::CLIMATE_MODE_COOL && !std::isnan(this->current_humidity)) {
      effective_target = std::max(this->target_temperature, this->calculate_dew_point_() + this->dew_point_offset_);
    }
    this->last_error_ = heating ? effective_target - this->current_temperature
                                : this->current_temperature - effective_target;
  } else {
    this->last_error_ = 0.0f;
  }

  this->save_integral_state_(true);

  this->request_recompute_();
  this->calculate_control_(true);
  this->update_action_();
  this->update_valve_output_(true);
  this->publish_child_states_();
  this->publish_state();
}

void PidThermostat::restore_number_values_() {
  this->restoring_ = true;
  for (auto *number : this->number_entities_) {
    if (number != nullptr) {
      number->init_and_restore();
    }
  }
  this->restoring_ = false;
}

void PidThermostat::save_number_value_(NumberKind kind, float value) {
  if (this->restoring_) {
    return;
  }
  for (auto *number : this->number_entities_) {
    if (number != nullptr && number->get_kind() == kind) {
      number->save_value(value);
      return;
    }
  }
}

void PidThermostat::restore_integral_state_() {
  if (!this->integral_pref_initialized_) {
    this->integral_pref_ = this->make_entity_preference<float>(0x49B1A123UL);
    this->integral_pref_initialized_ = true;
  }

  float restored_integral = NAN;
  if (this->integral_pref_.load(&restored_integral) && !std::isnan(restored_integral)) {
    this->integral_ = std::clamp(restored_integral, this->i_min_, this->i_max_);
    this->pid_i_ = this->integral_;
    this->last_saved_integral_ = this->integral_;
    ESP_LOGD(TAG, "Restored integral state = %.3f", this->integral_);
    return;
  }

  this->last_saved_integral_ = this->integral_;
}

void PidThermostat::save_integral_state_(bool force) {
  if (!this->integral_pref_initialized_) {
    this->integral_pref_ = this->make_entity_preference<float>(0x49B1A123UL);
    this->integral_pref_initialized_ = true;
  }

  if (!force && this->last_saved_integral_ == this->integral_) {
    return;
  }

  this->integral_pref_.save(&this->integral_);
  this->last_saved_integral_ = this->integral_;
}

void PidThermostat::set_kp(float value) {
  this->kp_ = value;
  this->save_number_value_(NUMBER_KIND_KP, this->kp_);
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_ki(float value) {
  this->ki_ = value;
  this->save_number_value_(NUMBER_KIND_KI, this->ki_);
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_kd(float value) {
  this->kd_ = value;
  this->save_number_value_(NUMBER_KIND_KD, this->kd_);
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_i_min(float value) {
  this->i_min_ = std::min(value, this->i_max_);
  this->save_number_value_(NUMBER_KIND_I_MIN, this->i_min_);
  if (this->i_max_ < this->i_min_) {
    this->i_max_ = this->i_min_;
    this->save_number_value_(NUMBER_KIND_I_MAX, this->i_max_);
  }
  this->integral_ = std::clamp(this->integral_, this->i_min_, this->i_max_);
  this->pid_i_ = this->integral_;
  this->save_integral_state_();
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_i_max(float value) {
  this->i_max_ = std::max(value, this->i_min_);
  this->save_number_value_(NUMBER_KIND_I_MAX, this->i_max_);
  if (this->i_min_ > this->i_max_) {
    this->i_min_ = this->i_max_;
    this->save_number_value_(NUMBER_KIND_I_MIN, this->i_min_);
  }
  this->integral_ = std::clamp(this->integral_, this->i_min_, this->i_max_);
  this->pid_i_ = this->integral_;
  this->save_integral_state_();
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_cold_tolerance(float value) {
  this->cold_tolerance_ = std::max(0.0f, value);
  this->save_number_value_(NUMBER_KIND_COLD_TOLERANCE, this->cold_tolerance_);
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_hot_tolerance(float value) {
  this->hot_tolerance_ = std::max(0.0f, value);
  this->save_number_value_(NUMBER_KIND_HOT_TOLERANCE, this->hot_tolerance_);
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_sampling_period(uint32_t period_ms) {
  this->sampling_period_ms_ = period_ms;
  this->save_number_value_(NUMBER_KIND_SAMPLING_PERIOD, this->get_sampling_period_seconds());
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_keep_alive(uint32_t keep_alive_ms) {
  this->keep_alive_ms_ = keep_alive_ms;
  this->save_number_value_(NUMBER_KIND_KEEP_ALIVE, this->get_keep_alive_seconds());
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_min_cycle_duration(uint32_t duration_ms) {
  this->min_cycle_duration_ms_ = duration_ms;
  this->save_number_value_(NUMBER_KIND_MIN_CYCLE_DURATION, this->get_min_cycle_duration_seconds());
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_min_off_cycle_duration(uint32_t duration_ms) {
  this->min_off_cycle_duration_ms_ = duration_ms;
  this->save_number_value_(NUMBER_KIND_MIN_OFF_CYCLE_DURATION, this->get_min_off_cycle_duration_seconds());
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_output_safety(float output_safety) {
  this->output_safety_ = std::clamp(output_safety, 0.0f, 100.0f);
  this->save_number_value_(NUMBER_KIND_OUTPUT_SAFETY, this->output_safety_);
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_pwm_period(uint32_t period_ms) {
  this->pwm_period_ms_ = std::max<uint32_t>(period_ms, 10000U);
  this->pwm_cycle_start_ms_ = millis();
  this->save_number_value_(NUMBER_KIND_PWM_PERIOD, this->get_pwm_period_seconds());
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_pwm_min(float value) {
  this->pwm_min_ = std::clamp(value, 0.0f, 100.0f);
  this->save_number_value_(NUMBER_KIND_PWM_MIN, this->pwm_min_);
  if (this->pwm_max_ < this->pwm_min_) {
    this->pwm_max_ = this->pwm_min_;
    this->save_number_value_(NUMBER_KIND_PWM_MAX, this->pwm_max_);
  }
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::set_pwm_max(float value) {
  this->pwm_max_ = std::clamp(value, 0.0f, 100.0f);
  this->save_number_value_(NUMBER_KIND_PWM_MAX, this->pwm_max_);
  if (this->pwm_min_ > this->pwm_max_) {
    this->pwm_min_ = this->pwm_max_;
    this->save_number_value_(NUMBER_KIND_PWM_MIN, this->pwm_min_);
  }
  this->request_recompute_();
  this->publish_child_states_();
}

void PidThermostat::update_temp_sensor_() {
  const float previous_temperature = this->current_temperature;
  const bool previous_fallback = this->using_fallback_temperature_;
  const uint32_t now = millis();
  const bool primary_valid = this->sensor_ != nullptr && !std::isnan(this->sensor_->state);
  const bool fallback_valid = this->fallback_sensor_ != nullptr && !std::isnan(this->fallback_sensor_->state);
  if (primary_valid)
    this->last_primary_temp_update_ms_ = now;
  if (fallback_valid)
    this->last_fallback_temp_update_ms_ = now;

  const bool primary_timed_out = this->sensor_timeout_ms_ > 0 && this->sensor_ != nullptr &&
                                 now - this->last_primary_temp_update_ms_ > this->sensor_timeout_ms_;
  const bool fallback_timed_out = this->fallback_sensor_timeout_ms_ > 0 && this->fallback_sensor_ != nullptr &&
                                  now - this->last_fallback_temp_update_ms_ > this->fallback_sensor_timeout_ms_;

  if (primary_valid && !primary_timed_out) {
    this->current_temperature = this->sensor_->state;
    this->using_fallback_temperature_ = false;
    this->last_temp_sensor_update_ms_ = this->last_primary_temp_update_ms_;
  } else if (fallback_valid && !fallback_timed_out) {
    this->current_temperature = this->fallback_sensor_->state;
    this->using_fallback_temperature_ = true;
    this->last_temp_sensor_update_ms_ = this->last_fallback_temp_update_ms_;
  } else {
    this->current_temperature = NAN;
    this->using_fallback_temperature_ = false;
  }

  const bool temperature_changed =
      (std::isnan(previous_temperature) != std::isnan(this->current_temperature)) ||
      (!std::isnan(previous_temperature) && !std::isnan(this->current_temperature) &&
       previous_temperature != this->current_temperature) ||
      previous_fallback != this->using_fallback_temperature_;

  if (!temperature_changed) {
    return;
  }

  this->request_recompute_();
  this->publish_state();
}

void PidThermostat::update_humidity_sensor_() {
  const float previous_humidity = this->current_humidity;
  const bool previous_fallback = this->using_fallback_humidity_;
  const uint32_t now = millis();
  const bool primary_valid = this->humidity_sensor_ != nullptr && !std::isnan(this->humidity_sensor_->state);
  const bool fallback_valid = this->fallback_humidity_sensor_ != nullptr && !std::isnan(this->fallback_humidity_sensor_->state);
  if (primary_valid)
    this->last_primary_humidity_update_ms_ = now;
  if (fallback_valid)
    this->last_fallback_humidity_update_ms_ = now;

  const bool primary_timed_out = this->humidity_sensor_timeout_ms_ > 0 && this->humidity_sensor_ != nullptr &&
                                 now - this->last_primary_humidity_update_ms_ > this->humidity_sensor_timeout_ms_;
  const bool fallback_timed_out = this->fallback_humidity_sensor_timeout_ms_ > 0 &&
                                  this->fallback_humidity_sensor_ != nullptr &&
                                  now - this->last_fallback_humidity_update_ms_ > this->fallback_humidity_sensor_timeout_ms_;

  if (primary_valid && !primary_timed_out) {
    this->current_humidity = this->humidity_sensor_->state;
    this->using_fallback_humidity_ = false;
    this->last_humidity_sensor_update_ms_ = this->last_primary_humidity_update_ms_;
  } else if (fallback_valid && !fallback_timed_out) {
    this->current_humidity = this->fallback_humidity_sensor_->state;
    this->using_fallback_humidity_ = true;
    this->last_humidity_sensor_update_ms_ = this->last_fallback_humidity_update_ms_;
  } else {
    this->current_humidity = NAN;
    this->using_fallback_humidity_ = false;
  }

  const bool humidity_changed =
      (std::isnan(previous_humidity) != std::isnan(this->current_humidity)) ||
      (!std::isnan(previous_humidity) && !std::isnan(this->current_humidity) &&
       previous_humidity != this->current_humidity) ||
      previous_fallback != this->using_fallback_humidity_;

  if (!humidity_changed) {
    return;
  }

  this->request_recompute_();
  this->publish_state();
}

void PidThermostat::check_timeouts_() {
  const float last_temp = this->current_temperature;
  const float last_humidity = this->current_humidity;
  const bool last_temp_fallback = this->using_fallback_temperature_;
  const bool last_humidity_fallback = this->using_fallback_humidity_;

  this->update_temp_sensor_();
  this->update_humidity_sensor_();

  if (last_temp != this->current_temperature || last_humidity != this->current_humidity ||
      last_temp_fallback != this->using_fallback_temperature_ ||
      last_humidity_fallback != this->using_fallback_humidity_) {
    this->request_recompute_();
  }
}

bool PidThermostat::should_sample_(uint32_t now) const {
  if (this->sampling_period_ms_ == 0) {
    return false;
  }
  return now - this->last_compute_ms_ >= this->sampling_period_ms_;
}

void PidThermostat::calculate_control_(bool force) {
  const uint32_t now = millis();
  if (!force && !this->pending_recompute_ && !this->should_sample_(now)) {
    return;
  }

  this->pending_recompute_ = false;

  if (this->mode == climate::CLIMATE_MODE_OFF) {
    this->control_output_ = 0.0f;
    this->pid_p_ = 0.0f;
    this->pid_i_ = 0.0f;
    this->pid_d_ = 0.0f;
    this->last_error_ = 0.0f;
    this->last_dt_seconds_ = 0.0f;
    this->effective_target_temperature_ = this->target_temperature;
    this->unclamped_output_ = 0.0f;
    this->last_compute_ms_ = now;
    return;
  }

  if (std::isnan(this->current_temperature) || std::isnan(this->target_temperature)) {
    this->control_output_ = this->mode == climate::CLIMATE_MODE_HEAT ? std::clamp(this->output_safety_, 0.0f, 100.0f) : 0.0f;
    this->pid_p_ = 0.0f;
    this->pid_i_ = this->integral_;
    this->pid_d_ = 0.0f;
    this->last_error_ = NAN;
    this->last_dt_seconds_ = 0.0f;
    this->effective_target_temperature_ = this->target_temperature;
    this->unclamped_output_ = this->control_output_;
    this->last_compute_ms_ = now;
    return;
  }

  float dt = 0.0f;
  if (this->last_compute_ms_ != 0 && now > this->last_compute_ms_) {
    dt = (now - this->last_compute_ms_) / 1000.0f;
  }
  if (dt <= 0.0f) {
    dt = 1.0f;
  }
  this->last_dt_seconds_ = dt;

  const bool heating = this->mode == climate::CLIMATE_MODE_HEAT;
  float effective_target = this->target_temperature;
  if (this->mode == climate::CLIMATE_MODE_COOL && !std::isnan(this->current_humidity)) {
    effective_target = std::max(this->target_temperature, this->calculate_dew_point_() + this->dew_point_offset_);
  }
  this->effective_target_temperature_ = effective_target;
  const float error = heating ? effective_target - this->current_temperature
                              : this->current_temperature - effective_target;

  this->pid_p_ = this->kp_ * error;
  // After a sensor dropout last_error_ is NAN; deriving from it would poison the
  // whole output with NAN, so the D term restarts at zero instead.
  this->pid_d_ = std::isnan(this->last_error_) ? 0.0f : this->kd_ * ((error - this->last_error_) / dt);

  // The integral term is only limited by i_min/i_max so it keeps integrating on
  // every calculation step, even while the output is clamped or blocked.
  const float integral_candidate = this->integral_ + this->ki_ * error * dt;
  this->integral_ = std::clamp(integral_candidate, this->i_min_, this->i_max_);
  this->save_integral_state_();

  this->unclamped_output_ = this->pid_p_ + this->integral_ + this->pid_d_;

  float output = this->pid_p_ + this->integral_ + this->pid_d_;

  if (output <= 0.0f) {
    output = 0.0f;
  } else {
    output = std::clamp(output, this->pwm_min_, this->pwm_max_);
  }

  // A tolerance of 0 disables the hard cut-off, so the PID alone (P + I + D)
  // decides the output around the setpoint.
  if (!heating) {
    if (this->cold_tolerance_ > 0.0f && this->current_temperature <= effective_target - this->cold_tolerance_) {
      output = 0.0f;
    }
  } else {
    if (this->hot_tolerance_ > 0.0f && this->current_temperature >= effective_target + this->hot_tolerance_) {
      output = 0.0f;
    }
  }

  this->last_error_ = error;
  this->control_output_ = std::clamp(output, 0.0f, 100.0f);
  this->pid_i_ = this->integral_;
  this->last_compute_ms_ = now;

  if (this->debug_) {
    ESP_LOGD(TAG,
             "Control output=%.1f error=%.3f p=%.3f i=%.3f d=%.3f dt=%.3f mode=%d current=%.2f target=%.2f",
             this->get_effective_control_output_(), error, this->pid_p_, this->pid_i_, this->pid_d_, this->last_dt_seconds_,
             this->mode, this->current_temperature, effective_target);
  }
}

void PidThermostat::update_action_() {
  const bool valve_active = this->get_effective_valve_state_();
  climate::ClimateAction new_action = climate::CLIMATE_ACTION_OFF;
  switch (this->mode) {
    case climate::CLIMATE_MODE_OFF:
      new_action = climate::CLIMATE_ACTION_OFF;
      break;
    case climate::CLIMATE_MODE_HEAT:
      new_action = valve_active ? climate::CLIMATE_ACTION_HEATING : climate::CLIMATE_ACTION_IDLE;
      break;
    case climate::CLIMATE_MODE_COOL:
      new_action = valve_active ? climate::CLIMATE_ACTION_COOLING : climate::CLIMATE_ACTION_IDLE;
      break;
    default:
      new_action = climate::CLIMATE_ACTION_OFF;
      break;
  }

  if (new_action != this->action) {
    this->action = new_action;
    this->publish_state();
  }
}

void PidThermostat::update_valve_output_(bool force) {
  bool desired_state = false;
  const uint32_t now = millis();
  const bool valve_control_enabled = this->get_valve_control_enabled_();
  const float active_output = this->get_effective_control_output_();

  if (valve_control_enabled != this->valve_control_enabled_state_) {
    this->valve_control_enabled_state_ = valve_control_enabled;
    if (valve_control_enabled) {
      this->reset_controller();
      return;
    }

    this->integral_ = 0.0f;
    this->pid_p_ = 0.0f;
    this->pid_i_ = 0.0f;
    this->pid_d_ = 0.0f;
    this->control_output_ = 0.0f;
    this->last_error_ = 0.0f;
    this->last_dt_seconds_ = 0.0f;
    this->unclamped_output_ = 0.0f;
    this->pwm_cycle_start_ms_ = now;
    this->save_integral_state_(true);
    this->request_recompute_();
    this->update_action_();
    this->publish_child_states_();
    this->publish_state();
  }

  // In manual mode the PID must stop driving the physical output so the user can
  // operate the valve switch directly from Home Assistant.
  if (!valve_control_enabled && !this->commissioning_mode_) {
    if (this->valve_switch_ != nullptr && this->valve_state_ != this->valve_switch_->state) {
      this->valve_state_ = this->valve_switch_->state;
      this->last_valve_state_change_ms_ = now;
      this->publish_valve_output_state_();
      this->update_action_();
      this->publish_state();
    }
    return;
  }

  if (this->mode != climate::CLIMATE_MODE_OFF && valve_control_enabled && active_output > 0.0f) {
    if (this->pwm_period_ms_ == 0 || active_output >= 100.0f) {
      desired_state = true;
    } else {
      const uint32_t elapsed_in_cycle = now - this->pwm_cycle_start_ms_;
      const uint32_t on_time_ms = static_cast<uint32_t>(this->pwm_period_ms_ * (active_output / 100.0f));
      desired_state = elapsed_in_cycle < on_time_ms;
    }
  }

  if (desired_state != this->valve_state_) {
    const uint32_t min_state_ms = this->valve_state_ ? this->min_cycle_duration_ms_ : this->min_off_cycle_duration_ms_;
    if (!force && min_state_ms > 0 && now - this->last_valve_state_change_ms_ < min_state_ms) {
      desired_state = this->valve_state_;
    }
  }

  if (desired_state != this->valve_state_) {
    this->valve_state_ = desired_state;
    this->last_valve_state_change_ms_ = now;
    this->publish_valve_output_state_();
    this->update_action_();
    this->publish_state();
  }

  if (this->valve_switch_ != nullptr) {
    if (desired_state) {
      if (force || !this->valve_switch_->state) {
        this->valve_switch_->turn_on();
      }
    } else {
      if (force || this->valve_switch_->state) {
        this->valve_switch_->turn_off();
      }
    }
  }

  this->publish_valve_output_state_();
  this->update_action_();
}

bool PidThermostat::get_valve_control_enabled_() {
  if (this->valve_control_enabled_func_.has_value()) {
    return (*this->valve_control_enabled_func_)();
  }
  return this->valve_control_enabled_value_;
}

float PidThermostat::get_effective_control_output_() const {
  if (this->commissioning_mode_) {
    return this->commissioning_output_;
  }
  return this->control_output_;
}

const char *PidThermostat::get_output_reason_() const {
  if (this->commissioning_mode_) {
    return "commissioning";
  }
  if (this->mode == climate::CLIMATE_MODE_OFF) {
    return "mode_off";
  }
  if (!this->valve_control_enabled_state_) {
    return "manual_override";
  }
  if (std::isnan(this->current_temperature) || std::isnan(this->target_temperature)) {
    if (this->mode == climate::CLIMATE_MODE_HEAT && this->control_output_ > 0.0f) {
      return "sensor_missing_safety_output";
    }
    return "sensor_missing";
  }

  if (this->mode == climate::CLIMATE_MODE_HEAT && this->hot_tolerance_ > 0.0f &&
      this->current_temperature >= this->effective_target_temperature_ + this->hot_tolerance_) {
    return "blocked_hot_tolerance";
  }
  if (this->mode == climate::CLIMATE_MODE_COOL && this->cold_tolerance_ > 0.0f &&
      this->current_temperature <= this->effective_target_temperature_ - this->cold_tolerance_) {
    return "blocked_cold_tolerance";
  }
  if (this->unclamped_output_ <= 0.0f) {
    return "pid_below_zero";
  }
  if (this->control_output_ >= this->pwm_max_) {
    return "clamped_pwm_max";
  }
  if (this->control_output_ > 0.0f && this->control_output_ < this->pwm_min_) {
    return "raised_to_pwm_min";
  }
  if (this->control_output_ > 0.0f) {
    return "active";
  }
  return "inactive";
}

void PidThermostat::publish_child_states_() {
  if (this->output_sensor_ != nullptr) {
    const float output = this->get_effective_control_output_();
    if ((std::isnan(this->output_sensor_->state) && std::isnan(output)) ||
        (!std::isnan(this->output_sensor_->state) && !std::isnan(output) && this->output_sensor_->state == output)) {
      // unchanged
    } else {
      this->output_sensor_->publish_state(output);
    }
  }
  if (this->valve_output_sensor_ != nullptr) {
    this->publish_valve_output_state_();
  }
  if (this->setpoint_sensor_ != nullptr) {
    const float setpoint = this->get_setpoint();
    if ((std::isnan(this->setpoint_sensor_->state) && std::isnan(setpoint)) ||
        (!std::isnan(this->setpoint_sensor_->state) && !std::isnan(setpoint) && this->setpoint_sensor_->state == setpoint)) {
      // unchanged
    } else {
      this->setpoint_sensor_->publish_state(setpoint);
    }
  }
  if (this->effective_setpoint_sensor_ != nullptr) {
    const float effective_setpoint = this->get_effective_setpoint();
    if ((std::isnan(this->effective_setpoint_sensor_->state) && std::isnan(effective_setpoint)) ||
        (!std::isnan(this->effective_setpoint_sensor_->state) && !std::isnan(effective_setpoint) &&
         this->effective_setpoint_sensor_->state == effective_setpoint)) {
      // unchanged
    } else {
      this->effective_setpoint_sensor_->publish_state(effective_setpoint);
    }
  }
  if (this->unclamped_output_sensor_ != nullptr) {
    const float unclamped_output = this->get_unclamped_output();
    if ((std::isnan(this->unclamped_output_sensor_->state) && std::isnan(unclamped_output)) ||
        (!std::isnan(this->unclamped_output_sensor_->state) && !std::isnan(unclamped_output) &&
         this->unclamped_output_sensor_->state == unclamped_output)) {
      // unchanged
    } else {
      this->unclamped_output_sensor_->publish_state(unclamped_output);
    }
  }
  if (this->current_temperature_sensor_ != nullptr) {
    const float current_temperature = this->current_temperature;
    if ((std::isnan(this->current_temperature_sensor_->state) && std::isnan(current_temperature)) ||
        (!std::isnan(this->current_temperature_sensor_->state) && !std::isnan(current_temperature) &&
         this->current_temperature_sensor_->state == current_temperature)) {
      // unchanged
    } else {
      this->current_temperature_sensor_->publish_state(current_temperature);
    }
  }
  if (this->current_humidity_sensor_ != nullptr) {
    const float current_humidity = this->current_humidity;
    if ((std::isnan(this->current_humidity_sensor_->state) && std::isnan(current_humidity)) ||
        (!std::isnan(this->current_humidity_sensor_->state) && !std::isnan(current_humidity) &&
         this->current_humidity_sensor_->state == current_humidity)) {
      // unchanged
    } else {
      this->current_humidity_sensor_->publish_state(current_humidity);
    }
  }
  if (this->dew_point_sensor_ != nullptr) {
    const float dew_point = this->calculate_dew_point_();
    if ((std::isnan(this->dew_point_sensor_->state) && std::isnan(dew_point)) ||
        (!std::isnan(this->dew_point_sensor_->state) && !std::isnan(dew_point) && this->dew_point_sensor_->state == dew_point)) {
      // unchanged
    } else {
      this->dew_point_sensor_->publish_state(dew_point);
    }
  }
  if (this->error_sensor_ != nullptr) {
    const float error = this->get_pid_error();
    if ((std::isnan(this->error_sensor_->state) && std::isnan(error)) ||
        (!std::isnan(this->error_sensor_->state) && !std::isnan(error) && this->error_sensor_->state == error)) {
      // unchanged
    } else {
      this->error_sensor_->publish_state(error);
    }
  }
  if (this->pid_p_sensor_ != nullptr) {
    const float pid_p = this->get_pid_p();
    if ((std::isnan(this->pid_p_sensor_->state) && std::isnan(pid_p)) ||
        (!std::isnan(this->pid_p_sensor_->state) && !std::isnan(pid_p) && this->pid_p_sensor_->state == pid_p)) {
      // unchanged
    } else {
      this->pid_p_sensor_->publish_state(pid_p);
    }
  }
  if (this->pid_i_sensor_ != nullptr) {
    const float pid_i = this->get_pid_i();
    if ((std::isnan(this->pid_i_sensor_->state) && std::isnan(pid_i)) ||
        (!std::isnan(this->pid_i_sensor_->state) && !std::isnan(pid_i) && this->pid_i_sensor_->state == pid_i)) {
      // unchanged
    } else {
      this->pid_i_sensor_->publish_state(pid_i);
    }
  }
  if (this->pid_d_sensor_ != nullptr) {
    const float pid_d = this->get_pid_d();
    if ((std::isnan(this->pid_d_sensor_->state) && std::isnan(pid_d)) ||
        (!std::isnan(this->pid_d_sensor_->state) && !std::isnan(pid_d) && this->pid_d_sensor_->state == pid_d)) {
      // unchanged
    } else {
      this->pid_d_sensor_->publish_state(pid_d);
    }
  }
  if (this->pid_dt_sensor_ != nullptr) {
    const float pid_dt = this->get_pid_dt();
    if ((std::isnan(this->pid_dt_sensor_->state) && std::isnan(pid_dt)) ||
        (!std::isnan(this->pid_dt_sensor_->state) && !std::isnan(pid_dt) && this->pid_dt_sensor_->state == pid_dt)) {
      // unchanged
    } else {
      this->pid_dt_sensor_->publish_state(pid_dt);
    }
  }
  if (this->commissioning_switch_ != nullptr) {
    if (this->commissioning_switch_->state != this->commissioning_mode_) {
      this->commissioning_switch_->publish_state(this->commissioning_mode_);
    }
  }
  if (this->mode_text_sensor_ != nullptr) {
    const char *mode_state = nullptr;
    switch (this->mode) {
      case climate::CLIMATE_MODE_HEAT:
        mode_state = "heat";
        break;
      case climate::CLIMATE_MODE_COOL:
        mode_state = "cool";
        break;
      default:
        mode_state = "off";
        break;
    }
    if (this->mode_text_sensor_->state != mode_state) {
      this->mode_text_sensor_->publish_state(mode_state);
    }
  }
  if (this->temperature_source_text_sensor_ != nullptr) {
    const char *temperature_state = nullptr;
    if (std::isnan(this->current_temperature)) {
      temperature_state = "none";
    } else if (this->using_fallback_temperature_) {
      temperature_state = "fallback";
    } else {
      temperature_state = "primary";
    }
    if (this->temperature_source_text_sensor_->state != temperature_state) {
      this->temperature_source_text_sensor_->publish_state(temperature_state);
    }
  }
  if (this->humidity_source_text_sensor_ != nullptr) {
    const char *humidity_state = nullptr;
    if (std::isnan(this->current_humidity)) {
      humidity_state = "none";
    } else if (this->using_fallback_humidity_) {
      humidity_state = "fallback";
    } else {
      humidity_state = "primary";
    }
    if (this->humidity_source_text_sensor_->state != humidity_state) {
      this->humidity_source_text_sensor_->publish_state(humidity_state);
    }
  }
  if (this->output_reason_text_sensor_ != nullptr) {
    const char *output_reason = this->get_output_reason_();
    if (this->output_reason_text_sensor_->state != output_reason) {
      this->output_reason_text_sensor_->publish_state(output_reason);
    }
  }
  for (auto *number_entity : this->number_entities_) {
    if (number_entity != nullptr) {
      number_entity->publish_from_parent();
    }
  }
}

float PidThermostat::calculate_dew_point_() const {
  if (std::isnan(this->current_temperature) || std::isnan(this->current_humidity) || this->current_humidity <= 0.0f) {
    return NAN;
  }
  const float a = 17.62f;
  const float b = 243.12f;
  const float gamma = std::log(this->current_humidity / 100.0f) + (a * this->current_temperature) / (b + this->current_temperature);
  return (b * gamma) / (a - gamma);
}

}  // namespace pid_thermostat
}  // namespace esphome
