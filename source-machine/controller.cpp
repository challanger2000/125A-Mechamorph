#include "controller.h"
#include "parameters.h"
#include "MechamorphGui.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace MechamorphMachine {

Controller::Controller() : statusExchangeReceiver_(this) {}

tresult PLUGIN_API Controller::queryInterface(const TUID iid, void** obj) {
    if (!obj) return kInvalidArgument;
    if (std::memcmp(iid, IDataExchangeReceiver::iid, 16) == 0) {
        *obj = static_cast<IDataExchangeReceiver*>(this);
        EditControllerEx1::addRef();
        return kResultOk;
    }
    return EditControllerEx1::queryInterface(iid, obj);
}

namespace {
constexpr int32 kStateVersion = 4;
}

tresult PLUGIN_API Controller::initialize(FUnknown* context) {
    auto r = EditControllerEx1::initialize(context);
    if (r != kResultOk)
        return r;

    auto* machine = new StringListParameter(
        STR16("Machine"), kMachine, nullptr,
        ParameterInfo::kCanAutomate | ParameterInfo::kIsList);
    machine->appendString(STR16("Tiny"));
    machine->appendString(STR16("Intricate"));
    machine->appendString(STR16("Heavy"));
    machine->appendString(STR16("Colossal"));
    machine->appendString(STR16("Pneumatic"));
    machine->appendString(STR16("Broken"));
    parameters.addParameter(machine);

    auto addPercent = [&](const TChar* title, ParamID id, double def) {
        auto* p = new RangeParameter(title, id, STR16("%"), 0.0, 100.0, def * 100.0);
        p->setPrecision(0);
        parameters.addParameter(p);
    };

    addPercent(STR16("Speed"), kSpeed, 0.32);
    addPercent(STR16("Load"), kLoad, 0.20);
    addPercent(STR16("Action"), kAction, 0.48);
    addPercent(STR16("Wear"), kWear, 0.18);
    addPercent(STR16("Scale"), kScale, 0.35);
    addPercent(STR16("Body"), kBody, 0.28);
    addPercent(STR16("Space"), kSpace, 0.18);

    auto* output = new RangeParameter(
        STR16("Output"), kOutput, STR16("dB"), -12.0, 12.0, -2.88);
    output->setPrecision(1);
    parameters.addParameter(output);

    parameters.addParameter(new RangeParameter(
        STR16("Pressure Status"), kPressureStatus, nullptr,
        0.0, 1.0, 0.0, 1, ParameterInfo::kIsReadOnly));
    parameters.addParameter(new RangeParameter(
        STR16("Friction Status"), kFrictionStatus, nullptr,
        0.0, 1.0, 0.0, 1, ParameterInfo::kIsReadOnly));
    parameters.addParameter(new RangeParameter(
        STR16("Stall Status"), kStallStatus, nullptr,
        0.0, 1.0, 0.0, 1, ParameterInfo::kIsReadOnly));

    return kResultOk;
}


tresult PLUGIN_API Controller::notify(IMessage* message) {
    if (message && statusExchangeReceiver_.onMessage(message)) return kResultTrue;
    return EditControllerEx1::notify(message);
}
void PLUGIN_API Controller::queueOpened(DataExchangeUserContextID id, uint32 blockSize, TBool& background) {
    if (id == kStatusExchangeContext && blockSize >= sizeof(StatusExchangeData)) background = false;
}
void PLUGIN_API Controller::queueClosed(DataExchangeUserContextID id) {
    if (id != kStatusExchangeContext) return;
    setParamNormalized(kPressureStatus,0.0); setParamNormalized(kFrictionStatus,0.0); setParamNormalized(kStallStatus,0.0);
}
void PLUGIN_API Controller::onDataExchangeBlocksReceived(DataExchangeUserContextID id, uint32 n, DataExchangeBlock* blocks, TBool) {
    if (id != kStatusExchangeContext || !blocks || n == 0) return;
    const StatusExchangeData* latest=nullptr;
    for (uint32 i=0;i<n;++i) if (blocks[i].data && blocks[i].size >= sizeof(StatusExchangeData)) latest=static_cast<const StatusExchangeData*>(blocks[i].data);
    if (!latest) return;
    setParamNormalized(kPressureStatus,std::clamp(latest->pressure,0.0,1.0));
    setParamNormalized(kFrictionStatus,std::clamp(latest->friction,0.0,1.0));
    setParamNormalized(kStallStatus,std::clamp(latest->stall,0.0,1.0));
}

