#include <windows.h>

#include "base/source/fobject.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"

#include <algorithm>
#include <cmath>
#include <vector>
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

HWND findVstguiChild(HWND parent) {
    struct Search { HWND found{}; } search;
    EnumChildWindows(parent, [](HWND child, LPARAM data)->BOOL {
        auto* s=reinterpret_cast<Search*>(data);
        wchar_t className[128]{};
        if(GetClassNameW(child,className,static_cast<int>(std::size(className)))>0) {
            const std::wstring name{className};
            if(name.rfind(L"VSTGUI",0)==0) {
                s->found=child;
                return FALSE;
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&search));
    return search.found;
}

bool clickUiScale(HWND vstguiChild,double currentZoom) {
    if(!vstguiChild) return false;
    // UIScale logical rect: x=48..154, y=294..324.
    // VSTGUI's user zoom scales both the control geometry and the platform child.
    const int x=static_cast<int>(std::lround(101.0*currentZoom));
    const int y=static_cast<int>(std::lround(309.0*currentZoom));
    const LPARAM p=MAKELPARAM(x,y);
    SendMessageW(vstguiChild,WM_LBUTTONDOWN,MK_LBUTTON,p);
    SendMessageW(vstguiChild,WM_LBUTTONUP,0,p);
    pump(80);
    return true;
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
        // Probe any factory class that actually implements IComponent.
        // This avoids coupling the host-side QA probe to SDK-version-specific
        // factory category constants.
        auto component=factory.createInstance<IComponent>(info.ID());
        if(!component) continue;
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
            if(scale->setContentScaleFactor(1.0f)!=kResultTrue) return 19;
            pump(40);
            scale->release();

            // Exercise the actual in-GUI UI-scale button, not merely host DPI/content scale.
            HWND vstguiChild=findVstguiChild(hwnd);
            if(!vstguiChild) {
                std::cerr<<"VSTGUI child HWND not found\n";
                return 20;
            }
            const double zoomBefore[]={1.0,1.25,1.5,2.0};
            const double zoomAfter []={1.25,1.5,2.0,1.0};
            for(int zi=0;zi<4;++zi) {
                if(!clickUiScale(vstguiChild,zoomBefore[zi])) return 21;
                int w=0,h=0;
                const int expectedW=static_cast<int>(std::lround(baseW*zoomAfter[zi]));
                const int expectedH=static_cast<int>(std::lround(baseH*zoomAfter[zi]));
                if(!validRect(view,w,h) || w!=expectedW || h!=expectedH) {
                    std::cerr<<"UI zoom "<<zoomAfter[zi]<<" produced "<<w<<"x"<<h
                             <<"; expected "<<expectedW<<"x"<<expectedH<<"\n";
                    return 22;
                }
            }

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
    std::cout<<"Mechamorph editor lifecycle + DPI multires + actual UI zoom 100/125/150/200% PASS ("<<editors<<" editor cycles)\n";
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
