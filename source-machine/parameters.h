#pragma once
#include "pluginterfaces/vst/vsttypes.h"

namespace MechamorphMachine {
enum ParamIds : Steinberg::Vst::ParamID {
    kMachine = 2000,
    kSpeed   = 2001,
    kLoad    = 2002,
    kAction  = 2003,
    kWear    = 2004,
    kOutput  = 2005
};
inline constexpr int kParamCount = 6;
} // namespace MechamorphMachine
