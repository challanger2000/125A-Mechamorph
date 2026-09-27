#include "MachineSpaceEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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

std::uint16_t readU16(const unsigned char* p) {
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8));
}
std::uint32_t readU32(const unsigned char* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

bool loadPcm16Mono(const fs::path& path, std::vector<float>& out, int& sampleRate) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<unsigned char> bytes(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>());
    if (bytes.size() < 44) return false;
    if (std::string(reinterpret_cast<char*>(bytes.data()),4)!="RIFF" ||
        std::string(reinterpret_cast<char*>(bytes.data()+8),4)!="WAVE")
        return false;

    std::uint16_t format=0, channels=0, bits=0;
    std::uint32_t sr=0;
    const unsigned char* data=nullptr;
    std::size_t dataBytes=0;
    std::size_t pos=12;
    while(pos+8<=bytes.size()){
        const auto chunkSize=readU32(bytes.data()+pos+4);
        const auto* id=bytes.data()+pos;
        pos+=8;
        if(pos+chunkSize>bytes.size()) return false;
        if(std::string(reinterpret_cast<const char*>(id),4)=="fmt " && chunkSize>=16){
            format=readU16(bytes.data()+pos);
            channels=readU16(bytes.data()+pos+2);
            sr=readU32(bytes.data()+pos+4);
            bits=readU16(bytes.data()+pos+14);
        } else if(std::string(reinterpret_cast<const char*>(id),4)=="data"){
            data=bytes.data()+pos;
            dataBytes=chunkSize;
        }
        pos+=chunkSize+(chunkSize&1u);
    }
    if(format!=1 || channels!=1 || bits!=16 || sr==0 || !data) return false;
    const auto frames=dataBytes/2;
    out.resize(frames);
    for(std::size_t i=0;i<frames;++i){
        const auto raw=static_cast<std::int16_t>(readU16(data+2*i));
        out[i]=static_cast<float>(raw/32768.0f);
    }
    sampleRate=static_cast<int>(sr);
    return true;
}

bool writeFloatStereo(
    const fs::path& path,
    const std::vector<float>& left,
    const std::vector<float>& right,
    std::uint32_t sr) {

    if(left.size()!=right.size()) return false;
    std::ofstream f(path,std::ios::binary);
    if(!f) return false;

    const std::uint32_t bytes=static_cast<std::uint32_t>(
        left.size()*2*sizeof(float));
    f.write("RIFF",4); writeU32(f,36u+bytes); f.write("WAVE",4);
    f.write("fmt ",4); writeU32(f,16); writeU16(f,3); writeU16(f,2);
    writeU32(f,sr); writeU32(f,sr*8); writeU16(f,8); writeU16(f,32);
    f.write("data",4); writeU32(f,bytes);
    for(std::size_t i=0;i<left.size();++i){
        f.write(reinterpret_cast<const char*>(&left[i]),sizeof(float));
        f.write(reinterpret_cast<const char*>(&right[i]),sizeof(float));
    }
    return static_cast<bool>(f);
}

struct Metrics {
    double peak=0.0;
    double rms=0.0;
    double tailRms=0.0;
    double stereoDiffRms=0.0;
    double lastAboveMs=0.0;
};

Metrics analyze(
    const std::vector<float>& l,
    const std::vector<float>& r,
    std::size_t activeFrames,
    double sr) {

    Metrics m;
    double ss=0.0, tail=0.0, diff=0.0;
    std::size_t tailN=0, last=0;

    for(std::size_t i=0;i<l.size();++i){
        const double a=std::max(std::fabs(l[i]),std::fabs(r[i]));
        m.peak=std::max(m.peak,a);
        ss+=0.5*(static_cast<double>(l[i])*l[i]+static_cast<double>(r[i])*r[i]);
        const double d=static_cast<double>(l[i])-r[i];
        diff+=d*d;
        if(i>=activeFrames){
            tail+=0.5*(static_cast<double>(l[i])*l[i]+static_cast<double>(r[i])*r[i]);
            ++tailN;
        }
        if(a>1.0e-5) last=i;
    }
    const double n=static_cast<double>(std::max<std::size_t>(1,l.size()));
    m.rms=std::sqrt(ss/n);
    m.stereoDiffRms=std::sqrt(diff/n);
    m.tailRms=tailN?std::sqrt(tail/static_cast<double>(tailN)):0.0;
    m.lastAboveMs=1000.0*static_cast<double>(last)/sr;
    return m;
}

