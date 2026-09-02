#include "core/clock.h"

namespace xe {

void Clock::Reset() {
    start_time_ = ClockImpl::now();
    last_time_ = start_time_;
    delta_time_ = 0.0f;
    total_time_ = 0.0f;
    frame_count_ = 0;
}

float Clock::Tick() {
    TimePoint now = ClockImpl::now();

    float dt = std::chrono::duration<float>(now - last_time_).count();
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.1f) dt = 0.1f;

    delta_time_ = dt;
    total_time_ = std::chrono::duration<float>(now - start_time_).count();

    last_time_ = now;
    frame_count_++;

    return delta_time_;
}

float Clock::GetFPS() const {
    if (delta_time_ <= 0.0f) return 0.0f;
    return 1.0f / delta_time_;
}

}  // namespace xe
