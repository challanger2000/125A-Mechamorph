#include "controller.h"
#include "parameters.h"
#include "state_format.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"

#include <algorithm>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Mechamorph {

tresult PLUGIN_API Controller::initialize(FUnknown* context) {
    auto r = EditController::initialize(context);
    if (r != kResultOk) return r;

    auto addPercent = [&](const TChar* title, ParamID id, double def) {
        auto* p = new RangeParameter(title, id, STR16("%"), 0.0, 100.0, def * 100.0);
        p->setPrecision(0);
        parameters.addParameter(p);
    };

    addPercent(STR16("Mechanize"), kMechanize, 0.35);
    addPercent(STR16("Crank"), kCrank, 0.35);
    addPercent(STR16("Clatter"), kClatter, 0.25);
    addPercent(STR16("Wobble"), kWobble, 0.15);
    addPercent(STR16("Air"), kAir, 0.20);
    addPercent(STR16("Body"), kBody, 0.35);
    addPercent(STR16("Wear"), kWear, 0.20);

    auto* output = new RangeParameter(STR16("Output"), kOutput, STR16("dB"), -12.0, 12.0, 0.0);
    output->setPrecision(1);
    parameters.addParameter(output);

    auto* bypass = new StringListParameter(
        STR16("Bypass"), kBypass, nullptr,
        ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass | ParameterInfo::kIsList);
    bypass->appendString(STR16("Off"));
    bypass->appendString(STR16("On"));
    parameters.addParameter(bypass);

    return kResultOk;
}

tresult PLUGIN_API Controller::setComponentState(IBStream* state) {
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    float values[kStateValueCount] {};
    int32 bp = 0;
    if (!readState(s, values, bp))
        return kResultFalse;

    const ParamID ids[kStateValueCount] = {
        kMechanize, kCrank, kClatter, kWobble,
        kAir, kBody, kWear, kOutput
    };
    for (int i = 0; i < kStateValueCount; ++i)
        setParamNormalized(ids[i], std::clamp<double>(values[i], 0.0, 1.0));
    setParamNormalized(kBypass, bp ? 1.0 : 0.0);
    return kResultOk;
}

} // namespace Mechamorph
