#include <windows.h>

#include "base/source/fobject.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"

#include <algorithm>
#include <iostream>
#include <string>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace VST3::Hosting;

namespace {
constexpr wchar_t kWindowClassName[] = L"125A_Mechamorph_EditorLifecycle";
constexpr int kCycles = 5;

LRESULT CALLBACK wndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return DefWindowProcW(h,m,w,l);
}

bool ensureWindowClass() {
    WNDCLASSEXW wc{};
    wc.cbSize=sizeof(wc);
    wc.lpfnWndProc=wndProc;
    wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=kWindowClassName;
    wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    return RegisterClassExW(&wc)!=0 || GetLastError()==ERROR_CLASS_ALREADY_EXISTS;
}

void pump(DWORD ms) {
    const ULONGLONG end=GetTickCount64()+ms;
    MSG msg{};
    do {
        while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(5);
    } while(GetTickCount64()<end);
}

class HostFrame final : public FObject, public IPlugFrame {
public:
    explicit HostFrame(HWND h):hwnd_(h){}
    tresult PLUGIN_API resizeView(IPlugView* view, ViewRect* r) override {
        if(!view || !r) return kInvalidArgument;
        const int w=std::max<int>(1,r->right-r->left);
        const int h=std::max<int>(1,r->bottom-r->top);
        SetWindowPos(hwnd_,nullptr,0,0,w,h,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        return view->onSize(r);
    }
    OBJ_METHODS(HostFrame,FObject)
    DEFINE_INTERFACES
        DEF_INTERFACE(IPlugFrame)
    END_DEFINE_INTERFACES(FObject)
    REFCOUNT_METHODS(FObject)
private:
    HWND hwnd_{};
};

struct ControllerHolder {
    IEditController* controller{};
    IConnectionPoint* componentCP{};
    IConnectionPoint* controllerCP{};
    bool initialized{};
    bool connected{};
    void close() {
        if(connected && componentCP && controllerCP) {
            componentCP->disconnect(controllerCP);
            controllerCP->disconnect(componentCP);
        }
        if(componentCP) componentCP->release();
        if(controllerCP) controllerCP->release();
        if(controller) {
            if(initialized) controller->terminate();
            controller->release();
        }
        *this={};
    }
    ~ControllerHolder(){ close(); }
};

bool acquireController(IComponent* component,const PluginFactory& factory,FUnknown* host,ControllerHolder& h) {
    if(component->queryInterface(IEditController::iid,reinterpret_cast<void**>(&h.controller))==kResultTrue && h.controller)
        return true;
    TUID cid{};
    if(component->getControllerClassId(cid)!=kResultTrue) return false;
    auto c=factory.createInstance<IEditController>(VST3::UID(cid));
    if(!c) return false;
    h.controller=c.take();
    if(h.controller->initialize(host)!=kResultOk) return false;
    h.initialized=true;
    const bool a=component->queryInterface(IConnectionPoint::iid,reinterpret_cast<void**>(&h.componentCP))==kResultTrue && h.componentCP;
    const bool b=h.controller->queryInterface(IConnectionPoint::iid,reinterpret_cast<void**>(&h.controllerCP))==kResultTrue && h.controllerCP;
    if(a && b) {
        const auto c2e=h.componentCP->connect(h.controllerCP);
        const auto e2c=h.controllerCP->connect(h.componentCP);
        h.connected=(c2e==kResultTrue && e2c==kResultTrue);
    }
    return true;
}

bool validRect(IPlugView* view, int& w, int& h) {
    ViewRect r{};
    if(view->getSize(&r)!=kResultTrue) return false;
    w=r.right-r.left;
    h=r.bottom-r.top;
    return w>0 && h>0;
}

int run(const std::string& path) {
    std::string error;
    auto module=Module::create(path,error);
    if(!module) {
        std::cerr<<"module load failed: "<<error<<"\n";
        return 1;
    }
    if(!ensureWindowClass()) return 2;

    HostApplication hostApplication;
    FUnknown* host=&hostApplication;
    auto factory=module->getFactory();
    factory.setHostContext(host);

    int editors=0;
    for(const auto& info:factory.classInfos()) {
        if(info.category()!=kVstAudioEffectClass) continue;
        auto component=factory.createInstance<IComponent>(info.ID());
        if(!component) return 3;
        if(component->initialize(host)!=kResultOk) return 4;

        ControllerHolder holder;
        if(!acquireController(component.get(),factory,host,holder)) {
            component->terminate();
            continue;
        }

        for(int cycle=0;cycle<kCycles;++cycle) {
            IPlugView* view=holder.controller->createView(ViewType::kEditor);
            if(!view) return 5;
            ++editors;

            int baseW=0,baseH=0;
            if(!validRect(view,baseW,baseH)) return 6;
            if(baseW!=1440 || baseH!=900) {
                std::cerr<<"unexpected base editor size "<<baseW<<"x"<<baseH<<"\n";
                return 7;
            }

            HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,kWindowClassName,L"Mechamorph QA",
                WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,baseW,baseH,
                nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
            if(!hwnd) return 8;

            auto* frame=new HostFrame(hwnd);
            if(view->setFrame(frame)!=kResultTrue) return 9;
            if(view->isPlatformTypeSupported(kPlatformTypeHWND)!=kResultTrue) return 10;
            if(view->attached(reinterpret_cast<void*>(hwnd),kPlatformTypeHWND)!=kResultTrue) return 11;
            ShowWindow(hwnd,SW_SHOWNA);
            UpdateWindow(hwnd);
            pump(80);

            IPlugViewContentScaleSupport* scale{};
            if(view->queryInterface(IPlugViewContentScaleSupport::iid,reinterpret_cast<void**>(&scale))!=kResultTrue || !scale) {
                std::cerr<<"editor does not expose IPlugViewContentScaleSupport\n";
                return 12;
            }
            for(float factor:{1.0f,1.25f,1.5f,2.0f}) {
                if(scale->setContentScaleFactor(factor)!=kResultTrue) {
                    std::cerr<<"content scale rejected: "<<factor<<"\n";
                    return 13;
                }
                pump(50);
                int w=0,h=0;
                if(!validRect(view,w,h) || w!=baseW || h!=baseH) {
                    std::cerr<<"logical editor size changed at content scale "<<factor<<": "<<w<<"x"<<h<<"\n";
                    return 14;
                }
            }
            scale->release();

            view->onFocus(true);
            pump(10);
            view->onFocus(false);
            if(view->removed()!=kResultTrue) return 15;
            if(view->setFrame(nullptr)!=kResultTrue) return 16;
            frame->release();
            DestroyWindow(hwnd);
            pump(20);
            view->release();
        }

        holder.close();
        if(component->terminate()!=kResultOk) return 17;
    }

    if(editors==0) return 18;
    std::cout<<"Mechamorph editor lifecycle + 100/125/150/200% content scale PASS ("<<editors<<" editor cycles)\n";
    return 0;
}
}

int main(int argc,char** argv) {
    if(argc!=2) return 64;
    const HRESULT hr=CoInitialize(nullptr);
    if(FAILED(hr)) return 65;
    const int rc=run(argv[1]);
    CoUninitialize();
    return rc;
}
