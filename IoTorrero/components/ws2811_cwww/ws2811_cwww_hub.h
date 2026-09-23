#pragma once

#include <cstddef>
#include <vector>

#include "esphome/components/light/addressable_light.h"
#include "esphome/core/component.h"

namespace esphome::ws2811_cwww {

class WS2811CWWWLightOutput;
class WS2811CWWWSegmentBoundaryNumber;

struct SegmentRange {
  int32_t start;
  int32_t end;
};

struct ChannelLevels {
  float cold_white;
  float warm_white;
};

struct BoundaryTransition {
  bool active{false};
  int32_t start_led{0};
  int32_t end_led{0};
  uint32_t start_time{0};
  uint32_t duration{0};
  std::vector<ChannelLevels> source_levels{};
  std::vector<ChannelLevels> target_levels{};
};

class WS2811CWWWHub final : public Component {
 public:
  explicit WS2811CWWWHub(light::LightState *parent_state)
      : parent_(static_cast<light::AddressableLight *>(parent_state->get_output())) {}

  void set_num_leds(int32_t num_leds) { this->num_leds_ = num_leds; }
  void set_active_leds(int32_t active_leds) { this->active_leds_ = active_leds; }
  void set_boundary_transition_length(uint32_t boundary_transition_length) {
    this->boundary_transition_length_ = boundary_transition_length;
  }
  void set_active_leds_transition_length(uint32_t active_leds_transition_length) {
    this->active_leds_transition_length_ = active_leds_transition_length;
  }
  void set_min_segment_length(int32_t min_segment_length) {
    this->min_segment_length_ = min_segment_length;
  }
  void add_boundary(int32_t boundary) { this->boundaries_.push_back(boundary); }
  void register_segment_light(WS2811CWWWLightOutput *light);
  void register_boundary_number(WS2811CWWWSegmentBoundaryNumber *number) {
    this->boundary_numbers_.push_back(number);
  }

  light::AddressableLight *get_parent() const { return this->parent_; }
  int32_t get_num_leds() const { return this->num_leds_; }
  int32_t get_active_leds() const { return this->active_leds_; }
  int32_t get_min_segment_length() const { return this->min_segment_length_; }
  size_t get_segment_count() const { return this->boundaries_.size() + 1; }
  size_t get_boundary_count() const { return this->boundaries_.size(); }
  int32_t get_boundary(size_t index) const { return this->boundaries_[index]; }
  SegmentRange get_segment_range(size_t segment_index) const;
  int32_t get_segment_size(size_t segment_index) const;
  int32_t set_boundary(size_t boundary_index, int32_t requested_value);
  int32_t set_active_leds_runtime(int32_t requested_value);

  void setup() override;
  void loop() override;
  void dump_config() override;

 protected:
  void apply_transition_frame_(float progress);
  void start_uniform_transition_(int32_t start_led, int32_t end_led, ChannelLevels source, ChannelLevels target, uint32_t duration);
  void start_mapped_transition_(int32_t start_led, int32_t end_led, std::vector<ChannelLevels> &&source_levels,
                                std::vector<ChannelLevels> &&target_levels, uint32_t duration);
  void start_boundary_transition_(size_t boundary_index, int32_t old_boundary, int32_t new_boundary);
  void start_active_leds_transition_(int32_t previous_active_leds, int32_t new_active_leds);
  ChannelLevels get_segment_levels_(size_t segment_index) const;
  ChannelLevels get_parent_led_levels_(int32_t led_index) const;
  void normalize_boundaries_for_active_leds_();
  void clear_inactive_leds_(int32_t start_led);
  void notify_boundary_update_(size_t boundary_index);

  light::AddressableLight *parent_;
  int32_t num_leds_{0};
  int32_t active_leds_{0};
  uint32_t boundary_transition_length_{700};
  uint32_t active_leds_transition_length_{900};
  int32_t min_segment_length_{1};
  std::vector<int32_t> boundaries_;
  std::vector<WS2811CWWWLightOutput *> segment_lights_;
  std::vector<WS2811CWWWSegmentBoundaryNumber *> boundary_numbers_;
  BoundaryTransition boundary_transition_{};
};

}  // namespace esphome::ws2811_cwww
