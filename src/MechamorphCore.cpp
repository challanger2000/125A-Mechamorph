#include "MechamorphCore.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace mechamorph {
namespace {
constexpr double kTwoPi = 6.283185307179586476925286766559;
constexpr float kTiny = 1.0e-20f;

float sanitize(float x) noexcept {
    return std::isfinite(x) ? x : 0.0f;
}
}

void DeterministicRng::seed(std::uint64_t s) noexcept {
    state_ = s ? s : 0x9E3779B97F4A7C15ULL;
}

std::uint64_t DeterministicRng::nextU64() noexcept {
    // xorshift64*: tiny deterministic realtime-safe generator.
    std::uint64_t x = state_;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    state_ = x;
    return x * 2685821657736338717ULL;
}

float DeterministicRng::uniform01() noexcept {
    constexpr double scale = 1.0 / 9007199254740992.0; // 2^53
    return static_cast<float>((nextU64() >> 11) * scale);
}

float DeterministicRng::bipolar() noexcept {
    return 2.0f * uniform01() - 1.0f;
}

void OnePoleEnvelope::prepare(double sampleRate, double timeMs) noexcept {
    const double t = std::max(timeMs * 0.001, 1.0 / std::max(sampleRate, 1.0));
    coefficient_ = static_cast<float>(std::exp(-1.0 / (t * sampleRate)));
}

float OnePoleEnvelope::process(float x) noexcept {
    const float v = std::fabs(sanitize(x));
    state_ = coefficient_ * state_ + (1.0f - coefficient_) * v;
    if (std::fabs(state_) < kTiny) state_ = 0.0f;
    return state_;
}

void InputAnalyzer::prepare(double sampleRate) noexcept {
    fast_.prepare(sampleRate, 2.0);
    slow_.prepare(sampleRate, 30.0);
}

void InputAnalyzer::reset() noexcept {
    fast_.reset();
    slow_.reset();
}

void InputAnalyzer::process(float monoSample, MechanicalState& state) noexcept {
    const float fast = fast_.process(monoSample);
    const float slow = slow_.process(monoSample);
    state.inputEnvelope = slow;
    state.transientStrength = std::max(0.0f, fast - slow);
}

void MechanicalDrive::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
}

void MechanicalDrive::reset() noexcept {
    drift_ = 0.0f;
}

void MechanicalDrive::process(const Parameters& p, MechanicalState& state) noexcept {
    // Prototype mapping only; to be replaced with measured ranges.
    const float crank = std::clamp(p.crank, 0.0f, 1.0f);
    const float wobble = std::clamp(p.wobble, 0.0f, 1.0f);
    const float wear = std::clamp(p.wear, 0.0f, 1.0f);

    state.targetSpeedHz = 0.35f + 3.65f * crank;

    // Slow deterministic drift derived from phase, not unrelated random timing.
    const float eccentric = 0.025f * wobble *
        static_cast<float>(std::sin(state.phase));
    const float loadSlow = -0.15f * state.load * wear;

    state.speedHz = std::max(0.05f, state.targetSpeedHz * (1.0f + eccentric + loadSlow));
    state.phase += kTwoPi * static_cast<double>(state.speedHz) / sampleRate_;
    if (state.phase >= kTwoPi)
        state.phase = std::fmod(state.phase, kTwoPi);

    state.wear = wear;
    state.backlash = wear * wear;
    state.looseness = std::clamp(0.15f * wear + 0.85f * p.clatter * wear, 0.0f, 1.0f);
}

void ModalResonator::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
    reset();
}

void ModalResonator::reset() noexcept {
    for (auto& m : modes_) {
        m.z1 = 0.0f;
        m.z2 = 0.0f;
    }
}

