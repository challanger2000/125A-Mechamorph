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
    const double sr = std::max(sampleRate, 1.0);
    activityAttack_ = static_cast<float>(std::exp(-1.0 / (0.020 * sr)));
    activityRelease_ = static_cast<float>(std::exp(-1.0 / (0.500 * sr)));
}

void InputAnalyzer::reset() noexcept {
    fast_.reset();
    slow_.reset();
    activity_ = 0.0f;
}

void InputAnalyzer::process(float monoSample, MechanicalState& state) noexcept {
    const float fast = fast_.process(monoSample);
    const float slow = slow_.process(monoSample);
    state.inputEnvelope = slow;
    state.transientStrength = std::max(0.0f, fast - slow);

    const float target = std::clamp(
        6.0f * slow + 10.0f * state.transientStrength,
        0.0f, 1.0f);
    const float coeff = target > activity_ ? activityAttack_ : activityRelease_;
    activity_ = coeff * activity_ + (1.0f - coeff) * target;
    if (std::fabs(activity_) < kTiny) activity_ = 0.0f;
    state.activity = activity_;
}

void MechanicalDrive::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
}

void MechanicalDrive::reset() noexcept {
}

void MechanicalDrive::process(const Parameters& p, MechanicalState& state) noexcept {
    // Prototype mapping only; to be replaced with measured ranges.
    const float crank = std::clamp(p.crank, 0.0f, 1.0f);
    const float wobble = std::clamp(p.wobble, 0.0f, 1.0f);
    const float wear = std::clamp(p.wear, 0.0f, 1.0f);

    state.targetSpeedHz = 0.35f + 3.65f * crank;

    // Slow deterministic drift derived from phase, not unrelated random timing.
    const float eccentric = 0.080f * wobble *
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
        // Energy-normalized excitation keeps long-decay/high-Q modes from
        // accumulating excessive gain under periodic input.
        // Final modal gains remain EMPIRICALLY TUNED until measured references exist.
        const float excitationNorm = std::sqrt(std::max(0.0f, 1.0f - r * r));
        m.b0 = modes[i].gain * excitationNorm;
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
        if (std::fabs(m.z1) < kTiny) m.z1 = 0.0f;
        if (std::fabs(m.z2) < kTiny) m.z2 = 0.0f;
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
    const float inflowPerSecond =
        air * state.activity * (0.35f + 1.15f * phasePump);

    // Pressure-dependent losses avoid an artificial "nothing happens until
    // inflow exceeds a fixed leak" threshold. This behaves more like a real
    // reservoir: any pump input can build some pressure, while leakage and
    // load drain proportionally to the pressure already present.
    // Rates remain EMPIRICALLY TUNED pending bellows measurements.
    const float leakRatePerSecond = 1.0f + 1.5f * state.wear;
    const float loadRatePerSecond =
        0.50f * state.inputEnvelope * air;
    const float outflowPerSecond =
        state.pressure * (leakRatePerSecond + loadRatePerSecond);

    state.pressure = std::clamp(
        state.pressure + dt * (inflowPerSecond - outflowPerSecond),
        0.0f, 1.0f);
    state.leak = leakRatePerSecond;

    const float white = rng.bipolar();
    // Very cheap low-pass coloration for first prototype.
    noiseState_ += 0.05f * (white - noiseState_);
    if (std::fabs(noiseState_) < kTiny) noiseState_ = 0.0f;
    return noiseState_ * state.pressure * air * 0.12f;
}

void FrictionEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
    reset();
}

void FrictionEngine::reset() noexcept {
    roughnessState_ = 0.0f;
    slipState_ = 0.0f;
}

float FrictionEngine::process(
    const Parameters& p,
    const MechanicalState& state,
    DeterministicRng& rng) noexcept {

    const float wear = std::clamp(p.wear, 0.0f, 1.0f);
    const float activity = std::clamp(state.activity, 0.0f, 1.0f);
    if (wear <= 0.0f || activity <= 0.0f)
        return 0.0f;

    const float speedNorm = std::clamp(state.speedHz / 4.0f, 0.0f, 1.0f);
    const float load = std::clamp(state.load, 0.0f, 1.0f);

    // Reduced physically-informed prototype:
    // rough contact noise follows speed while a slower nonlinear state
    // approximates stick/slip transitions. Coefficients remain EMPIRICALLY TUNED.
    const float white = rng.bipolar();
    const float roughAlpha = std::clamp(
        static_cast<float>((120.0 + 1600.0 * speedNorm) / sampleRate_),
        0.001f, 0.25f);
    roughnessState_ += roughAlpha * (white - roughnessState_);

    const float contactDrive =
        roughnessState_ +
        0.20f * static_cast<float>(std::sin(3.0 * state.phase));
    const float nonlinear = std::tanh((2.0f + 6.0f * wear) * contactDrive);

    const float slipAlpha = std::clamp(
        static_cast<float>((35.0 + 220.0 * speedNorm) / sampleRate_),
        0.0005f, 0.08f);
    slipState_ += slipAlpha * (nonlinear - slipState_);

    if (std::fabs(roughnessState_) < kTiny) roughnessState_ = 0.0f;
    if (std::fabs(slipState_) < kTiny) slipState_ = 0.0f;

    const float pressure = 0.20f + 0.80f * load;
    const float friction =
        (0.70f * roughnessState_ + 0.30f * slipState_) *
        wear * activity * pressure * 0.060f;

    return sanitize(friction);
}

void GearEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
    reset();
}

void GearEngine::reset() noexcept {
    previousPhase_ = 0.0;
    backlashRemaining_ = 0;
    backlashAmplitude_ = 0.0f;
}

float GearEngine::process(const Parameters& p, const MechanicalState& state, DeterministicRng& rng) noexcept {
    float out = 0.0f;

    if (backlashRemaining_ > 0) {
        --backlashRemaining_;
        if (backlashRemaining_ == 0) {
            out += backlashAmplitude_;
            backlashAmplitude_ = 0.0f;
        }
    }
    constexpr int teeth = 24;
    const double sector = kTwoPi / static_cast<double>(teeth);
    const int prevIndex = static_cast<int>(previousPhase_ / sector);
    const int currentIndex = static_cast<int>(state.phase / sector);
    const double oldPhase = previousPhase_;
    previousPhase_ = state.phase;

    bool fired = currentIndex != prevIndex;
    if (state.phase < sector && oldPhase > kTwoPi - sector)
        fired = true;

    if (!fired) return out;

    const float wear = std::clamp(p.wear, 0.0f, 1.0f);
    const float hardness = 0.75f + 0.25f * rng.uniform01();
    const float irregularity = 1.0f + (0.04f + 0.10f * wear) * rng.bipolar();
    const float primary =
        0.070f *
        std::clamp(p.mechanize, 0.0f, 1.0f) *
        state.activity *
        hardness * irregularity;

    out += primary;

    // Backlash is represented as a bounded delayed re-contact event tied to
    // the same gear tooth impact. No free-running random clacks.
    if (backlashRemaining_ <= 0 && primary != 0.0f) {
        const float backlash = std::clamp(state.backlash, 0.0f, 1.0f);
        const float probability = 0.10f + 0.80f * backlash;
        if (backlash > 0.0f && rng.uniform01() < probability) {
            const float delayMs =
                0.4f + (0.8f + 2.2f * backlash) * rng.uniform01();
            backlashRemaining_ = std::max(
                1, static_cast<int>(delayMs * 0.001f * static_cast<float>(sampleRate_)));
            backlashAmplitude_ =
                primary *
                (0.10f + 0.45f * backlash) *
                (0.75f + 0.25f * rng.uniform01());
        }
    }

    return sanitize(out);
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
    return 0.12f *
           std::clamp(p.mechanize, 0.0f, 1.0f) *
           state.activity *
           variation;
}

void RattleEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
}

void RattleEngine::reset() noexcept {
    for (auto& e : events_) e = {};
    transientLatched_ = false;
}

