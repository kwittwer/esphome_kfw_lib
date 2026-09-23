#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome::ws2811_cwww {

class WS2811CWWWHub;

class WS2811CWWWSegmentBoundaryNumber final : public number::Number, public Component {
 public:
  WS2811CWWWSegmentBoundaryNumber(WS2811CWWWHub *hub, int32_t boundary_index)
      : hub_(hub), boundary_index_(boundary_index) {}

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  int32_t get_boundary_index() const { return this->boundary_index_; }
  bool is_active_leds_control() const { return this->boundary_index_ < 0; }
  void publish_from_hub();

 protected:
  void control(float value) override;

  WS2811CWWWHub *hub_;
  int32_t boundary_index_;
  ESPPreferenceObject pref_;
};

}  // namespace esphome::ws2811_cwww
