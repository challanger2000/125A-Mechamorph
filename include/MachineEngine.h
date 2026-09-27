#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace mechamorph::machine {

enum class MachinePhase : std::uint8_t {
    Stopped,
    Starting,
    Running,
    Loaded,
    Stopping
};

enum class SampleRole : std::uint8_t {
    Start,
    RunBed,
    Action,
    Load,
    Release,
    Stop,
    Count
};

struct MachineParameters {
    float speed = 0.35f;
    float load = 0.25f;
    float wear = 0.20f;
    float activity = 0.50f;
    float body = 0.35f;
    float output = 0.50f;
};

struct MachineState {
    MachinePhase phase = MachinePhase::Stopped;
    double drivePhase = 0.0;
    float speedHz = 0.0f;
    float targetSpeedHz = 0.0f;
    float load = 0.0f;
    float wear = 0.0f;
    float backlash = 0.0f;
    float friction = 0.0f;
    float energy = 0.0f;
    float runBlend = 0.0f;
};

struct SampleView {
    const float* data = nullptr;
    std::size_t frames = 0;
    int sampleRate = 48000;

    bool valid() const noexcept {
        return data != nullptr && frames > 0 && sampleRate > 0;
    }
};

struct SampleSet {
    static constexpr std::size_t kMaxVariants = 8;

    std::array<SampleView, kMaxVariants> variants{};
    std::size_t count = 0;

    bool add(SampleView sample) noexcept {
        if (!sample.valid() || count >= kMaxVariants) return false;
        variants[count++] = sample;
        return true;
    }
};

class MachineEngine {
public:
    static constexpr std::size_t kMaxVoices = 16;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;

    void setParameters(const MachineParameters& p) noexcept;
    const MachineParameters& parameters() const noexcept { return params_; }
    const MachineState& state() const noexcept { return state_; }

    bool setSampleSet(SampleRole role, const SampleSet& set) noexcept;

    void start() noexcept;
    void stop() noexcept;
    void setLoaded(bool loaded) noexcept;

    float processSample() noexcept;
    void process(float* mono, std::size_t frames) noexcept;

private:
    struct Voice {
        SampleView sample{};
        double position = 0.0;
        double rate = 1.0;
        float gain = 0.0f;
        bool loop = false;
        bool active = false;
    };

    static float clamp01(float x) noexcept;
    static float intensityCurve(float x) noexcept;

    void updateMachineState() noexcept;
    void updateTransitions() noexcept;
    void triggerRole(SampleRole role, float gain, bool loop) noexcept;
    void ensureRunBed() noexcept;
    float renderVoices() noexcept;
    std::size_t chooseVariant(const SampleSet& set) noexcept;
    float random01() noexcept;

    double sampleRate_ = 48000.0;
    MachineParameters params_{};
    MachineState state_{};

    std::array<SampleSet, static_cast<std::size_t>(SampleRole::Count)> sets_{};
    std::array<Voice, kMaxVoices> voices_{};

    bool requestStart_ = false;
    bool requestStop_ = false;
    bool loaded_ = false;
    bool wasLoaded_ = false;

    float startProgress_ = 0.0f;
    float stopProgress_ = 0.0f;
    float runEnvelope_ = 0.0f;
    float actionAccumulator_ = 0.0f;

    std::uint64_t rngState_ = 0x125A5A5A12345678ULL;
};

} // namespace mechamorph::machine
