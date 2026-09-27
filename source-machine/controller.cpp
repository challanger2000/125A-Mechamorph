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
constexpr int32 kStateVersion = 3;
}

tresult PLUGIN_API Controller::initialize(FUnknown* context) {
    auto r = EditController::initialize(context);
    if (r != kResultOk)
        return r;

    auto* machine = new StringListParameter(
        STR16("Machine"), kMachine, nullptr,
        ParameterInfo::kCanAutomate | ParameterInfo::kIsList);
    machine->appendString(STR16("Tiny"));
    machine->appendString(STR16("Clockwork"));
    machine->appendString(STR16("Heavy"));
    machine->appendString(STR16("Colossal"));
    machine->appendString(STR16("Pneumatic"));
    machine->appendString(STR16("Broken"));
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
    addPercent(STR16("Scale"), kScale, 0.35);
    addPercent(STR16("Body"), kBody, 0.28);
    addPercent(STR16("Space"), kSpace, 0.18);

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
    if (!s.readInt32(version))
        return kResultFalse;

    float machine=0.0f, speed=0.32f, load=0.20f, action=0.48f;
    float wear=0.18f, scale=0.35f, body=0.28f, space=0.18f, output=0.38f;

    if (version == 1) {
        if (!s.readFloat(machine) ||
            !s.readFloat(speed) ||
            !s.readFloat(load) ||
            !s.readFloat(action) ||
            !s.readFloat(wear) ||
            !s.readFloat(scale) ||
            !s.readFloat(output))
            return kResultFalse;
    } else if (version == 2) {
        if (!s.readFloat(machine) ||
            !s.readFloat(speed) ||
            !s.readFloat(load) ||
            !s.readFloat(action) ||
            !s.readFloat(wear) ||
            !s.readFloat(scale) ||
            !s.readFloat(space) ||
            !s.readFloat(output))
            return kResultFalse;
    } else if (version == kStateVersion) {
        if (!s.readFloat(machine) ||
            !s.readFloat(speed) ||
            !s.readFloat(load) ||
            !s.readFloat(action) ||
            !s.readFloat(wear) ||
            !s.readFloat(scale) ||
            !s.readFloat(body) ||
            !s.readFloat(space) ||
            !s.readFloat(output))
            return kResultFalse;
    } else {
        return kResultFalse;
    }

    setParamNormalized(kMachine, std::clamp<double>(machine,0.0,1.0));
    setParamNormalized(kSpeed, std::clamp<double>(speed,0.0,1.0));
    setParamNormalized(kLoad, std::clamp<double>(load,0.0,1.0));
    setParamNormalized(kAction, std::clamp<double>(action,0.0,1.0));
    setParamNormalized(kWear, std::clamp<double>(wear,0.0,1.0));
    setParamNormalized(kScale, std::clamp<double>(scale,0.0,1.0));
    setParamNormalized(kBody, std::clamp<double>(body,0.0,1.0));
    setParamNormalized(kSpace, std::clamp<double>(space,0.0,1.0));
    setParamNormalized(kOutput, std::clamp<double>(output,0.0,1.0));

    return kResultOk;
}

} // namespace MechamorphMachine
