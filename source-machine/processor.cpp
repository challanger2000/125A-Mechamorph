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
constexpr int32 kStateVersion = 4;
}

Processor::Processor() {
    setControllerClass(ControllerUID);

    machineParams_.speed = 0.32f;
    machineParams_.load = 0.20f;
    machineParams_.action = 0.48f;
    machineParams_.wear = 0.18f;
    machineParams_.scale = 0.35f;
    machineParams_.clatter = 0.10f;
    machineParams_.body = 0.0f;
    machineParams_.pressure = 0.0f;
    machineParams_.output = 0.38f;
    bodyAmount_ = 0.28f;
    spaceAmount_ = 0.18f;
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
    spaceEngine_.prepare(sampleRate_);
    updateEngineParameters();
    return kResultOk;
}

tresult PLUGIN_API Processor::setActive(TBool state) {
    if (state) {
        transportWasPlaying_ = false;
        activeNoteId_ = -1;
        activePitch_ = -1;
        engine_.reset();
        engine_.setSampleSet(assets_.profile(machineIndex_));
        spaceEngine_.reset();
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

void Processor::applyMachineProfile(bool restartIfRunning) noexcept {
    const bool wasRunning = engine_.state() != mechamorph::machine::State::Stopped;
    engine_.reset();
    engine_.setSampleSet(assets_.profile(machineIndex_));
    updateEngineParameters();
    if (restartIfRunning && wasRunning)
        engine_.start();
}

void Processor::applyParameter(ParamID id, float normalized) noexcept {
    const float v = std::clamp(std::isfinite(normalized) ? normalized : 0.0f, 0.0f, 1.0f);

    switch (id) {
        case kMachine: {
            const int next = std::clamp(static_cast<int>(std::floor(v * 6.0f)), 0, 5);
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
        case kScale:
            machineParams_.scale = v;
            break;
        case kSpace:
            spaceAmount_ = v;
            break;
        case kBody:
            bodyAmount_ = v;
            break;
        case kOutput:
            machineParams_.output = v;
            break;
        default:
            break;
    }
}

void Processor::updateEngineParameters() noexcept {
    // Internal machine personality: pressure is not another user macro.
    static constexpr float kPressureByMachine[6] = {
        0.00f, // TINY
        0.06f, // INTRICATE
        0.24f, // HEAVY
        0.34f, // COLOSSAL
        1.00f, // PNEUMATIC
        0.28f  // BROKEN
    };
    machineParams_.pressure =
        kPressureByMachine[std::clamp(machineIndex_, 0, 5)];

    engine_.setParameters(machineParams_);

    // BODY is now a true independent control.
    // SCALE only changes how strongly the same cabinet/body is excited.
    const float scaleBodyCoupling =
        0.88f + 0.24f * machineParams_.scale;
    const float effectiveBody =
        std::clamp(bodyAmount_ * scaleBodyCoupling, 0.0f, 1.0f);

    spaceEngine_.setBody(effectiveBody);
    spaceEngine_.setSpace(spaceAmount_);
    spaceEngine_.setScale(machineParams_.scale);
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
            case kScale: return 5;
            case kSpace: return 6;
            case kOutput: return 7;
            case kBody: return 8;
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

            if (e.type == Event::kNoteOnEvent && e.noteOn.velocity > 0.0f) {
                // Monophonic retrigger gate:
                // every Note On restarts the complete machine cycle and becomes
                // the only note allowed to stop that new cycle.
                engine_.reset();
                engine_.setSampleSet(assets_.profile(machineIndex_));
                updateEngineParameters();
                engine_.start();

                activeNoteId_ = e.noteOn.noteId;
                activePitch_ = e.noteOn.pitch;
            } else if (
                e.type == Event::kNoteOffEvent ||
                (e.type == Event::kNoteOnEvent && e.noteOn.velocity <= 0.0f)) {

                const int32 noteId =
                    e.type == Event::kNoteOffEvent ? e.noteOff.noteId : e.noteOn.noteId;
                const int16 pitch =
                    e.type == Event::kNoteOffEvent ? e.noteOff.pitch : e.noteOn.pitch;

                // Prefer VST3 noteId when the host supplies one. Fall back to
                // pitch for hosts that use noteId=-1.
                const bool idMatches =
                    activeNoteId_ >= 0 && noteId >= 0 && noteId == activeNoteId_;
                const bool pitchMatches =
                    (activeNoteId_ < 0 || noteId < 0) && pitch == activePitch_;

                if (idMatches || pitchMatches) {
                    engine_.stop();
                    activeNoteId_ = -1;
                    activePitch_ = -1;
                }
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

    // DAW transport owns the global stop condition. This keeps short MIDI
    // trigger notes independent from machine lifetime, while transport stop
    // reliably silences/reset the machine.
    bool transportPlaying = transportWasPlaying_;
    if (data.processContext &&
        (data.processContext->state & ProcessContext::kPlaying) != 0) {
        transportPlaying = true;
    } else if (data.processContext) {
        transportPlaying = false;
    }

    if (transportWasPlaying_ && !transportPlaying) {
        activeNoteId_ = -1;
        activePitch_ = -1;
        engine_.reset();
        engine_.setSampleSet(assets_.profile(machineIndex_));
        spaceEngine_.reset();
        updateEngineParameters();
    }
    transportWasPlaying_ = transportPlaying;

    float peak = 0.0f;
    for (int32 i = 0; i < data.numSamples; ++i) {
        applyParamsAt(i);
        applyEventsAt(i);

        float mono = 0.0f;
        engine_.process(&mono, 1);

        float outL = 0.0f;
        float outR = 0.0f;
        spaceEngine_.process(mono, outL, outR);

        left[i] = outL;
        right[i] = outR;
        peak = std::max(peak, std::max(std::fabs(outL), std::fabs(outR)));
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
    if (!s.readInt32(version))
        return kResultFalse;

    float machine = 0.0f;
    float speed = 0.32f;
    float load = 0.20f;
    float action = 0.48f;
    float wear = 0.18f;
    float scale = 0.35f;
    float body = 0.28f;
    float space = 0.18f;
    float output = 0.38f;

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
    } else if (version == 3 || version == kStateVersion) {
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

    const std::array<float, 9> restoredValues{
        machine, speed, load, action, wear, scale, body, space, output
    };
    for (float value : restoredValues) {
        if (!std::isfinite(value))
            return kResultFalse;
    }

    if (version <= 3) {
        // Legacy states used three machines encoded as 0 / 0.5 / 1.0.
        const int legacyIndex = std::clamp(
            static_cast<int>(std::lround(std::clamp(machine, 0.0f, 1.0f) * 2.0f)),
            0, 2);
        // Preserve conceptual identity as closely as possible:
        // Projector -> TINY, Handcrank -> INTRICATE, Industrial -> HEAVY.
        static constexpr int kLegacyToCurrent[3] = {0, 1, 2};
        machineIndex_ = kLegacyToCurrent[legacyIndex];
    } else {
        machineIndex_ = std::clamp(
            static_cast<int>(std::floor(std::clamp(machine, 0.0f, 1.0f) * 6.0f)),
            0, 5);
    }

    machineParams_.speed = std::clamp(speed, 0.0f, 1.0f);
    machineParams_.load = std::clamp(load, 0.0f, 1.0f);
    machineParams_.action = std::clamp(action, 0.0f, 1.0f);
    machineParams_.wear = std::clamp(wear, 0.0f, 1.0f);
    machineParams_.scale = std::clamp(scale, 0.0f, 1.0f);
    machineParams_.clatter = 0.08f + 0.20f * machineParams_.wear;
    bodyAmount_ = std::clamp(body, 0.0f, 1.0f);
    spaceAmount_ = std::clamp(space, 0.0f, 1.0f);
    machineParams_.output = std::clamp(output, 0.0f, 1.0f);

    applyMachineProfile(false);
    spaceEngine_.reset();
    updateEngineParameters();
    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state) {
    if (!state) return kResultFalse;

    IBStreamer s(state, kLittleEndian);
    const float machine = static_cast<float>(machineIndex_) / 5.0f;

    if (!s.writeInt32(kStateVersion) ||
        !s.writeFloat(machine) ||
        !s.writeFloat(machineParams_.speed) ||
        !s.writeFloat(machineParams_.load) ||
        !s.writeFloat(machineParams_.action) ||
        !s.writeFloat(machineParams_.wear) ||
        !s.writeFloat(machineParams_.scale) ||
        !s.writeFloat(bodyAmount_) ||
        !s.writeFloat(spaceAmount_) ||
        !s.writeFloat(machineParams_.output))
        return kResultFalse;

    return kResultOk;
}

uint32 PLUGIN_API Processor::getTailSamples() {
    const double samples = std::max(0.0, sampleRate_) * 8.0;
    return static_cast<uint32>(std::min<double>(samples, 0xFFFFFFFEu));
}

} // namespace MechamorphMachine
