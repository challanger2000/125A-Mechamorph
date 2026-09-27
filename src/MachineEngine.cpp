#include "MachineEngine.h"

#include <algorithm>
#include <cmath>

namespace mechamorph::machine {
namespace {
constexpr double kTwoPi = 6.283185307179586476925286766559;
}

float MachineEngine::clamp01(float x) noexcept {
    return std::clamp(std::isfinite(x) ? x : 0.0f, 0.0f, 1.0f);
}

float MachineEngine::intensityCurve(float x) noexcept {
    x = clamp01(x);
    if (x <= 0.5f) return 1.4f * x;
    if (x <= 0.75f) return 0.70f + 0.80f * (x - 0.5f);
    return 0.90f + 0.40f * (x - 0.75f);
}

void MachineEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
    reset();
}

void MachineEngine::reset() noexcept {
    state_ = {};
    for (auto& v : voices_) v = {};
    requestStart_ = false;
    requestStop_ = false;
    loaded_ = false;
    wasLoaded_ = false;
    startProgress_ = 0.0f;
    stopProgress_ = 0.0f;
    runEnvelope_ = 0.0f;
    actionAccumulator_ = 0.0f;
    rngState_ = 0x125A5A5A12345678ULL;
}

void MachineEngine::setParameters(const MachineParameters& p) noexcept {
    params_ = p;
    params_.speed = clamp01(params_.speed);
    params_.load = clamp01(params_.load);
    params_.wear = clamp01(params_.wear);
    params_.activity = clamp01(params_.activity);
    params_.body = clamp01(params_.body);
    params_.output = clamp01(params_.output);
}

bool MachineEngine::setSampleSet(SampleRole role, const SampleSet& set) noexcept {
    const auto index = static_cast<std::size_t>(role);
    if (index >= sets_.size()) return false;
    sets_[index] = set;
    return true;
}

void MachineEngine::start() noexcept {
    requestStart_ = true;
    requestStop_ = false;
}

void MachineEngine::stop() noexcept {
    requestStop_ = true;
    requestStart_ = false;
}

void MachineEngine::setLoaded(bool loaded) noexcept {
    loaded_ = loaded;
}

float MachineEngine::random01() noexcept {
    std::uint64_t x = rngState_;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rngState_ = x;
    const std::uint64_t y = x * 2685821657736338717ULL;
    constexpr double scale = 1.0 / 9007199254740992.0;
    return static_cast<float>((y >> 11) * scale);
}

std::size_t MachineEngine::chooseVariant(const SampleSet& set) noexcept {
    if (set.count == 0) return 0;
    const auto i = static_cast<std::size_t>(random01() * static_cast<float>(set.count));
    return std::min(i, set.count - 1);
}

void MachineEngine::triggerRole(SampleRole role, float gain, bool loop) noexcept {
    const auto index = static_cast<std::size_t>(role);
    if (index >= sets_.size()) return;

    const auto& set = sets_[index];
    if (set.count == 0) return;

    Voice* target = nullptr;
    for (auto& v : voices_) {
        if (!v.active) {
            target = &v;
            break;
        }
    }
    if (!target) {
        // Deterministic voice stealing: quietest/oldest approximation = first slot.
        target = &voices_[0];
    }

    const auto variant = chooseVariant(set);
    target->sample = set.variants[variant];
    target->position = 0.0;
    target->gain = std::max(0.0f, gain);
    target->loop = loop;
    target->active = target->sample.valid();

    // Sample rate conversion and speed coupling.
    const double srRatio = static_cast<double>(target->sample.sampleRate) / sampleRate_;
    const float speed = intensityCurve(params_.speed);
    const double speedFactor = loop ? (0.72 + 0.56 * speed) : (0.94 + 0.12 * speed);
    target->rate = srRatio * speedFactor;
}

void MachineEngine::updateTransitions() noexcept {
    if (requestStart_ && state_.phase == MachinePhase::Stopped) {
        state_.phase = MachinePhase::Starting;
        startProgress_ = 0.0f;
        triggerRole(SampleRole::Start, 1.0f, false);
        requestStart_ = false;
    }

    if (requestStop_ &&
        state_.phase != MachinePhase::Stopped &&
        state_.phase != MachinePhase::Stopping) {
        state_.phase = MachinePhase::Stopping;
        stopProgress_ = 0.0f;
        triggerRole(SampleRole::Release, 0.8f, false);
        triggerRole(SampleRole::Stop, 1.0f, false);
        requestStop_ = false;
    }

    if (state_.phase == MachinePhase::Starting) {
        startProgress_ += static_cast<float>(1.0 / (0.45 * sampleRate_));
        if (startProgress_ >= 1.0f) {
            startProgress_ = 1.0f;
            state_.phase = loaded_ ? MachinePhase::Loaded : MachinePhase::Running;
            ensureRunBed();
        }
    }

    if (state_.phase == MachinePhase::Stopping) {
        stopProgress_ += static_cast<float>(1.0 / (0.55 * sampleRate_));
        if (stopProgress_ >= 1.0f) {
            stopProgress_ = 1.0f;
            state_.phase = MachinePhase::Stopped;
            runEnvelope_ = 0.0f;
            for (auto& v : voices_) {
                if (v.loop) v.active = false;
            }
        }
    }

    if (state_.phase == MachinePhase::Running && loaded_) {
        state_.phase = MachinePhase::Loaded;
        triggerRole(SampleRole::Load, 0.75f, false);
    } else if (state_.phase == MachinePhase::Loaded && !loaded_) {
        state_.phase = MachinePhase::Running;
        triggerRole(SampleRole::Release, 0.55f, false);
    }

    wasLoaded_ = loaded_;
}

