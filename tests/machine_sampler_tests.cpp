#include "MachineSamplerEngine.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace mechamorph::machine;

int main() {
    constexpr double sr = 48000.0;

    // Synthetic fixtures are only for state/scheduler QA, not sonic evaluation.
    std::vector<float> start(2400, 0.0f);
    std::vector<float> run(4800, 0.0f);
    std::vector<float> action(1200, 0.0f);
    std::vector<float> load(1800, 0.0f);
    std::vector<float> release(1200, 0.0f);
    std::vector<float> stop(2400, 0.0f);

    start[0] = 0.7f;
    stop[0] = 0.7f;
    action[0] = 0.8f;
    load[0] = 0.5f;
    release[0] = 0.4f;

    for (std::size_t i = 0; i < run.size(); ++i)
        run[i] = static_cast<float>(0.08 * std::sin(2.0 * 3.141592653589793 * i / 240.0));

    SampleSet set;
    assert(set.start.add({start.data(), start.size(), sr, false, "start"}));
    assert(set.run.add({run.data(), run.size(), sr, true, "run"}));
    assert(set.action.add({action.data(), action.size(), sr, false, "action"}));
    assert(set.load.add({load.data(), load.size(), sr, false, "load"}));
    assert(set.release.add({release.data(), release.size(), sr, false, "release"}));
    assert(set.stop.add({stop.data(), stop.size(), sr, false, "stop"}));

    Engine engine;
    engine.prepare(sr);
    engine.setSampleSet(&set);

    Parameters p;
    p.speed = 0.5f;
    p.load = 0.4f;
    p.wear = 0.3f;
    p.clatter = 0.5f;
    p.output = 0.5f;
    engine.setParameters(p);

    std::vector<float> out(static_cast<std::size_t>(2.0 * sr), 0.0f);

    engine.start();
    engine.process(out.data(), static_cast<std::size_t>(0.35 * sr));
    assert(engine.state() == State::Running || engine.state() == State::Loaded);

    engine.triggerAction(0.9f);
    engine.setLoadActive(true);
    engine.process(
        out.data() + static_cast<std::size_t>(0.35 * sr),
        static_cast<std::size_t>(0.45 * sr));
    assert(engine.state() == State::Loaded);

    engine.setLoadActive(false);
    engine.process(
        out.data() + static_cast<std::size_t>(0.80 * sr),
        static_cast<std::size_t>(0.20 * sr));

    engine.stop();
    engine.process(
        out.data() + static_cast<std::size_t>(1.00 * sr),
        static_cast<std::size_t>(1.00 * sr));
    assert(engine.state() == State::Stopped);

    float peak = 0.0f;
    double energy = 0.0;
    for (float v : out) {
        assert(std::isfinite(v));
        peak = std::max(peak, std::fabs(v));
        energy += static_cast<double>(v) * v;
    }
    assert(peak > 0.0f);
    assert(peak < 2.0f);
    assert(energy > 0.0);

    // Restart must not revive stale loop voices or duplicate the old run bed.
    engine.start();
    std::vector<float> restart(static_cast<std::size_t>(0.60 * sr), 0.0f);
    engine.process(restart.data(), restart.size());
    assert(engine.state() == State::Running || engine.state() == State::Loaded);
    float restartPeak = 0.0f;
    for (float v : restart) restartPeak = std::max(restartPeak, std::fabs(v));
    assert(restartPeak > 0.0f);
    assert(restartPeak < 2.0f);

    engine.stop();
    std::vector<float> restartStop(static_cast<std::size_t>(0.60 * sr), 0.0f);
    engine.process(restartStop.data(), restartStop.size());
    assert(engine.state() == State::Stopped);

    // Autonomous cam/action behaviour: action amount > 0 must create
    // deterministic action events while the machine runs.
    SampleSet autoSet = set;
    Engine autoEngine;
    autoEngine.prepare(sr);
    autoEngine.setSampleSet(&autoSet);

    Parameters autoP = p;
    autoP.action = 1.0f;
    autoP.speed = 0.7f;
    autoP.clatter = 0.0f;
    autoEngine.setParameters(autoP);

    std::vector<float> autonomous(static_cast<std::size_t>(1.20 * sr), 0.0f);
    autoEngine.start();
    autoEngine.process(autonomous.data(), autonomous.size());

    // Remove expected continuous run-bed energy by checking for peaks larger
    // than the small run-loop amplitude.
    int actionLikePeaks = 0;
    bool above = false;
    for (float v : autonomous) {
        const bool now = std::fabs(v) > 0.20f;
        if (now && !above) ++actionLikePeaks;
        above = now;
    }
    assert(actionLikePeaks >= 2);

    // Clip sample-rate conversion: a 24 kHz clip rendered at 48 kHz must last
    // approximately twice as many output frames at rate=1.
    std::vector<float> halfRateClip(2400, 0.1f);
    SampleSet rateSet;
    assert(rateSet.start.add({
        halfRateClip.data(), halfRateClip.size(), 24000.0, false, "24k_start"}));

    Engine rateEngine;
    rateEngine.prepare(sr);
    rateEngine.setSampleSet(&rateSet);
    Parameters rateP;
    rateP.output = 0.5f;
    rateEngine.setParameters(rateP);
    rateEngine.start();

    std::vector<float> rateOut(6000, 0.0f);
    rateEngine.process(rateOut.data(), rateOut.size());

    std::size_t nonZeroFrames = 0;
    for (float v : rateOut)
        if (std::fabs(v) > 1.0e-6f) ++nonZeroFrames;

    // 2400 source frames at 24 kHz -> about 4800 frames at 48 kHz.
    assert(nonZeroFrames > 4500);
    assert(nonZeroFrames < 5100);

    std::cout << "Machine sampler engine tests PASS\n";
    return 0;
}
