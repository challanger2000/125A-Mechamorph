#include "MachineCharacterEngine.h"

#include <algorithm>
#include <cmath>

namespace mechamorph::machine {
namespace {
constexpr double kPi = 3.14159265358979323846;
float clamp01(float x) noexcept {
    return std::clamp(std::isfinite(x) ? x : 0.0f, 0.0f, 1.0f);
}
}

float MachineCharacterEngine::Resonator::process(float x) noexcept {
    const float y = gain * x + a1 * y1 + a2 * y2;
    y2 = y1;
    y1 = std::isfinite(y) ? y : 0.0f;
    return y1;
}

void MachineCharacterEngine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(1.0, sampleRate);
    updateCoefficients();
    reset();
}

void MachineCharacterEngine::reset() noexcept {
    for (auto& m : bodyModes_) m.reset();
    for (auto& d : delay_) d.fill(0.0f);
    write_.fill(0);
    dampState_.fill(0.0f);
    early1_ = early2_ = early3_ = 0.0f;
}

void MachineCharacterEngine::setParameters(const CharacterParameters& p) noexcept {
    params_.body = clamp01(p.body);
    params_.space = clamp01(p.space);
    params_.scale = clamp01(p.scale);
    updateCoefficients();
}

void MachineCharacterEngine::updateCoefficients() noexcept {
    const float scale = clamp01(params_.scale);

    // BODY is calibrated from the early "Retro Machines IR" character:
    // bright initial technical impulse, quickly darkening into low/mid body.
    const std::array<float,4> baseFreq { 180.0f, 430.0f, 980.0f, 2350.0f };
    const std::array<float,4> baseDecay { 0.045f, 0.036f, 0.026f, 0.018f };
    const std::array<float,4> gain { 1.40f, 1.05f, 0.72f, 0.42f };

    for (std::size_t i=0;i<bodyModes_.size();++i) {
        const float frequency = baseFreq[i] * (1.10f - 0.55f * scale);
        const float decay = baseDecay[i] * (0.80f + 1.55f * scale);
        const float r = static_cast<float>(std::exp(-1.0 / (decay * sampleRate_)));
        const float w = static_cast<float>(2.0 * kPi * frequency / sampleRate_);
        bodyModes_[i].a1 = 2.0f * r * std::cos(w);
        bodyModes_[i].a2 = -(r*r);
        // Normalize excitation against Q so high-r modes cannot explode.
        bodyModes_[i].gain =
            gain[i] * (1.0f - r) * (0.80f + 0.45f * scale);
    }

    // SPACE interpolates between a darker compact industrial hall
    // (PA-hall reference) and a brighter/larger factory hall at high SCALE.
    const std::array<float,4> msSmall { 37.1f, 53.7f, 71.9f, 89.3f };
    const std::array<float,4> msLarge { 61.7f, 83.9f, 109.1f, 137.3f };
    for (std::size_t i=0;i<kDelayCount;++i) {
        const float ms = msSmall[i] + scale * (msLarge[i] - msSmall[i]);
        const std::size_t n = static_cast<std::size_t>(ms * 0.001 * sampleRate_);
        length_[i] = std::clamp<std::size_t>(n, 2, kMaxDelay-1);
    }
}

float MachineCharacterEngine::process(float input) noexcept {
    const float body = clamp01(params_.body);
    const float space = clamp01(params_.space);
    const float scale = clamp01(params_.scale);

    float bodySignal = 0.0f;
    for (auto& mode : bodyModes_)
        bodySignal += mode.process(input);

    const float bodied =
        input * (1.0f - 0.22f * body) +
        bodySignal * body;

    // Sparse early reflections first; bigger machines get wider timing.
    early1_ += 0.20f * (bodied - early1_);
    early2_ += 0.085f * (early1_ - early2_);
    early3_ += 0.035f * (early2_ - early3_);
    const float early =
        0.28f * early1_ +
        0.18f * early2_ +
        0.12f * early3_;

    std::array<float,kDelayCount> tap{};
    float sum = 0.0f;
    for (std::size_t i=0;i<kDelayCount;++i) {
        const auto len = length_[i];
        const auto read = (write_[i] + kMaxDelay - len) % kMaxDelay;
        tap[i] = delay_[i][read];
        sum += tap[i];
    }

    // Darker compact PA-hall at low SCALE, brighter/larger factory hall at high SCALE.
    const float damping = 0.18f - 0.07f * scale;
    const float feedback = 0.56f + 0.18f * scale + 0.10f * space;

    // Energy-preserving 4x4 Hadamard feedback matrix.
    const std::array<float,kDelayCount> h {
        0.5f * (tap[0] + tap[1] + tap[2] + tap[3]),
        0.5f * (tap[0] - tap[1] + tap[2] - tap[3]),
        0.5f * (tap[0] + tap[1] - tap[2] - tap[3]),
        0.5f * (tap[0] - tap[1] - tap[2] + tap[3])
    };

    for (std::size_t i=0;i<kDelayCount;++i) {
        dampState_[i] += damping * (h[i] - dampState_[i]);
        const float writeValue =
            0.10f * bodied +
            feedback * dampState_[i];
        delay_[i][write_[i]] = std::clamp(writeValue, -1.5f, 1.5f);
        write_[i] = (write_[i] + 1) % kMaxDelay;
    }

    const float wetHall = 0.18f * sum + 0.20f * early;
    const float wet = space * space;
    return bodied * (1.0f - 0.42f * wet) + wetHall * (0.58f * wet);
}

} // namespace mechamorph::machine
