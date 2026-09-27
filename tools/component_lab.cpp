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
    const char b[2] = {static_cast<char>(v & 0xff), static_cast<char>((v >> 8) & 0xff)};
    f.write(b, 2);
}
void writeU32(std::ofstream& f, std::uint32_t v) {
    const char b[4] = {
        static_cast<char>(v & 0xff), static_cast<char>((v >> 8) & 0xff),
        static_cast<char>((v >> 16) & 0xff), static_cast<char>((v >> 24) & 0xff)
    };
    f.write(b, 4);
}
bool writeFloatWav(const std::string& path, const std::vector<float>& x, std::uint32_t sr) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    const std::uint32_t bytes = static_cast<std::uint32_t>(x.size()*sizeof(float));
    f.write("RIFF",4); writeU32(f,36u+bytes); f.write("WAVE",4);
    f.write("fmt ",4); writeU32(f,16); writeU16(f,3); writeU16(f,1);
    writeU32(f,sr); writeU32(f,sr*4); writeU16(f,4); writeU16(f,32);
    f.write("data",4); writeU32(f,bytes);
    f.write(reinterpret_cast<const char*>(x.data()), static_cast<std::streamsize>(bytes));
    return static_cast<bool>(f);
}

struct Metrics {
    float peak=0.0f;
    double rms=0.0;
    double centroid=0.0;
    double t60ms=0.0;
};

double estimateZeroCrossFrequency(
    const std::vector<float>& x,
    std::size_t a,
    std::size_t b,
    double sr) {
    a = std::min(a,x.size());
    b = std::min(b,x.size());
    if (b<=a+2) return 0.0;
    std::size_t crossings=0;
    for (std::size_t i=a+1;i<b;++i)
        if (x[i-1] <= 0.0f && x[i] > 0.0f) ++crossings;
    const double seconds=static_cast<double>(b-a)/sr;
    return seconds>0.0?static_cast<double>(crossings)/seconds:0.0;
}

Metrics analyze(const std::vector<float>& x, double sr) {
    Metrics m;
    double sumSq=0.0;
    std::size_t peakIndex=0;
    for (std::size_t i=0;i<x.size();++i) {
        const float a=std::fabs(x[i]);
        if (a>m.peak) { m.peak=a; peakIndex=i; }
        sumSq += static_cast<double>(x[i])*x[i];
    }
    m.rms=std::sqrt(sumSq/std::max<std::size_t>(1,x.size()));

    const float threshold=m.peak*0.001f;
    std::size_t last=peakIndex;
    for (std::size_t i=peakIndex;i<x.size();++i)
        if (std::fabs(x[i])>=threshold) last=i;
    m.t60ms=1000.0*static_cast<double>(last-peakIndex)/sr;

    // Simple DFT centroid over first 4096 samples after trigger.
    const std::size_t N=std::min<std::size_t>(4096,x.size());
    double weighted=0.0,total=0.0;
    for (std::size_t k=0;k<=N/2;++k) {
        double re=0.0, im=0.0;
        for (std::size_t n=0;n<N;++n) {
            const double w=0.5-0.5*std::cos(2.0*3.14159265358979323846*n/(N-1));
            const double a=2.0*3.14159265358979323846*k*n/N;
            re += x[n]*w*std::cos(a);
            im -= x[n]*w*std::sin(a);
        }
        const double p=re*re+im*im;
        const double hz=static_cast<double>(k)*sr/N;
        weighted += hz*p;
        total += p;
    }
    m.centroid=total>0.0?weighted/total:0.0;
    return m;
}

}

