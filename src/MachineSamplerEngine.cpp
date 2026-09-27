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
    Role role,
    float gain,
    float rate,
    std::size_t startOffset) noexcept {

    clip_ = clip;
    role_ = role;
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
    runBlend_ = 0.0f;
    inertiaState_ = 0.0f;
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
    const float scale = clamp01(params_.scale);

    float gain = std::clamp(force, 0.0f, 1.5f);
    float rate = 1.0f;

    const float sampleRateRatio =
        static_cast<float>(clip->sampleRate / sampleRate_);

    if (role == Role::Run) {
        const float speed = clamp01(params_.speed);
        // Playback pitch/rate contributes only part of perceived size.
        // Keep the range moderate so SCALE does not collapse into "slow sample".
        const float scaleRate = 1.08f - 0.38f * scale;
        rate = (0.65f + 0.85f * speed) * scaleRate * sampleRateRatio;
        // Large machines carry more body energy but not a simple loudness boost.
        gain *= 0.56f + 0.08f * scale - 0.08f * load;
    } else {
        const float scaleRate = 1.10f - 0.42f * scale;
        rate *= sampleRateRatio * scaleRate;
        rate *= 1.0f + (0.006f + 0.055f * wear * wear) * rng_.bipolar();
        gain *= (0.88f + 0.22f * scale) *
                (1.0f + (0.06f + 0.22f * wear * wear) * rng_.bipolar());
    }

    // Never simulate wear by entering a one-shot part-way through its sample.
    // That can erase the defining contact transient and makes "wear" behave
    // like accidental transient trimming. Timing variation belongs to event
    // scheduling / backlash, while the physical recorded gesture stays intact.
    const std::size_t startOffset = 0;

    target->start(*clip, role, gain, rate, startOffset);
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

    // A new start interrupts any residual stop/release tail from the previous
    // cycle. This prevents stale machine tails from overlapping a fresh start.
    for (auto& v : voices_) v.reset();
    for (auto& e : pending_) e = {};

    state_ = State::Starting;
    // START is itself a mechanical event and must not be hidden by a fade-in.
    activity_ = 1.0f;
    runLoopSpawned_ = false;
    spawn(Role::Start, 1.0f);

    const int runDelay = static_cast<int>(
        sampleRate_ * (0.70 + 0.55 * (1.0 - clamp01(params_.speed))));
    schedule(Role::Run, runDelay, 1.0f);
}

void Engine::stop() noexcept {
    if (state_ == State::Stopped || state_ == State::Stopping)
        return;

    state_ = State::Stopping;
    loadActive_ = false;
    spawn(Role::Stop, 1.0f);
    const float scale = clamp01(params_.scale);
    const double stopSeconds = 0.25 + 2.75 * scale * scale;
    stopCountdown_ = static_cast<int>(stopSeconds * sampleRate_);
}

