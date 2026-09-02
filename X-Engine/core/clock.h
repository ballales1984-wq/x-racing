#pragma once

#include <chrono>
#include <cstdint>

namespace xe {

class Clock {
public:
    void Reset();

    float Tick();

    float GetDeltaTime() const { return delta_time_; }
    float GetTotalTime() const { return total_time_; }
    uint64_t GetFrameCount() const { return frame_count_; }
    float GetFPS() const;

private:
    using ClockImpl = std::chrono::high_resolution_clock;
    using TimePoint = ClockImpl::time_point;

    TimePoint start_time_{};
    TimePoint last_time_{};
    float delta_time_ = 0.0f;
    float total_time_ = 0.0f;
    uint64_t frame_count_ = 0;
};

}  // namespace xe
