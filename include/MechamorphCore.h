#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mechamorph {

struct Parameters {
    float mechanize = 0.35f;
    float crank = 0.35f;
    float clatter = 0.25f;
    float wobble = 0.15f;
    float air = 0.20f;
    float body = 0.35f;
    float wear = 0.20f;
    float output = 0.50f;
};

struct MechanicalState {
    double phase = 0.0;
    float speedHz = 0.0f;
    float targetSpeedHz = 0.0f;
    float load = 0.0f;
    float wear = 0.0f;
    float backlash = 0.0f;
    float looseness = 0.0f;
    float pressure = 0.0f;
    float leak = 0.0f;
    float inputEnvelope = 0.0f;
    float transientStrength = 0.0f;
};

class DeterministicRng {
public:
    void seed(std::uint64_t s) noexcept;
    std::uint64_t nextU64() noexcept;
    float uniform01() noexcept;
    float bipolar() noexcept;

private:
    std::uint64_t state_ = 0x9E3779B97F4A7C15ULL;
};

class OnePoleEnvelope {
public:
    void prepare(double sampleRate, double timeMs) noexcept;
    void reset() noexcept { state_ = 0.0f; }
    float process(float x) noexcept;

private:
    float coefficient_ = 0.0f;
    float state_ = 0.0f;
};

class InputAnalyzer {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void process(float monoSample, MechanicalState& state) noexcept;

private:
    OnePoleEnvelope fast_;
    OnePoleEnvelope slow_;
};

class MechanicalDrive {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void process(const Parameters& p, MechanicalState& state) noexcept;

private:
    double sampleRate_ = 48000.0;
};

struct ModalMode {
    float frequencyHz = 440.0f;
    float decaySeconds = 0.1f;
    float gain = 0.0f;
};

class ModalResonator {
public:
    static constexpr std::size_t kMaxModes = 12;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setModes(const ModalMode* modes, std::size_t count) noexcept;
    float process(float excitation) noexcept;

private:
    struct ModeState {
        float a1 = 0.0f;
        float a2 = 0.0f;
        float b0 = 0.0f;
        float z1 = 0.0f;
        float z2 = 0.0f;
    };

    double sampleRate_ = 48000.0;
    std::array<ModeState, kMaxModes> modes_{};
    std::size_t modeCount_ = 0;
};

class AirEngine {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    float process(const Parameters& p, MechanicalState& state, DeterministicRng& rng) noexcept;

private:
    double sampleRate_ = 48000.0;
    float noiseState_ = 0.0f;
};

class GearEngine {
public:
    void reset() noexcept;
    float process(const Parameters& p, const MechanicalState& state, DeterministicRng& rng) noexcept;

private:
    double previousPhase_ = 0.0;
};

class RatchetEngine {
public:
    void reset() noexcept;
    float process(const Parameters& p, const MechanicalState& state, DeterministicRng& rng) noexcept;

private:
    double previousPhase_ = 0.0;
};

class RattleEngine {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    float process(const Parameters& p, const MechanicalState& state, DeterministicRng& rng) noexcept;

private:
    static constexpr std::size_t kMaxEvents = 8;
    struct Event {
        int remaining = 0;
        float amplitude = 0.0f;
    };
    std::array<Event, kMaxEvents> events_{};
    double sampleRate_ = 48000.0;
    bool transientLatched_ = false;
};

class Core {
public:
    void prepare(double sampleRate, std::size_t maxBlockSize) noexcept;
    void reset() noexcept;
    void setParameters(const Parameters& p) noexcept { params_ = p; }
    const Parameters& parameters() const noexcept { return params_; }
    const MechanicalState& state() const noexcept { return state_; }

    void process(float* left, float* right, std::size_t frames) noexcept;

private:
    float processOne(float input) noexcept;
    static float clamp01(float x) noexcept;

    Parameters params_{};
    MechanicalState state_{};
    DeterministicRng rng_{};
    InputAnalyzer analyzer_{};
    MechanicalDrive drive_{};
    ModalResonator body_{};
    AirEngine air_{};
    GearEngine gear_{};
    RatchetEngine ratchet_{};
    RattleEngine rattle_{};
    double sampleRate_ = 48000.0;
};

} // namespace mechamorph