void ModalResonator::setModes(const ModalMode* modes, std::size_t count) noexcept {
    modeCount_ = std::min(count, kMaxModes);
    for (std::size_t i = 0; i < modeCount_; ++i) {
        const float nyquistSafe = static_cast<float>(0.45 * sampleRate_);
        const float f = std::clamp(modes[i].frequencyHz, 20.0f, nyquistSafe);
        const float decay = std::max(modes[i].decaySeconds, 0.005f);
        const float r = static_cast<float>(std::exp(-1.0 / (decay * sampleRate_)));
        const float theta = static_cast<float>(kTwoPi * f / sampleRate_);

        auto& m = modes_[i];
        m.a1 = 2.0f * r * std::cos(theta);
        m.a2 = -(r * r);
        m.b0 = modes[i].gain * (1.0f - r);
        m.z1 = 0.0f;
        m.z2 = 0.0f;
    }
    for (std::size_t i = modeCount_; i < kMaxModes; ++i)
        modes_[i] = {};
}

float ModalResonator::process(float excitation) noexcept {
    excitation = sanitize(excitation);
    float y = 0.0f;
    for (std::size_t i = 0; i < modeCount_; ++i) {
        auto& m = modes_[i];
        const float out = m.b0 * excitation + m.a1 * m.z1 + m.a2 * m.z2;
        m.z2 = m.z1;
        m.z1 = sanitize(out);
        y += m.z1;
    }
    return sanitize(y);
}

void AirEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
}

void AirEngine::reset() noexcept {
    noiseState_ = 0.0f;
}

float AirEngine::process(const Parameters& p, MechanicalState& state, DeterministicRng& rng) noexcept {
    const float air = std::clamp(p.air, 0.0f, 1.0f);
    const float phasePump = 0.5f + 0.5f * static_cast<float>(std::sin(state.phase));
    const float dt = static_cast<float>(1.0 / sampleRate_);

    // Prototype rates are expressed per second so behaviour remains sample-rate invariant.
    const float inflowPerSecond = air * (0.35f + 1.15f * phasePump);
    const float leakPerSecond = 0.18f + 0.90f * state.wear;
    const float consumptionPerSecond = 0.55f * state.inputEnvelope * air;

    state.pressure = std::clamp(
        state.pressure + dt * (inflowPerSecond - leakPerSecond - consumptionPerSecond),
        0.0f, 1.0f);
    state.leak = leakPerSecond;

    const float white = rng.bipolar();
    // Very cheap low-pass coloration for first prototype.
    noiseState_ += 0.05f * (white - noiseState_);
    if (std::fabs(noiseState_) < kTiny) noiseState_ = 0.0f;
    return noiseState_ * state.pressure * air * 0.12f;
}

void RatchetEngine::reset() noexcept {
    previousPhase_ = 0.0;
}

float RatchetEngine::process(const Parameters& p, const MechanicalState& state, DeterministicRng& rng) noexcept {
    const int teeth = 12;
    const double sector = kTwoPi / static_cast<double>(teeth);
    const int prevIndex = static_cast<int>(previousPhase_ / sector);
    const int currentIndex = static_cast<int>(state.phase / sector);
    const double oldPhase = previousPhase_;
    previousPhase_ = state.phase;

    bool fired = currentIndex != prevIndex;
    // Phase wrap is also one tooth crossing.
    if (state.phase < sector && oldPhase > kTwoPi - sector)
        fired = true;

    if (!fired) return 0.0f;

    const float wear = std::clamp(p.wear, 0.0f, 1.0f);
    const float variation = 1.0f + 0.12f * wear * rng.bipolar();
    return 0.12f * std::clamp(p.mechanize, 0.0f, 1.0f) * variation;
}

void RattleEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
}

void RattleEngine::reset() noexcept {
    for (auto& e : events_) e = {};
}

float RattleEngine::process(const Parameters& p, const MechanicalState& state, DeterministicRng& rng) noexcept {
    // Trigger cluster only on sufficiently strong transients.
    if (state.transientStrength > 0.02f && p.clatter > 0.0f) {
        const int requested = 1 + static_cast<int>(4.0f * std::clamp(p.clatter * (0.3f + state.wear), 0.0f, 1.0f));
        int placed = 0;
        for (auto& e : events_) {
            if (placed >= requested) break;
            if (e.remaining <= 0) {
                const float delayMs = 3.0f + 35.0f * rng.uniform01();
                e.remaining = static_cast<int>(delayMs * 0.001 * sampleRate_);
                e.amplitude = (0.03f + 0.07f * rng.uniform01()) *
                              std::clamp(p.clatter, 0.0f, 1.0f);
                ++placed;
            }
        }
    }

    float out = 0.0f;
    for (auto& e : events_) {
        if (e.remaining > 0) {
            --e.remaining;
            if (e.remaining == 0) {
                out += e.amplitude * (0.7f + 0.3f * rng.uniform01());
                e.amplitude = 0.0f;
            }
        }
    }
    return out;
}

