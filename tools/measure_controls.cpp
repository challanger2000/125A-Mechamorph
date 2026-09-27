#include "MechamorphCore.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kSr = 48000.0;

std::vector<float> makeFixture() {
    const std::size_t n = static_cast<std::size_t>(4.0 * kSr);
    std::vector<float> x(n, 0.0f);
    for (std::size_t i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / kSr;
        if (t < 1.0) {
            const double ph = std::fmod(110.0 * t, 1.0);
            x[i] = static_cast<float>(0.16 * (2.0 * ph - 1.0));
        } else if (t < 2.0) {
            const double u = t - 1.0;
            x[i] = static_cast<float>(
                0.06 * std::sin(2.0 * kPi * 110.0 * u) +
                0.05 * std::sin(2.0 * kPi * 164.81 * u) +
                0.04 * std::sin(2.0 * kPi * 220.0 * u));
        } else if (t < 3.0) {
            const double u = t - 2.0;
            const double w = std::fmod(u, 0.20);
            if (w < 0.015)
                x[i] = static_cast<float>(0.70 * std::exp(-w * 220.0));
        } else {
            const double u = t - 3.0;
            x[i] = static_cast<float>(
                0.10 * std::sin(2.0 * kPi * 330.0 * u) *
                (0.55 + 0.45 * std::sin(2.0 * kPi * 2.3 * u)));
        }
    }
    return x;
}

struct Metrics {
    double rms = 0.0;
    double peak = 0.0;
    double deltaRms = 0.0;
    double correlation = 0.0;
};

Metrics measure(const std::vector<float>& dry, const std::vector<float>& wet) {
    double xx = 0.0, yy = 0.0, xy = 0.0, dd = 0.0;
    double peak = 0.0;
    for (std::size_t i = 0; i < dry.size(); ++i) {
        const double x = dry[i];
        const double y = wet[i];
        const double d = y - x;
        xx += x * x;
        yy += y * y;
        xy += x * y;
        dd += d * d;
        peak = std::max(peak, std::fabs(y));
    }
    const double n = static_cast<double>(std::max<std::size_t>(1, dry.size()));
    Metrics m;
    m.rms = std::sqrt(yy / n);
    m.peak = peak;
    m.deltaRms = std::sqrt(dd / n);
    m.correlation = (xx > 0.0 && yy > 0.0) ? xy / std::sqrt(xx * yy) : 0.0;
    return m;
}

enum class Control {
    Mechanize, Crank, Clatter, Wobble, Air, Body, Wear
};

void setControl(mechamorph::Parameters& p, Control c, float v) {
    switch (c) {
        case Control::Mechanize: p.mechanize = v; break;
        case Control::Crank: p.crank = v; break;
        case Control::Clatter: p.clatter = v; break;
        case Control::Wobble: p.wobble = v; break;
        case Control::Air: p.air = v; break;
        case Control::Body: p.body = v; break;
        case Control::Wear: p.wear = v; break;
    }
}

const char* name(Control c) {
    switch (c) {
        case Control::Mechanize: return "MECHANIZE";
        case Control::Crank: return "CRANK";
        case Control::Clatter: return "CLATTER";
        case Control::Wobble: return "WOBBLE";
        case Control::Air: return "AIR";
        case Control::Body: return "BODY";
        case Control::Wear: return "WEAR";
    }
    return "UNKNOWN";
}

} // namespace

int main() {
    const auto dry = makeFixture();
    const std::array<float, 5> positions {0.0f, 0.25f, 0.50f, 0.75f, 1.0f};
    const std::array<Control, 7> controls {
        Control::Mechanize, Control::Crank, Control::Clatter, Control::Wobble,
        Control::Air, Control::Body, Control::Wear
    };

    std::cout << "control,position,peak,rms,delta_rms,correlation\n";
    std::cout << std::fixed << std::setprecision(8);

    for (Control c : controls) {
        for (float pos : positions) {
            mechamorph::Parameters p;
            p.mechanize = 0.65f;
            p.crank = 0.45f;
            p.clatter = 0.45f;
            p.wobble = 0.35f;
            p.air = 0.40f;
            p.body = 0.55f;
            p.wear = 0.40f;
            p.output = 0.50f;
            setControl(p, c, pos);

            auto y = dry;
            mechamorph::Core core;
            core.prepare(kSr, 512);
            core.setParameters(p);
            core.process(y.data(), nullptr, y.size());

            const Metrics m = measure(dry, y);
            std::cout << name(c) << ','
                      << pos << ','
                      << m.peak << ','
                      << m.rms << ','
                      << m.deltaRms << ','
                      << m.correlation << '\n';
        }
    }
    return 0;
}
