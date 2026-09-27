#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace mechamorph::machine {

enum class State : std::uint8_t {
    Stopped,
    Starting,
    Running,
    Loaded,
    Releasing,
    Stopping
};

enum class Role : std::uint8_t {
    Start,
    Run,
    Action,
    Load,
    Release,
    Stop
};

struct Clip {
    const float* mono = nullptr;
    std::size_t frames = 0;
    double sampleRate = 48000.0;
    bool loop = false;
    const char* name = nullptr;
};

struct Pool {
    static constexpr std::size_t kMaxClips = 32;
    std::array<Clip, kMaxClips> clips {};
    std::size_t count = 0;

    bool add(const Clip& clip) noexcept {
        if (count >= kMaxClips || !clip.mono || clip.frames == 0)
            return false;
        clips[count++] = clip;
        return true;
    }
};

struct SampleSet {
    Pool start;
    Pool run;
    Pool action;
    Pool load;
    Pool release;
    Pool stop;
};

struct Parameters {
    float speed = 0.50f;
    float load = 0.0f;
    float wear = 0.15f;
    float action = 0.35f;
    float clatter = 0.20f;
    float body = 0.30f;
    float pressure = 0.0f;
    float scale = 0.35f;   // internal physical size/mass, 0=tiny, 1=colossal
    float output = 0.50f;
};

class Rng {
public:
    void seed(std::uint64_t value) noexcept;
    std::uint64_t nextU64() noexcept;
    float uniform01() noexcept;
    float bipolar() noexcept;

private:
    std::uint64_t state_ = 0xA125A125A125A125ULL;
};

class Voice {
public:
    void reset() noexcept;
    void start(const Clip& clip, Role role, float gain, float rate, std::size_t startOffset = 0) noexcept;
    float process() noexcept;
    bool active() const noexcept { return active_; }
    bool looping() const noexcept { return clip_.loop; }
    Role role() const noexcept { return role_; }
    double sourceSampleRate() const noexcept { return clip_.sampleRate; }
    void setRate(double rate) noexcept { rate_ = std::clamp(rate, 0.25, 4.0); }

private:
    Clip clip_ {};
    Role role_ = Role::Action;
    double position_ = 0.0;
    double rate_ = 1.0;
    float gain_ = 1.0f;
    bool active_ = false;
};

class Engine {
public:
    static constexpr std::size_t kMaxVoices = 12;

    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setSampleSet(const SampleSet* set) noexcept { set_ = set; }
    void setParameters(const Parameters& p) noexcept { params_ = p; }

    void start() noexcept;
    void stop() noexcept;
    void triggerAction(float force = 1.0f) noexcept;
    void setLoadActive(bool active) noexcept;

    void process(float* monoOut, std::size_t frames) noexcept;

    State state() const noexcept { return state_; }
    double phase() const noexcept { return phase_; }

private:
    struct PendingEvent {
        Role role = Role::Action;
        int remainingSamples = 0;
        float force = 1.0f;
        bool active = false;
    };

    static constexpr std::size_t kMaxPending = 8;

    const Pool* poolFor(Role role) const noexcept;
    const Clip* chooseClip(Role role) noexcept;
    void spawn(Role role, float force, bool preferLoop = false) noexcept;
    void schedule(Role role, int delaySamples, float force) noexcept;
    void updateMachineState() noexcept;
    float renderVoices() noexcept;
    bool hasActiveRole(Role role) const noexcept;
    float outputGain() const noexcept;

    double sampleRate_ = 48000.0;
    const SampleSet* set_ = nullptr;
    Parameters params_ {};
    State state_ = State::Stopped;
    Rng rng_ {};

    std::array<Voice, kMaxVoices> voices_ {};
    std::array<PendingEvent, kMaxPending> pending_ {};
    std::array<std::size_t, 6> lastClipIndex_ {
        static_cast<std::size_t>(-1), static_cast<std::size_t>(-1),
        static_cast<std::size_t>(-1), static_cast<std::size_t>(-1),
        static_cast<std::size_t>(-1), static_cast<std::size_t>(-1)
    };

    double phase_ = 0.0;
    double previousPhase_ = 0.0;
    float inertiaState_ = 0.0f;
    float activity_ = 0.0f;
    float runBlend_ = 0.0f;
    bool runLoopSpawned_ = false;
    bool loadActive_ = false;
    int stopCountdown_ = 0;
};

} // namespace mechamorph::machine
