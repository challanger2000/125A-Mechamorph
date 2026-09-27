#include "MachineSamplerEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using mechamorph::machine::Clip;
using mechamorph::machine::Engine;
using mechamorph::machine::Parameters;
using mechamorph::machine::Role;
using mechamorph::machine::SampleSet;

namespace {

struct OwnedClip {
    std::vector<float> audio;
    std::string name;
    Role role = Role::Action;
    bool loop = false;
};

std::uint16_t readU16(std::istream& f) {
    unsigned char b[2]{};
    f.read(reinterpret_cast<char*>(b), 2);
    return static_cast<std::uint16_t>(b[0] | (b[1] << 8));
}

std::uint32_t readU32(std::istream& f) {
    unsigned char b[4]{};
    f.read(reinterpret_cast<char*>(b), 4);
    return static_cast<std::uint32_t>(
        b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

bool loadPcm16MonoWav(const fs::path& path, std::vector<float>& out, int& sampleRate) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    char id[4]{};
    f.read(id, 4);
    if (std::strncmp(id, "RIFF", 4) != 0) return false;
    (void)readU32(f);
    f.read(id, 4);
    if (std::strncmp(id, "WAVE", 4) != 0) return false;

    std::uint16_t format = 0;
    std::uint16_t channels = 0;
    std::uint16_t bits = 0;
    std::uint32_t sr = 0;
    std::vector<std::int16_t> pcm;

    while (f && !f.eof()) {
        if (!f.read(id, 4)) break;
        const std::uint32_t size = readU32(f);

        if (std::strncmp(id, "fmt ", 4) == 0) {
            format = readU16(f);
            channels = readU16(f);
            sr = readU32(f);
            (void)readU32(f);
            (void)readU16(f);
            bits = readU16(f);
            if (size > 16)
                f.seekg(static_cast<std::streamoff>(size - 16), std::ios::cur);
        } else if (std::strncmp(id, "data", 4) == 0) {
            if (format != 1 || channels != 1 || bits != 16)
                return false;
            const std::size_t samples = size / sizeof(std::int16_t);
            pcm.resize(samples);
            f.read(reinterpret_cast<char*>(pcm.data()), static_cast<std::streamsize>(size));
        } else {
            f.seekg(static_cast<std::streamoff>(size), std::ios::cur);
        }

        if (size & 1)
            f.seekg(1, std::ios::cur);
    }

    if (pcm.empty() || sr == 0) return false;

    out.resize(pcm.size());
    for (std::size_t i = 0; i < pcm.size(); ++i)
        out[i] = static_cast<float>(pcm[i] / 32768.0f);
    sampleRate = static_cast<int>(sr);
    return true;
}

Role roleFromName(const std::string& name) {
    if (name.rfind("start__", 0) == 0) return Role::Start;
    if (name.rfind("run__", 0) == 0) return Role::Run;
    if (name.rfind("action__", 0) == 0) return Role::Action;
    if (name.rfind("load__", 0) == 0) return Role::Load;
    if (name.rfind("release__", 0) == 0) return Role::Release;
    if (name.rfind("stop__", 0) == 0) return Role::Stop;
    return Role::Action;
}

bool addToSet(SampleSet& set, const Clip& clip, Role role) {
    switch (role) {
        case Role::Start: return set.start.add(clip);
        case Role::Run: return set.run.add(clip);
        case Role::Action: return set.action.add(clip);
        case Role::Load: return set.load.add(clip);
        case Role::Release: return set.release.add(clip);
        case Role::Stop: return set.stop.add(clip);
    }
    return false;
}

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

bool writePcm16Wav(
    const fs::path& path,
    const std::vector<float>& x,
    std::uint32_t sampleRate) {

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    constexpr std::uint16_t channels = 1;
    constexpr std::uint16_t bits = 16;
    constexpr std::uint16_t format = 1;
    const std::uint32_t dataBytes =
        static_cast<std::uint32_t>(x.size() * sizeof(std::int16_t));
    const std::uint32_t byteRate = sampleRate * channels * sizeof(std::int16_t);
    const std::uint16_t blockAlign = channels * sizeof(std::int16_t);

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

    for (float v : x) {
        const float clamped = std::clamp(v, -1.0f, 1.0f);
        const auto s = static_cast<std::int16_t>(std::lrint(clamped * 32767.0f));
        writeU16(f, static_cast<std::uint16_t>(s));
    }
    return static_cast<bool>(f);
}

void renderBlock(Engine& engine, std::vector<float>& out, std::size_t& pos, double seconds, int sr) {
    const std::size_t count = std::min<std::size_t>(
        out.size() - pos,
        static_cast<std::size_t>(seconds * sr));
    engine.process(out.data() + pos, count);
    pos += count;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: machine_real_render <sample_dir> <output.wav>\n";
        return 2;
    }

    const fs::path sampleDir = argv[1];
    const fs::path outputPath = argv[2];
    constexpr int sr = 48000;

    std::vector<std::unique_ptr<OwnedClip>> owned;
    SampleSet set;

    for (const auto& entry : fs::directory_iterator(sampleDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".wav")
            continue;

        auto clip = std::make_unique<OwnedClip>();
        clip->name = entry.path().stem().string();
        clip->role = roleFromName(clip->name);
        clip->loop = clip->role == Role::Run;

        int fileRate = 0;
        if (!loadPcm16MonoWav(entry.path(), clip->audio, fileRate)) {
            std::cerr << "Could not load " << entry.path() << "\n";
            return 3;
        }

        Clip view;
        view.mono = clip->audio.data();
        view.frames = clip->audio.size();
        view.sampleRate = static_cast<double>(fileRate);
        view.loop = clip->loop;
        view.name = clip->name.c_str();

        if (!addToSet(set, view, clip->role)) {
            std::cerr << "Pool full or invalid clip: " << clip->name << "\n";
            return 4;
        }

        owned.push_back(std::move(clip));
    }

    std::cout
        << "Loaded pools: start=" << set.start.count
        << " run=" << set.run.count
        << " action=" << set.action.count
        << " load=" << set.load.count
        << " release=" << set.release.count
        << " stop=" << set.stop.count << "\n";

    if (set.run.count == 0 || set.action.count == 0) {
        std::cerr << "Need at least RUN and ACTION clips\n";
        return 5;
    }

    Engine engine;
    engine.prepare(sr);
    engine.setSampleSet(&set);

    Parameters p;
    p.speed = 0.32f;
    p.load = 0.20f;
    p.wear = 0.18f;
    p.action = 0.48f;
    p.clatter = 0.22f;
    p.body = 0.30f;
    p.pressure = 0.0f;
    p.output = 0.38f;
    engine.setParameters(p);

    constexpr double duration = 40.0;
    std::vector<float> out(static_cast<std::size_t>(duration * sr), 0.0f);
    std::size_t pos = 0;

    engine.start();
    renderBlock(engine, out, pos, 6.0, sr);

    p.speed = 0.50f;
    p.action = 0.58f;
    engine.setParameters(p);
    renderBlock(engine, out, pos, 5.0, sr);

    p.speed = 0.68f;
    p.load = 0.70f;
    p.action = 0.70f;
    p.clatter = 0.38f;
    p.wear = 0.30f;
    engine.setParameters(p);
    engine.setLoadActive(true);
    renderBlock(engine, out, pos, 17.0, sr);

    p.speed = 0.42f;
    p.load = 0.25f;
    p.action = 0.42f;
    p.clatter = 0.18f;
    engine.setParameters(p);
    engine.setLoadActive(false);
    renderBlock(engine, out, pos, 5.0, sr);

    engine.stop();
    renderBlock(engine, out, pos, 7.0, sr);

    float peak = 0.0f;
    double rms = 0.0;
    for (float v : out) {
        if (!std::isfinite(v)) {
            std::cerr << "Non-finite sample\n";
            return 6;
        }
        peak = std::max(peak, std::fabs(v));
        rms += static_cast<double>(v) * v;
    }
    rms = std::sqrt(rms / std::max<std::size_t>(1, out.size()));

    if (peak > 0.95f) {
        const float scale = 0.95f / peak;
        for (float& v : out) v *= scale;
        peak = 0.95f;
    }

    if (!writePcm16Wav(outputPath, out, sr)) {
        std::cerr << "Could not write " << outputPath << "\n";
        return 7;
    }

    std::cout << "Rendered " << outputPath
              << " peak=" << peak
              << " rms=" << rms
              << " final_state=" << static_cast<int>(engine.state())
              << "\n";
    return 0;
}
