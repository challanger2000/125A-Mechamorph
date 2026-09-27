#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace MechamorphMachine {

class MachineSpaceEngine {
public:
    void prepare(double sampleRate);
    void reset() noexcept;

    void setBody(float normalized) noexcept;
    void setSpace(float normalized) noexcept;
    void setScale(float normalized) noexcept;

    void process(float input, float& left, float& right) noexcept;

private:
    static float clamp01(float x) noexcept;
    float readHistory(std::size_t delaySamples) const noexcept;
    float renderSparseBody(bool right) const noexcept;
    float renderSparseSpace(bool right) const noexcept;
    void processTailStereo(float input, float& left, float& right) noexcept;

    double sampleRate_ = 48000.0;
    float body_ = 0.0f;
    float space_ = 0.0f;
    float scale_ = 0.35f;

    std::vector<float> history_;
    std::size_t historyWrite_ = 0;

    static constexpr std::size_t kDelayCount = 4;
    std::array<std::vector<float>, kDelayCount> delays_{};
    std::array<std::size_t, kDelayCount> delayWrite_{};
    std::array<float, kDelayCount> dampingState_{};
};

} // namespace MechamorphMachine
