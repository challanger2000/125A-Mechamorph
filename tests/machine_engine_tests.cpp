#include "MachineEngine.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using namespace mechamorph::machine;

int main() {
    constexpr double sr = 48000.0;

    // Synthetic placeholders only for engine/state validation.
    std::array<float, 512> start{};
    std::array<float, 1024> run{};
    std::array<float, 128> action{};
    std::array<float, 256> load{};
    std::array<float, 256> release{};
    std::array<float, 512> stop{};

    for (std::size_t i=0;i<start.size();++i)
        start[i]=static_cast<float>(0.5*std::exp(-5.0*i/start.size())*std::sin(0.04*i));
    for (std::size_t i=0;i<run.size();++i)
        run[i]=static_cast<float>(0.08*std::sin(2.0*3.14159265358979323846*12.0*i/run.size()));
    action[0]=0.8f;
    load[0]=0.5f;
    release[0]=-0.4f;
    stop[0]=0.7f;

    MachineEngine engine;
    engine.prepare(sr);

    auto makeSet=[&](const float* data,std::size_t frames){
        SampleSet set;
        assert(set.add({data,frames,48000}));
        return set;
    };

    assert(engine.setSampleSet(SampleRole::Start, makeSet(start.data(),start.size())));
    assert(engine.setSampleSet(SampleRole::RunBed, makeSet(run.data(),run.size())));
    assert(engine.setSampleSet(SampleRole::Action, makeSet(action.data(),action.size())));
    assert(engine.setSampleSet(SampleRole::Load, makeSet(load.data(),load.size())));
    assert(engine.setSampleSet(SampleRole::Release, makeSet(release.data(),release.size())));
    assert(engine.setSampleSet(SampleRole::Stop, makeSet(stop.data(),stop.size())));

    MachineParameters p;
    p.speed=0.4f;
    p.load=0.6f;
    p.wear=0.3f;
    p.activity=0.6f;
    p.output=0.5f;
    engine.setParameters(p);

    std::vector<float> out(static_cast<std::size_t>(3.0*sr),0.0f);
    engine.start();

    double energy=0.0;
    for (std::size_t i=0;i<out.size();++i) {
        if (i==static_cast<std::size_t>(1.0*sr)) engine.setLoaded(true);
        if (i==static_cast<std::size_t>(1.8*sr)) engine.setLoaded(false);
        if (i==static_cast<std::size_t>(2.2*sr)) engine.stop();

        out[i]=engine.processSample();
        assert(std::isfinite(out[i]));
        energy += std::fabs(out[i]);
    }

    assert(energy>0.1);
    assert(engine.state().phase==MachinePhase::Stopped);

    // Deterministic reset/start sequence.
    engine.reset();
    engine.setParameters(p);
    engine.setSampleSet(SampleRole::Start, makeSet(start.data(),start.size()));
    engine.setSampleSet(SampleRole::RunBed, makeSet(run.data(),run.size()));
    engine.setSampleSet(SampleRole::Action, makeSet(action.data(),action.size()));
    engine.start();

    std::vector<float> a(4096),b(4096);
    for(auto& s:a) s=engine.processSample();

    engine.reset();
    engine.setParameters(p);
    engine.setSampleSet(SampleRole::Start, makeSet(start.data(),start.size()));
    engine.setSampleSet(SampleRole::RunBed, makeSet(run.data(),run.size()));
    engine.setSampleSet(SampleRole::Action, makeSet(action.data(),action.size()));
    engine.start();
    for(auto& s:b) s=engine.processSample();

    for(std::size_t i=0;i<a.size();++i)
        assert(a[i]==b[i]);

    std::cout<<"Machine engine tests PASS\n";
    return 0;
}
