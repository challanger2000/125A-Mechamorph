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

// 125A control law:
// 0% = neutral/off
// 20..50% = quickly reaches a clearly useful musical range
// 50..75% = strong
// 75..100% = creative/extreme
float intensityCurve(float x) noexcept {
    x = std::clamp(std::isfinite(x) ? x : 0.0f, 0.0f, 1.0f);
    if (x <= 0.5f)
        return 1.4f * x; // 50% -> 70%
    if (x <= 0.75f)
        return 0.70f + 0.80f * (x - 0.5f); // 75% -> 90%
    return 0.90f + 0.40f * (x - 0.75f);    // 100% -> 100%
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
    const float crank = intensityCurve(p.crank);
    const float wobble = intensityCurve(p.wobble);
    const float wear = intensityCurve(p.wear);

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
    const float air = intensityCurve(p.air);
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

void PipeEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
    setFrequency(440.0f);
    reset();
}

void PipeEngine::reset() noexcept {
    delay_.fill(0.0f);
    writeIndex_ = 0;
    boreLowpass_ = 0.0f;
    previousPressure_ = 0.0f;
    chiffEnv_ = 0.0f;
}

void PipeEngine::setFrequency(float frequencyHz) noexcept {
    const float f = std::clamp(frequencyHz, 40.0f, static_cast<float>(0.20 * sampleRate_));
    const std::size_t d = static_cast<std::size_t>(std::max(2.0, std::round(sampleRate_ / f)));
    delaySamples_ = std::min<std::size_t>(kMaxDelay - 2, d);
}

float PipeEngine::process(float pressure, float aperture, DeterministicRng& rng) noexcept {
    pressure = std::clamp(pressure, 0.0f, 1.0f);
    aperture = std::clamp(aperture, 0.0f, 1.0f);

    const std::size_t readIndex =
        (writeIndex_ + kMaxDelay - delaySamples_) % kMaxDelay;
    const float bore = delay_[readIndex];

    // Onset/chiff is driven by positive pressure change, not continuously.
    const float rise = std::max(0.0f, pressure - previousPressure_);
    chiffEnv_ = std::max(chiffEnv_ * 0.9965f, 4.0f * rise);
    previousPressure_ = pressure;

    const float thresholdedPressure =
        std::max(0.0f, pressure * aperture - 0.08f);
    const float jetNoise =
        rng.bipolar() * chiffEnv_ * (0.010f + 0.020f * pressure);

    // Reduced jet/bore interaction. The nonlinearity injects energy while
    // the delayed bore pressure supplies the acoustic feedback.
    const float jet =
        std::tanh(2.6f * thresholdedPressure - 1.15f * bore) +
        jetNoise;

    // Frequency-dependent bore loss proxy: one-pole low-pass in the loop.
    boreLowpass_ += 0.18f * (bore - boreLowpass_);
    const float feedback = 0.985f * boreLowpass_;

    const float excitation =
        thresholdedPressure > 0.0f ? (0.020f * jet + feedback) : 0.985f * feedback;

    delay_[writeIndex_] = sanitize(excitation);
    writeIndex_ = (writeIndex_ + 1) % kMaxDelay;

    // Radiation emphasizes the pressure difference rather than raw bore state.
    const float radiated = 0.75f * (bore - boreLowpass_) + 0.25f * bore;
    return sanitize(radiated * 0.65f);
}

void ValveEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
    // Fast but finite mechanical opening/closing.
    apertureAttack_ = static_cast<float>(std::exp(-1.0 / (0.0035 * sampleRate_)));
    apertureRelease_ = static_cast<float>(std::exp(-1.0 / (0.0055 * sampleRate_)));
    reset();
}

void ValveEngine::reset() noexcept {
    open_ = false;
    aperture_ = 0.0f;
    targetAperture_ = 0.0f;
    chuffEnv_ = 0.0f;
    clickEnv_ = 0.0f;
    noiseState_ = 0.0f;
    clickPolarity_ = 1.0f;
    chuffDecay_ = static_cast<float>(std::exp(-1.0 / (0.018 * sampleRate_)));
    clickDecay_ = static_cast<float>(std::exp(-1.0 / (0.0012 * sampleRate_)));
}

void ValveEngine::open(float pressure, float force, DeterministicRng& rng) noexcept {
    open_ = true;
    targetAperture_ = std::clamp(0.65f + 0.35f * force, 0.0f, 1.0f);
    chuffEnv_ = std::clamp(pressure, 0.0f, 1.0f) * (0.55f + 0.45f * force);
    clickEnv_ = 0.55f + 0.35f * force;
    clickPolarity_ = rng.uniform01() < 0.5f ? -1.0f : 1.0f;
}

