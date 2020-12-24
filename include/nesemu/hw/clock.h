#pragma once

#include <nesemu/utils/steady_timer.h>

namespace hw::clock {
class CPUClock : public utils::SteadyTimer<22, 39375000> {};  // ~1.79MHz
}  // namespace hw::clock
