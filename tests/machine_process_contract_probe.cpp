#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace VST3::Hosting;

namespace {

void trace(const std::string& s){ std::cout<<"[process-probe] "<<s<<std::endl; }
int fail(int code,const std::string& s){ std::cerr<<"[process-probe] FAIL "<<code<<": "<<s<<std::endl; return code; }

bool finiteBuffers(const std::vector<float>& l,const std::vector<float>& r){
    for(float v:l) if(!std::isfinite(v)) return false;
    for(float v:r) if(!std::isfinite(v)) return false;
    return true;
}

bool addParam(ParameterChanges& changes,ParamID id,int32 offset,ParamValue value){
    int32 qi=0,pi=0;
    auto* q=changes.addParameterData(id,qi);
    return q && q->addPoint(offset,value,pi)==kResultTrue;
}

Event noteEvent(bool on,int32 offset,int32 noteId){
    Event e{};
    e.busIndex=0;
    e.sampleOffset=offset;
    e.ppqPosition=0.0;
    e.flags=Event::kIsLive;
    if(on){
        e.type=Event::kNoteOnEvent;
        e.noteOn.channel=0;
        e.noteOn.pitch=48;
        e.noteOn.tuning=0.0f;
        e.noteOn.velocity=0.8f;
        e.noteOn.length=0;
        e.noteOn.noteId=noteId;
    } else {
        e.type=Event::kNoteOffEvent;
        e.noteOff.channel=0;
        e.noteOff.pitch=48;
        e.noteOff.tuning=0.0f;
        e.noteOff.velocity=0.0f;
        e.noteOff.noteId=noteId;
    }
    return e;
}

bool runBlock(IComponent* component,IAudioProcessor* processor,ProcessModes mode,
              double sampleRate,int32 blockSize){
    ProcessSetup setup{};
    setup.processMode=mode;
    setup.symbolicSampleSize=kSample32;
    setup.maxSamplesPerBlock=blockSize;
    setup.sampleRate=sampleRate;
    if(processor->setupProcessing(setup)!=kResultTrue){
        std::cerr<<"[FAIL] setupProcessing rate="<<sampleRate<<" block="<<blockSize<<" mode="<<mode<<"\n";
        return false;
    }

    if(component->setActive(true)!=kResultTrue) return false;
    if(processor->setProcessing(true)!=kResultTrue){
        component->setActive(false);
        return false;
    }

    std::vector<float> left(static_cast<size_t>(blockSize),0.0f);
    std::vector<float> right(static_cast<size_t>(blockSize),0.0f);
    Sample32* chans[2]{left.data(),right.data()};
    AudioBusBuffers out{};
    out.numChannels=2;
    out.silenceFlags=0;
    out.channelBuffers32=chans;

    EventList events(8);
    auto on=noteEvent(true,0,12501);
    if(events.addEvent(on)!=kResultTrue) return false;

    ParameterChanges params(8);
    if(!addParam(params,2001,0,0.10)) return false; // SPEED
    if(blockSize>1 && !addParam(params,2001,blockSize-1,0.90)) return false;
    if(!addParam(params,2007,0,0.80)) return false; // OUTPUT
    if(!addParam(params,2004,0,std::numeric_limits<double>::quiet_NaN())) return false; // WEAR robustness

    ParameterChanges status(8);
    ProcessData data{};
    data.processMode=mode;
    data.symbolicSampleSize=kSample32;
    data.numSamples=blockSize;
    data.numInputs=0;
    data.numOutputs=1;
    data.inputs=nullptr;
    data.outputs=&out;
    data.inputEvents=&events;
    data.inputParameterChanges=&params;
    data.outputParameterChanges=&status;

    if(processor->process(data)!=kResultOk || !finiteBuffers(left,right)){
        std::cerr<<"[FAIL] NoteOn/automation process rate="<<sampleRate<<" block="<<blockSize<<" mode="<<mode<<"\n";
        return false;
    }

    // Matching NoteOff on a following block.
    std::fill(left.begin(),left.end(),0.0f);
    std::fill(right.begin(),right.end(),0.0f);
    EventList offs(4);
    auto off=noteEvent(false,0,12501);
    if(offs.addEvent(off)!=kResultTrue) return false;
    data.inputEvents=&offs;
    data.inputParameterChanges=nullptr;
    status.clearQueue();
    if(processor->process(data)!=kResultOk || !finiteBuffers(left,right)){
        std::cerr<<"[FAIL] NoteOff process rate="<<sampleRate<<" block="<<blockSize<<" mode="<<mode<<"\n";
        return false;
    }

    // Zero-sample parameter flush is a legal host pattern.
    ParameterChanges flush(2);
    if(!addParam(flush,2002,0,0.50)) return false; // LOAD
    ProcessData zero{};
    zero.processMode=mode;
    zero.symbolicSampleSize=kSample32;
    zero.numSamples=0;
    zero.inputParameterChanges=&flush;
    if(processor->process(zero)!=kResultOk){
        std::cerr<<"[FAIL] zero-sample parameter flush\n";
        return false;
    }

    const bool stopOk=processor->setProcessing(false)==kResultTrue;
    const bool inactiveOk=component->setActive(false)==kResultTrue;
    return stopOk && inactiveOk;
}

int run(const std::string& path){
    trace("start: "+path);
    std::string error;
    trace("loading module");
    auto module=Module::create(path,error);
    if(!module) return fail(1,"module load: "+error);
    trace("module loaded");

    HostApplication hostApplication;
    FUnknown* host=&hostApplication;
    auto factory=module->getFactory();
    factory.setHostContext(host);
    trace("factory ready");

    for(const auto& info:factory.classInfos()){
        trace("factory class: "+info.name());
        auto component=factory.createInstance<IComponent>(info.ID());
        if(!component){ trace("not an IComponent; skip"); continue; }
        trace("component created");
        if(component->initialize(host)!=kResultOk) return fail(2,"component initialize");
        trace("component initialized");

        IAudioProcessor* processor{};
        if(component->queryInterface(IAudioProcessor::iid,reinterpret_cast<void**>(&processor))!=kResultTrue || !processor){
            trace("component is not IAudioProcessor; skip");
            component->terminate();
            continue;
        }
        trace("audio processor acquired");

        if(processor->canProcessSampleSize(kSample32)!=kResultTrue){
            std::cerr<<"[FAIL] 32-bit processing unsupported\n"; return fail(3,"32-bit processing unsupported");
        }
        if(processor->canProcessSampleSize(kSample64)==kResultTrue){
            std::cerr<<"[FAIL] plugin unexpectedly claims 64-bit processing\n"; return fail(4,"unexpected 64-bit support");
        }

        if(component->getBusCount(kAudio,kInput)!=0 ||
           component->getBusCount(kAudio,kOutput)!=1 ||
           component->getBusCount(kEvent,kInput)!=1){
            std::cerr<<"[FAIL] bus topology mismatch\n"; return fail(5,"bus topology mismatch");
        }

        SpeakerArrangement stereo=SpeakerArr::kStereo;
        if(processor->setBusArrangements(nullptr,0,&stereo,1)!=kResultTrue){
            std::cerr<<"[FAIL] stereo instrument arrangement rejected\n"; return fail(6,"stereo arrangement rejected");
        }
        trace("activating buses");
        component->activateBus(kAudio,kOutput,0,true);
        component->activateBus(kEvent,kInput,0,true);

        for(auto mode:{kRealtime,kOffline}){
            for(double sr:{44100.0,48000.0,96000.0,192000.0}){
                for(int32 block:{1,16,64,257,1024}){
                    trace("matrix mode="+std::to_string(static_cast<int>(mode))+
                          " sr="+std::to_string(static_cast<int>(sr))+
                          " block="+std::to_string(block));
                    if(!runBlock(component.get(),processor,mode,sr,block)){
                        processor->release();
                        component->terminate();
                        return fail(7,"process matrix case failed");
                    }
                }
            }
        }

        // Repeated activation after the stress matrix must still succeed.
        trace("process matrix complete; activation loop");
        ProcessSetup finalSetup{};
        finalSetup.processMode=kRealtime;
        finalSetup.symbolicSampleSize=kSample32;
        finalSetup.maxSamplesPerBlock=128;
        finalSetup.sampleRate=48000.0;
        if(processor->setupProcessing(finalSetup)!=kResultTrue) return fail(8,"final setupProcessing");
        for(int i=0;i<8;++i){
            if(component->setActive(true)!=kResultTrue) return fail(9,"activation loop setActive true");
            if(processor->setProcessing(true)!=kResultTrue) return fail(10,"activation loop setProcessing true");
            if(processor->setProcessing(false)!=kResultTrue) return fail(11,"activation loop setProcessing false");
            if(component->setActive(false)!=kResultTrue) return fail(12,"activation loop setActive false");
        }

        trace("activation loop complete; releasing processor");
        processor->release();
        trace("terminating component");
        if(component->terminate()!=kResultOk) return fail(13,"component terminate");
        trace("component terminated");
        std::cout<<"Mechamorph VST3 process contract PASS: realtime/offline, 4 rates, 5 block sizes, MIDI, automation, NaN, activate/deactivate\n";
        return 0;
    }

    return fail(14,"no audio processor class");
}
}

int main(int argc,char** argv){
    if(argc!=2) return 64;
    return run(argv[1]);
}