void ValveEngine::close(float pressure, float force, DeterministicRng& rng) noexcept {
    open_ = false;
    targetAperture_ = 0.0f;
    chuffEnv_ = std::max(chuffEnv_, 0.25f * std::clamp(pressure, 0.0f, 1.0f));
    clickEnv_ = 0.45f + 0.30f * force;
    clickPolarity_ = rng.uniform01() < 0.5f ? -1.0f : 1.0f;
}

float ValveEngine::process(float pressure, DeterministicRng& rng) noexcept {
    pressure = std::clamp(pressure, 0.0f, 1.0f);

    const float coeff = targetAperture_ > aperture_ ? apertureAttack_ : apertureRelease_;
    aperture_ = coeff * aperture_ + (1.0f - coeff) * targetAperture_;
    if (std::fabs(aperture_) < kTiny) aperture_ = 0.0f;

    const float white = rng.bipolar();
    // Air path: low/mid turbulent component rather than full-band hiss.
    noiseState_ += 0.10f * (white - noiseState_);
    if (std::fabs(noiseState_) < kTiny) noiseState_ = 0.0f;

    const float chuff = noiseState_ * chuffEnv_ * 0.16f;
    chuffEnv_ *= chuffDecay_;
    if (chuffEnv_ < 1.0e-6f) chuffEnv_ = 0.0f;

    // Opening/closing contact: very short click with a small opposite-polarity
    // recoil on the following sample through the envelope decay.
    const float click = clickPolarity_ * clickEnv_ * 0.14f;
    clickEnv_ *= clickDecay_;
    if (clickEnv_ < 1.0e-6f) clickEnv_ = 0.0f;

    // Sustained air flow while the valve is open. This is deliberately quiet;
    // the sounding element (pipe/reed) should be driven by pressure/aperture,
    // while this remains the audible valve/air mechanism itself.
    const float flow = noiseState_ * aperture_ * pressure * 0.035f;

    return sanitize(click + chuff + flow);
}

void ContactClackEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
    reset();
}

void ContactClackEngine::reset() noexcept {
    envelope_ = 0.0f;
    envelopeDecay_ = 0.0f;
    reson1_ = reson2_ = reson1Prev_ = reson2Prev_ = 0.0f;
    excitation_ = 0.0f;
}

float ContactClackEngine::trigger(
    float force,
    float hardness,
    float material,
    DeterministicRng& rng) noexcept {

    force = std::clamp(force, 0.0f, 1.0f);
    hardness = std::clamp(hardness, 0.0f, 1.0f);
    material = std::clamp(material, 0.0f, 1.0f);

    // Very short contact excitation: harder contact = shorter/brighter transient.
    const float decayMs = 1.8f - 1.2f * hardness;
    envelopeDecay_ = static_cast<float>(
        std::exp(-1.0 / (std::max(0.0002f, decayMs * 0.001f) * sampleRate_)));
    envelope_ = force;

    // Two deliberately short contact/body resonances.
    // A small pawl/lever clack should decay in tens of milliseconds, not ring
    // like a struck musical resonator.
    const float f1 = 700.0f + 1500.0f * material;
    const float f2 = 1900.0f + 4300.0f * material;
    const float d1 = 0.006f + 0.006f * material;
    const float d2 = 0.0025f + 0.0045f * material;
    const float r1 = static_cast<float>(std::exp(-1.0 / (d1 * sampleRate_)));
    const float r2 = static_cast<float>(std::exp(-1.0 / (d2 * sampleRate_)));

    reson1A1_ = 2.0f * r1 * std::cos(static_cast<float>(kTwoPi * f1 / sampleRate_));
    reson1A2_ = -(r1 * r1);
    reson2A1_ = 2.0f * r2 * std::cos(static_cast<float>(kTwoPi * f2 / sampleRate_));
    reson2A2_ = -(r2 * r2);

    // Small stochastic contact asperity keeps repeated hits from sounding identical.
    excitation_ = force * (0.75f + 0.25f * rng.uniform01());
    return excitation_;
}

