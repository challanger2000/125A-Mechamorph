#include "processor.h"
#include "ids.h"
#include "parameters.h"
#include "state_format.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace Mechamorph {

Processor::Processor() {
    setControllerClass(ControllerUID);
}

tresult PLUGIN_API Processor::initialize(FUnknown* context) {
    auto r = AudioEffect::initialize(context);
    if (r != kResultOk) return r;
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup) {
    auto r = AudioEffect::setupProcessing(setup);
    if (r != kResultOk) return r;
    sampleRate_ = setup.sampleRate > 1.0 ? setup.sampleRate : 48000.0;
    core_.prepare(sampleRate_, static_cast<std::size_t>(std::max(1, setup.maxSamplesPerBlock)));
    updateCoreParameters();
    return kResultOk;
}

tresult PLUGIN_API Processor::setActive(TBool state) {
    if (state)
        core_.reset();
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 symbolicSampleSize) {
    return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API Processor::setBusArrangements(
    SpeakerArrangement* inputs, int32 numIns,
    SpeakerArrangement* outputs, int32 numOuts) {
    if (!inputs || !outputs || numIns != 1 || numOuts != 1)
        return kResultFalse;

    const bool mono = inputs[0] == SpeakerArr::kMono && outputs[0] == SpeakerArr::kMono;
    const bool stereo = inputs[0] == SpeakerArr::kStereo && outputs[0] == SpeakerArr::kStereo;
    if (!mono && !stereo)
        return kResultFalse;

    return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

void Processor::applyParameter(ParamID id, float normalized) {
    const float v = std::clamp(std::isfinite(normalized) ? normalized : 0.f, 0.f, 1.f);
    switch (id) {
        case kMechanize: coreParams_.mechanize = v; break;
        case kCrank: coreParams_.crank = v; break;
        case kClatter: coreParams_.clatter = v; break;
        case kWobble: coreParams_.wobble = v; break;
        case kAir: coreParams_.air = v; break;
        case kBody: coreParams_.body = v; break;
        case kWear: coreParams_.wear = v; break;
        case kOutput: coreParams_.output = v; break;
        case kBypass: {
            const bool nextBypass = v >= 0.5f;
            if (nextBypass && !bypass_)
                core_.reset();
            bypass_ = nextBypass;
            break;
        }
        default: break;
    }
}

void Processor::updateCoreParameters() {
    core_.setParameters(coreParams_);
}

tresult PLUGIN_API Processor::process(ProcessData& data) {
    if (data.numSamples <= 0 || data.numInputs < 1 || data.numOutputs < 1)
        return kResultOk;

    auto& in = data.inputs[0];
    auto& out = data.outputs[0];
    if (!in.channelBuffers32 || !out.channelBuffers32)
        return kResultFalse;

    const int32 channels = std::min(in.numChannels, out.numChannels);
    if (channels < 1 || channels > 2)
        return kResultFalse;

    for (int32 ch = 0; ch < channels; ++ch) {
        if (!in.channelBuffers32[ch] || !out.channelBuffers32[ch])
            return kResultFalse;
        if (out.channelBuffers32[ch] != in.channelBuffers32[ch]) {
            std::memcpy(out.channelBuffers32[ch], in.channelBuffers32[ch],
                        static_cast<std::size_t>(data.numSamples) * sizeof(float));
        }
    }

    struct QueueCursor {
        IParamValueQueue* queue = nullptr;
        int32 nextPoint = 0;
    };
    std::array<QueueCursor, 9> cursors {};

    auto slotFor = [](ParamID id) -> int {
        switch (id) {
            case kMechanize: return 0;
            case kCrank: return 1;
            case kClatter: return 2;
            case kWobble: return 3;
            case kAir: return 4;
            case kBody: return 5;
            case kWear: return 6;
            case kOutput: return 7;
            case kBypass: return 8;
            default: return -1;
        }
    };

    if (data.inputParameterChanges) {
        const int32 count = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < count; ++i) {
            if (auto* q = data.inputParameterChanges->getParameterData(i)) {
                const int slot = slotFor(q->getParameterId());
                if (slot >= 0)
                    cursors[static_cast<std::size_t>(slot)].queue = q;
            }
        }
    }

    auto applyPointsAt = [&](int32 sampleOffset) {
        bool changed = false;
        for (auto& cursor : cursors) {
            if (!cursor.queue) continue;
            const int32 pointCount = cursor.queue->getPointCount();
            while (cursor.nextPoint < pointCount) {
                int32 offset = 0;
                ParamValue value = 0.0;
                if (cursor.queue->getPoint(cursor.nextPoint, offset, value) != kResultTrue) {
                    ++cursor.nextPoint;
                    continue;
                }
                offset = std::clamp(offset, 0, data.numSamples);
                if (offset > sampleOffset)
                    break;
                applyParameter(cursor.queue->getParameterId(), static_cast<float>(value));
                ++cursor.nextPoint;
                changed = true;
            }
        }
        if (changed)
            updateCoreParameters();
    };

    auto nextAutomationOffset = [&](int32 after) {
        int32 next = data.numSamples;
        for (auto& cursor : cursors) {
            if (!cursor.queue) continue;
            const int32 pointCount = cursor.queue->getPointCount();
            if (cursor.nextPoint >= pointCount) continue;
            int32 offset = 0;
            ParamValue value = 0.0;
            if (cursor.queue->getPoint(cursor.nextPoint, offset, value) == kResultTrue) {
                offset = std::clamp(offset, 0, data.numSamples);
                if (offset > after)
                    next = std::min(next, offset);
            }
        }
        return next;
    };

    float* left = out.channelBuffers32[0];
    float* right = channels == 2 ? out.channelBuffers32[1] : nullptr;

    int32 position = 0;
    applyPointsAt(0);

    while (position < data.numSamples) {
        const int32 next = nextAutomationOffset(position);
        const int32 end = std::max(position + 1, next);
        const int32 count = std::min(data.numSamples, end) - position;

        if (!bypass_) {
            core_.process(
                left + position,
                right ? right + position : nullptr,
                static_cast<std::size_t>(count));
        }

        position += count;
        applyPointsAt(position);
    }

    // Preserve any legal point exactly at the block end for the next block.
    applyPointsAt(data.numSamples);

    // Conservative VST3 silence reporting. While processing we do not claim
    // silence without scanning/proving it. Hard bypass may safely mirror input.
    out.silenceFlags = bypass_ ? in.silenceFlags : 0;
    return kResultOk;
}

tresult PLUGIN_API Processor::setState(IBStream* state) {
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    float values[kStateValueCount] {};
    int32 bp = 0;
    if (!readState(s, values, bp))
        return kResultFalse;

    coreParams_.mechanize = values[0];
    coreParams_.crank = values[1];
    coreParams_.clatter = values[2];
    coreParams_.wobble = values[3];
    coreParams_.air = values[4];
    coreParams_.body = values[5];
    coreParams_.wear = values[6];
    coreParams_.output = values[7];
    bypass_ = bp != 0;
    updateCoreParameters();
    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state) {
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    const float values[kStateValueCount] = {
        coreParams_.mechanize, coreParams_.crank, coreParams_.clatter, coreParams_.wobble,
        coreParams_.air, coreParams_.body, coreParams_.wear, coreParams_.output
    };
    return writeState(s, values, bypass_ ? 1 : 0) ? kResultOk : kResultFalse;
}

uint32 PLUGIN_API Processor::getTailSamples() {
    // Prototype conservative tail: activity release + air pressure decay + body decay.
    const double samples = std::max(0.0, sampleRate_) * 8.0;
    return static_cast<uint32>(std::min<double>(samples, 0xFFFFFFFEu));
}

} // namespace Mechamorph
