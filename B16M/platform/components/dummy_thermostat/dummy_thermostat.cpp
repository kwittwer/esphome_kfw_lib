#include "dummy_thermostat.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace dummy_thermostat {

static const char *const TAG = "dummy_thermostat";

void DummyThermostat::setup() {
  // Restore state
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->target_temperature = 21.0f;
  }
  
  // Initialize current_temperature with NAN
  this->current_temperature = NAN;
  
  // Initialize current_humidity with NAN
  this->current_humidity = NAN;
  
  // Set initial action
  this->action = climate::CLIMATE_ACTION_OFF;
  
  if (this->fallback_sensor_ != nullptr) {
    this->last_fallback_sensor_update_ = esphome::millis();
    this->fallback_sensor_->add_on_state_callback([this](float state) {
      this->last_fallback_sensor_update_ = esphome::millis();
      this->update_temp_sensor_();
    });
  }
  
  if (this->fallback_humidity_sensor_ != nullptr) {
    this->fallback_humidity_sensor_->add_on_state_callback([this](float state) {
      this->update_humidity_sensor_();
    });
  }

  
  // Initial update
  this->update_temp_sensor_();
  this->update_humidity_sensor_();
  this->calculate_action_from_valve_and_mode_();
  this->update_valve_output_();
  this->publish_state();
}

void DummyThermostat::loop() {
  const uint32_t now = esphome::millis();

  // Check sensor timeout
  if (this->temp_sensor_timeout_ > 0 && this->fallback_sensor_ != nullptr) {
    const bool timeout_active =
        (now - this->last_temp_sensor_update_) > this->temp_sensor_timeout_ * 1000UL;
    if (timeout_active != this->using_fallback_temperature_) {
      this->using_fallback_temperature_ = timeout_active;
      if (timeout_active) {
        ESP_LOGW(TAG, "Temperature sensor timeout! Switching to fallback sensor.");
      } else {
        ESP_LOGI(TAG, "Temperature sensor recovered. Switching back to main sensor.");
        this->fallback_sensor_timed_out_ = false;
      }
      this->update_temp_sensor_();
    }
  }

  // Check fallback sensor timeout (only when in fallback mode)
  if (this->fallback_sensor_timeout_ > 0 && this->fallback_sensor_ != nullptr
      && this->using_fallback_temperature_) {
    const bool timed_out =
        (now - this->last_fallback_sensor_update_) > this->fallback_sensor_timeout_ * 1000UL;
    if (timed_out != this->fallback_sensor_timed_out_) {
      this->fallback_sensor_timed_out_ = timed_out;
      if (timed_out) {
        ESP_LOGW(TAG, "Fallback sensor timeout! No valid temperature source available.");
        this->current_temperature = NAN;
        this->publish_state();
      } else {
        ESP_LOGI(TAG, "Fallback sensor recovered.");
        this->update_temp_sensor_();
      }
    }
  }

  // Check humidity sensor timeout
  if (this->humidity_sensor_timeout_ > 0 && this->fallback_humidity_sensor_ != nullptr) {
    const bool timeout_active =
        (now - this->last_humidity_sensor_update_) > this->humidity_sensor_timeout_ * 1000UL;
    if (timeout_active != this->using_fallback_humidity_) {
      this->using_fallback_humidity_ = timeout_active;
      if (timeout_active) {
        ESP_LOGW(TAG, "Humidity sensor timeout! Switching to fallback sensor.");
      } else {
        ESP_LOGI(TAG, "Humidity sensor recovered. Switching back to main sensor.");
      }
      this->update_humidity_sensor_();
    }
  }
  
  // Check valve state switch timeout
  if (this->valve_update_timeout_ > 0) {
    if (now - this->last_valve_state_update_ > this->valve_update_timeout_ * 1000UL) {
      if (!this->valve_timeout_active_) {
        ESP_LOGW(TAG, "Valve update timeout! Switching to local controll.");
        this->valve_timeout_active_ = true;
      }
    } else {
      if (this->valve_timeout_active_) {
        ESP_LOGI(TAG, "Valve update timeout recovered. Switching back to main external controll.");
        this->valve_timeout_active_ = false;
      }
    }
  }
  
  // Update valve state based on mode
  if (this->valve_timeout_active_) {
    this->calculate_local_valve_state_();
  }
  
  // Update action based on current state
  this->calculate_action_from_valve_and_mode_();
  
  // Update physical valve output
  this->update_valve_output_();

}

