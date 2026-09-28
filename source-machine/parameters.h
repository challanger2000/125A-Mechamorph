#pragma once
#include "pluginterfaces/vst/vsttypes.h"

namespace MechamorphMachine {
enum ParamIds : Steinberg::Vst::ParamID {
    kMachine = 2000,
    kSpeed   = 2001,
    kLoad    = 2002,
    kAction  = 2003,
    kWear    = 2004,
    kScale   = 2005,
    kSpace   = 2006,
    kOutput  = 2007,
    kBody    = 2008,
    kPressureStatus = 2100,
    kFrictionStatus = 2101,
    kStallStatus    = 2102
};
inline constexpr int kParamCount = 9;
inline constexpr int kStatusParamCount = 3;
} // namespace MechamorphMachine
