#include "controller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include <cstring>

namespace MechamorphMachine {

Steinberg::IPlugView* PLUGIN_API Controller::createView(Steinberg::FIDString name) {
    if(std::strcmp(name,Steinberg::Vst::ViewType::kEditor)==0){
        auto* editor=new VSTGUI::VST3Editor(this,"view","mechamorph.uidesc");
        editor->setAllowedZoomFactors({1.0,1.25,1.5,2.0});
        return editor;
    }
    return nullptr;
}

} // namespace MechamorphMachine