tresult PLUGIN_API Controller::setComponentState(IBStream* state) {
    if (!state) return kResultFalse;

    IBStreamer s(state, kLittleEndian);
    int32 version = 0;
    if (!s.readInt32(version))
        return kResultFalse;

    float machine=0.0f, speed=0.32f, load=0.20f, action=0.48f;
    float wear=0.18f, scale=0.35f, body=0.28f, space=0.18f, output=0.38f;

    if (version == 1) {
        if (!s.readFloat(machine) ||
            !s.readFloat(speed) ||
            !s.readFloat(load) ||
            !s.readFloat(action) ||
            !s.readFloat(wear) ||
            !s.readFloat(scale) ||
            !s.readFloat(output))
            return kResultFalse;
    } else if (version == 2) {
        if (!s.readFloat(machine) ||
            !s.readFloat(speed) ||
            !s.readFloat(load) ||
            !s.readFloat(action) ||
            !s.readFloat(wear) ||
            !s.readFloat(scale) ||
            !s.readFloat(space) ||
            !s.readFloat(output))
            return kResultFalse;
    } else if (version == 3 || version == kStateVersion) {
        if (!s.readFloat(machine) ||
            !s.readFloat(speed) ||
            !s.readFloat(load) ||
            !s.readFloat(action) ||
            !s.readFloat(wear) ||
            !s.readFloat(scale) ||
            !s.readFloat(body) ||
            !s.readFloat(space) ||
            !s.readFloat(output))
            return kResultFalse;
    } else {
        return kResultFalse;
    }

    const std::array<float, 9> restoredValues{
        machine, speed, load, action, wear, scale, body, space, output
    };
    for (float value : restoredValues) {
        if (!std::isfinite(value))
            return kResultFalse;
    }

    if (version <= 3) {
        const int legacyIndex = std::clamp(
            static_cast<int>(std::lround(std::clamp<double>(machine,0.0,1.0) * 2.0)),
            0, 2);
        static constexpr int kLegacyToCurrent[3] = {0, 1, 2};
        setParamNormalized(
            kMachine,
            static_cast<double>(kLegacyToCurrent[legacyIndex]) / 5.0);
    } else {
        setParamNormalized(kMachine, std::clamp<double>(machine,0.0,1.0));
    }
    setParamNormalized(kSpeed, std::clamp<double>(speed,0.0,1.0));
    setParamNormalized(kLoad, std::clamp<double>(load,0.0,1.0));
    setParamNormalized(kAction, std::clamp<double>(action,0.0,1.0));
    setParamNormalized(kWear, std::clamp<double>(wear,0.0,1.0));
    setParamNormalized(kScale, std::clamp<double>(scale,0.0,1.0));
    setParamNormalized(kBody, std::clamp<double>(body,0.0,1.0));
    setParamNormalized(kSpace, std::clamp<double>(space,0.0,1.0));
    setParamNormalized(kOutput, std::clamp<double>(output,0.0,1.0));

    return kResultOk;
}


