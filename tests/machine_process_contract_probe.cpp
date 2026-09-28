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
    std::string error;
    auto module=Module::create(path,error);
    if(!module){ std::cerr<<"[FAIL] module load "<<error<<"\n"; return 1; }

    HostApplication hostApplication;
    FUnknown* host=&hostApplication;
    auto factory=module->getFactory();
    factory.setHostContext(host);

    for(const auto& info:factory.classInfos()){
        auto component=factory.createInstance<IComponent>(info.ID());
        if(!component) continue;
        if(component->initialize(host)!=kResultOk) return 2;

        IAudioProcessor* processor{};
        if(component->queryInterface(IAudioProcessor::iid,reinterpret_cast<void**>(&processor))!=kResultTrue || !processor){
            component->terminate();
            continue;
        }

        if(processor->canProcessSampleSize(kSample32)!=kResultTrue){
            std::cerr<<"[FAIL] 32-bit processing unsupported\n"; return 3;
        }
        if(processor->canProcessSampleSize(kSample64)==kResultTrue){
            std::cerr<<"[FAIL] plugin unexpectedly claims 64-bit processing\n"; return 4;
        }

        if(component->getBusCount(kAudio,kInput)!=0 ||
           component->getBusCount(kAudio,kOutput)!=1 ||
           component->getBusCount(kEvent,kInput)!=1){
            std::cerr<<"[FAIL] bus topology mismatch\n"; return 5;
        }

        SpeakerArrangement stereo=SpeakerArr::kStereo;
        if(processor->setBusArrangements(nullptr,0,&stereo,1)!=kResultTrue){
            std::cerr<<"[FAIL] stereo instrument arrangement rejected\n"; return 6;
        }
        component->activateBus(kAudio,kOutput,0,true);
        component->activateBus(kEvent,kInput,0,true);

        for(auto mode:{kRealtime,kOffline}){
            for(double sr:{44100.0,48000.0,96000.0,192000.0}){
                for(int32 block:{1,16,64,257,1024}){
                    if(!runBlock(component.get(),processor,mode,sr,block)){
                        processor->release();
                        component->terminate();
                        return 7;
                    }
                }
            }
        }

        // Repeated activation after the stress matrix must still succeed.
        ProcessSetup finalSetup{};
        finalSetup.processMode=kRealtime;
        finalSetup.symbolicSampleSize=kSample32;
        finalSetup.maxSamplesPerBlock=128;
        finalSetup.sampleRate=48000.0;
        if(processor->setupProcessing(finalSetup)!=kResultTrue) return 8;
        for(int i=0;i<8;++i){
            if(component->setActive(true)!=kResultTrue) return 9;
            if(processor->setProcessing(true)!=kResultTrue) return 10;
            if(processor->setProcessing(false)!=kResultTrue) return 11;
            if(component->setActive(false)!=kResultTrue) return 12;
        }

        processor->release();
        if(component->terminate()!=kResultOk) return 13;
        std::cout<<"Mechamorph VST3 process contract PASS: realtime/offline, 4 rates, 5 block sizes, MIDI, automation, NaN, activate/deactivate\n";
        return 0;
    }

    std::cerr<<"[FAIL] no audio processor class\n";
    return 14;
}
}

int main(int argc,char** argv){
    if(argc!=2) return 64;
    return run(argv[1]);
}
