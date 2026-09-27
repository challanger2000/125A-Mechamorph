#include "processor.h"
#include "ids.h"
#include "parameters.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace MechamorphMachine {
namespace {
constexpr int32 kStateVersion = 1;
}

Processor::Processor() {
    setControllerClass(ControllerUID);

    machineParams_.speed = 0.32f;
    machineParams_.load = 0.20f;
    machineParams_.action = 0.48f;
    machineParams_.wear = 0.18f;
    machineParams_.clatter = 0.10f;
    machineParams_.body = 0.0f;
    machineParams_.pressure = 0.0f;
    machineParams_.output = 0.38f;
}

tresult PLUGIN_API Processor::initialize(FUnknown* context) {
    auto r = AudioEffect::initialize(context);
    if (r != kResultOk)
        return r;

    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    addEventInput(STR16("Event In"), 16);

    if (!assets_.load())
        return kResultFalse;

    engine_.setSampleSet(assets_.profile(machineIndex_));
    return kResultOk;
}

tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup) {
    auto r = AudioEffect::setupProcessing(setup);
    if (r != kResultOk)
        return r;

    sampleRate_ = setup.sampleRate > 1.0 ? setup.sampleRate : 48000.0;
    engine_.prepare(sampleRate_);
    engine_.setSampleSet(assets_.profile(machineIndex_));
    updateEngineParameters();
    return kResultOk;
}

