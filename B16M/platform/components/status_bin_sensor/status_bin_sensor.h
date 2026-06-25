#pragma once

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace status_bin_sensor {

class StatusBinSensor : public Component {
 public:
  StatusBinSensor();

  void dump_config() override;
  void loop() override;
  float get_setup_priority() const override;
  float get_loop_priority() const override;

 protected:
  binary_sensor::BinarySensor *error_flag_{nullptr};
  binary_sensor::BinarySensor *warning_flag_{nullptr};

 public:
  void set_error_flag(binary_sensor::BinarySensor *sensor) { error_flag_ = sensor; }
  void set_warning_flag(binary_sensor::BinarySensor *sensor) { warning_flag_ = sensor; }
};

extern StatusBinSensor *global_status_bin_sensor;

}  // namespace status_bin_sensor
}  // namespace esphome
