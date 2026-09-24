#include "status_bin_sensor.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

namespace esphome {
namespace status_bin_sensor {

static const char *const TAG = "status_bin_sensor";

StatusBinSensor *global_status_bin_sensor = nullptr;

unsigned long error_set_time_ = 0;
unsigned long warning_set_time_ = 0;


StatusBinSensor::StatusBinSensor() { global_status_bin_sensor = this; }

void StatusBinSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "Status Binary Sensor:");
  if (error_flag_ != nullptr)
    ESP_LOGCONFIG(TAG, "  Error Flag sensor configured");
  if (warning_flag_ != nullptr)
    ESP_LOGCONFIG(TAG, "  Warning Flag sensor configured");
}

void StatusBinSensor::loop() {
  bool error = (App.get_app_state() & STATUS_LED_ERROR) != 0u;
  bool warning = (App.get_app_state() & STATUS_LED_WARNING) != 0u;
 // if (error)ESP_LOGI("main", "eror is 1");
 // else ESP_LOGI("main", "eror is 0");

  if (error_flag_ != nullptr) {
 //   ESP_LOGI("main", "eror test");
    error_flag_->publish_state(error);
    if (error) error_set_time_ = millis();
    else if (millis() - error_set_time_ > 5000) error_flag_->publish_state(false);
  }

  if (warning_flag_ != nullptr) {
 //   ESP_LOGI("main", "warn test");
    warning_flag_->publish_state(warning);
    if (warning) warning_set_time_ = millis();
    else if (millis() - warning_set_time_ > 5000) warning_flag_->publish_state(false);
  }

}

float StatusBinSensor::get_setup_priority() const { return setup_priority::HARDWARE; }
float StatusBinSensor::get_loop_priority() const { return 50.0f; }

}  // namespace status_bin_sensor
}  // namespace esphome
