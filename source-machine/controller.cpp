#include "controller.h"
#include "parameters.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace MechamorphMachine {
namespace {
constexpr int32 kStateVersion = 1;
}

tresult PLUGIN_API Controller::initialize(FUnknown* context) {
    auto r = EditController::initialize(context);
    if (r != kResultOk)
        return r;

    auto* machine = new StringListParameter(
        STR16("Machine"), kMachine, nullptr,
        ParameterInfo::kCanAutomate | ParameterInfo::kIsList);
    machine->appendString(STR16("Projector"));
    machine->appendString(STR16("Handcrank"));
    machine->appendString(STR16("Industrial"));
    parameters.addParameter(machine);

    auto addPercent = [&](const TChar* title, ParamID id, double def) {
        auto* p = new RangeParameter(title, id, STR16("%"), 0.0, 100.0, def * 100.0);
        p->setPrecision(0);
        parameters.addParameter(p);
    };

    addPercent(STR16("Speed"), kSpeed, 0.32);
    addPercent(STR16("Load"), kLoad, 0.20);
    addPercent(STR16("Action"), kAction, 0.48);
    addPercent(STR16("Wear"), kWear, 0.18);

    auto* output = new RangeParameter(
        STR16("Output"), kOutput, STR16("dB"), -12.0, 12.0, -2.88);
    output->setPrecision(1);
    parameters.addParameter(output);

    return kResultOk;
}

tresult PLUGIN_API Controller::setComponentState(IBStream* state) {
    if (!state) return kResultFalse;

    IBStreamer s(state, kLittleEndian);
    int32 version = 0;
    float values[kParamCount]{};

    if (!s.readInt32(version) || version != kStateVersion)
        return kResultFalse;

    for (float& v : values) {
        if (!s.readFloat(v))
            return kResultFalse;
    }

    const ParamID ids[kParamCount] = {
        kMachine, kSpeed, kLoad, kAction, kWear, kOutput
    };

    for (int i = 0; i < kParamCount; ++i)
        setParamNormalized(ids[i], std::clamp<double>(values[i], 0.0, 1.0));

    return kResultOk;
}

} // namespace MechamorphMachine
