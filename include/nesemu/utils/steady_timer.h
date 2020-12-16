#pragma once

#include <nesemu/utils/lcm.h>

#include <chrono>
#include <thread>


namespace utils {

template <intmax_t Num, intmax_t Den = 1>
class SteadyTimer {
public:
  void start() { next_ = Clock::now() + Period(1); }

  void sleep() {
    std::this_thread::sleep_until(next_);
    next_ += Period(1);
  }

  bool ready() {
    if (Clock::now() > next_) {
      next_ += Period(1);
      return true;
    }
    return false;
  }

private:
  using Clock     = std::chrono::steady_clock;
  using Period    = std::chrono::duration<Clock::rep, std::ratio<Num, Den>>;
  using TimePoint = std::chrono::time_point<
      Clock,
      std::chrono::duration<Clock::rep, std::ratio<1, lcm::lcm(Clock::duration::period::den, Period::period::den)>>>;

  TimePoint next_;
};
}  // namespace utils
