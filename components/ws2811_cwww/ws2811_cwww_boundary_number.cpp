#include "ws2811_cwww_boundary_number.h"

#include <cmath>

#include "esphome/core/log.h"
#include "ws2811_cwww_hub.h"

namespace esphome::ws2811_cwww {

static const char *const TAG = "ws2811_cwww.boundary";

void WS2811CWWWSegmentBoundaryNumber::setup() {
  if (this->is_active_leds_control()) {
    this->traits.set_min_value(static_cast<float>(this->hub_->get_segment_count()));
    this->traits.set_max_value(static_cast<float>(this->hub_->get_num_leds()));
  } else {
    this->traits.set_min_value(0);
    this->traits.set_max_value(this->hub_->get_num_leds() - 2);
  }
  this->traits.set_step(1);

  this->pref_ = this->make_entity_preference<float>();
  float restored_value;
  if (this->pref_.load(&restored_value)) {
    if (this->is_active_leds_control()) {
      this->hub_->set_active_leds_runtime(static_cast<int32_t>(lroundf(restored_value)));
    } else {
      this->hub_->set_boundary(this->boundary_index_, static_cast<int32_t>(lroundf(restored_value)));
    }
  }

  this->publish_from_hub();
}

void WS2811CWWWSegmentBoundaryNumber::dump_config() {
  LOG_NUMBER("", this->is_active_leds_control() ? "WS2811 CWWW Active LEDs" : "WS2811 CWWW Boundary", this);
  if (!this->is_active_leds_control()) {
    ESP_LOGCONFIG(TAG, "  Boundary index: %ld", static_cast<long>(this->boundary_index_));
  }
}

void WS2811CWWWSegmentBoundaryNumber::publish_from_hub() {
  if (this->is_active_leds_control()) {
    this->publish_state(static_cast<float>(this->hub_->get_active_leds()));
  } else {
    this->publish_state(static_cast<float>(this->hub_->get_boundary(this->boundary_index_)));
  }
}

void WS2811CWWWSegmentBoundaryNumber::control(float value) {
  const int32_t requested_value = static_cast<int32_t>(lroundf(value));
  const int32_t applied_value = this->is_active_leds_control()
                                  ? this->hub_->set_active_leds_runtime(requested_value)
                                  : this->hub_->set_boundary(this->boundary_index_, requested_value);
  if (applied_value >= 0) {
    const float saved_value = static_cast<float>(applied_value);
    this->pref_.save(&saved_value);
    this->publish_state(static_cast<float>(applied_value));
  }
}

}  // namespace esphome::ws2811_cwww