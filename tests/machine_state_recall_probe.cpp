#include "base/source/fstreamer.h"
#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace VST3::Hosting;

namespace {
constexpr int32 kStateVersion = 4;
constexpr std::array<ParamID,9> kIds{2000,2001,2002,2003,2004,2005,2008,2006,2007};

bool nearly(double a,double b){ return std::fabs(a-b)<1.0e-6; }

bool writeState(MemoryStream& ms,int32 version,const std::initializer_list<float>& values){
    IBStreamer s(&ms,kLittleEndian);
    if(!s.writeInt32(version)) return false;
    for(float v:values) if(!s.writeFloat(v)) return false;
    ms.seek(0,IBStream::kIBSeekSet,nullptr);
    return true;
}

bool readV4(IComponent* c,std::array<float,9>& values){
    MemoryStream ms;
    if(c->getState(&ms)!=kResultTrue) return false;
    ms.seek(0,IBStream::kIBSeekSet,nullptr);
    IBStreamer s(&ms,kLittleEndian);
    int32 version=0;
    if(!s.readInt32(version) || version!=kStateVersion) return false;
    for(float& v:values) if(!s.readFloat(v) || !std::isfinite(v)) return false;
    return true;
}

bool same(const std::array<float,9>& a,const std::array<float,9>& b){
    for(size_t i=0;i<a.size();++i) if(!nearly(a[i],b[i])) return false;
    return true;
}

int run(const std::string& path){
    std::string error;
    auto module=Module::create(path,error);
    if(!module){ std::cerr<<"[FAIL] load: "<<error<<"\n"; return 1; }

    HostApplication hostApplication;
    FUnknown* host=&hostApplication;
    auto factory=module->getFactory();
    factory.setHostContext(host);

    for(const auto& info:factory.classInfos()){
        auto source=factory.createInstance<IComponent>(info.ID());
        if(!source) continue;
        if(source->initialize(host)!=kResultOk){ std::cerr<<"[FAIL] initialize\n"; return 2; }

        std::array<float,9> defaults{};
        if(!readV4(source.get(),defaults)){ std::cerr<<"[FAIL] read default v4 state\n"; return 3; }
        const std::array<float,9> expectedDefaults{0.0f,0.32f,0.20f,0.48f,0.18f,0.35f,0.28f,0.18f,0.38f};
        if(!same(defaults,expectedDefaults)){ std::cerr<<"[FAIL] default v4 values mismatch\n"; return 4; }

        TUID controllerCid{};
        if(source->getControllerClassId(controllerCid)!=kResultTrue){ std::cerr<<"[FAIL] controller CID\n"; return 5; }
        auto controller=factory.createInstance<IEditController>(VST3::UID(controllerCid));
        if(!controller || controller->initialize(host)!=kResultOk){ std::cerr<<"[FAIL] controller init\n"; return 6; }

        MemoryStream controllerState;
        if(!writeState(controllerState,4,{0.6f,0.11f,0.22f,0.33f,0.44f,0.55f,0.66f,0.77f,0.88f}) ||
           controller->setComponentState(&controllerState)!=kResultTrue){
            std::cerr<<"[FAIL] controller setComponentState v4\n"; return 7;
        }
        const std::array<double,9> expectedController{0.6,0.11,0.22,0.33,0.44,0.55,0.66,0.77,0.88};
        for(size_t i=0;i<kIds.size();++i){
            const double actual=controller->getParamNormalized(kIds[i]);
            if(!nearly(actual,expectedController[i])){
                std::cerr<<"[FAIL] controller param "<<kIds[i]<<" expected "<<expectedController[i]<<" got "<<actual<<"\n";
                return 8;
            }
        }
        controller->terminate();
        controller.reset();

        // Fresh-instance v4 roundtrip.
        MemoryStream custom;
        if(!writeState(custom,4,{0.8f,0.12f,0.23f,0.34f,0.45f,0.56f,0.67f,0.78f,0.89f}) ||
           source->setState(&custom)!=kResultTrue){
            std::cerr<<"[FAIL] set custom v4\n"; return 9;
        }
        std::array<float,9> customRoundtrip{};
        if(!readV4(source.get(),customRoundtrip)){ std::cerr<<"[FAIL] re-save custom v4\n"; return 10; }
        const std::array<float,9> expectedCustom{0.8f,0.12f,0.23f,0.34f,0.45f,0.56f,0.67f,0.78f,0.89f};
        if(!same(customRoundtrip,expectedCustom)){ std::cerr<<"[FAIL] v4 roundtrip mismatch\n"; return 11; }

        // Legacy v1: body/space use modern safe defaults; old machine 1.0 maps to current HEAVY (index 2 => 0.4).
        MemoryStream v1;
        if(!writeState(v1,1,{1.0f,0.10f,0.20f,0.30f,0.40f,0.50f,0.90f}) ||
           source->setState(&v1)!=kResultTrue){ std::cerr<<"[FAIL] v1 migration rejected\n"; return 12; }
        std::array<float,9> migrated1{};
        if(!readV4(source.get(),migrated1)){ return 13; }
        const std::array<float,9> expected1{0.4f,0.10f,0.20f,0.30f,0.40f,0.50f,0.28f,0.18f,0.90f};
        if(!same(migrated1,expected1)){ std::cerr<<"[FAIL] v1 migration mismatch\n"; return 14; }

        // Legacy v2 added SPACE, BODY still defaults safely.
        MemoryStream v2;
        if(!writeState(v2,2,{0.5f,0.15f,0.25f,0.35f,0.45f,0.55f,0.65f,0.75f}) ||
           source->setState(&v2)!=kResultTrue){ std::cerr<<"[FAIL] v2 migration rejected\n"; return 15; }
        std::array<float,9> migrated2{};
        if(!readV4(source.get(),migrated2)){ return 16; }
        const std::array<float,9> expected2{0.2f,0.15f,0.25f,0.35f,0.45f,0.55f,0.28f,0.65f,0.75f};
        if(!same(migrated2,expected2)){ std::cerr<<"[FAIL] v2 migration mismatch\n"; return 17; }

        // Legacy v3 had BODY+SPACE and the old three-machine encoding.
        MemoryStream v3;
        if(!writeState(v3,3,{0.0f,0.16f,0.26f,0.36f,0.46f,0.56f,0.66f,0.76f,0.86f}) ||
           source->setState(&v3)!=kResultTrue){ std::cerr<<"[FAIL] v3 migration rejected\n"; return 18; }
        std::array<float,9> migrated3{};
        if(!readV4(source.get(),migrated3)){ return 19; }
        const std::array<float,9> expected3{0.0f,0.16f,0.26f,0.36f,0.46f,0.56f,0.66f,0.76f,0.86f};
        if(!same(migrated3,expected3)){ std::cerr<<"[FAIL] v3 migration mismatch\n"; return 20; }

        MemoryStream invalid;
        if(!writeState(invalid,4,{0.0f,0.32f,std::numeric_limits<float>::quiet_NaN(),0.48f,0.18f,0.35f,0.28f,0.18f,0.38f})){
            return 21;
        }
        if(source->setState(&invalid)==kResultTrue){ std::cerr<<"[FAIL] NaN state accepted\n"; return 22; }

        if(source->terminate()!=kResultOk){ std::cerr<<"[FAIL] terminate\n"; return 23; }
        std::cout<<"Mechamorph state/recall contract PASS\n";
        return 0;
    }
    std::cerr<<"[FAIL] no IComponent class\n";
    return 24;
}
}

int main(int argc,char** argv){
    if(argc!=2) return 64;
    return run(argv[1]);
}
