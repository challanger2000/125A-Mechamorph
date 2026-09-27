#include "MachineCharacterEngine.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

using namespace mechamorph::machine;

namespace {

struct Metrics {
    double peak = 0.0;
    double rms = 0.0;
    double e50 = 0.0;
    double e250 = 0.0;
    double e1000 = 0.0;
    double t60ms = 0.0;
};

Metrics measure(float body, float space, float scale) {
    constexpr double sr = 48000.0;
    constexpr double seconds = 6.0;
    const std::size_t n = static_cast<std::size_t>(seconds * sr);

    MachineCharacterEngine e;
    e.prepare(sr);
    e.setParameters({body, space, scale});

    std::vector<float> y(n, 0.0f);
    for (std::size_t i=0;i<n;++i) {
        const float x = i==0 ? 1.0f : 0.0f;
        y[i] = e.process(x);
    }

    Metrics m;
    double ss = 0.0;
    for (float v:y) {
        const double a=std::fabs(v);
        m.peak=std::max(m.peak,a);
        ss += static_cast<double>(v)*v;
    }
    m.rms=std::sqrt(ss/y.size());

    auto tailEnergy=[&](double ms){
        const std::size_t a=std::min<std::size_t>(
            y.size(),
            static_cast<std::size_t>(ms*0.001*sr));
        double e=0.0;
        for(std::size_t i=a;i<y.size();++i)
            e += static_cast<double>(y[i])*y[i];
        return e;
    };

    const double total=tailEnergy(0.0);
    m.e50 = total>0.0 ? tailEnergy(50.0)/total : 0.0;
    m.e250 = total>0.0 ? tailEnergy(250.0)/total : 0.0;
    m.e1000 = total>0.0 ? tailEnergy(1000.0)/total : 0.0;

    if(total>0.0) {
        const double threshold = total * 1.0e-6;
        double remaining=total;
        for(std::size_t i=0;i<y.size();++i) {
            remaining -= static_cast<double>(y[i])*y[i];
            if(remaining <= threshold) {
                m.t60ms = 1000.0 * static_cast<double>(i)/sr;
                break;
            }
        }
    }

    return m;
}

}

int main() {
    std::cout << "CONTROL,VALUE,SCALE,PEAK,RMS,E_AFTER_50MS,E_AFTER_250MS,E_AFTER_1000MS,T60_MS\n";

    for(float v : {0.0f,0.5f,1.0f}) {
        const auto m=measure(v,0.0f,0.35f);
        std::cout<<"BODY,"<<v<<",0.35,"
                 <<m.peak<<','<<m.rms<<','<<m.e50<<','<<m.e250<<','<<m.e1000<<','<<m.t60ms<<"\n";
    }

    for(float v : {0.0f,0.5f,1.0f}) {
        const auto m=measure(0.35f,v,0.35f);
        std::cout<<"SPACE,"<<v<<",0.35,"
                 <<m.peak<<','<<m.rms<<','<<m.e50<<','<<m.e250<<','<<m.e1000<<','<<m.t60ms<<"\n";
    }

    for(float s : {0.0f,0.5f,1.0f}) {
        const auto m=measure(0.35f,0.65f,s);
        std::cout<<"SPACE_SCALE,0.65,"<<s<<','
                 <<m.peak<<','<<m.rms<<','<<m.e50<<','<<m.e250<<','<<m.e1000<<','<<m.t60ms<<"\n";
    }

    return 0;
}
