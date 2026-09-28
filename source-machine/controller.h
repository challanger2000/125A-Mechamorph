#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/lib/cview.h"
#include "vstgui/uidescription/uidescription.h"
#include "vstgui/uidescription/uiattributes.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "pluginterfaces/vst/ivstdataexchange.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "status_exchange.h"

namespace MechamorphMachine {

class Controller final : public Steinberg::Vst::EditControllerEx1,
                         public VSTGUI::VST3EditorDelegate,
                         public Steinberg::Vst::IDataExchangeReceiver {
public:
    Controller();
    ~Controller() override = default;
    Steinberg::uint32 PLUGIN_API addRef() override { return Steinberg::Vst::EditControllerEx1::addRef(); }
    Steinberg::uint32 PLUGIN_API release() override { return Steinberg::Vst::EditControllerEx1::release(); }
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid, void** obj) override;
    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IEditController*>(new Controller());
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) override;
    void PLUGIN_API queueOpened(Steinberg::Vst::DataExchangeUserContextID userContextID, Steinberg::uint32 blockSize, Steinberg::TBool& dispatchOnBackgroundThread) override;
    void PLUGIN_API queueClosed(Steinberg::Vst::DataExchangeUserContextID userContextID) override;
    void PLUGIN_API onDataExchangeBlocksReceived(Steinberg::Vst::DataExchangeUserContextID userContextID, Steinberg::uint32 numBlocks, Steinberg::Vst::DataExchangeBlock* blocks, Steinberg::TBool onBackgroundThread) override;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override;

    VSTGUI::CView* createCustomView(VSTGUI::UTF8StringPtr name,
                                    const VSTGUI::UIAttributes& attributes,
                                    const VSTGUI::IUIDescription* description,
                                    VSTGUI::VST3Editor* editor) override;
private:
    Steinberg::Vst::DataExchangeReceiverHandler statusExchangeReceiver_;
};

} // namespace MechamorphMachine
