#include "ws2811_cwww_light.h"

#include "esphome/core/log.h"

namespace esphome::ws2811_cwww {

static const char *const TAG = "ws2811_cwww.light";

namespace {

struct WhiteLevels {
  float cold_white;
  float warm_white;
};

float clamp01(float value) {
  if (value < 0.0f) {
    return 0.0f;
  }
  if (value > 1.0f) {
    return 1.0f;
  }
  return value;
}

float lerp(float start, float end, float progress) {
  return start + (end - start) * progress;
}

uint32_t choose_duration(uint32_t configured, uint32_t fallback) {
  if (configured > 0) {
    return configured;
  }
  if (fallback > 0) {
    return fallback;
  }
  return 1;
}

bool has_visible_output(const WhiteLevels &levels) {
  return levels.cold_white > 0.0f || levels.warm_white > 0.0f;
}

int32_t ring_index_for_led(int32_t index, int32_t size) {
  const int32_t edge_distance = std::min(index, size - index - 1);
  const int32_t max_edge_distance = (size - 1) / 2;
  return max_edge_distance - edge_distance;
}

WhiteLevels compute_channel_levels(const light::LightColorValues &values, bool constant_brightness, bool include_brightness) {
  auto converted = values;
  if (!include_brightness) {
    converted.set_state(true);
    converted.set_brightness(1.0f);
  }

  WhiteLevels levels{};
  converted.as_cwww(&levels.cold_white, &levels.warm_white, constant_brightness);
  return levels;
}

}  // namespace

void WS2811CWWWLightTransformer::start() {
  if (this->light_.is_effect_active()) {
    return;
  }

  this->light_.correction_.set_local_brightness(255);

  const auto start_levels = compute_channel_levels(
      this->get_start_values(), this->light_.is_constant_brightness(), true);
  const auto target_levels = compute_channel_levels(
      this->get_target_values(), this->light_.is_constant_brightness(), true);

  this->start_cold_white_ = start_levels.cold_white;
  this->start_warm_white_ = start_levels.warm_white;
  this->target_cold_white_ = target_levels.cold_white;
  this->target_warm_white_ = target_levels.warm_white;

  const bool start_visible = has_visible_output(start_levels);
  const bool target_visible = has_visible_output(target_levels);
  this->center_out_transition_ = this->light_.is_center_power_transition() && start_visible != target_visible;
  this->turning_on_ = !start_visible && target_visible;
  this->ring_count_ = std::max<int32_t>(1, (this->light_.size() + 1) / 2);

  if (this->center_out_transition_) {
    const float overlap = clamp01(this->light_.get_center_power_transition_overlap());
    this->ring_duration_fraction_ = 1.0f / (1.0f + (this->ring_count_ - 1) * (1.0f - overlap));
    this->ring_offset_fraction_ = this->ring_duration_fraction_ * (1.0f - overlap);
    this->effective_length_ = this->turning_on_
                                  ? choose_duration(this->light_.get_power_on_transition_length(), this->length_)
                                  : choose_duration(this->light_.get_power_off_transition_length(), this->length_);
  } else {
    this->ring_duration_fraction_ = 1.0f;
    this->ring_offset_fraction_ = 1.0f;
    this->effective_length_ = choose_duration(0, this->length_);
  }
}

float WS2811CWWWLightTransformer::get_effective_progress_() const {
  if (this->effective_length_ == 0) {
    return 1.0f;
  }

  const uint32_t elapsed = esphome::millis() - this->start_time_;
  if (elapsed >= this->effective_length_) {
    return 1.0f;
  }

  return clamp01(elapsed / static_cast<float>(this->effective_length_));
}

bool WS2811CWWWLightTransformer::is_finished() {
  return this->get_effective_progress_() >= 1.0f;
}

optional<light::LightColorValues> WS2811CWWWLightTransformer::apply() {
  const float smoothed_progress = LightTransformer::smoothed_progress(this->get_effective_progress_());

  if (this->light_.is_effect_active()) {
    return light::LightColorValues::lerp(this->get_start_values(), this->get_target_values(), smoothed_progress);
  }

  const float cold_white = this->start_cold_white_ +
                           (this->target_cold_white_ - this->start_cold_white_) * smoothed_progress;
  const float warm_white = this->start_warm_white_ +
                           (this->target_warm_white_ - this->start_warm_white_) * smoothed_progress;

  if (!this->center_out_transition_) {
    for (auto led : this->light_) {
      led.set_red(light::to_uint8_scale(warm_white));
      led.set_green(light::to_uint8_scale(cold_white));
      led.set_blue(0);
      led.set_white(0);
    }

    this->light_.schedule_show();
    return {};
  }

  for (int32_t index = 0; index < this->light_.size(); index++) {
    const int32_t ring_index = ring_index_for_led(index, this->light_.size());
    const int32_t phase_index = this->turning_on_ ? ring_index : (this->ring_count_ - 1 - ring_index);
    const float local_progress = clamp01(
        (smoothed_progress - phase_index * this->ring_offset_fraction_) /
        this->ring_duration_fraction_);
    const float local_cold_white = lerp(this->start_cold_white_, this->target_cold_white_, local_progress);
    const float local_warm_white = lerp(this->start_warm_white_, this->target_warm_white_, local_progress);

    auto led = this->light_[index];
    led.set_red(light::to_uint8_scale(local_warm_white));
    led.set_green(light::to_uint8_scale(local_cold_white));
    led.set_blue(0);
    led.set_white(0);
  }

  this->light_.schedule_show();
  return {};
}

void WS2811CWWWLightOutput::clear_effect_data() {
  for (auto led : this->all()) {
    led.set_effect_data(0);
  }
}

light::LightTraits WS2811CWWWLightOutput::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::COLD_WARM_WHITE});
  traits.set_min_mireds(this->cold_white_temperature_);
  traits.set_max_mireds(this->warm_white_temperature_);
  return traits;
}

