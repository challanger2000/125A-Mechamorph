#include <windows.h>
#include <wincodec.h>

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

void trace(const std::string& message) {
    std::cout << "[editor-probe] " << message << std::endl;
}

int fail(int code, const std::string& message) {
    std::cerr << "[editor-probe] FAIL " << code << ": " << message << std::endl;
    return code;
}

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

bool saveWindowClientPng(HWND hwnd, const std::wstring& path) {
    if(!hwnd || path.empty()) return false;
    RECT rc{};
    if(!GetClientRect(hwnd,&rc)) return false;
    const int width=rc.right-rc.left;
    const int height=rc.bottom-rc.top;
    if(width<=0 || height<=0) return false;

    HDC src=GetDC(hwnd);
    if(!src) return false;
    HDC mem=CreateCompatibleDC(src);
    HBITMAP bitmap=mem ? CreateCompatibleBitmap(src,width,height) : nullptr;
    if(!mem || !bitmap) {
        if(bitmap) DeleteObject(bitmap);
        if(mem) DeleteDC(mem);
        ReleaseDC(hwnd,src);
        return false;
    }
    HGDIOBJ old=SelectObject(mem,bitmap);
    BOOL rendered=PrintWindow(hwnd,mem,PW_CLIENTONLY);
    if(!rendered)
        rendered=BitBlt(mem,0,0,width,height,src,0,0,SRCCOPY|CAPTUREBLT);

    BITMAPINFO info{};
    info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=width;
    info.bmiHeader.biHeight=-height;
    info.bmiHeader.biPlanes=1;
    info.bmiHeader.biBitCount=32;
    info.bmiHeader.biCompression=BI_RGB;
    std::vector<BYTE> pixels(static_cast<size_t>(width)*static_cast<size_t>(height)*4u);
    const int rows=rendered ? GetDIBits(mem,bitmap,0,static_cast<UINT>(height),pixels.data(),&info,DIB_RGB_COLORS) : 0;

    SelectObject(mem,old);
    DeleteObject(bitmap);
    DeleteDC(mem);
    ReleaseDC(hwnd,src);
    if(rows!=height) return false;

    IWICImagingFactory* factory=nullptr;
    IWICStream* stream=nullptr;
    IWICBitmapEncoder* encoder=nullptr;
    IWICBitmapFrameEncode* frame=nullptr;
    IPropertyBag2* props=nullptr;
    HRESULT hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr)) hr=factory->CreateStream(&stream);
    if(SUCCEEDED(hr)) hr=stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE);
    if(SUCCEEDED(hr)) hr=factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder);
    if(SUCCEEDED(hr)) hr=encoder->Initialize(stream,WICBitmapEncoderNoCache);
    if(SUCCEEDED(hr)) hr=encoder->CreateNewFrame(&frame,&props);
    if(SUCCEEDED(hr)) hr=frame->Initialize(props);
    if(SUCCEEDED(hr)) hr=frame->SetSize(static_cast<UINT>(width),static_cast<UINT>(height));
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;
    if(SUCCEEDED(hr)) hr=frame->SetPixelFormat(&format);
    if(SUCCEEDED(hr) && format!=GUID_WICPixelFormat32bppBGRA) hr=E_FAIL;
    if(SUCCEEDED(hr)) hr=frame->WritePixels(static_cast<UINT>(height),static_cast<UINT>(width*4),static_cast<UINT>(pixels.size()),pixels.data());
    if(SUCCEEDED(hr)) hr=frame->Commit();
    if(SUCCEEDED(hr)) hr=encoder->Commit();

    if(props) props->Release();
    if(frame) frame->Release();
    if(encoder) encoder->Release();
    if(stream) stream->Release();
    if(factory) factory->Release();
    return SUCCEEDED(hr);
}

