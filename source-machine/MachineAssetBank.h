#pragma once

#include "MachineSamplerEngine.h"

#include <array>
#include <string>
#include <vector>

namespace MechamorphMachine {

class MachineAssetBank {
public:
    bool load() noexcept;
    const mechamorph::machine::SampleSet* profile(int index) const noexcept;
    std::size_t loadedAssetCount() const noexcept { return assets_.size(); }

private:
    struct OwnedAsset {
        std::vector<float> audio;
        std::string name;
        int role = 0;
        int profileMask = 0;
        double sampleRate = 48000.0;
    };

    bool parsePcm16MonoWav(
        const unsigned char* bytes,
        std::size_t size,
        std::vector<float>& out,
        double& sampleRate) noexcept;

    void buildProfiles() noexcept;

    std::vector<OwnedAsset> assets_;
    std::array<mechamorph::machine::SampleSet, 3> profiles_{};
};

} // namespace MechamorphMachine