std::unique_ptr<light::LightTransformer> WS2811CWWWLightOutput::create_default_transition() {
  return make_unique<WS2811CWWWLightTransformer>(*this);
}

void WS2811CWWWLightOutput::update_state(light::LightState *state) {
  const auto values = state->current_values;
  this->correction_.set_local_brightness(light::to_uint8_scale(values.get_brightness() * values.get_state()));

  if (this->is_effect_active()) {
    return;
  }

  const auto levels = compute_channel_levels(values, this->constant_brightness_, false);
  for (auto led : this->all()) {
    led.set_red(light::to_uint8_scale(levels.warm_white));
    led.set_green(light::to_uint8_scale(levels.cold_white));
    led.set_blue(0);
    led.set_white(0);
  }

  this->schedule_show();
}

void WS2811CWWWLightOutput::write_state(light::LightState *state) {
  this->hub_->get_parent()->schedule_show();
  this->mark_shown_();
}

void WS2811CWWWLightOutput::on_segment_bounds_changed() {
  if (this->state_parent_ == nullptr) {
    return;
  }

  this->update_state(this->state_parent_);
  this->write_state(this->state_parent_);
}

ChannelLevels WS2811CWWWLightOutput::get_current_channel_levels() const {
  ChannelLevels levels{};
  if (this->state_parent_ == nullptr) {
    return levels;
  }

  this->state_parent_->current_values_as_cwww(&levels.cold_white, &levels.warm_white, this->constant_brightness_);
  return levels;
}

void WS2811CWWWLightOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "WS2811 CWWW Segment Light");
  auto range = this->hub_->get_segment_range(this->segment_index_);
  ESP_LOGCONFIG(TAG, "  Segment index: %ld", static_cast<long>(this->segment_index_));
  ESP_LOGCONFIG(TAG, "  Segment start: %ld", static_cast<long>(range.start));
  ESP_LOGCONFIG(TAG, "  Segment end: %ld", static_cast<long>(range.end));
  ESP_LOGCONFIG(TAG, "  Segment size: %ld", static_cast<long>(this->size()));
  ESP_LOGCONFIG(TAG, "  Reversed: %s", YESNO(this->reversed_));
  ESP_LOGCONFIG(TAG, "  Cold white temperature: %.1f mireds", this->cold_white_temperature_);
  ESP_LOGCONFIG(TAG, "  Warm white temperature: %.1f mireds", this->warm_white_temperature_);
  ESP_LOGCONFIG(TAG, "  Constant brightness: %s", YESNO(this->constant_brightness_));
  ESP_LOGCONFIG(TAG, "  Center power transition: %s", YESNO(this->center_power_transition_));
  ESP_LOGCONFIG(TAG, "  Center power overlap: %.0f%%", this->center_power_transition_overlap_ * 100.0f);
  ESP_LOGCONFIG(TAG, "  Power-on transition length: %lums", static_cast<unsigned long>(this->power_on_transition_length_));
  ESP_LOGCONFIG(TAG, "  Power-off transition length: %lums", static_cast<unsigned long>(this->power_off_transition_length_));
}

light::ESPColorView WS2811CWWWLightOutput::get_view_internal(int32_t index) const {
  auto range = this->hub_->get_segment_range(this->segment_index_);
  int32_t parent_index = range.start + index;
  if (this->reversed_) {
    parent_index = range.end - index;
  }

  auto view = (*this->hub_->get_parent())[parent_index];
  view.raw_set_color_correction(&this->correction_);
  return view;
}

}  // namespace esphome::ws2811_cwww
