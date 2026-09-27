#pragma once

#include <array>
#include <cstddef>

namespace mechamorph::machine {

struct CharacterParameters {
    float body = 0.0f;   // 0=dry/no body, 1=strong machine enclosure
    float space = 0.0f;  // 0=dry/direct, 1=large industrial hall
    float scale = 0.35f; // shared physical size
};

class MachineCharacterEngine {
public:
    void prepare(double sampleRate) noexcept;
    void reset() noexcept;
    void setParameters(const CharacterParameters& p) noexcept;
    float process(float input) noexcept;

private:
    struct Resonator {
        float y1 = 0.0f;
        float y2 = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
        float gain = 0.0f;

        void reset() noexcept { y1 = y2 = 0.0f; }
        float process(float x) noexcept;
    };

    static constexpr std::size_t kDelayCount = 4;
    static constexpr std::size_t kMaxDelay = 65536;

    void updateCoefficients() noexcept;

    double sampleRate_ = 48000.0;
    CharacterParameters params_{};

    std::array<Resonator,4> bodyModes_{};

    std::array<std::array<float,kMaxDelay>,kDelayCount> delay_{};
    std::array<std::size_t,kDelayCount> write_{};
    std::array<std::size_t,kDelayCount> length_{};
    std::array<float,kDelayCount> dampState_{};

    float early1_ = 0.0f;
    float early2_ = 0.0f;
    float early3_ = 0.0f;
};

} // namespace mechamorph::machine
