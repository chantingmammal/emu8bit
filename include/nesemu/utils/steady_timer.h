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
    // If skipping, call ready() to ensure that next_ is updated correctly
    if (skip_) {
      ready();
    } else {
      std::this_thread::sleep_until(next_);
      next_ += Period(1);
    }
  }

  bool ready() {
    if (Clock::now() > next_) {
      next_ += Period(1);
      return true;
    }
    return false;
  }

  void skip(bool skip) { skip_ = skip; }

private:
  using Clock     = std::chrono::steady_clock;
  using Period    = std::chrono::duration<Clock::rep, std::ratio<Num, Den>>;
  using TimePoint = std::chrono::time_point<
      Clock,
      std::chrono::duration<Clock::rep, std::ratio<1, lcm::lcm(Clock::duration::period::den, Period::period::den)>>>;

  bool      skip_ = {false};
  TimePoint next_;
};
}  // namespace utils
