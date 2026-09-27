#include "MechamorphCore.h"

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

    std::cout << "Mechamorph core tests PASS\n";
    return 0;
}
