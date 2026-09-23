#pragma once

#include "esphome/components/light/addressable_light.h"

#include "ws2811_cwww_hub.h"

namespace esphome::ws2811_cwww {

class WS2811CWWWLightOutput;

class WS2811CWWWLightTransformer final : public light::LightTransformer {
 public:
  explicit WS2811CWWWLightTransformer(WS2811CWWWLightOutput &light) : light_(light) {}

  void start() override;
  optional<light::LightColorValues> apply() override;
  bool is_finished() override;

 protected:
  float get_effective_progress_() const;

  WS2811CWWWLightOutput &light_;
  float start_cold_white_{0.0f};
  float start_warm_white_{0.0f};
  float target_cold_white_{0.0f};
  float target_warm_white_{0.0f};
  bool center_out_transition_{false};
  bool turning_on_{false};
  int32_t ring_count_{1};
  float ring_duration_fraction_{1.0f};
  float ring_offset_fraction_{1.0f};
  uint32_t effective_length_{0};
};

class WS2811CWWWLightOutput final : public light::AddressableLight {
 public:
  WS2811CWWWLightOutput(WS2811CWWWHub *hub, int32_t segment_index, bool reversed)
      : hub_(hub),
        segment_index_(segment_index),
        reversed_(reversed) {}

  int32_t size() const override { return this->hub_->get_segment_size(this->segment_index_); }
  void clear_effect_data() override;
  light::LightTraits get_traits() override;
  std::unique_ptr<light::LightTransformer> create_default_transition() override;
  void update_state(light::LightState *state) override;
  void write_state(light::LightState *state) override;
  void dump_config() override;
  void on_segment_bounds_changed();
  int32_t get_segment_index() const { return this->segment_index_; }
  ChannelLevels get_current_channel_levels() const;

  void set_cold_white_temperature(float cold_white_temperature) {
    this->cold_white_temperature_ = cold_white_temperature;
  }
  void set_warm_white_temperature(float warm_white_temperature) {
    this->warm_white_temperature_ = warm_white_temperature;
  }
  void set_constant_brightness(bool constant_brightness) {
    this->constant_brightness_ = constant_brightness;
  }
  bool is_constant_brightness() const { return this->constant_brightness_; }
  void set_center_power_transition(bool center_power_transition) {
    this->center_power_transition_ = center_power_transition;
  }
  bool is_center_power_transition() const {
    return this->center_power_transition_;
  }
  void set_center_power_transition_overlap(float overlap) {
    this->center_power_transition_overlap_ = overlap;
  }
  float get_center_power_transition_overlap() const {
    return this->center_power_transition_overlap_;
  }
  void set_power_on_transition_length(uint32_t power_on_transition_length) {
    this->power_on_transition_length_ = power_on_transition_length;
  }
  uint32_t get_power_on_transition_length() const {
    return this->power_on_transition_length_;
  }
  void set_power_off_transition_length(uint32_t power_off_transition_length) {
    this->power_off_transition_length_ = power_off_transition_length;
  }
  uint32_t get_power_off_transition_length() const {
    return this->power_off_transition_length_;
  }

 protected:
  friend class WS2811CWWWLightTransformer;

  light::ESPColorView get_view_internal(int32_t index) const override;

  WS2811CWWWHub *hub_;
  int32_t segment_index_;
  bool reversed_;
  float cold_white_temperature_{153.84615f};
  float warm_white_temperature_{370.37036f};
  bool constant_brightness_{false};
  bool center_power_transition_{true};
  float center_power_transition_overlap_{0.4f};
  uint32_t power_on_transition_length_{1000};
  uint32_t power_off_transition_length_{1000};
};

}  // namespace esphome::ws2811_cwww