std::wstring screenshotPathFromEnvironment() {
    wchar_t buffer[32768]{};
    const DWORD length=GetEnvironmentVariableW(L"MECHAMORPH_EDITOR_SCREENSHOT",buffer,static_cast<DWORD>(std::size(buffer)));
    if(length==0 || length>=std::size(buffer)) return {};
    return std::wstring(buffer,length);
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

bool ctrlClickAt(HWND vstguiChild,int x,int y) {
    if(!vstguiChild) return false;
    const LPARAM p=MAKELPARAM(x,y);
    SendMessageW(vstguiChild,WM_LBUTTONDOWN,MK_LBUTTON|MK_CONTROL,p);
    SendMessageW(vstguiChild,WM_LBUTTONUP,MK_CONTROL,p);
    pump(50);
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
        trace("controller close: begin");
        if(connected && componentCP && controllerCP) {
            trace("controller close: disconnect component -> controller");
            const auto a=componentCP->disconnect(controllerCP);
            trace("controller close: component disconnect result "+std::to_string(a));
            trace("controller close: disconnect controller -> component");
            const auto b=controllerCP->disconnect(componentCP);
            trace("controller close: controller disconnect result "+std::to_string(b));
        }
        if(componentCP) {
            trace("controller close: release componentCP");
            componentCP->release();
            componentCP=nullptr;
        }
        if(controllerCP) {
            trace("controller close: release controllerCP");
            controllerCP->release();
            controllerCP=nullptr;
        }
        if(controller) {
            if(initialized) {
                trace("controller close: terminate controller");
                const auto tr=controller->terminate();
                trace("controller close: controller terminate result "+std::to_string(tr));
            }
            trace("controller close: release controller");
            controller->release();
            controller=nullptr;
        }
        initialized=false;
        connected=false;
        trace("controller close: complete");
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
        return fail(1,"module load failed: "+error);
    }
    if(!ensureWindowClass()) return fail(2,"window class registration failed");

    HostApplication hostApplication;
    FUnknown* host=&hostApplication;
    auto factory=module->getFactory();
    factory.setHostContext(host);
    trace("module loaded; factory ready");

    int editors=0;
    for(const auto& info:factory.classInfos()) {
        trace("factory class: "+info.name());
        // Probe any factory class that actually implements IComponent.
        // This avoids coupling the host-side QA probe to SDK-version-specific
        // factory category constants.
        auto component=factory.createInstance<IComponent>(info.ID());
        if(!component) continue;
        if(component->initialize(host)!=kResultOk) return fail(4,"component initialize failed");
        trace("component initialized");

        ControllerHolder holder;
        if(!acquireController(component.get(),factory,host,holder)) {
            trace("no controller for class; skipping");
            component->terminate();
            continue;
        }
        trace("controller acquired");

        for(int cycle=0;cycle<kCycles;++cycle) {
            trace("cycle "+std::to_string(cycle+1)+"/"+std::to_string(kCycles)+" createView");
            IPlugView* view=holder.controller->createView(ViewType::kEditor);
            if(!view) return fail(5,"createView returned null");
            ++editors;

            int baseW=0,baseH=0;
            if(!validRect(view,baseW,baseH)) return fail(6,"base getSize failed");
            if(baseW!=1440 || baseH!=900) {
                std::cerr<<"unexpected base editor size "<<baseW<<"x"<<baseH<<"\n";
                return fail(7,"unexpected base editor size");
            }

            HWND hwnd=CreateWindowExW(WS_EX_TOOLWINDOW,kWindowClassName,L"Mechamorph QA",
                WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,baseW,baseH,
                nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
            if(!hwnd) return fail(8,"host HWND creation failed");

            auto* frame=new HostFrame(hwnd);
            trace("cycle "+std::to_string(cycle+1)+" setFrame");
            if(view->setFrame(frame)!=kResultTrue) return fail(9,"setFrame failed");
            if(view->isPlatformTypeSupported(kPlatformTypeHWND)!=kResultTrue) return fail(10,"HWND platform not supported");
            trace("cycle "+std::to_string(cycle+1)+" attached");
            if(view->attached(reinterpret_cast<void*>(hwnd),kPlatformTypeHWND)!=kResultTrue) return fail(11,"attached failed");
            ShowWindow(hwnd,SW_SHOWNA);
            UpdateWindow(hwnd);
            pump(80);

            if(cycle==0) {
                const auto screenshotPath=screenshotPathFromEnvironment();
                if(!screenshotPath.empty()) {
                    HWND vstguiCapture=findVstguiChild(hwnd);
                    if(!vstguiCapture) return fail(30,"VSTGUI child HWND not found for screenshot");
                    trace("cycle 1 capturing rendered editor screenshot");
                    if(!saveWindowClientPng(vstguiCapture,screenshotPath))
                        return fail(31,"rendered editor screenshot capture failed");
                    trace("cycle 1 screenshot captured");
                }
            }

            IPlugViewContentScaleSupport* scale{};
            if(view->queryInterface(IPlugViewContentScaleSupport::iid,reinterpret_cast<void**>(&scale))!=kResultTrue || !scale) {
                return fail(12,"editor does not expose IPlugViewContentScaleSupport");
            }
            for(float factor:{1.0f,1.25f,1.5f,2.0f}) {
                trace("cycle "+std::to_string(cycle+1)+" content-scale "+std::to_string(factor));
                if(scale->setContentScaleFactor(factor)!=kResultTrue) {
                    std::cerr<<"content scale rejected: "<<factor<<"\n";
                    return fail(13,"content scale rejected");
                }
                pump(50);
                int w=0,h=0;
                const int expectedW=static_cast<int>(std::lround(baseW*factor));
                const int expectedH=static_cast<int>(std::lround(baseH*factor));
                if(!validRect(view,w,h) || w!=expectedW || h!=expectedH) {
                    std::cerr<<"content scale "<<factor<<" produced "<<w<<"x"<<h
                             <<"; expected "<<expectedW<<"x"<<expectedH<<"\n";
                    return fail(14,"content scale size mismatch");
                }
            }
            trace("cycle "+std::to_string(cycle+1)+" content-scale reset 1.0");
            if(scale->setContentScaleFactor(1.0f)!=kResultTrue) return fail(19,"content scale reset rejected");
            pump(40);
            {
                int w=0,h=0;
                if(!validRect(view,w,h) || w!=baseW || h!=baseH) {
                    std::cerr<<"content scale reset did not restore base size: "<<w<<"x"<<h<<"\n";
                    return fail(23,"content scale reset size mismatch");
                }
            }
            scale->release();

            // Exercise the actual in-GUI UI-scale button, not merely host DPI/content scale.
            trace("cycle "+std::to_string(cycle+1)+" locating VSTGUI child");
            HWND vstguiChild=findVstguiChild(hwnd);
            if(!vstguiChild) return fail(20,"VSTGUI child HWND not found");
            const double zoomBefore[]={1.0,1.25,1.5,2.0};
            const double zoomAfter []={1.25,1.5,2.0,1.0};
            for(int zi=0;zi<4;++zi) {
                trace("cycle "+std::to_string(cycle+1)+" UI zoom click "
                      +std::to_string(zoomBefore[zi])+" -> "+std::to_string(zoomAfter[zi]));
                if(!clickUiScale(vstguiChild,zoomBefore[zi])) return fail(21,"UI scale click failed");
                int w=0,h=0;
                const int expectedW=static_cast<int>(std::lround(baseW*zoomAfter[zi]));
                const int expectedH=static_cast<int>(std::lround(baseH*zoomAfter[zi]));
                if(!validRect(view,w,h) || w!=expectedW || h!=expectedH) {
                    std::cerr<<"UI zoom "<<zoomAfter[zi]<<" produced "<<w<<"x"<<h
                             <<"; expected "<<expectedW<<"x"<<expectedH<<"\n";
                    return fail(22,"UI zoom size mismatch");
                }
            }

            trace("cycle "+std::to_string(cycle+1)+" focus on/off");
            // Ctrl+click default reset matrix for every user-facing control.
            {
                struct DefaultCase { ParamID id; int x; int y; double def; const char* name; };
                const DefaultCase cases[] = {
                    {2000,720,190,0.00,"Machine"},
                    {2001,143,610,0.32,"Speed"},
                    {2002,385,610,0.20,"Load"},
                    {2003,627,610,0.48,"Action"},
                    {2004,869,610,0.18,"Wear"},
                    {2005,1154,610,0.35,"Scale"},
                    {2008,1368,470,0.28,"Body"},
                    {2006,1368,626,0.18,"Space"},
                    {2007,1368,782,0.38,"Output"}
                };
                trace("cycle "+std::to_string(cycle+1)+" Ctrl+click default reset matrix");
                for(const auto& dc:cases){
                    const double testValue = dc.def < 0.75 ? 0.91 : 0.10;
                    if(holder.controller->setParamNormalized(dc.id,testValue)!=kResultTrue)
                        return fail(27,std::string("setParamNormalized failed for ")+dc.name);
                    pump(20);
                    if(!ctrlClickAt(vstguiChild,dc.x,dc.y))
                        return fail(28,std::string("Ctrl+click dispatch failed for ")+dc.name);
                    const double actual=holder.controller->getParamNormalized(dc.id);
                    if(std::fabs(actual-dc.def)>1.0e-6){
                        std::cerr<<"[editor-probe] "<<dc.name<<" Ctrl+click expected "<<dc.def
                                 <<" got "<<actual<<std::endl;
                        return fail(29,std::string("Ctrl+click default mismatch for ")+dc.name);
                    }
                }
            }

            view->onFocus(true);
            pump(10);
            view->onFocus(false);
            trace("cycle "+std::to_string(cycle+1)+" removed");
            if(view->removed()!=kResultTrue) return fail(15,"removed failed");
            trace("cycle "+std::to_string(cycle+1)+" clear frame");
            if(view->setFrame(nullptr)!=kResultTrue) return fail(16,"clear frame failed");
            frame->release();
            DestroyWindow(hwnd);
            pump(20);
            view->release();
            trace("cycle "+std::to_string(cycle+1)+" complete");
        }

        trace("closing controller");
        holder.close();
        if(component->terminate()!=kResultOk) return fail(17,"component terminate failed");
        trace("component terminated");
    }

    if(editors==0) return fail(18,"no editor instances exercised");
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
