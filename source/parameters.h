#pragma once
#include "pluginterfaces/vst/vsttypes.h"

namespace Mechamorph {

enum ParamIds : Steinberg::Vst::ParamID {
    kMechanize = 1000,
    kCrank = 1001,
    kClatter = 1002,
    kWobble = 1003,
    kAir = 1004,
    kBody = 1005,
    kWear = 1006,
    kOutput = 1007,
    kBypass = 1008
};

} // namespace Mechamorph
