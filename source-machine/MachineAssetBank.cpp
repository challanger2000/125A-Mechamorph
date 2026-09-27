#include "MachineAssetBank.h"
#include "machine_assets_generated.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace MechamorphMachine {
namespace {

std::uint16_t u16(const unsigned char* p) noexcept {
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8));
}
std::uint32_t u32(const unsigned char* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

bool addClip(
    mechamorph::machine::SampleSet& set,
    const mechamorph::machine::Clip& clip,
    int role) noexcept {

    using mechamorph::machine::Role;
    switch (static_cast<Role>(role)) {
        case Role::Start:   return set.start.add(clip);
        case Role::Run:     return set.run.add(clip);
        case Role::Action:  return set.action.add(clip);
        case Role::Load:    return set.load.add(clip);
        case Role::Release: return set.release.add(clip);
        case Role::Stop:    return set.stop.add(clip);
    }
    return false;
}

} // namespace

bool MachineAssetBank::parsePcm16MonoWav(
    const unsigned char* bytes,
    std::size_t size,
    std::vector<float>& out,
    double& sampleRate) noexcept {

    if (!bytes || size < 44) return false;
    if (std::memcmp(bytes, "RIFF", 4) != 0 || std::memcmp(bytes + 8, "WAVE", 4) != 0)
        return false;

    std::uint16_t format = 0;
    std::uint16_t channels = 0;
    std::uint16_t bits = 0;
    std::uint32_t sr = 0;
    const unsigned char* data = nullptr;
    std::size_t dataBytes = 0;

    std::size_t pos = 12;
    while (pos + 8 <= size) {
        const auto* id = bytes + pos;
        const std::uint32_t chunkSize = u32(bytes + pos + 4);
        pos += 8;
        if (pos + chunkSize > size) return false;

        if (std::memcmp(id, "fmt ", 4) == 0 && chunkSize >= 16) {
            format = u16(bytes + pos);
            channels = u16(bytes + pos + 2);
            sr = u32(bytes + pos + 4);
            bits = u16(bytes + pos + 14);
        } else if (std::memcmp(id, "data", 4) == 0) {
            data = bytes + pos;
            dataBytes = chunkSize;
        }

        pos += chunkSize + (chunkSize & 1u);
    }

    if (format != 1 || channels != 1 || bits != 16 || sr == 0 || !data || dataBytes < 2)
        return false;

    const std::size_t frames = dataBytes / 2;
    out.resize(frames);
    for (std::size_t i = 0; i < frames; ++i) {
        const auto raw = static_cast<std::int16_t>(u16(data + 2 * i));
        out[i] = static_cast<float>(raw / 32768.0f);
    }
    sampleRate = static_cast<double>(sr);
    return true;
}

bool MachineAssetBank::load() noexcept {
    assets_.clear();
    for (auto& p : profiles_) p = {};

#if !defined(_WIN32)
    return false;
#else
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&MachineAssetBank::load),
            &module)) {
        // Member-function address cannot be used portably as module anchor.
        // Fall back to the address of this translation unit helper via module
        // handle lookup of the current process is not sufficient for a VST DLL.
        module = nullptr;
    }

    // Use an ordinary free-function address as a reliable DLL anchor.
    if (!module) {
        const auto anchor = reinterpret_cast<const wchar_t*>(&u16);
        if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                anchor,
                &module))
            return false;
    }

    assets_.reserve(kEmbeddedAssetCount);

    for (const auto& meta : kEmbeddedAssets) {
        HRSRC res = FindResourceW(module, MAKEINTRESOURCEW(meta.resourceId), RT_RCDATA);
        if (!res) return false;
        HGLOBAL loaded = LoadResource(module, res);
        if (!loaded) return false;
        const DWORD sz = SizeofResource(module, res);
        const auto* bytes = static_cast<const unsigned char*>(LockResource(loaded));
        if (!bytes || sz == 0) return false;

        OwnedAsset asset;
        asset.name = meta.name;
        asset.role = meta.role;
        asset.profileMask = meta.profileMask;
        if (!parsePcm16MonoWav(bytes, static_cast<std::size_t>(sz),
                               asset.audio, asset.sampleRate))
            return false;

        assets_.push_back(std::move(asset));
    }

    buildProfiles();
    return !assets_.empty();
#endif
}

void MachineAssetBank::buildProfiles() noexcept {
    for (auto& p : profiles_) p = {};

    for (auto& asset : assets_) {
        mechamorph::machine::Clip clip;
        clip.mono = asset.audio.data();
        clip.frames = asset.audio.size();
        clip.sampleRate = asset.sampleRate;
        clip.loop = asset.role == static_cast<int>(mechamorph::machine::Role::Run);
        clip.name = asset.name.c_str();

        for (int profileIndex = 0; profileIndex < 3; ++profileIndex) {
            const int bit = 1 << profileIndex;
            if ((asset.profileMask & bit) != 0)
                addClip(profiles_[static_cast<std::size_t>(profileIndex)], clip, asset.role);
        }
    }
}

const mechamorph::machine::SampleSet* MachineAssetBank::profile(int index) const noexcept {
    index = std::clamp(index, 0, 2);
    return &profiles_[static_cast<std::size_t>(index)];
}

} // namespace MechamorphMachine