float ContactClackEngine::process() noexcept {
    if (envelope_ <= 0.0f && std::fabs(reson1_) < kTiny && std::fabs(reson2_) < kTiny)
        return 0.0f;

    const float contact = excitation_ * envelope_;
    envelope_ *= envelopeDecay_;
    if (envelope_ < 1.0e-6f) envelope_ = 0.0f;

    const float y1 = 0.018f * contact + reson1A1_ * reson1_ + reson1A2_ * reson1Prev_;
    reson1Prev_ = reson1_;
    reson1_ = sanitize(y1);

    const float y2 = 0.010f * contact + reson2A1_ * reson2_ + reson2A2_ * reson2Prev_;
    reson2Prev_ = reson2_;
    reson2_ = sanitize(y2);

    if (std::fabs(reson1_) < kTiny) reson1_ = 0.0f;
    if (std::fabs(reson2_) < kTiny) reson2_ = 0.0f;

    return sanitize(0.22f * contact + 0.55f * reson1_ + 0.40f * reson2_);
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

    const float wear = intensityCurve(p.wear);
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

    const float wear = intensityCurve(p.wear);
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

    const float wear = intensityCurve(p.wear);
    const float variation = 1.0f + 0.18f * wear * rng.bipolar();
    return 0.12f *
           intensityCurve(p.mechanize) *
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
    const float clatter = intensityCurve(p.clatter);
    const bool trigger = transientHigh && !transientLatched_ && clatter > 0.0f;
    transientLatched_ = transientHigh;

    if (trigger) {
        const int requested = 1 + static_cast<int>(
            6.0f * std::clamp(clatter * (0.35f + state.wear), 0.0f, 1.0f));
        int placed = 0;
        for (auto& e : events_) {
            if (placed >= requested) break;
            if (e.remaining <= 0) {
                const float delayMs = 3.0f + 35.0f * rng.uniform01();
                e.remaining = std::max(1, static_cast<int>(delayMs * 0.001 * sampleRate_));
                e.amplitude = (0.045f + 0.11f * rng.uniform01()) * clatter;
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
    clack_.prepare(sampleRate_);
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
    clack_.reset();
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

    const float mechanize = intensityCurve(p.mechanize);
    const float bodyAmount = intensityCurve(p.body);
    const float wobbleAmount = intensityCurve(p.wobble);
    const float wearAmount = intensityCurve(p.wear);

    // Analyze channel energy without phase cancellation. The mechanical state
    // remains shared, but each audio channel excites its own matching body.
    const float analysisInput = stereo
        ? 0.5f * (std::fabs(inputL) + std::fabs(inputR))
        : std::fabs(inputL);

    analyzer_.process(analysisInput, state_);
    state_.load = std::clamp(state_.inputEnvelope * mechanize, 0.0f, 1.0f);
    drive_.process(p, state_);

    const float gear = gear_.process(p, state_, rngGear_);
    const float ratchet = ratchet_.process(p, state_, rngRatchet_);

    if (gear > 0.0f)
        clack_.trigger(std::clamp(gear * 9.0f, 0.0f, 1.0f),
                       0.55f + 0.25f * state_.wear,
                       0.35f,
                       rngGear_);
    if (ratchet > 0.0f)
        clack_.trigger(std::clamp(ratchet * 7.0f, 0.0f, 1.0f),
                       0.72f,
                       0.55f,
                       rngRatchet_);
    const float clack = clack_.process();
    const float rattle = rattle_.process(p, state_, rngRattle_);
    const float air = air_.process(p, state_, rngAir_);
    const float friction = friction_.process(p, state_, rngFriction_);

    const float sharedMechanicalExcitation =
        0.35f * gear + 0.35f * ratchet + clack + rattle + air + friction;
    const float sourceExcitation = 0.42f * mechanize;

    const float bodyL = bodyL_.process(
        sourceExcitation * inputL + 1.35f * sharedMechanicalExcitation) * bodyAmount;
    const float bodyR = stereo
        ? bodyR_.process(sourceExcitation * inputR + 1.35f * sharedMechanicalExcitation) * bodyAmount
        : bodyL;

    const float directMechanics =
        0.12f * gear +
        0.12f * ratchet +
        0.80f * clack +
        0.55f * rattle +
        1.20f * air +
        0.70f * friction;

    const float sourceRetention = 1.0f - 0.78f * bodyAmount;
    const float mechanizedL =
        sourceRetention * inputL +
        1.10f * bodyL +
        directMechanics;
    const float mechanizedR =
        sourceRetention * inputR +
        1.10f * bodyR +
        directMechanics;

    const float driveWobble =
        1.0f +
        (0.035f * wobbleAmount + 0.020f * wearAmount) *
        static_cast<float>(std::sin(state_.phase));

    const float wet = mechanize;
    float yL = (1.0f - wet) * inputL + wet * (mechanizedL * driveWobble);
    float yR = (1.0f - wet) * inputR + wet * (mechanizedR * driveWobble);

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