void Core::prepare(double sampleRate, std::size_t /*maxBlockSize*/) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
    rng_.seed(0x125A0001ULL);
    analyzer_.prepare(sampleRate_);
    drive_.prepare(sampleRate_);
    body_.prepare(sampleRate_);
    air_.prepare(sampleRate_);
    rattle_.prepare(sampleRate_);

    // Placeholder prototype body. All values are EMPIRICALLY TUNED and
    // must be replaced/calibrated from measurements before product claims.
    const ModalMode wood[] = {
        { 170.0f, 0.11f, 0.55f },
        { 315.0f, 0.08f, 0.38f },
        { 520.0f, 0.07f, 0.26f },
        { 780.0f, 0.055f, 0.18f },
        { 1180.0f, 0.045f, 0.12f },
        { 1760.0f, 0.035f, 0.08f },
    };
    body_.setModes(wood, sizeof(wood) / sizeof(wood[0]));
    reset();
}

void Core::reset() noexcept {
    state_ = {};
    rng_.seed(0x125A0001ULL);
    analyzer_.reset();
    drive_.reset();
    body_.reset();
    air_.reset();
    ratchet_.reset();
    rattle_.reset();
}

float Core::clamp01(float x) noexcept {
    return std::clamp(std::isfinite(x) ? x : 0.0f, 0.0f, 1.0f);
}

float Core::processOne(float input) noexcept {
    input = sanitize(input);

    Parameters p = params_;
    p.mechanize = clamp01(p.mechanize);
    p.crank = clamp01(p.crank);
    p.clatter = clamp01(p.clatter);
    p.wobble = clamp01(p.wobble);
    p.air = clamp01(p.air);
    p.body = clamp01(p.body);
    p.wear = clamp01(p.wear);
    p.output = clamp01(p.output);

    analyzer_.process(input, state_);
    state_.load = std::clamp(state_.inputEnvelope * p.mechanize, 0.0f, 1.0f);
    drive_.process(p, state_);

    const float ratchet = ratchet_.process(p, state_, rng_);
    const float rattle = rattle_.process(p, state_, rng_);
    const float air = air_.process(p, state_, rng_);

    const float bodyExcitation =
        (0.18f * input * p.mechanize) +
        ratchet + rattle + air;

    const float body = body_.process(bodyExcitation) * p.body;
    const float machine = 0.55f * body + 0.22f * ratchet + 0.28f * rattle + air;

    // Exact dry at mechanize=0.
    const float wet = clamp01(p.mechanize);
    float y = input + wet * machine;

    // Prototype output mapping: -12 dB .. +12 dB, midpoint = unity.
    const float outputDb = 24.0f * (p.output - 0.5f);
    const float gain = std::pow(10.0f, outputDb / 20.0f);
    y *= gain;

    return sanitize(y);
}

void Core::process(float* left, float* right, std::size_t frames) noexcept {
    if (!left || frames == 0) return;

    if (!right) {
        for (std::size_t i = 0; i < frames; ++i)
            left[i] = processOne(left[i]);
        return;
    }

    // First prototype intentionally derives one shared mechanical state from mono sum
    // to prevent uncorrelated left/right machine behaviour.
    for (std::size_t i = 0; i < frames; ++i) {
        const float l = sanitize(left[i]);
        const float r = sanitize(right[i]);
        const float mono = 0.5f * (l + r);
        const float processedMono = processOne(mono);
        const float delta = processedMono - mono;
        left[i] = sanitize(l + delta);
        right[i] = sanitize(r + delta);
    }
}

} // namespace mechamorph