float RattleEngine::process(const Parameters& p, const MechanicalState& state, DeterministicRng& rng) noexcept {
    // Trigger one bounded cluster on a transient threshold crossing.
    const bool transientHigh = state.transientStrength > 0.02f;
    const bool trigger = transientHigh && !transientLatched_ && p.clatter > 0.0f;
    transientLatched_ = transientHigh;

    if (trigger) {
        const int requested = 1 + static_cast<int>(
            4.0f * std::clamp(p.clatter * (0.3f + state.wear), 0.0f, 1.0f));
        int placed = 0;
        for (auto& e : events_) {
            if (placed >= requested) break;
            if (e.remaining <= 0) {
                const float delayMs = 3.0f + 35.0f * rng.uniform01();
                e.remaining = std::max(1, static_cast<int>(delayMs * 0.001 * sampleRate_));
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
    rngAir_.seed(0x125A0001ULL);
    rngFriction_.seed(0x125A0002ULL);
    rngGear_.seed(0x125A0003ULL);
    rngRatchet_.seed(0x125A0004ULL);
    rngRattle_.seed(0x125A0005ULL);
    analyzer_.prepare(sampleRate_);
    drive_.prepare(sampleRate_);
    bodyL_.prepare(sampleRate_);
    bodyR_.prepare(sampleRate_);
    air_.prepare(sampleRate_);
    friction_.prepare(sampleRate_);
    gear_.prepare(sampleRate_);
    rattle_.prepare(sampleRate_);

    // Placeholder prototype body. All values are EMPIRICALLY TUNED and
    // must be replaced/calibrated from measurements before product claims.
    const ModalMode wood[] = {
        { 170.0f, 0.11f, 0.080f },
        { 315.0f, 0.08f, 0.060f },
        { 520.0f, 0.07f, 0.045f },
        { 780.0f, 0.055f, 0.030f },
        { 1180.0f, 0.045f, 0.020f },
        { 1760.0f, 0.035f, 0.012f },
    };
    bodyL_.setModes(wood, sizeof(wood) / sizeof(wood[0]));
    bodyR_.setModes(wood, sizeof(wood) / sizeof(wood[0]));
    reset();
}

void Core::reset() noexcept {
    state_ = {};
    rngAir_.seed(0x125A0001ULL);
    rngFriction_.seed(0x125A0002ULL);
    rngGear_.seed(0x125A0003ULL);
    rngRatchet_.seed(0x125A0004ULL);
    rngRattle_.seed(0x125A0005ULL);
    analyzer_.reset();
    drive_.reset();
    bodyL_.reset();
    bodyR_.reset();
    air_.reset();
    friction_.reset();
    gear_.reset();
    ratchet_.reset();
    rattle_.reset();
}

float Core::clamp01(float x) noexcept {
    return std::clamp(std::isfinite(x) ? x : 0.0f, 0.0f, 1.0f);
}

void Core::processFrame(
    float inputL,
    float inputR,
    bool stereo,
    float& outputL,
    float& outputR) noexcept {

    inputL = sanitize(inputL);
    inputR = sanitize(inputR);

    Parameters p = params_;
    p.mechanize = clamp01(p.mechanize);
    p.crank = clamp01(p.crank);
    p.clatter = clamp01(p.clatter);
    p.wobble = clamp01(p.wobble);
    p.air = clamp01(p.air);
    p.body = clamp01(p.body);
    p.wear = clamp01(p.wear);
    p.output = clamp01(p.output);

    // Analyze channel energy without phase cancellation. The mechanical state
    // remains shared, but each audio channel excites its own matching body.
    const float analysisInput = stereo
        ? 0.5f * (std::fabs(inputL) + std::fabs(inputR))
        : std::fabs(inputL);

    analyzer_.process(analysisInput, state_);
    state_.load = std::clamp(state_.inputEnvelope * p.mechanize, 0.0f, 1.0f);
    drive_.process(p, state_);

    const float gear = gear_.process(p, state_, rngGear_);
    const float ratchet = ratchet_.process(p, state_, rngRatchet_);
    const float rattle = rattle_.process(p, state_, rngRattle_);
    const float air = air_.process(p, state_, rngAir_);
    const float friction = friction_.process(p, state_, rngFriction_);

    const float sharedMechanicalExcitation =
        gear + ratchet + rattle + air + friction;
    const float sourceExcitation = 0.18f * p.mechanize;

    const float bodyL = bodyL_.process(
        sourceExcitation * inputL + sharedMechanicalExcitation) * p.body;
    const float bodyR = stereo
        ? bodyR_.process(sourceExcitation * inputR + sharedMechanicalExcitation) * p.body
        : bodyL;

    const float directMechanics =
        0.16f * gear +
        0.20f * ratchet +
        0.28f * rattle +
        air +
        0.35f * friction;

    const float sourceRetention = 1.0f - 0.55f * p.body;
    const float mechanizedL =
        sourceRetention * inputL +
        0.85f * bodyL +
        directMechanics;
    const float mechanizedR =
        sourceRetention * inputR +
        0.85f * bodyR +
        directMechanics;

    const float wet = clamp01(p.mechanize);
    float yL = (1.0f - wet) * inputL + wet * mechanizedL;
    float yR = (1.0f - wet) * inputR + wet * mechanizedR;

    const float outputDb = 24.0f * (p.output - 0.5f);
    const float gain = std::pow(10.0f, outputDb / 20.0f);
    yL *= gain;
    yR *= gain;

    outputL = sanitize(yL);
    outputR = sanitize(yR);
}

void Core::process(float* left, float* right, std::size_t frames) noexcept {
    if (!left || frames == 0) return;

    if (!right) {
        for (std::size_t i = 0; i < frames; ++i) {
            float outL = 0.0f;
            float dummy = 0.0f;
            processFrame(left[i], 0.0f, false, outL, dummy);
            left[i] = outL;
        }
        return;
    }

    for (std::size_t i = 0; i < frames; ++i) {
        float outL = 0.0f;
        float outR = 0.0f;
        processFrame(left[i], right[i], true, outL, outR);
        left[i] = outL;
        right[i] = outR;
    }
}

} // namespace mechamorph
