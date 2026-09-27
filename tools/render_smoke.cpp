#include "MechamorphCore.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

void writeU16(std::ofstream& f, std::uint16_t v) {
    const char b[2] = {
        static_cast<char>(v & 0xff),
        static_cast<char>((v >> 8) & 0xff)
    };
    f.write(b, 2);
}

void writeU32(std::ofstream& f, std::uint32_t v) {
    const char b[4] = {
        static_cast<char>(v & 0xff),
        static_cast<char>((v >> 8) & 0xff),
        static_cast<char>((v >> 16) & 0xff),
        static_cast<char>((v >> 24) & 0xff)
    };
    f.write(b, 4);
}

bool writeFloatWav(const std::string& path, const std::vector<float>& x, std::uint32_t sampleRate) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    constexpr std::uint16_t channels = 1;
    constexpr std::uint16_t bits = 32;
    constexpr std::uint16_t format = 3; // IEEE float
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(x.size() * sizeof(float));
    const std::uint32_t byteRate = sampleRate * channels * sizeof(float);
    const std::uint16_t blockAlign = channels * sizeof(float);

    f.write("RIFF", 4);
    writeU32(f, 36u + dataBytes);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    writeU32(f, 16);
    writeU16(f, format);
    writeU16(f, channels);
    writeU32(f, sampleRate);
    writeU32(f, byteRate);
    writeU16(f, blockAlign);
    writeU16(f, bits);
    f.write("data", 4);
    writeU32(f, dataBytes);
    f.write(reinterpret_cast<const char*>(x.data()), static_cast<std::streamsize>(dataBytes));
    return static_cast<bool>(f);
}

} // namespace

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : "mechamorph-smoke.wav";
    constexpr std::uint32_t sr = 48000;
    constexpr double seconds = 8.0;
    const std::size_t n = static_cast<std::size_t>(seconds * sr);

    std::vector<float> x(n, 0.0f);

    // Deterministic synthetic fixture:
    // 0-2 s saw-like lead, 2-4 s sustained chord proxy,
    // 4-6 s repeated transients, 6-8 s decaying metallic-ish excitation.
    for (std::size_t i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / sr;
        float v = 0.0f;

        if (t < 2.0) {
            const double phase = std::fmod(110.0 * t, 1.0);
            v = static_cast<float>(0.18 * (2.0 * phase - 1.0));
        } else if (t < 4.0) {
            const double tt = t - 2.0;
            v = static_cast<float>(
                0.07 * std::sin(2.0 * 3.141592653589793 * 110.0 * tt) +
                0.06 * std::sin(2.0 * 3.141592653589793 * 164.81 * tt) +
                0.05 * std::sin(2.0 * 3.141592653589793 * 220.0 * tt));
        } else if (t < 6.0) {
            const double tt = t - 4.0;
            const double within = std::fmod(tt, 0.25);
            if (within < 0.012)
                v = static_cast<float>(0.8 * std::exp(-within * 240.0));
        } else {
            const double tt = t - 6.0;
            const double within = std::fmod(tt, 0.5);
            if (within < 0.18) {
                v = static_cast<float>(
                    0.22 * std::exp(-within * 18.0) *
                    (std::sin(2.0 * 3.141592653589793 * 730.0 * within) +
                     0.5 * std::sin(2.0 * 3.141592653589793 * 1110.0 * within)));
            }
        }

        x[i] = v;
    }

    mechamorph::Core core;
    core.prepare(sr, 512);

    mechamorph::Parameters p;
    p.mechanize = 0.65f;
    p.crank = 0.42f;
    p.clatter = 0.58f;
    p.wobble = 0.40f;
    p.air = 0.45f;
    p.body = 0.62f;
    p.wear = 0.52f;
    p.output = 0.42f;
    core.setParameters(p);

    core.process(x.data(), nullptr, x.size());

    float peak = 0.0f;
    double sumSq = 0.0;
    for (float v : x) {
        if (!std::isfinite(v)) {
            std::cerr << "Non-finite output\n";
            return 2;
        }
        peak = std::max(peak, std::fabs(v));
        sumSq += static_cast<double>(v) * v;
    }
    const double rms = std::sqrt(sumSq / std::max<std::size_t>(1, x.size()));

    if (!writeFloatWav(out, x, sr)) {
        std::cerr << "Could not write " << out << "\n";
        return 3;
    }

    std::cout << "Wrote " << out
              << " peak=" << peak
              << " rms=" << rms
              << "\n";
    return 0;
}