void DummyThermostat::dump_config() {
  ESP_LOGCONFIG(TAG, "Dummy Thermostat:");
  // ESP_LOGCONFIG(TAG, "  Sensor: %s", this->sensor_ ? this->sensor_->get_name().c_str() : "None");
  ESP_LOGCONFIG(TAG, "  Fallback Sensor: %s", this->fallback_sensor_ ? this->fallback_sensor_->get_name().c_str() : "None");
  ESP_LOGCONFIG(TAG, "  Temp Sensor Timeout: %u seconds", this->temp_sensor_timeout_);
  ESP_LOGCONFIG(TAG, "  Fallback Sensor Timeout: %u seconds", this->fallback_sensor_timeout_);
  // ESP_LOGCONFIG(TAG, "  Humidity Sensor: %s", this->humidity_sensor_ ? this->humidity_sensor_->get_name().c_str() : "None");
  ESP_LOGCONFIG(TAG, "  Fallback Humidity Sensor: %s", this->fallback_humidity_sensor_ ? this->fallback_humidity_sensor_->get_name().c_str() : "None");
  ESP_LOGCONFIG(TAG, "  Humidity Sensor Timeout: %u seconds", this->humidity_sensor_timeout_);
  
// #ifdef USE_SWITCH
//   ESP_LOGCONFIG(TAG, "  Valve State Switch: %s", this->valve_state_switch_ ? this->valve_state_switch_->get_name().c_str() : "None");
//   ESP_LOGCONFIG(TAG, "  Valve State Switch Timeout: %u seconds", this->valve_state_switch_timeout_);
// #endif

// #ifdef USE_SELECT
//   ESP_LOGCONFIG(TAG, "  Valve State Select: %s", this->valve_state_select_ ? this->valve_state_select_->get_name().c_str() : "None");
// #endif

// #ifdef USE_TEXT_SENSOR
//   ESP_LOGCONFIG(TAG, "  Valve State Text Sensor: %s", this->valve_state_text_sensor_ ? this->valve_state_text_sensor_->get_name().c_str() : "None");
// #endif
  
  // if (!this->status_topic_.empty()) {
  //   ESP_LOGCONFIG(TAG, "  MQTT Status Topic: %s", this->status_topic_.c_str());
  //   if (this->status_update_interval_ > 0) {
  //     ESP_LOGCONFIG(TAG, "  MQTT Status: Periodic (%u sec) + on change", this->status_update_interval_);
  //   } else {
  //     ESP_LOGCONFIG(TAG, "  MQTT Status: On change only");
  //   }
  // }
  
  ESP_LOGCONFIG(TAG, "  Valve Control Enabled (Master Switch): %s", this->get_valve_control_enabled_() ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Use Local Valve Control: %s", this->get_use_local_valve_control_() ? "YES (Local Hysteresis)" : "NO (External Input)");
  
  if (this->get_use_local_valve_control_()) {
    ESP_LOGCONFIG(TAG, "  Heating Deadband: %.1f°C", this->heating_deadband_);
    ESP_LOGCONFIG(TAG, "  Heating Overrun: %.1f°C", this->heating_overrun_);
    ESP_LOGCONFIG(TAG, "  Cooling Deadband: %.1f°C", this->cooling_deadband_);
    ESP_LOGCONFIG(TAG, "  Cooling Overrun: %.1f°C", this->cooling_overrun_);
  }
}

void DummyThermostat::update_temp_sensor_() {
  float temperature = NAN;
  const uint32_t now = esphome::millis();
  
  if (this->using_fallback_temperature_ && this->fallback_sensor_ != nullptr) {
    temperature = this->fallback_sensor_->state;
    this->last_fallback_sensor_update_ = now;
  } 
  // else if (this->sensor_ != nullptr) {
  //   temperature = this->sensor_->state;
  // }
  
  if (!std::isnan(temperature) && temperature != this->current_temperature) {
    this->current_temperature = temperature;
    this->publish_state();
  }
}

void DummyThermostat::update_humidity_sensor_() {
  float humidity = NAN;
  const uint32_t now = esphome::millis();
  
  if (this->using_fallback_humidity_ && this->fallback_humidity_sensor_ != nullptr) {
    humidity = this->fallback_humidity_sensor_->state;
    this->last_humidity_sensor_update_ = now;
  } 
  // else if (this->humidity_sensor_ != nullptr) {
  //   humidity = this->humidity_sensor_->state;
  // }
  
  if (!std::isnan(humidity) && humidity != this->current_humidity) {
    this->current_humidity = humidity;
    this->publish_state();
  }
}



void DummyThermostat::calculate_local_valve_state_() {
  uint32_t now = esphome::millis();
  if(now < (this->last_loop_update_+500))
  {
    return;
  }
  else
  {
    this->last_loop_update_ = esphome::millis();
  }

  if (std::isnan(this->current_temperature) || std::isnan(this->target_temperature)) {
    return;
  }
  
  bool new_valve_state = this->valve_state_;
  
  switch (this->mode) {
    case climate::CLIMATE_MODE_HEAT:
      // Heating logic with hysteresis
      if (this->current_temperature < this->target_temperature - this->heating_deadband_) {
        new_valve_state = true;
        ESP_LOGI(TAG, "Local valve: Start heating (%.1f°C < %.1f°C - %.1f°C)", 
                 this->current_temperature, this->target_temperature, this->heating_deadband_);
      } else if (this->current_temperature >= this->target_temperature + this->heating_overrun_) {
        new_valve_state = false;
        ESP_LOGI(TAG, "Local valve: Stop heating (%.1f°C >= %.1f°C + %.1f°C)", 
                 this->current_temperature, this->target_temperature, this->heating_overrun_);
      }
      break;
      
    case climate::CLIMATE_MODE_COOL:
      // COOLING DISABLED IN LOCAL MODE
      new_valve_state = false;
      ESP_LOGW(TAG, "Local valve: Cooling not supported in local mode - valve OFF");
      break;
      
    case climate::CLIMATE_MODE_HEAT_COOL:
      // AUTO mode: ONLY HEATING in local mode (cooling disabled)
      if (this->current_temperature < this->target_temperature - this->heating_deadband_) {
        new_valve_state = true;
        ESP_LOGI(TAG, "Local valve: Start heating in AUTO mode (%.1f°C < %.1f°C - %.1f°C)", 
                 this->current_temperature, this->target_temperature, this->heating_deadband_);
      } else if (this->current_temperature >= this->target_temperature + this->heating_overrun_) {
        new_valve_state = false;
        ESP_LOGI(TAG, "Local valve: Stop heating in AUTO mode (%.1f°C >= %.1f°C + %.1f°C)", 
                 this->current_temperature, this->target_temperature, this->heating_overrun_);
      }
      // Note: Cooling part removed - only heating works in local mode
      break;
      
    default:
      new_valve_state = false;
      break;
  }
  
  if (new_valve_state != this->valve_state_) {
    ESP_LOGI(TAG, "Local valve state changed: %s", new_valve_state ? "ON" : "OFF");
    this->valve_state_ = new_valve_state;
     this->publish_state();
  }
}

void DummyThermostat::calculate_action_from_valve_and_mode_() {
  climate::ClimateAction new_action = climate::CLIMATE_ACTION_OFF;
  
  switch (this->mode) {
    case climate::CLIMATE_MODE_OFF:
      new_action = climate::CLIMATE_ACTION_OFF;
      break;
      
    case climate::CLIMATE_MODE_HEAT:
      if (this->valve_state_) {
        new_action = climate::CLIMATE_ACTION_HEATING;
      } else {
        new_action = climate::CLIMATE_ACTION_IDLE;
      }
      break;
      
    case climate::CLIMATE_MODE_COOL:
      if (this->valve_state_) {
        new_action = climate::CLIMATE_ACTION_COOLING;
      } else {
        new_action = climate::CLIMATE_ACTION_IDLE;
      }
      break;
      
    case climate::CLIMATE_MODE_HEAT_COOL:
      if (this->valve_state_) {
        // Determine if heating or cooling based on temperature
        if (!std::isnan(this->current_temperature) && !std::isnan(this->target_temperature)) {
          if (this->current_temperature < this->target_temperature) {
            new_action = climate::CLIMATE_ACTION_HEATING;
          } else if (this->current_temperature > this->target_temperature) {
            new_action = climate::CLIMATE_ACTION_COOLING;
          } else {
            new_action = climate::CLIMATE_ACTION_IDLE;
          }
        } else {
          new_action = climate::CLIMATE_ACTION_IDLE;
        }
      } else {
        new_action = climate::CLIMATE_ACTION_IDLE;
      }
      break;
      
    default:
      new_action = climate::CLIMATE_ACTION_OFF;
      break;
  }
  
  if (new_action != this->action) {
    ESP_LOGI(TAG, "Action changed to: %s (mode=%d, valve=%s, T=%.1f°C, Tset=%.1f°C)", 
             new_action == climate::CLIMATE_ACTION_HEATING ? "HEATING" :
             new_action == climate::CLIMATE_ACTION_COOLING ? "COOLING" :
             new_action == climate::CLIMATE_ACTION_IDLE ? "IDLE" : "OFF",
             this->mode, this->valve_state_ ? "ON" : "OFF",
             this->current_temperature, this->target_temperature);
    this->action = new_action;

  }
}

void DummyThermostat::update_valve_output_() {
  bool control_enabled = this->get_valve_control_enabled_();
  bool output_state = control_enabled && this->valve_state_;
  if(this->mode == climate::CLIMATE_MODE_OFF) output_state = false;
  
  // Update binary output
#ifdef USE_OUTPUT
  if (this->valve_output_ != nullptr) {
    if (output_state) {
      if (this->valve_output_->state != output_state) this->valve_output_->turn_on();
    } else {
      if (this->valve_output_->state != output_state) this->valve_output_->turn_off();
    }
    
    if (output_state != this->last_valve_state_) {
      ESP_LOGD(TAG, "Valve output turned %s (control_enabled=%s, valve_state=%s)", 
               output_state ? "ON" : "OFF",
               control_enabled ? "YES" : "NO",
               this->valve_state_ ? "ON" : "OFF");
    }
  }
#endif

  // Update switch
#ifdef USE_SWITCH
  if (this->valve_switch_ != nullptr) {    
    if (output_state ) {
      if (this->valve_switch_->state != output_state) this->valve_switch_->turn_on();
    } else {
      if (this->valve_switch_->state != output_state) this->valve_switch_->turn_off();
    }

    if (output_state != this->last_valve_state_) {
      ESP_LOGD(TAG, "Valve switch turned %s (control_enabled=%s, valve_state=%s)", 
               output_state ? "ON" : "OFF",
               control_enabled ? "YES" : "NO",
               this->valve_state_ ? "ON" : "OFF");
      this->publish_state();
    }
  }
#endif
  
  this->last_valve_state_ = output_state;

}

bool DummyThermostat::get_valve_control_enabled_() {
  if (this->valve_control_enabled_func_.has_value()) {
    return (*this->valve_control_enabled_func_)();
  }

  return this->valve_control_enabled_value_;
}

bool DummyThermostat::get_use_local_valve_control_() {
  if (this->use_local_valve_control_func_.has_value()) {
    return (*this->use_local_valve_control_func_)();
  }
  return this->use_local_valve_control_value_;
}

climate::ClimateTraits DummyThermostat::traits() {
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
  traits.set_visual_max_temperature(30.0f);
  traits.set_visual_temperature_step(0.5f);
  
  return traits;
}

void DummyThermostat::control(const climate::ClimateCall &call) {
  if (call.get_mode().has_value()) {
    this->mode = *call.get_mode();
  }
  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
  }
  if (this->get_use_local_valve_control_() || this->valve_timeout_active_) {
    this->calculate_local_valve_state_();
  }
  this->calculate_action_from_valve_and_mode_();
  this->update_valve_output_();
  this->publish_state();
}

void DummyThermostat::set_valve_state(bool state) 
{ 
    ESP_LOGI(TAG, "Valve updated by service %s", state ? "true" : "false");
    this->valve_state_ = state; 
    this->last_valve_state_update_ = esphome::millis();
  this->calculate_action_from_valve_and_mode_();
    this->update_valve_output_();
  this->publish_state();
}

void DummyThermostat::set_current_temperature(float value) 
{ 
    ESP_LOGI(TAG, "Current Temp updated by service %.2f", value);
    this->current_temperature= value; 
    this->last_temp_sensor_update_ = esphome::millis();
  if (this->get_use_local_valve_control_() || this->valve_timeout_active_) {
    this->calculate_local_valve_state_();
  }
  this->calculate_action_from_valve_and_mode_();
  this->update_valve_output_();
    this->publish_state();
}
void DummyThermostat::set_current_humidity(float value) 
{ 
    ESP_LOGI(TAG, "Current Hum updated by service %.2f", value);
    this->current_humidity= value; 
    this->last_humidity_sensor_update_ = esphome::millis();
  this->calculate_action_from_valve_and_mode_();
    this->publish_state();
}


}  // namespace dummy_thermostat
}  // namespace esphome