VSTGUI::CView* Controller::createCustomView(VSTGUI::UTF8StringPtr name,
                                             const VSTGUI::UIAttributes& attributes,
                                             const VSTGUI::IUIDescription* description,
                                             VSTGUI::VST3Editor* editor) {
    if(!name || !editor) return nullptr;

    VSTGUI::CPoint origin{0,0}, size{64,64};
    attributes.getPointAttribute("origin",origin);
    attributes.getPointAttribute("size",size);
    const VSTGUI::CRect r(origin.x,origin.y,origin.x+size.x,origin.y+size.y);

    auto knob=[&](const char* n, ParamID id, GuiKnob::Style style,
                   const char* bitmapName, float defaultValue)->VSTGUI::CView* {
        if(std::strcmp(name,n)!=0) return nullptr;
        auto* bitmap=description ? description->getBitmap(bitmapName) : nullptr;
        return static_cast<VSTGUI::CView*>(new GuiKnob(r,editor,id,style,bitmap,defaultValue));
    };
    auto label=[&](const char* n,const char* text,double fs,bool muted=false)->VSTGUI::CView* {
        return std::strcmp(name,n)==0 ? static_cast<VSTGUI::CView*>(new GuiLabel(r,text,fs,muted)) : nullptr;
    };

    if(std::strcmp(name,"Faceplate")==0){
        auto* bitmap=description ? description->getBitmap("mech-faceplate") : nullptr;
        return new GuiFaceplate(r,bitmap);
    }
    if(std::strcmp(name,"DangerSign")==0){
        auto* bitmap=description ? description->getBitmap("mech-danger-sign") : nullptr;
        return new GuiDecoration(r,bitmap);
    }
    if(std::strcmp(name,"BrandLogo")==0) return new GuiLogo(r);
    if(std::strcmp(name,"UIScale")==0) return new GuiScale(r,editor);

    if(auto* v=knob("Machine",kMachine,GuiKnob::Style::Machine,"mech-knob-machine",0.00f)) return v;
    if(auto* v=knob("Speed",kSpeed,GuiKnob::Style::Main,"mech-knob-main",0.32f)) return v;
    if(auto* v=knob("Load",kLoad,GuiKnob::Style::Main,"mech-knob-main",0.20f)) return v;
    if(auto* v=knob("Action",kAction,GuiKnob::Style::Main,"mech-knob-main",0.48f)) return v;
    if(auto* v=knob("Wear",kWear,GuiKnob::Style::Main,"mech-knob-main",0.18f)) return v;
    if(auto* v=knob("Scale",kScale,GuiKnob::Style::Scale,"mech-knob-scale",0.35f)) return v;
    if(auto* v=knob("Body",kBody,GuiKnob::Style::Utility,"mech-knob-utility",0.28f)) return v;
    if(auto* v=knob("Space",kSpace,GuiKnob::Style::Utility,"mech-knob-utility",0.18f)) return v;
    if(auto* v=knob("Output",kOutput,GuiKnob::Style::Utility,"mech-knob-utility",0.38f)) return v;

    if(std::strcmp(name,"PressureLamp")==0) return new GuiStatusLamp(r,editor,kPressureStatus,GuiStatusLamp::Kind::Pressure);
    if(std::strcmp(name,"FrictionLamp")==0) return new GuiStatusLamp(r,editor,kFrictionStatus,GuiStatusLamp::Kind::Friction);
    if(std::strcmp(name,"StallLamp")==0) return new GuiStatusLamp(r,editor,kStallStatus,GuiStatusLamp::Kind::Stall);

    if(auto* v=label("BrandTitle","MECHAMORPH",28.0)) return v;
    if(auto* v=label("BrandSubtitle","MECHANICAL INSTRUMENT",11.0,true)) return v;
    if(auto* v=label("MachineLabel","MACHINE",13.0)) return v;
    if(auto* v=label("MachinePosRow1","1 TINY   2 INTRICATE   3 HEAVY",8.5,true)) return v;
    if(auto* v=label("MachinePosRow2","4 COLOSSAL   5 PNEUMATIC   6 BROKEN",8.5,true)) return v;
    if(auto* v=label("SpeedLabel","SPEED",12.0)) return v;
    if(auto* v=label("LoadLabel","LOAD",12.0)) return v;
    if(auto* v=label("ActionLabel","ACTION",12.0)) return v;
    if(auto* v=label("WearLabel","WEAR",12.0)) return v;
    if(auto* v=label("ScaleLabel","SCALE",12.0)) return v;
    if(auto* v=label("PressureLabel","PRESSURE",9.0,true)) return v;
    if(auto* v=label("FrictionLabel","FRICTION",9.0,true)) return v;
    if(auto* v=label("StallLabel","STALL",9.0,true)) return v;

    return nullptr;
}

} // namespace MechamorphMachine