int main(int argc, char** argv) {
    constexpr double sr=48000.0;
    const std::string prefix=argc>1?argv[1]:"clack";

    struct Variant { const char* name; float force; float hardness; float material; };
    const Variant variants[] = {
        {"wood_soft", 0.65f,0.30f,0.15f},
        {"wood_hard", 0.85f,0.70f,0.25f},
        {"mixed_pawl",0.80f,0.78f,0.55f},
        {"metal_hard",0.90f,0.90f,0.90f}
    };

    std::cout<<"variant,peak,rms,centroid_hz,t60_ms\n";

    for (const auto& v:variants) {
        mechamorph::ContactClackEngine clack;
        mechamorph::DeterministicRng rng;
        rng.seed(0x125AC1ACULL);
        clack.prepare(sr);
        clack.trigger(v.force,v.hardness,v.material,rng);

        std::vector<float> x(static_cast<std::size_t>(0.25*sr),0.0f);
        for (auto& s:x) s=clack.process();

        const auto m=analyze(x,sr);

        auto windowRms = [&](std::size_t a, std::size_t b) {
            a = std::min(a,x.size());
            b = std::min(b,x.size());
            if (b<=a) return 0.0;
            double ss=0.0;
            for (std::size_t i=a;i<b;++i) ss += static_cast<double>(x[i])*x[i];
            return std::sqrt(ss/static_cast<double>(b-a));
        };

        const std::size_t openEnd =
            std::min(x.size(), openAt + static_cast<std::size_t>(0.030*sr));
        const std::size_t holdStart =
            std::min(x.size(), openAt + static_cast<std::size_t>(0.050*sr));
        const std::size_t holdEnd =
            closeAt > static_cast<std::size_t>(0.020*sr)
                ? closeAt - static_cast<std::size_t>(0.020*sr)
                : closeAt;
        const std::size_t closeEnd =
            std::min(x.size(), closeAt + static_cast<std::size_t>(0.030*sr));

        const double openRms=windowRms(openAt,openEnd);
        const double holdRms=windowRms(holdStart,holdEnd);
        const double closeRms=windowRms(closeAt,closeEnd);

        std::cout<<v.name<<','<<m.peak<<','<<m.rms<<','<<m.centroid<<','<<m.t60ms
                 <<",open_rms="<<openRms
                 <<",hold_rms="<<holdRms
                 <<",close_rms="<<closeRms<<"\n";

        if (!writeFloatWav(prefix+std::string("-")+v.name+".wav",x,static_cast<std::uint32_t>(sr)))
            return 2;
    }
    struct ValveVariant { const char* name; float pressure; float force; double holdMs; };
    const ValveVariant valves[] = {
        {"valve_low_pressure", 0.30f, 0.45f, 120.0},
        {"valve_medium", 0.60f, 0.65f, 160.0},
        {"valve_high_pressure", 0.90f, 0.85f, 180.0}
    };

    for (const auto& v : valves) {
        mechamorph::ValveEngine valve;
        mechamorph::DeterministicRng rng;
        rng.seed(0x125A7A1EULL);
        valve.prepare(sr);

        std::vector<float> x(static_cast<std::size_t>(0.35*sr),0.0f);
        const std::size_t openAt = static_cast<std::size_t>(0.025*sr);
        const std::size_t closeAt = openAt + static_cast<std::size_t>(v.holdMs*0.001*sr);

        for (std::size_t i=0;i<x.size();++i) {
            if (i==openAt) valve.open(v.pressure,v.force,rng);
            if (i==closeAt) valve.close(v.pressure,v.force,rng);
            x[i]=valve.process(v.pressure,rng);
        }

        const auto m=analyze(x,sr);
        std::cout<<v.name<<','<<m.peak<<','<<m.rms<<','<<m.centroid<<','<<m.t60ms<<"\n";

        if (!writeFloatWav(prefix+std::string("-")+v.name+".wav",x,static_cast<std::uint32_t>(sr)))
            return 3;
    }

    struct PipeVariant { const char* name; float pressure; };
    const PipeVariant pipes[] = {
        {"pipe_low_pressure",0.30f},
        {"pipe_medium_pressure",0.60f},
        {"pipe_high_pressure",0.90f}
    };

    for (const auto& v:pipes) {
        mechamorph::PipeEngine pipe;
        mechamorph::DeterministicRng rng;
        rng.seed(0x125A91PEULL);
        pipe.prepare(sr);
        pipe.setFrequency(440.0f);

        std::vector<float> x(static_cast<std::size_t>(0.80*sr),0.0f);
        const std::size_t on=static_cast<std::size_t>(0.05*sr);
        const std::size_t off=static_cast<std::size_t>(0.58*sr);

        float pressure=0.0f;
        for (std::size_t i=0;i<x.size();++i) {
            const float target=(i>=on && i<off)?v.pressure:0.0f;
            const float coeff=target>pressure?0.0025f:0.0012f;
            pressure += coeff*(target-pressure);
            const float aperture=(i>=on && i<off)?1.0f:0.0f;
            x[i]=pipe.process(pressure,aperture,rng);
        }

        const auto m=analyze(x,sr);
        const std::size_t freqA=static_cast<std::size_t>(0.25*sr);
        const std::size_t freqB=static_cast<std::size_t>(0.50*sr);
        const double estimatedHz=estimateZeroCrossFrequency(x,freqA,freqB,sr);

        std::cout<<v.name<<','<<m.peak<<','<<m.rms<<','<<m.centroid<<','<<m.t60ms
                 <<",freq_hz="<<estimatedHz<<"\n";

        if (!writeFloatWav(prefix+std::string("-")+v.name+".wav",x,static_cast<std::uint32_t>(sr)))
            return 4;
    }

    return 0;
}
