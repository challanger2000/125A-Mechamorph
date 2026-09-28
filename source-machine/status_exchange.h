#pragma once

#include "pluginterfaces/vst/ivstdataexchange.h"

namespace MechamorphMachine {
constexpr Steinberg::Vst::DataExchangeUserContextID kStatusExchangeContext = 0x125A4D01u;
struct StatusExchangeData {
    double pressure {0.0};
    double friction {0.0};
    double stall {0.0};
};
} // namespace MechamorphMachine
