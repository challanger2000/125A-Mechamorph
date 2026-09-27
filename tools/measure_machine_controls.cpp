#include "MachineSamplerEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <vector>

using namespace mechamorph::machine;

namespace {

struct Result {
    double meanAbs = 0.0;
    double rms = 0.0;
    double peak = 0.0;
    int impulseCount = 0;
    double meanGapMs = 0.0;
    double gapStdMs = 0.0;
};

Result analyze(const std::vector<float>& x, double sr) {
    Result r;
    double ss = 0.0;
    double sa = 0.0;
    for (float v : x) {
        const double a = std::fabs(v);
        sa += a;
        ss += static_cast<double>(v) * v;
        r.peak = std::max(r.peak, a);
    }
    if (!x.empty()) {
        r.meanAbs = sa / x.size();
        r.rms = std::sqrt(ss / x.size());
    }

    // Detect short mechanical events by envelope peaks.
    const std::size_t win = static_cast<std::size_t>(0.003 * sr);
    std::vector<double> env(x.size(), 0.0);
    double acc = 0.0;
    for (std::size_t i=0;i<x.size();++i) {
        acc += std::fabs(x[i]);
        if (i>=win) acc -= std::fabs(x[i-win]);
        env[i] = acc / std::max<std::size_t>(1, std::min(i+1,win));
    }

    const double threshold = std::max(0.02, r.rms * 2.2);
    const std::size_t refractory = static_cast<std::size_t>(0.018 * sr);
    std::vector<std::size_t> hits;
    std::size_t last = 0;
    bool haveLast = false;
    for (std::size_t i=1;i+1<env.size();++i) {
        if (env[i] >= threshold &&
            env[i] >= env[i-1] &&
            env[i] > env[i+1] &&
            (!haveLast || i-last >= refractory)) {
            hits.push_back(i);
            last=i;
            haveLast=true;
        }
    }
    r.impulseCount = static_cast<int>(hits.size());

    if (hits.size() >= 2) {
        std::vector<double> gaps;
        gaps.reserve(hits.size()-1);
        for (std::size_t i=1;i<hits.size();++i)
            gaps.push_back(1000.0 * (hits[i]-hits[i-1]) / sr);
        r.meanGapMs = std::accumulate(gaps.begin(),gaps.end(),0.0)/gaps.size();
        double var=0.0;
        for (double g:gaps) {
            const double d=g-r.meanGapMs;
            var += d*d;
        }
        r.gapStdMs=std::sqrt(var/gaps.size());
    }
    return r;
}

SampleSet makeSyntheticSet(std::vector<float>& run, std::vector<float>& action) {
    SampleSet set;
    set.run.add({run.data(),run.size(),48000.0,true,"run"});
    set.action.add({action.data(),action.size(),48000.0,false,"action"});
    return set;
}

Result renderCase(float actionValue, float wearValue, float scaleValue=0.35f) {
    constexpr double sr=48000.0;

    std::vector<float> run(static_cast<std::size_t>(0.5*sr),0.0f);
    for (std::size_t i=0;i<run.size();++i)
        run[i]=0.035f*static_cast<float>(std::sin(2.0*3.14159265358979323846*12.0*i/sr));

    std::vector<float> action(static_cast<std::size_t>(0.08*sr),0.0f);
    action[0]=0.9f;
    for (std::size_t i=1;i<action.size();++i)
        action[i]=0.15f*std::exp(-static_cast<float>(i)/(0.006f*sr));

    auto set=makeSyntheticSet(run,action);

    Engine e;
    e.prepare(sr);
    e.setSampleSet(&set);

    Parameters p;
    p.speed=0.50f;
    p.load=0.25f;
    p.action=actionValue;
    p.wear=wearValue;
    p.clatter=0.08f + 0.20f*wearValue;
    p.scale=scaleValue;
    p.output=0.35f;
    e.setParameters(p);
    e.start();

    std::vector<float> out(static_cast<std::size_t>(12.0*sr),0.0f);
    e.process(out.data(),out.size());
    return analyze(out,sr);
}

}

int main() {
    std::cout<<"CONTROL,VALUE,EVENTS,MEAN_GAP_MS,GAP_STD_MS,RMS,PEAK\n";

    for (float v : {0.0f,0.25f,0.50f,0.75f,1.0f}) {
        const auto r=renderCase(v,0.20f);
        std::cout<<"ACTION,"<<v<<','<<r.impulseCount<<','<<r.meanGapMs<<','<<r.gapStdMs<<','<<r.rms<<','<<r.peak<<"\n";
    }

    for (float v : {0.0f,0.25f,0.50f,0.75f,1.0f}) {
        const auto r=renderCase(0.55f,v);
        std::cout<<"WEAR,"<<v<<','<<r.impulseCount<<','<<r.meanGapMs<<','<<r.gapStdMs<<','<<r.rms<<','<<r.peak<<"\n";
    }

    return 0;
}