void MachineEngine::ensureRunBed() noexcept {
    bool hasRunBed = false;
    for (const auto& v : voices_) {
        if (v.active && v.loop) {
            hasRunBed = true;
            break;
        }
    }
    if (!hasRunBed)
        triggerRole(SampleRole::RunBed, 0.75f, true);
}

void MachineEngine::updateMachineState() noexcept {
    const float speed = intensityCurve(params_.speed);
    const float wear = intensityCurve(params_.wear);
    const float load = loaded_ ? intensityCurve(params_.load) : 0.0f;

    state_.targetSpeedHz = 0.45f + 2.55f * speed;

    // Loaded machine slows slightly; wear creates bounded cyclic irregularity.
    const float loadSlow = 0.12f * load;
    const float eccentricity =
        0.015f * wear * static_cast<float>(std::sin(state_.drivePhase));
    state_.speedHz =
        std::max(0.05f, state_.targetSpeedHz * (1.0f - loadSlow + eccentricity));

    state_.drivePhase += kTwoPi * static_cast<double>(state_.speedHz) / sampleRate_;
    if (state_.drivePhase >= kTwoPi)
        state_.drivePhase = std::fmod(state_.drivePhase, kTwoPi);

    state_.load = load;
    state_.wear = wear;
    state_.backlash = wear * wear;
    state_.friction = wear * (0.25f + 0.75f * load);

    const bool active =
        state_.phase != MachinePhase::Stopped &&
        state_.phase != MachinePhase::Stopping;
    const float targetEnergy = active ? intensityCurve(params_.activity) : 0.0f;
    const float energyCoeff =
        targetEnergy > state_.energy ? 0.0018f : 0.0007f;
    state_.energy += energyCoeff * (targetEnergy - state_.energy);

    const float targetRun =
        (state_.phase == MachinePhase::Running || state_.phase == MachinePhase::Loaded)
        ? 1.0f : 0.0f;
    runEnvelope_ += 0.0012f * (targetRun - runEnvelope_);
    state_.runBlend = runEnvelope_;

    // ACTION events are tied to drive cycles, not random time.
    const float actionsPerCycle = 2.0f + 4.0f * intensityCurve(params_.activity);
    actionAccumulator_ +=
        static_cast<float>(state_.speedHz * actionsPerCycle / sampleRate_);
    if (actionAccumulator_ >= 1.0f) {
        actionAccumulator_ -= 1.0f;
        const float variation = 0.85f + 0.25f * random01();
        const float gain = state_.energy * variation * (0.65f + 0.35f * state_.wear);
        triggerRole(SampleRole::Action, gain, false);
    }

    if (state_.phase == MachinePhase::Running || state_.phase == MachinePhase::Loaded)
        ensureRunBed();
}

float MachineEngine::renderVoices() noexcept {
    float sum = 0.0f;

    for (auto& v : voices_) {
        if (!v.active || !v.sample.valid()) continue;

        const std::size_t i0 =
            static_cast<std::size_t>(std::max(0.0, std::floor(v.position)));
        if (i0 >= v.sample.frames) {
            if (v.loop) {
                v.position = std::fmod(v.position, static_cast<double>(v.sample.frames));
            } else {
                v.active = false;
                continue;
            }
        }

        const std::size_t a =
            static_cast<std::size_t>(std::max(0.0, std::floor(v.position))) % v.sample.frames;
        const std::size_t b = (a + 1) % v.sample.frames;
        const float frac = static_cast<float>(v.position - std::floor(v.position));
        const float s = v.sample.data[a] + frac * (v.sample.data[b] - v.sample.data[a]);

        float roleGain = v.gain;
        if (v.loop) roleGain *= runEnvelope_;

        sum += s * roleGain;
        v.position += v.rate;

        if (v.loop && v.position >= static_cast<double>(v.sample.frames))
            v.position = std::fmod(v.position, static_cast<double>(v.sample.frames));
        else if (!v.loop && v.position >= static_cast<double>(v.sample.frames))
            v.active = false;
    }

    const float outputDb = 18.0f * (params_.output - 0.5f);
    const float outGain = std::pow(10.0f, outputDb / 20.0f);
    return sum * outGain;
}

float MachineEngine::processSample() noexcept {
    updateTransitions();
    updateMachineState();
    return renderVoices();
}

void MachineEngine::process(float* mono, std::size_t frames) noexcept {
    if (!mono || frames == 0) return;
    for (std::size_t i = 0; i < frames; ++i)
        mono[i] = processSample();
}

} // namespace mechamorph::machine
