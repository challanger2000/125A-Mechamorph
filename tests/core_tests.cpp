#include "MechamorphCore.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

using mechamorph::Core;
using mechamorph::Parameters;

static bool finiteBuffer(const std::vector<float>& x) {
    for (float v : x)
        if (!std::isfinite(v)) return false;
    return true;
}

int main() {
    constexpr double sr = 48000.0;
    constexpr std::size_t n = 48000;

    Core core;
    core.prepare(sr, 512);

    // 1) Neutral path must be exact within float arithmetic.
    Parameters p;
    p.mechanize = 0.0f;
    p.output = 0.5f;
    core.setParameters(p);

    std::vector<float> l(n), r(n);
    for (std::size_t i = 0; i < n; ++i) {
        l[i] = static_cast<float>(0.2 * std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / sr));
        r[i] = l[i];
    }
    const auto original = l;
    core.process(l.data(), r.data(), n);
    for (std::size_t i = 0; i < n; ++i)
        assert(l[i] == original[i] && r[i] == original[i]);

    // 2) Silence and strong settings must remain finite.
    core.reset();
    p.mechanize = 1.0f;
    p.crank = 1.0f;
    p.clatter = 1.0f;
    p.wobble = 1.0f;
    p.air = 1.0f;
    p.body = 1.0f;
    p.wear = 1.0f;
    p.output = 0.5f;
    core.setParameters(p);
    std::fill(l.begin(), l.end(), 0.0f);
    std::fill(r.begin(), r.end(), 0.0f);
    core.process(l.data(), r.data(), n);
    assert(finiteBuffer(l) && finiteBuffer(r));

    // 3) Impulse stress must remain finite and bounded.
    core.reset();
    std::fill(l.begin(), l.end(), 0.0f);
    std::fill(r.begin(), r.end(), 0.0f);
    l[0] = r[0] = 1.0f;
    core.process(l.data(), r.data(), n);
    assert(finiteBuffer(l) && finiteBuffer(r));
    float maxAbs = 0.0f;
    for (float v : l) maxAbs = std::max(maxAbs, std::fabs(v));
    assert(maxAbs < 8.0f);

    // 4) Determinism after reset.
    core.reset();
    std::vector<float> a(4096, 0.0f), b(4096, 0.0f);
    a[0] = 1.0f;
    core.process(a.data(), nullptr, a.size());
    core.reset();
    b[0] = 1.0f;
    core.process(b.data(), nullptr, b.size());
    for (std::size_t i = 0; i < a.size(); ++i)
        assert(a[i] == b[i]);

    // 5) Reduced friction engine: neutral at zero wear, active under loaded motion.
    {
        mechamorph::FrictionEngine friction;
        mechamorph::DeterministicRng rng;
        mechamorph::MechanicalState s;
        friction.prepare(sr);
        rng.seed(0x125AF001ULL);
        s.activity = 1.0f;
        s.speedHz = 2.0f;
        s.load = 0.6f;

        Parameters q;
        q.wear = 0.0f;
        double zeroEnergy = 0.0;
        for (int i = 0; i < 4096; ++i) {
            s.phase = std::fmod(s.phase + 2.0 * 3.14159265358979323846 * s.speedHz / sr,
                                2.0 * 3.14159265358979323846);
            zeroEnergy += std::fabs(friction.process(q, s, rng));
        }
        assert(zeroEnergy == 0.0);

        friction.reset();
        rng.seed(0x125AF001ULL);
        q.wear = 1.0f;
        double activeEnergy = 0.0;
        for (int i = 0; i < 48000; ++i) {
            s.phase = std::fmod(s.phase + 2.0 * 3.14159265358979323846 * s.speedHz / sr,
                                2.0 * 3.14159265358979323846);
            const float y = friction.process(q, s, rng);
            assert(std::isfinite(y));
            activeEnergy += std::fabs(y);
        }
        assert(activeEnergy > 0.01);
    }

    // 6) Sample-rate safety sweep.
    for (double testRate : {44100.0, 48000.0, 88200.0, 96000.0, 192000.0}) {
        Core rateCore;
        rateCore.prepare(testRate, 1024);
        rateCore.setParameters(p);
        std::vector<float> x(8192, 0.0f);
        x[0] = 1.0f;
        rateCore.process(x.data(), nullptr, x.size());
        assert(finiteBuffer(x));
        float peak = 0.0f;
        for (float v : x) peak = std::max(peak, std::fabs(v));
        assert(peak < 8.0f);
    }

    // 7) Regression guard against modal gain runaway under sustained/polyphonic material.
    {
        Core musicalCore;
        musicalCore.prepare(sr, 512);
        Parameters q;
        q.mechanize = 0.65f;
        q.crank = 0.42f;
        q.clatter = 0.58f;
        q.wobble = 0.40f;
        q.air = 0.45f;
        q.body = 0.62f;
        q.wear = 0.52f;
        q.output = 0.42f;
        musicalCore.setParameters(q);

        std::vector<float> x(static_cast<std::size_t>(2.0 * sr), 0.0f);
        float peak = 0.0f;
        for (std::size_t i = 0; i < x.size(); ++i) {
            const double tt = static_cast<double>(i) / sr;
            x[i] = static_cast<float>(
                0.07 * std::sin(2.0 * 3.14159265358979323846 * 110.0 * tt) +
                0.06 * std::sin(2.0 * 3.14159265358979323846 * 164.81 * tt) +
                0.05 * std::sin(2.0 * 3.14159265358979323846 * 220.0 * tt));
        }
        musicalCore.process(x.data(), nullptr, x.size());
        assert(finiteBuffer(x));
        for (float v : x) peak = std::max(peak, std::fabs(v));
        assert(peak < 1.0f);
    }

    // 8) Machine activity must decay back toward silence after excitation.
    {
        Core tailCore;
        tailCore.prepare(sr, 512);
        Parameters q;
        q.mechanize = 1.0f;
        q.crank = 0.8f;
        q.clatter = 0.8f;
        q.wobble = 0.5f;
        q.air = 0.8f;
        q.body = 0.8f;
        q.wear = 0.5f;
        q.output = 0.5f;
        tailCore.setParameters(q);

        std::vector<float> x(static_cast<std::size_t>(6.0 * sr), 0.0f);
        x[0] = 1.0f;
        tailCore.process(x.data(), nullptr, x.size());
        assert(finiteBuffer(x));

        float finalPeak = 0.0f;
        const std::size_t start = static_cast<std::size_t>(5.0 * sr);
        for (std::size_t i = start; i < x.size(); ++i)
            finalPeak = std::max(finalPeak, std::fabs(x[i]));
        assert(finalPeak < 1.0e-3f);
    }

    // 9) Sustained excitation must still decay within the advertised finite tail.
    {
        Core tailCore;
        tailCore.prepare(sr, 512);
        Parameters q;
        q.mechanize = 1.0f;
        q.crank = 0.8f;
        q.clatter = 0.4f;
        q.wobble = 0.4f;
        q.air = 1.0f;
        q.body = 0.8f;
        q.wear = 0.0f;
        q.output = 0.5f;
        tailCore.setParameters(q);

        std::vector<float> x(static_cast<std::size_t>(13.0 * sr), 0.0f);
        const std::size_t active = static_cast<std::size_t>(4.0 * sr);
        for (std::size_t i = 0; i < active; ++i)
            x[i] = static_cast<float>(0.2 * std::sin(2.0 * 3.14159265358979323846 * 220.0 * i / sr));

        tailCore.process(x.data(), nullptr, x.size());
        assert(finiteBuffer(x));

        float finalPeak = 0.0f;
        const std::size_t start = static_cast<std::size_t>(12.0 * sr);
        for (std::size_t i = start; i < x.size(); ++i)
            finalPeak = std::max(finalPeak, std::fabs(x[i]));
        assert(finalPeak < 1.0e-4f);
    }

    // Anti-phase stereo must still excite the machine and preserve finite stereo output.
    {
        Core stereoCore;
        stereoCore.prepare(sr, 512);
        Parameters q;
        q.mechanize = 1.0f;
        q.crank = 0.5f;
        q.clatter = 0.3f;
        q.wobble = 0.3f;
        q.air = 0.4f;
        q.body = 0.8f;
        q.wear = 0.4f;
        q.output = 0.5f;
        stereoCore.setParameters(q);

        std::vector<float> l2(8192), r2(8192);
        std::vector<float> dryL(8192);
        for (std::size_t i = 0; i < l2.size(); ++i) {
            const float v = static_cast<float>(
                0.2 * std::sin(2.0 * 3.14159265358979323846 * 330.0 * i / sr));
            l2[i] = v;
            r2[i] = -v;
            dryL[i] = v;
        }

        stereoCore.process(l2.data(), r2.data(), l2.size());
        assert(finiteBuffer(l2) && finiteBuffer(r2));

        double deltaEnergy = 0.0;
        for (std::size_t i = 0; i < l2.size(); ++i)
            deltaEnergy += std::fabs(static_cast<double>(l2[i] - dryL[i]));
        assert(deltaEnergy > 0.01);
    }

    std::cout << "Mechamorph core tests PASS\n";
    return 0;
}