tresult PLUGIN_API Processor::setActive(TBool state) {
    if (state) {
        heldNotes_ = 0;
        engine_.reset();
        engine_.setSampleSet(assets_.profile(machineIndex_));
        updateEngineParameters();
    }
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 symbolicSampleSize) {
    return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API Processor::setBusArrangements(
    SpeakerArrangement* inputs, int32 numIns,
    SpeakerArrangement* outputs, int32 numOuts) {

    if (numIns != 0 || !outputs || numOuts != 1 || outputs[0] != SpeakerArr::kStereo)
        return kResultFalse;

    return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

void Processor::applyMachineProfile(bool restartIfHeld) noexcept {
    engine_.reset();
    engine_.setSampleSet(assets_.profile(machineIndex_));
    updateEngineParameters();
    if (restartIfHeld && heldNotes_ > 0)
        engine_.start();
}

void Processor::applyParameter(ParamID id, float normalized) noexcept {
    const float v = std::clamp(std::isfinite(normalized) ? normalized : 0.0f, 0.0f, 1.0f);

    switch (id) {
        case kMachine: {
            const int next = std::clamp(static_cast<int>(std::floor(v * 3.0f)), 0, 2);
            if (next != machineIndex_) {
                machineIndex_ = next;
                applyMachineProfile(true);
            }
            break;
        }
        case kSpeed:
            machineParams_.speed = v;
            break;
        case kLoad:
            machineParams_.load = v;
            break;
        case kAction:
            machineParams_.action = v;
            break;
        case kWear:
            machineParams_.wear = v;
            // CLATTER remains an internal consequence of wear.
            machineParams_.clatter = 0.08f + 0.20f * v;
            break;
        case kOutput:
            machineParams_.output = v;
            break;
        default:
            break;
    }
}

void Processor::updateEngineParameters() noexcept {
    engine_.setParameters(machineParams_);
}

tresult PLUGIN_API Processor::process(ProcessData& data) {
    // VST3 parameter flushes can arrive with zero audio samples/buffers.
    if (data.numSamples <= 0) {
        bool changed = false;
        if (data.inputParameterChanges) {
            const int32 count = data.inputParameterChanges->getParameterCount();
            for (int32 i = 0; i < count; ++i) {
                auto* q = data.inputParameterChanges->getParameterData(i);
                if (!q) continue;
                const int32 points = q->getPointCount();
                for (int32 p = 0; p < points; ++p) {
                    int32 offset = 0;
                    ParamValue value = 0.0;
                    if (q->getPoint(p, offset, value) == kResultTrue) {
                        applyParameter(q->getParameterId(), static_cast<float>(value));
                        changed = true;
                    }
                }
            }
        }
        if (changed)
            updateEngineParameters();
        return kResultOk;
    }

    if (data.numOutputs < 1)
        return kResultFalse;

    auto& out = data.outputs[0];
    if (out.numChannels != 2 || !out.channelBuffers32 ||
        !out.channelBuffers32[0] || !out.channelBuffers32[1])
        return kResultFalse;

    float* left = out.channelBuffers32[0];
    float* right = out.channelBuffers32[1];
    std::fill(left, left + data.numSamples, 0.0f);
    std::fill(right, right + data.numSamples, 0.0f);

    struct Cursor {
        IParamValueQueue* queue = nullptr;
        int32 point = 0;
    };
    std::array<Cursor, kParamCount> cursors{};

    auto slotFor = [](ParamID id) -> int {
        switch (id) {
            case kMachine: return 0;
            case kSpeed: return 1;
            case kLoad: return 2;
            case kAction: return 3;
            case kWear: return 4;
            case kOutput: return 5;
            default: return -1;
        }
    };

    if (data.inputParameterChanges) {
        const int32 count = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < count; ++i) {
            auto* q = data.inputParameterChanges->getParameterData(i);
            if (!q) continue;
            const int slot = slotFor(q->getParameterId());
            if (slot >= 0)
                cursors[static_cast<std::size_t>(slot)].queue = q;
        }
    }

    int32 eventIndex = 0;
    const int32 eventCount = data.inputEvents ? data.inputEvents->getEventCount() : 0;

    auto applyEventsAt = [&](int32 sampleOffset) {
        while (eventIndex < eventCount) {
            Event e{};
            if (data.inputEvents->getEvent(eventIndex, e) != kResultOk) {
                ++eventIndex;
                continue;
            }

            const int32 offset = std::clamp(e.sampleOffset, 0, data.numSamples);
            if (offset > sampleOffset)
                break;

            if (e.type == Event::kNoteOnEvent) {
                if (e.noteOn.velocity > 0.0f) {
                    if (heldNotes_ == 0)
                        engine_.start();
                    ++heldNotes_;
                } else {
                    if (heldNotes_ > 0) --heldNotes_;
                    if (heldNotes_ == 0) engine_.stop();
                }
            } else if (e.type == Event::kNoteOffEvent) {
                if (heldNotes_ > 0) --heldNotes_;
                if (heldNotes_ == 0) engine_.stop();
            }

            ++eventIndex;
        }
    };

    auto applyParamsAt = [&](int32 sampleOffset) {
        bool changed = false;
        for (auto& cursor : cursors) {
            if (!cursor.queue) continue;
            const int32 count = cursor.queue->getPointCount();
            while (cursor.point < count) {
                int32 offset = 0;
                ParamValue value = 0.0;
                if (cursor.queue->getPoint(cursor.point, offset, value) != kResultTrue) {
                    ++cursor.point;
                    continue;
                }
                offset = std::clamp(offset, 0, data.numSamples);
                if (offset > sampleOffset)
                    break;
                applyParameter(cursor.queue->getParameterId(), static_cast<float>(value));
                ++cursor.point;
                changed = true;
            }
        }
        if (changed)
            updateEngineParameters();
    };

    float peak = 0.0f;
    for (int32 i = 0; i < data.numSamples; ++i) {
        applyParamsAt(i);
        applyEventsAt(i);

        float mono = 0.0f;
        engine_.process(&mono, 1);
        left[i] = mono;
        right[i] = mono;
        peak = std::max(peak, std::fabs(mono));
    }

    applyParamsAt(data.numSamples);
    applyEventsAt(data.numSamples);

    out.silenceFlags = peak < 1.0e-12f ? 0x3ull : 0;
    return kResultOk;
}

tresult PLUGIN_API Processor::setState(IBStream* state) {
    if (!state) return kResultFalse;

    IBStreamer s(state, kLittleEndian);
    int32 version = 0;
    float machine = 0.0f;
    float speed = 0.0f;
    float load = 0.0f;
    float action = 0.0f;
    float wear = 0.0f;
    float output = 0.0f;

    if (!s.readInt32(version) || version != kStateVersion ||
        !s.readFloat(machine) ||
        !s.readFloat(speed) ||
        !s.readFloat(load) ||
        !s.readFloat(action) ||
        !s.readFloat(wear) ||
        !s.readFloat(output))
        return kResultFalse;

    machineIndex_ = std::clamp(static_cast<int>(std::floor(std::clamp(machine, 0.0f, 1.0f) * 3.0f)), 0, 2);
    machineParams_.speed = std::clamp(speed, 0.0f, 1.0f);
    machineParams_.load = std::clamp(load, 0.0f, 1.0f);
    machineParams_.action = std::clamp(action, 0.0f, 1.0f);
    machineParams_.wear = std::clamp(wear, 0.0f, 1.0f);
    machineParams_.clatter = 0.08f + 0.20f * machineParams_.wear;
    machineParams_.output = std::clamp(output, 0.0f, 1.0f);

    applyMachineProfile(heldNotes_ > 0);
    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state) {
    if (!state) return kResultFalse;

    IBStreamer s(state, kLittleEndian);
    const float machine = static_cast<float>(machineIndex_) / 2.0f;

    if (!s.writeInt32(kStateVersion) ||
        !s.writeFloat(machine) ||
        !s.writeFloat(machineParams_.speed) ||
        !s.writeFloat(machineParams_.load) ||
        !s.writeFloat(machineParams_.action) ||
        !s.writeFloat(machineParams_.wear) ||
        !s.writeFloat(machineParams_.output))
        return kResultFalse;

    return kResultOk;
}

uint32 PLUGIN_API Processor::getTailSamples() {
    const double samples = std::max(0.0, sampleRate_) * 8.0;
    return static_cast<uint32>(std::min<double>(samples, 0xFFFFFFFEu));
}

} // namespace MechamorphMachine