void Engine::triggerAction(float force) noexcept {
    if (state_ == State::Stopped || state_ == State::Stopping)
        return;

    spawn(Role::Action, force);

    const float wear = clamp01(params_.wear);
    const float clatter = clamp01(
        0.35f * params_.clatter +
        0.85f * wear * wear);

    if (clatter > 0.0f) {
        const int secondary = static_cast<int>(
            1.0f + 4.0f * clatter * (0.35f + 0.65f * wear));
        for (int i = 0; i < secondary; ++i) {
            const float scale = clamp01(params_.scale);
            const float ms =
                5.0f +
                (28.0f + 95.0f * scale * scale) * rng_.uniform01();
            // Secondary backlash/re-contact is still an ACTION/contact event.
            // Never use RELEASE material as generic clatter.
            schedule(
                Role::Action,
                static_cast<int>(ms * 0.001f * sampleRate_),
                force * (0.18f + 0.18f * rng_.uniform01()));
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
        spawn(Role::Load, 0.55f);
    } else {
        if (state_ == State::Loaded)
            state_ = State::Releasing;
        spawn(Role::Release, 0.52f);
    }
}

void Engine::updateMachineState() noexcept {
    if (state_ == State::Stopped)
        return;

    const float speed = clamp01(params_.speed);
    const float wear = clamp01(params_.wear);
    const float scale = clamp01(params_.scale);

    const float continuousLoad = clamp01(params_.load);

    // SCALE is physical mass/size, not a pitch macro.
    // Larger machines accelerate more slowly and have lower natural cycle rate.
    const float targetSpeed =
        (0.40f + 3.60f * speed) *
        (1.08f - 0.70f * scale);

    // Colossal machines should feel inertial rather than merely slow.
    const float inertiaSeconds = 0.02f + 2.80f * scale * scale * scale;
    const float inertiaCoeff =
        static_cast<float>(1.0 - std::exp(-1.0 / (inertiaSeconds * sampleRate_)));
    inertiaState_ += inertiaCoeff * (targetSpeed - inertiaState_);

    double phaseSpeed = std::max(0.03f, inertiaState_);
    // LOAD is a real continuous machine control. A loaded machine slows and
    // feels heavier even when no discrete LOAD gesture is currently playing.
    phaseSpeed *= 1.0 - 0.18 * continuousLoad;
    phaseSpeed *= 1.0 + 0.010 * wear * std::sin(phase_);

    previousPhase_ = phase_;
    phase_ += kTwoPi * phaseSpeed / sampleRate_;
    if (phase_ >= kTwoPi)
        phase_ = std::fmod(phase_, kTwoPi);

    // Keep continuous RUN beds mechanically coupled to live speed/load.
    // This is crucial: changing machine speed or engaging load must change
    // the running mechanism itself, not just future one-shot events.
    const float speedForRate = clamp01(params_.speed);
    const float loadForRate = continuousLoad;
    const float scaleRate = 1.08f - 0.38f * scale;
    const double wearEccentricity =
        1.0 +
        (0.004 + 0.012 * static_cast<double>(wear)) *
        static_cast<double>(wear) *
        std::sin(phase_);
    const double runRateScale =
        (0.65 + 0.85 * speedForRate) *
        static_cast<double>(scaleRate) *
        (1.0 - 0.18 * loadForRate) *
        wearEccentricity;

    for (auto& v : voices_) {
        if (v.active() && v.role() == Role::Run) {
            const double sourceRatio = v.sourceSampleRate() / sampleRate_;
            v.setRate(sourceRatio * runRateScale);
        }
    }

    const float actionAmount = clamp01(params_.action);
    if (actionAmount > 0.0f &&
        state_ != State::Stopping &&
        state_ != State::Stopped) {

        const float densityScale = 1.0f - 0.82f * scale;
        const int camsPerRev = std::max(
            1,
            1 + static_cast<int>(std::floor(9.0f * actionAmount * densityScale)));
        const double sector = kTwoPi / static_cast<double>(camsPerRev);
        const int previousSector = static_cast<int>(previousPhase_ / sector);
        const int currentSector = static_cast<int>(phase_ / sector);

        const bool wrapped = phase_ < previousPhase_;
        if (wrapped || currentSector != previousSector) {
            const float loadAmount = clamp01(params_.load);

            // Worn linkages do not hit every contact with identical force.
            const float forceVariation =
                1.0f + (0.04f + 0.26f * wear * wear) * rng_.bipolar();

            float force =
                (0.48f +
                 0.18f * actionAmount +
                 0.10f * loadAmount +
                 0.42f * scale) *
                forceVariation;

            force = std::clamp(force, 0.15f, 1.25f);

            // Severe wear can occasionally fail to engage a tooth/cam cleanly.
            // Keep this rare below the creative range.
            const float missProbability =
                wear > 0.65f
                    ? 0.10f * ((wear - 0.65f) / 0.35f)
                    : 0.0f;

            if (missProbability <= 0.0f || rng_.uniform01() >= missProbability)
                triggerAction(force);
        }
    }

    if (state_ == State::Stopping)
        activity_ = std::max(0.0f, activity_ - static_cast<float>(1.0 / (0.35 * sampleRate_)));
    else
        activity_ = 1.0f;

    const bool runState =
        state_ == State::Running ||
        state_ == State::Loaded ||
        state_ == State::Releasing;
    const float runTarget = runState ? 1.0f : 0.0f;
    const float runCoeff =
        runTarget > runBlend_
            ? static_cast<float>(1.0 / (0.55 * sampleRate_))
            : static_cast<float>(1.0 / (0.20 * sampleRate_));
    runBlend_ += runCoeff * (runTarget - runBlend_);
    runBlend_ = std::clamp(runBlend_, 0.0f, 1.0f);

    if (state_ == State::Releasing)
        state_ = State::Running;

    if (state_ == State::Stopping) {
        if (stopCountdown_ > 0)
            --stopCountdown_;
        if (stopCountdown_ <= 0 && activity_ <= 0.0f) {
            // Drive is now stopped. Keep STOP / RELEASE one-shots alive as a
            // natural acoustic tail, but remove continuous machine beds.
            state_ = State::Stopped;
            runLoopSpawned_ = false;
            for (auto& v : voices_) {
                if (v.active() &&
                    (v.role() == Role::Run || v.role() == Role::Load)) {
                    v.reset();
                }
            }
            for (auto& e : pending_) e = {};
        }
    }
}

float Engine::renderVoices() noexcept {
    float out = 0.0f;
    for (auto& v : voices_) {
        const float sample = v.process();
        if (v.role() == Role::Run)
            out += sample * activity_ * runBlend_;
        else if (v.role() == Role::Load)
            out += sample * activity_;
        else
            out += sample;
    }
    return out;
}

bool Engine::hasActiveRole(Role role) const noexcept {
    for (const auto& v : voices_) {
        if (v.active() && v.role() == role)
            return true;
    }
    return false;
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

        monoOut[i] = renderVoices() * outputGain();
    }
}

} // namespace mechamorph::machine
