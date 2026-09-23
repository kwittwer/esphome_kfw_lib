#include "ws2811_cwww_hub.h"

#include <algorithm>

#include "esphome/core/log.h"
#include "ws2811_cwww_boundary_number.h"
#include "ws2811_cwww_light.h"

namespace esphome::ws2811_cwww {

static const char *const TAG = "ws2811_cwww.hub";

namespace {

float clamp01(float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

float smoothstep(float value) {
  return value * value * value * (value * (value * 6.0f - 15.0f) + 10.0f);
}

float lerp(float start, float end, float progress) {
  return start + (end - start) * progress;
}

ChannelLevels black_levels() {
  return {0.0f, 0.0f};
}

}  // namespace

void WS2811CWWWHub::register_segment_light(WS2811CWWWLightOutput *light) {
  const auto index = static_cast<size_t>(light->get_segment_index());
  if (this->segment_lights_.size() <= index) {
    this->segment_lights_.resize(index + 1, nullptr);
  }
  this->segment_lights_[index] = light;
}

SegmentRange WS2811CWWWHub::get_segment_range(size_t segment_index) const {
  if (segment_index == 0) {
    return {0, this->boundaries_[0]};
  }
  if (segment_index >= this->boundaries_.size()) {
    return {this->boundaries_[segment_index - 1] + 1, this->active_leds_ - 1};
  }
  return {this->boundaries_[segment_index - 1] + 1, this->boundaries_[segment_index]};
}

int32_t WS2811CWWWHub::get_segment_size(size_t segment_index) const {
  auto range = this->get_segment_range(segment_index);
  return range.end - range.start + 1;
}

int32_t WS2811CWWWHub::set_boundary(size_t boundary_index, int32_t requested_value) {
  if (boundary_index >= this->boundaries_.size()) {
    return -1;
  }

  const int32_t min_value = boundary_index == 0 ? this->min_segment_length_ - 1 : this->boundaries_[boundary_index - 1] + this->min_segment_length_;
  const int32_t max_value = boundary_index + 1 < this->boundaries_.size()
                                ? this->boundaries_[boundary_index + 1] - this->min_segment_length_
                                : this->active_leds_ - this->min_segment_length_ - 1;
  const int32_t applied_value = std::min(std::max(requested_value, min_value), max_value);

  if (this->boundaries_[boundary_index] != applied_value) {
    const int32_t old_boundary = this->boundaries_[boundary_index];
    this->start_boundary_transition_(boundary_index, old_boundary, applied_value);
    this->boundaries_[boundary_index] = applied_value;
    this->notify_boundary_update_(boundary_index);
  } else {
    for (auto *number : this->boundary_numbers_) {
      if (number->get_boundary_index() == static_cast<int32_t>(boundary_index)) {
        number->publish_from_hub();
      }
    }
  }

  return applied_value;
}

int32_t WS2811CWWWHub::set_active_leds_runtime(int32_t requested_value) {
  const int32_t min_value = static_cast<int32_t>(this->get_segment_count()) * this->min_segment_length_;
  const int32_t applied_value = std::min(std::max(requested_value, min_value), this->num_leds_);

  if (applied_value == this->active_leds_) {
    return applied_value;
  }

  const int32_t previous_active = this->active_leds_;
  this->start_active_leds_transition_(previous_active, applied_value);
  this->active_leds_ = applied_value;
  this->normalize_boundaries_for_active_leds_();

  for (size_t index = 0; index < this->boundaries_.size(); index++) {
    for (auto *number : this->boundary_numbers_) {
      if (number->get_boundary_index() == static_cast<int32_t>(index)) {
        number->publish_from_hub();
      }
    }
  }
  for (auto *light : this->segment_lights_) {
    if (light != nullptr) {
      light->on_segment_bounds_changed();
    }
  }

  this->apply_transition_frame_(0.0f);

  return applied_value;
}

void WS2811CWWWHub::setup() {}

void WS2811CWWWHub::loop() {
  if (!this->boundary_transition_.active) {
    return;
  }

  const uint32_t duration = this->boundary_transition_.duration == 0 ? 1 : this->boundary_transition_.duration;
  const uint32_t elapsed = esphome::millis() - this->boundary_transition_.start_time;
  const float progress = smoothstep(clamp01(elapsed / static_cast<float>(duration)));

  this->apply_transition_frame_(progress);

  if (elapsed >= duration) {
    this->boundary_transition_.active = false;
  }
}

void WS2811CWWWHub::apply_transition_frame_(float progress) {
  if (!this->boundary_transition_.active) {
    return;
  }

  for (int32_t led_index = this->boundary_transition_.start_led; led_index <= this->boundary_transition_.end_led; led_index++) {
    const size_t offset = static_cast<size_t>(led_index - this->boundary_transition_.start_led);
    auto led = (*this->parent_)[led_index];
    const auto &source = this->boundary_transition_.source_levels[offset];
    const auto &target = this->boundary_transition_.target_levels[offset];
    led.set_red(light::to_uint8_scale(lerp(source.warm_white, target.warm_white, progress)));
    led.set_green(light::to_uint8_scale(lerp(source.cold_white, target.cold_white, progress)));
    led.set_blue(0);
    led.set_white(0);
  }

  this->parent_->schedule_show();
}

void WS2811CWWWHub::dump_config() {
  ESP_LOGCONFIG(TAG, "WS2811 CWWW Hub");
  ESP_LOGCONFIG(TAG, "  Max LEDs: %ld", static_cast<long>(this->num_leds_));
  ESP_LOGCONFIG(TAG, "  Active LEDs: %ld", static_cast<long>(this->active_leds_));
  ESP_LOGCONFIG(TAG, "  Min segment length: %ld", static_cast<long>(this->min_segment_length_));
  ESP_LOGCONFIG(TAG, "  Boundary transition length: %lums", static_cast<unsigned long>(this->boundary_transition_length_));
  ESP_LOGCONFIG(TAG, "  Active LEDs transition length: %lums", static_cast<unsigned long>(this->active_leds_transition_length_));
  for (size_t index = 0; index < this->boundaries_.size(); index++) {
    ESP_LOGCONFIG(TAG, "  Boundary %u: %ld", static_cast<unsigned>(index), static_cast<long>(this->boundaries_[index]));
  }
}

ChannelLevels WS2811CWWWHub::get_segment_levels_(size_t segment_index) const {
  if (segment_index >= this->segment_lights_.size() || this->segment_lights_[segment_index] == nullptr) {
    return {};
  }
  return this->segment_lights_[segment_index]->get_current_channel_levels();
}

ChannelLevels WS2811CWWWHub::get_parent_led_levels_(int32_t led_index) const {
  auto led = (*this->parent_)[led_index];
  return {
      led.get_green() / 255.0f,
      led.get_red() / 255.0f,
  };
}

void WS2811CWWWHub::normalize_boundaries_for_active_leds_() {
  const int32_t boundary_count = static_cast<int32_t>(this->boundaries_.size());
  int32_t previous_boundary = -1;
  for (int32_t index = 0; index < boundary_count; index++) {
    const int32_t min_value = previous_boundary + this->min_segment_length_;
    const int32_t remaining_segments = boundary_count - index;
    const int32_t max_value = this->active_leds_ - remaining_segments * this->min_segment_length_ - 1;
    int32_t clamped = std::max(this->boundaries_[index], min_value);
    clamped = std::min(clamped, max_value);
    this->boundaries_[index] = clamped;
    previous_boundary = clamped;
  }
}

void WS2811CWWWHub::clear_inactive_leds_(int32_t start_led) {
  for (int32_t led_index = start_led; led_index < this->num_leds_; led_index++) {
    auto led = (*this->parent_)[led_index];
    led.set_red(0);
    led.set_green(0);
    led.set_blue(0);
    led.set_white(0);
  }
  this->parent_->schedule_show();
}

void WS2811CWWWHub::start_boundary_transition_(size_t boundary_index, int32_t old_boundary, int32_t new_boundary) {
  if (this->boundary_transition_length_ == 0 || old_boundary == new_boundary) {
    return;
  }

  const bool moved_left = new_boundary < old_boundary;
  const int32_t start_led = moved_left ? new_boundary + 1 : old_boundary + 1;
  const int32_t end_led = moved_left ? old_boundary : new_boundary;
  if (end_led < start_led) {
    return;
  }

  this->start_uniform_transition_(
      start_led,
      end_led,
      moved_left ? this->get_segment_levels_(boundary_index) : this->get_segment_levels_(boundary_index + 1),
      moved_left ? this->get_segment_levels_(boundary_index + 1) : this->get_segment_levels_(boundary_index),
      this->boundary_transition_length_);
}

void WS2811CWWWHub::start_active_leds_transition_(int32_t previous_active_leds, int32_t new_active_leds) {
  if (this->active_leds_transition_length_ == 0 || previous_active_leds == new_active_leds) {
    if (new_active_leds < previous_active_leds) {
      this->clear_inactive_leds_(new_active_leds);
    }
    return;
  }

  if (new_active_leds > previous_active_leds) {
    const int32_t start_led = previous_active_leds;
    const int32_t end_led = new_active_leds - 1;
    if (end_led < start_led) {
      return;
    }

    this->start_uniform_transition_(
        start_led,
        end_led,
        black_levels(),
        this->get_segment_levels_(this->get_segment_count() - 1),
        this->active_leds_transition_length_);
    return;
  }

  const int32_t start_led = new_active_leds;
  const int32_t end_led = previous_active_leds - 1;
  if (end_led < start_led) {
    return;
  }

  std::vector<ChannelLevels> source_levels;
  std::vector<ChannelLevels> target_levels;
  source_levels.reserve(static_cast<size_t>(end_led - start_led + 1));
  target_levels.reserve(static_cast<size_t>(end_led - start_led + 1));
  for (int32_t led_index = start_led; led_index <= end_led; led_index++) {
    source_levels.push_back(this->get_parent_led_levels_(led_index));
    target_levels.push_back(black_levels());
  }
  this->start_mapped_transition_(start_led, end_led, std::move(source_levels), std::move(target_levels), this->active_leds_transition_length_);
}

void WS2811CWWWHub::start_uniform_transition_(int32_t start_led, int32_t end_led, ChannelLevels source,
                                              ChannelLevels target, uint32_t duration) {
  if (end_led < start_led) {
    return;
  }
  const size_t count = static_cast<size_t>(end_led - start_led + 1);
  std::vector<ChannelLevels> source_levels(count, source);
  std::vector<ChannelLevels> target_levels(count, target);
  this->start_mapped_transition_(start_led, end_led, std::move(source_levels), std::move(target_levels), duration);
}

void WS2811CWWWHub::start_mapped_transition_(int32_t start_led, int32_t end_led,
                                             std::vector<ChannelLevels> &&source_levels,
                                             std::vector<ChannelLevels> &&target_levels, uint32_t duration) {
  this->boundary_transition_.active = true;
  this->boundary_transition_.start_led = start_led;
  this->boundary_transition_.end_led = end_led;
  this->boundary_transition_.start_time = esphome::millis();
  this->boundary_transition_.duration = duration;
  this->boundary_transition_.source_levels = std::move(source_levels);
  this->boundary_transition_.target_levels = std::move(target_levels);
}

void WS2811CWWWHub::notify_boundary_update_(size_t boundary_index) {
  for (auto *number : this->boundary_numbers_) {
    if (number->get_boundary_index() == static_cast<int32_t>(boundary_index)) {
      number->publish_from_hub();
    }
  }

  for (auto *light : this->segment_lights_) {
    light->on_segment_bounds_changed();
  }
}

}  // namespace esphome::ws2811_cwww
