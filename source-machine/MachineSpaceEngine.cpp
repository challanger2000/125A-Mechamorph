#include "MachineSpaceEngine.h"
#include "machine_space_ir_generated.h"

#include <algorithm>
#include <cmath>

namespace MechamorphMachine {

float MachineSpaceEngine::clamp01(float x) noexcept {
    return std::clamp(std::isfinite(x) ? x : 0.0f, 0.0f, 1.0f);
}

void MachineSpaceEngine::prepare(double sampleRate) {
    sampleRate_ = std::max(1.0, sampleRate);

    // Enough for ~1.6 seconds of sparse early-reflection history.
    const auto historyFrames =
        static_cast<std::size_t>(std::ceil(sampleRate_ * 1.65));
    history_.assign(std::max<std::size_t>(historyFrames, 1024), 0.0f);

    const std::array<double, kDelayCount> baseMs{31.1, 37.3, 43.7, 53.9};
    for (std::size_t i=0;i<kDelayCount;++i) {
        const auto frames = static_cast<std::size_t>(
            std::ceil(sampleRate_ * baseMs[i] * 0.001 * 1.75));
        delays_[i].assign(std::max<std::size_t>(frames, 64), 0.0f);
    }
    reset();
}

void MachineSpaceEngine::reset() noexcept {
    std::fill(history_.begin(),history_.end(),0.0f);
    historyWrite_=0;
    for(auto& d:delays_) std::fill(d.begin(),d.end(),0.0f);
    delayWrite_.fill(0);
    dampingState_.fill(0.0f);
}

void MachineSpaceEngine::setBody(float normalized) noexcept { body_=clamp01(normalized); }
void MachineSpaceEngine::setSpace(float normalized) noexcept { space_=clamp01(normalized); }
void MachineSpaceEngine::setScale(float normalized) noexcept { scale_=clamp01(normalized); }

float MachineSpaceEngine::readHistory(std::size_t delaySamples) const noexcept {
    if(history_.empty()) return 0.0f;
    delaySamples=std::min(delaySamples,history_.size()-1);
    const auto index=(historyWrite_+history_.size()-delaySamples)%history_.size();
    return history_[index];
}

float MachineSpaceEngine::renderSparseBody(bool right) const noexcept {
    if(body_<=0.0f) return 0.0f;
    const double srScale=sampleRate_/48000.0;
    const double sizeScale=0.78+0.48*static_cast<double>(scale_);
    float sum=0.0f;
    for(std::size_t i=0;i<kBodyTapsCount;++i){
        const auto delay=static_cast<std::size_t>(
            std::llround(kBodyTaps[i].delay48k*srScale*sizeScale));
        const float g=right?kBodyTaps[i].right:kBodyTaps[i].left;
        sum+=readHistory(delay)*g;
    }
    return sum;
}

float MachineSpaceEngine::renderSparseSpace(bool right) const noexcept {
    if(space_<=0.0f) return 0.0f;

    const double srScale=sampleRate_/48000.0;
    const double roomScale=0.80+0.62*static_cast<double>(scale_);
    const float reactorBlend=scale_*scale_;
    float warehouse=0.0f;
    float reactor=0.0f;

    for(std::size_t i=0;i<kWarehouseTapsCount;++i){
        const auto delay=static_cast<std::size_t>(
            std::llround(kWarehouseTaps[i].delay48k*srScale*roomScale));
        const float g=right?kWarehouseTaps[i].right:kWarehouseTaps[i].left;
        warehouse+=readHistory(delay)*g;
    }
    for(std::size_t i=0;i<kReactorTapsCount;++i){
        const auto delay=static_cast<std::size_t>(
            std::llround(kReactorTaps[i].delay48k*srScale*roomScale));
        const float g=right?kReactorTaps[i].right:kReactorTaps[i].left;
        reactor+=readHistory(delay)*g;
    }

    return (1.0f-reactorBlend)*warehouse+reactorBlend*reactor;
}

float MachineSpaceEngine::processTail(float input, bool right) noexcept {
    if(space_<=0.0f) return 0.0f;

    const std::array<double,kDelayCount> baseMs{31.1,37.3,43.7,53.9};
    const double roomScale=0.82+0.72*static_cast<double>(scale_);
    std::array<float,kDelayCount> read{};

    for(std::size_t i=0;i<kDelayCount;++i){
        auto& line=delays_[i];
        const auto wanted=std::min<std::size_t>(
            line.size()-1,
            static_cast<std::size_t>(
                std::llround(sampleRate_*baseMs[i]*0.001*roomScale)));
        const auto readIndex=(delayWrite_[i]+line.size()-wanted)%line.size();
        read[i]=line[readIndex];
    }

    // Hadamard-like mixing keeps the tail diffuse without modulation.
    const float m0= read[0]+read[1]+read[2]+read[3];
    const float m1= read[0]-read[1]+read[2]-read[3];
    const float m2= read[0]+read[1]-read[2]-read[3];
    const float m3= read[0]-read[1]-read[2]+read[3];
    const std::array<float,kDelayCount> mixed{
        0.5f*m0,0.5f*m1,0.5f*m2,0.5f*m3
    };

    const float feedback=0.46f+0.24f*space_+0.10f*scale_;
    const float damping=0.16f-0.06f*scale_;

    for(std::size_t i=0;i<kDelayCount;++i){
        dampingState_[i]+=damping*(mixed[i]-dampingState_[i]);
        delays_[i][delayWrite_[i]]=
            input*0.22f+dampingState_[i]*feedback;
        delayWrite_[i]=(delayWrite_[i]+1)%delays_[i].size();
    }

    if(right)
        return 0.35f*(read[0]-read[1]-read[2]+read[3]);
    return 0.35f*(read[0]+read[1]-read[2]-read[3]);
}

void MachineSpaceEngine::process(float input, float& left, float& right) noexcept {
    if(history_.empty()){
        left=right=input;
        return;
    }

    history_[historyWrite_]=input;

    const float bodyL=renderSparseBody(false);
    const float bodyR=renderSparseBody(true);

    // BODY is resonance/cabinet, not a second wet/dry reverb.
    const float bodyAmount=0.52f*body_;
    const float baseL=input*(1.0f-0.18f*body_)+bodyL*bodyAmount;
    const float baseR=input*(1.0f-0.18f*body_)+bodyR*bodyAmount;

    const float earlyL=renderSparseSpace(false);
    const float earlyR=renderSparseSpace(true);
    const float tailL=processTail(0.5f*(baseL+baseR),false);
    const float tailR=processTail(0.5f*(baseL+baseR),true);

    // Musical range up to 50%; upper half deliberately becomes cinematic.
    const float wet=space_<=0.5f
        ? 0.55f*space_
        : 0.275f+0.95f*(space_-0.5f);

    left=baseL*(1.0f-0.22f*wet)+wet*(0.46f*earlyL+0.72f*tailL);
    right=baseR*(1.0f-0.22f*wet)+wet*(0.46f*earlyR+0.72f*tailR);

    historyWrite_=(historyWrite_+1)%history_.size();
}

} // namespace MechamorphMachine
