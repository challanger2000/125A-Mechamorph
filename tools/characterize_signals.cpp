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

enum class Fixture { Impulse, Sine, Step, Noise, Burst };

const char* fixtureName(Fixture f) {
    switch (f) {
        case Fixture::Impulse: return "impulse";
        case Fixture::Sine: return "sine_440";
        case Fixture::Step: return "step";
        case Fixture::Noise: return "white_noise";
        case Fixture::Burst: return "burst_1k";
    }
    return "unknown";
}

std::vector<float> makeFixture(Fixture f) {
    const std::size_t n = static_cast<std::size_t>(2.0 * kSr);
    std::vector<float> x(n, 0.0f);

    switch (f) {
        case Fixture::Impulse:
            x[0] = 1.0f;
            break;
        case Fixture::Sine:
            for (std::size_t i = 0; i < n; ++i)
                x[i] = static_cast<float>(0.2 * std::sin(2.0 * kPi * 440.0 * i / kSr));
            break;
        case Fixture::Step:
            for (std::size_t i = n / 4; i < 3 * n / 4; ++i)
                x[i] = 0.2f;
            break;
        case Fixture::Noise: {
            std::uint32_t s = 0x125A1234u;
            for (std::size_t i = 0; i < n; ++i) {
                s = 1664525u * s + 1013904223u;
                const float u = static_cast<float>((s >> 8) * (1.0 / 16777216.0));
                x[i] = 0.12f * (2.0f * u - 1.0f);
            }
            break;
        }
        case Fixture::Burst:
            for (std::size_t i = 0; i < static_cast<std::size_t>(0.05 * kSr); ++i) {
                const double t = static_cast<double>(i) / kSr;
                x[i] = static_cast<float>(0.35 * std::sin(2.0 * kPi * 1000.0 * t));
            }
            break;
    }
    return x;
}

struct Metrics {
    double peak = 0.0;
    double rms = 0.0;
    double dc = 0.0;
    double deltaRms = 0.0;
    double correlation = 0.0;
    std::size_t firstAbove = 0;
    std::size_t lastAbove = 0;
};

Metrics measure(const std::vector<float>& dry, const std::vector<float>& wet) {
    Metrics m;
    double xx=0.0, yy=0.0, xy=0.0, dd=0.0, sum=0.0;
    const double threshold = 1e-5;
    bool found=false;

    for (std::size_t i=0;i<wet.size();++i) {
        const double x=dry[i], y=wet[i], d=y-x;
        m.peak=std::max(m.peak,std::fabs(y));
        xx+=x*x; yy+=y*y; xy+=x*y; dd+=d*d; sum+=y;
        if (std::fabs(y)>threshold) {
            if (!found) { m.firstAbove=i; found=true; }
            m.lastAbove=i;
        }
    }
    const double n=static_cast<double>(wet.size());
    m.rms=std::sqrt(yy/n);
    m.dc=sum/n;
    m.deltaRms=std::sqrt(dd/n);
    m.correlation=(xx>0.0&&yy>0.0)?xy/std::sqrt(xx*yy):0.0;
    return m;
}

mechamorph::Parameters makeParams(float mechanize) {
    mechamorph::Parameters p;
    p.mechanize=mechanize;
    p.crank=0.35f;
    p.clatter=0.25f;
    p.wobble=0.15f;
    p.air=0.20f;
    p.body=0.35f;
    p.wear=0.20f;
    p.output=0.50f;
    return p;
}

}

int main() {
    const std::array<Fixture,5> fixtures{
        Fixture::Impulse, Fixture::Sine, Fixture::Step, Fixture::Noise, Fixture::Burst
    };
    const std::array<float,4> amounts{0.0f,0.35f,0.50f,1.0f};

    std::cout<<"fixture,mechanize,peak,rms,dc,delta_rms,correlation,first_ms,last_ms\n";
    std::cout<<std::fixed<<std::setprecision(8);

    for (auto f:fixtures) {
        const auto dry=makeFixture(f);
        for (float amount:amounts) {
            auto y=dry;
            mechamorph::Core core;
            core.prepare(kSr,512);
            core.setParameters(makeParams(amount));
            core.process(y.data(),nullptr,y.size());
            const auto m=measure(dry,y);
            std::cout<<fixtureName(f)<<','<<amount<<','<<m.peak<<','<<m.rms<<','<<m.dc<<','
                     <<m.deltaRms<<','<<m.correlation<<','
                     <<(1000.0*m.firstAbove/kSr)<<','<<(1000.0*m.lastAbove/kSr)<<'\n';
        }
    }
    return 0;
}
