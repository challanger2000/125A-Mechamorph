#include "MachineSamplerEngine.h"

#include <algorithm>
#include <cmath>

namespace mechamorph::machine {
namespace {
constexpr double kTwoPi = 6.283185307179586476925286766559;

float clamp01(float x) noexcept {
    return std::clamp(std::isfinite(x) ? x : 0.0f, 0.0f, 1.0f);
}
}

void Rng::seed(std::uint64_t value) noexcept {
    state_ = value ? value : 0xA125A125A125A125ULL;
}

std::uint64_t Rng::nextU64() noexcept {
    std::uint64_t x = state_;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    state_ = x;
    return x * 2685821657736338717ULL;
}

float Rng::uniform01() noexcept {
    constexpr double scale = 1.0 / 9007199254740992.0;
    return static_cast<float>((nextU64() >> 11) * scale);
}

float Rng::bipolar() noexcept {
    return 2.0f * uniform01() - 1.0f;
}

void Voice::reset() noexcept {
    clip_ = {};
    position_ = 0.0;
    rate_ = 1.0;
    gain_ = 1.0f;
    active_ = false;
}

void Voice::start(
    const Clip& clip,
    float gain,
    float rate,
    std::size_t startOffset) noexcept {

    clip_ = clip;
    gain_ = gain;
    rate_ = std::clamp<double>(rate, 0.25, 4.0);
    position_ = static_cast<double>(std::min(startOffset, clip.frames - 1));
    active_ = clip.mono && clip.frames > 0;
}

float Voice::process() noexcept {
    if (!active_ || !clip_.mono || clip_.frames == 0)
        return 0.0f;

    const std::size_t i0 = static_cast<std::size_t>(position_);
    const std::size_t i1 = std::min(i0 + 1, clip_.frames - 1);
    const float frac = static_cast<float>(position_ - static_cast<double>(i0));

    float y =
        clip_.mono[i0] +
        frac * (clip_.mono[i1] - clip_.mono[i0]);

    if (clip_.loop && clip_.frames > 64) {
        const std::size_t fadeFrames = std::min<std::size_t>(
            clip_.frames / 4,
            std::max<std::size_t>(16, static_cast<std::size_t>(0.006 * clip_.sampleRate)));

        const double fadeStart = static_cast<double>(clip_.frames - fadeFrames);
        if (position_ >= fadeStart) {
            const double rel = position_ - fadeStart;
            const float mix = static_cast<float>(
                std::clamp(rel / static_cast<double>(fadeFrames), 0.0, 1.0));

            const double startPos = std::clamp(rel, 0.0, static_cast<double>(fadeFrames - 1));
            const std::size_t s0 = static_cast<std::size_t>(startPos);
            const std::size_t s1 = std::min(s0 + 1, clip_.frames - 1);
            const float sFrac = static_cast<float>(startPos - static_cast<double>(s0));
            const float startY =
                clip_.mono[s0] +
                sFrac * (clip_.mono[s1] - clip_.mono[s0]);

            y = (1.0f - mix) * y + mix * startY;
        }
    }

    position_ += rate_;

    if (position_ >= static_cast<double>(clip_.frames - 1)) {
        if (clip_.loop) {
            position_ = std::fmod(position_, static_cast<double>(clip_.frames - 1));
        } else {
            active_ = false;
        }
    }

    return y * gain_;
}

void Engine::prepare(double sampleRate) noexcept {
    sampleRate_ = std::max(sampleRate, 1.0);
    reset();
}

void Engine::reset() noexcept {
    state_ = State::Stopped;
    phase_ = 0.0;
    previousPhase_ = 0.0;
    activity_ = 0.0f;
    runLoopSpawned_ = false;
    loadActive_ = false;
    stopCountdown_ = 0;
    rng_.seed(0x125A4D414348494EULL);
    for (auto& v : voices_) v.reset();
    for (auto& e : pending_) e = {};
}

const Pool* Engine::poolFor(Role role) const noexcept {
    if (!set_) return nullptr;
    switch (role) {
        case Role::Start: return &set_->start;
        case Role::Run: return &set_->run;
        case Role::Action: return &set_->action;
        case Role::Load: return &set_->load;
        case Role::Release: return &set_->release;
        case Role::Stop: return &set_->stop;
    }
    return nullptr;
}

const Clip* Engine::chooseClip(Role role) noexcept {
    const Pool* pool = poolFor(role);
    if (!pool || pool->count == 0)
        return nullptr;

    const std::size_t index = std::min<std::size_t>(
        pool->count - 1,
        static_cast<std::size_t>(rng_.uniform01() * static_cast<float>(pool->count)));

    return &pool->clips[index];
}

void Engine::spawn(Role role, float force, bool preferLoop) noexcept {
    const Clip* clip = chooseClip(role);
    if (!clip)
        return;

    Voice* target = nullptr;
    for (auto& v : voices_) {
        if (!v.active()) {
            target = &v;
            break;
        }
    }
    if (!target)
        return;

    const float wear = clamp01(params_.wear);
    const float load = clamp01(params_.load);

    float gain = std::clamp(force, 0.0f, 1.5f);
    float rate = 1.0f;

    const float sampleRateRatio =
        static_cast<float>(clip->sampleRate / sampleRate_);

    if (role == Role::Run) {
        const float speed = clamp01(params_.speed);
        rate = (0.65f + 0.85f * speed) * sampleRateRatio;
        gain *= 0.55f + 0.30f * load;
    } else {
        rate *= sampleRateRatio;
        rate *= 1.0f + (0.008f + 0.025f * wear) * rng_.bipolar();
        gain *= 1.0f + 0.10f * wear * rng_.bipolar();
    }

    std::size_t startOffset = 0;
    if (!preferLoop && clip->frames > 64) {
        const std::size_t maxOffset = static_cast<std::size_t>(
            0.003 * sampleRate_ * wear);
        if (maxOffset > 0)
            startOffset = std::min<std::size_t>(
                clip->frames - 1,
                static_cast<std::size_t>(rng_.uniform01() * maxOffset));
    }

    target->start(*clip, gain, rate, startOffset);
}

void Engine::schedule(Role role, int delaySamples, float force) noexcept {
    for (auto& e : pending_) {
        if (!e.active) {
            e.role = role;
            e.remainingSamples = std::max(0, delaySamples);
            e.force = force;
            e.active = true;
            return;
        }
    }
}

void Engine::start() noexcept {
    if (state_ != State::Stopped)
        return;

    state_ = State::Starting;
    // START is itself a mechanical event and must not be hidden by a fade-in.
    activity_ = 1.0f;
    runLoopSpawned_ = false;
    spawn(Role::Start, 1.0f);

    const int runDelay = static_cast<int>(
        sampleRate_ * (0.080 + 0.120 * (1.0 - clamp01(params_.speed))));
    schedule(Role::Run, runDelay, 1.0f);
}

void Engine::stop() noexcept {
    if (state_ == State::Stopped || state_ == State::Stopping)
        return;

    state_ = State::Stopping;
    loadActive_ = false;
    spawn(Role::Stop, 1.0f);
    stopCountdown_ = static_cast<int>(0.350 * sampleRate_);
}

void Engine::triggerAction(float force) noexcept {
    if (state_ == State::Stopped || state_ == State::Stopping)
        return;

    spawn(Role::Action, force);

    const float clatter = clamp01(params_.clatter);
    const float wear = clamp01(params_.wear);
    if (clatter > 0.0f) {
        const int secondary = static_cast<int>(3.0f * clatter * (0.4f + wear));
        for (int i = 0; i < secondary; ++i) {
            const float ms = 5.0f + 28.0f * rng_.uniform01();
            schedule(
                Role::Release,
                static_cast<int>(ms * 0.001f * sampleRate_),
                force * (0.35f + 0.25f * rng_.uniform01()));
        }
    }
}

void Engine::setLoadActive(bool active) noexcept {
    if (loadActive_ == active)
        return;

    loadActive_ = active;

    if (active) {
        if (state_ == State::Running)
            state_ = State::Loaded;
        spawn(Role::Load, 1.0f);
    } else {
        if (state_ == State::Loaded)
            state_ = State::Releasing;
        spawn(Role::Release, 0.8f);
    }
}

void Engine::updateMachineState() noexcept {
    if (state_ == State::Stopped)
        return;

    const float speed = clamp01(params_.speed);
    const float wear = clamp01(params_.wear);

    double phaseSpeed = 0.40 + 3.60 * speed;
    phaseSpeed *= 1.0 - 0.18 * clamp01(params_.load) * (loadActive_ ? 1.0 : 0.0);
    phaseSpeed *= 1.0 + 0.035 * wear * std::sin(phase_);

    previousPhase_ = phase_;
    phase_ += kTwoPi * phaseSpeed / sampleRate_;
    if (phase_ >= kTwoPi)
        phase_ = std::fmod(phase_, kTwoPi);

    const float actionAmount = clamp01(params_.action);
    if (actionAmount > 0.0f &&
        state_ != State::Stopping &&
        state_ != State::Stopped) {

        const int camsPerRev = 1 + static_cast<int>(std::floor(5.0f * actionAmount));
        const double sector = kTwoPi / static_cast<double>(camsPerRev);
        const int previousSector = static_cast<int>(previousPhase_ / sector);
        const int currentSector = static_cast<int>(phase_ / sector);

        const bool wrapped = phase_ < previousPhase_;
        if (wrapped || currentSector != previousSector) {
            const float force =
                0.55f +
                0.35f * actionAmount +
                0.10f * clamp01(params_.load);
            triggerAction(force);
        }
    }

    if (state_ == State::Stopping)
        activity_ = std::max(0.0f, activity_ - static_cast<float>(1.0 / (0.35 * sampleRate_)));
    else
        activity_ = 1.0f;

    if (state_ == State::Releasing)
        state_ = State::Running;

    if (state_ == State::Stopping) {
        if (stopCountdown_ > 0)
            --stopCountdown_;
        if (stopCountdown_ <= 0 && activity_ <= 0.0f) {
            state_ = State::Stopped;
            runLoopSpawned_ = false;
            for (auto& v : voices_) v.reset();
            for (auto& e : pending_) e = {};
        }
    }
}

float Engine::renderVoices() noexcept {
    float out = 0.0f;
    for (auto& v : voices_)
        out += v.process();
    return out;
}

float Engine::outputGain() const noexcept {
    const float db = 24.0f * (clamp01(params_.output) - 0.5f);
    return std::pow(10.0f, db / 20.0f);
}

void Engine::process(float* monoOut, std::size_t frames) noexcept {
    if (!monoOut || frames == 0)
        return;

    for (std::size_t i = 0; i < frames; ++i) {
        for (auto& e : pending_) {
            if (!e.active)
                continue;

            if (e.remainingSamples > 0)
                --e.remainingSamples;

            if (e.remainingSamples <= 0) {
                if (e.role == Role::Run) {
                    spawn(Role::Run, e.force, true);
                    runLoopSpawned_ = true;
                    if (state_ == State::Starting)
                        state_ = loadActive_ ? State::Loaded : State::Running;
                } else {
                    spawn(e.role, e.force);
                }
                e.active = false;
            }
        }

        updateMachineState();

        // If a run clip ended unexpectedly, restart it while machine is active.
        if (state_ != State::Stopped && state_ != State::Stopping && set_) {
            bool anyRunLoopCandidate = set_->run.count > 0;
            if (anyRunLoopCandidate && !runLoopSpawned_) {
                spawn(Role::Run, 1.0f, true);
                runLoopSpawned_ = true;
            }
        }

        monoOut[i] = renderVoices() * activity_ * outputGain();
    }
}

} // namespace mechamorph::machine