Metrics render(
    const std::vector<float>& source,
    int sourceSr,
    float body,
    float space,
    float scale,
    const fs::path& output) {

    const double sr=48000.0;
    const double activeSeconds=6.0;
    const double totalSeconds=10.0;
    const auto activeFrames=static_cast<std::size_t>(activeSeconds*sr);
    const auto totalFrames=static_cast<std::size_t>(totalSeconds*sr);

    std::vector<float> l(totalFrames,0.0f),r(totalFrames,0.0f);

    MechamorphMachine::MachineSpaceEngine engine;
    engine.prepare(sr);
    engine.setBody(body);
    engine.setSpace(space);
    engine.setScale(scale);

    double sourcePos=0.0;
    const double sourceRate=static_cast<double>(sourceSr)/sr;

    for(std::size_t i=0;i<totalFrames;++i){
        float in=0.0f;
        if(i<activeFrames && !source.empty()){
            const auto a=static_cast<std::size_t>(sourcePos)%source.size();
            const auto b=(a+1)%source.size();
            const float frac=static_cast<float>(sourcePos-std::floor(sourcePos));
            in=source[a]+frac*(source[b]-source[a]);
            sourcePos+=sourceRate;
            if(sourcePos>=static_cast<double>(source.size()))
                sourcePos=std::fmod(sourcePos,static_cast<double>(source.size()));
        }
        engine.process(in,l[i],r[i]);
    }

    writeFloatStereo(output,l,r,static_cast<std::uint32_t>(sr));
    return analyze(l,r,activeFrames,sr);
}

}

int main(int argc,char** argv){
    if(argc<3){
        std::cerr<<"usage: measure_machine_space <mono_pcm16_machine.wav> <output_dir>\n";
        return 2;
    }

    std::vector<float> source;
    int sourceSr=0;
    if(!loadPcm16Mono(argv[1],source,sourceSr)){
        std::cerr<<"failed to load source wav\n";
        return 3;
    }

    const fs::path outDir=argv[2];
    fs::create_directories(outDir);

    std::cout<<"CONTROL,VALUE,PEAK,RMS,TAIL_RMS,STEREO_DIFF_RMS,LAST_ABOVE_MS\n";

    for(float v:{0.0f,0.5f,1.0f}){
        const auto name="BODY_"+std::to_string(static_cast<int>(v*100.0f))+".wav";
        const auto m=render(source,sourceSr,v,0.0f,0.35f,outDir/name);
        std::cout<<"BODY,"<<v<<','<<m.peak<<','<<m.rms<<','<<m.tailRms<<','
                 <<m.stereoDiffRms<<','<<m.lastAboveMs<<"\n";
    }

    for(float v:{0.0f,0.5f,1.0f}){
        const auto name="SPACE_"+std::to_string(static_cast<int>(v*100.0f))+".wav";
        const auto m=render(source,sourceSr,0.35f,v,0.35f,outDir/name);
        std::cout<<"SPACE,"<<v<<','<<m.peak<<','<<m.rms<<','<<m.tailRms<<','
                 <<m.stereoDiffRms<<','<<m.lastAboveMs<<"\n";
    }

    for(float scale:{0.0f,0.5f,1.0f}){
        const auto name="SPACE_SCALE_"+std::to_string(static_cast<int>(scale*100.0f))+".wav";
        const auto m=render(source,sourceSr,0.35f,0.55f,scale,outDir/name);
        std::cout<<"SPACE_SCALE,"<<scale<<','<<m.peak<<','<<m.rms<<','<<m.tailRms<<','
                 <<m.stereoDiffRms<<','<<m.lastAboveMs<<"\n";
    }

    return 0;
}
