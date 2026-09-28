#include "MechamorphGui.h"
#include "branding_master.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cfont.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <utility>
#include <string_view>
#include <vector>

namespace MechamorphMachine {
namespace {
constexpr double kPi=3.14159265358979323846;
constexpr VSTGUI::CColor kPanelTop{67,68,68,255};
constexpr VSTGUI::CColor kPanelBottom{20,22,23,255};
constexpr VSTGUI::CColor kIvory{224,215,196,255};
constexpr VSTGUI::CColor kLogoSilver{217,217,217,255};
constexpr VSTGUI::CColor kLogoRed{215,25,32,255};

void fillRound(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r,double radius,
               VSTGUI::CColor a,VSTGUI::CColor b) {
    auto* p=c->createRoundRectGraphicsPath(r,radius);
    if(!p)return;
    auto* g=VSTGUI::CGradient::create(0.0,1.0,a,b);
    if(g){ c->fillLinearGradient(p,*g,{r.left,r.top},{r.left,r.bottom}); g->forget(); }
    p->forget();
}
void strokeRound(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r,double radius,VSTGUI::CColor col,double w=1.0){
    auto* p=c->createRoundRectGraphicsPath(r,radius);
    if(!p)return;
    c->setFrameColor(col); c->setLineWidth(w); c->drawGraphicsPath(p,VSTGUI::CDrawContext::kPathStroked); p->forget();
}
void radial(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r,VSTGUI::CColor a,VSTGUI::CColor b){
    auto* p=c->createGraphicsPath(); if(!p)return; p->addEllipse(r);
    auto* g=VSTGUI::CGradient::create(0.0,1.0,a,b);
    if(g){ c->fillRadialGradient(p,*g,r.getCenter(),std::max(r.getWidth(),r.getHeight())*.55,{-r.getWidth()*.12,-r.getHeight()*.15}); g->forget(); }
    p->forget();
}

struct LogoSubpath { std::vector<VSTGUI::CPoint> points; };
struct LogoPath { std::vector<LogoSubpath> subpaths; bool red{false}; };

std::vector<LogoPath> parseMasterLogo(){
    std::vector<LogoPath> result;
    result.reserve(Branding::kMasterPathCount);
    for(const auto& source:Branding::kMasterPaths){
        const std::string_view d{source.d};
        LogoPath path; path.red=source.red;
        const char* p=d.data(); const char* end=d.data()+d.size();
        char command=0; LogoSubpath* current=nullptr;
        while(p<end){
            while(p<end&&(std::isspace(static_cast<unsigned char>(*p))||*p==',')) ++p;
            if(p>=end) break;
            if(std::isalpha(static_cast<unsigned char>(*p))){
                command=*p++;
                if(command=='Z'||command=='z'){ command=0; current=nullptr; continue; }
            }
            if(command!='M'&&command!='m'&&command!='L'&&command!='l'){ ++p; continue; }
            char* next=nullptr;
            const double x=std::strtod(p,&next); if(next==p||next>end) break; p=next;
            while(p<end&&(std::isspace(static_cast<unsigned char>(*p))||*p==',')) ++p;
            const double y=std::strtod(p,&next); if(next==p||next>end) break; p=next;
            if(command=='M'||command=='m'){
                path.subpaths.emplace_back(); current=&path.subpaths.back();
                current->points.emplace_back(x,y); command=(command=='M')?'L':'l';
            } else if(current) current->points.emplace_back(x,y);
        }
        if(!path.subpaths.empty()) result.emplace_back(std::move(path));
    }
    return result;
}
}

GuiFaceplate::GuiFaceplate(const VSTGUI::CRect& s,VSTGUI::CBitmap* background):CView(s){
    setMouseEnabled(false);
    setBackground(background);
}
GuiFaceplate::GuiFaceplate(const GuiFaceplate& o):CView(o){}
void GuiFaceplate::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize();
    if(auto* bg=getDrawBackground()){
        bg->draw(c,r,{0,0});
    } else {
        c->setFillColor({8,9,10,255});
        c->drawRect(r,VSTGUI::kDrawFilled);
    }
    setDirty(false);
}

GuiLogo::GuiLogo(const VSTGUI::CRect& s):CView(s){setMouseEnabled(false);}
GuiLogo::GuiLogo(const GuiLogo& o):CView(o){}
void GuiLogo::draw(VSTGUI::CDrawContext* c){
    static const auto logo=parseMasterLogo();
    const auto r=getViewSize();
    constexpr double masterWidth=1774.0, masterHeight=887.0;
    const double scale=std::min(r.getWidth()/masterWidth,r.getHeight()/masterHeight);
    const double x0=r.left+(r.getWidth()-masterWidth*scale)*0.5;
    const double y0=r.top +(r.getHeight()-masterHeight*scale)*0.5;
    c->setDrawMode(VSTGUI::kAntiAliasing);
    for(const auto& sourcePath:logo){
        auto* path=c->createGraphicsPath(); if(!path) continue;
        for(const auto& subpath:sourcePath.subpaths){
            if(subpath.points.empty()) continue;
            const auto toView=[&](const VSTGUI::CPoint& p){ return VSTGUI::CPoint{x0+p.x*scale,y0+p.y*scale}; };
            path->beginSubpath(toView(subpath.points.front()));
            for(std::size_t i=1;i<subpath.points.size();++i) path->addLine(toView(subpath.points[i]));
            path->closeSubpath();
        }
        c->setFillColor(sourcePath.red?kLogoRed:kLogoSilver);
        c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathFilledEvenOdd);
        path->forget();
    }
    setDirty(false);
}

GuiKnob::GuiKnob(const VSTGUI::CRect& s,VSTGUI::IControlListener* l,int32_t tag,Style st,
                 VSTGUI::CBitmap* background,float defaultValue)
:CKnobBase(s,l,tag,background),style_(st){
    setStartAngle((float)(135.0/180.0*kPi)); setRangeAngle((float)(270.0/180.0*kPi));
    setDefaultValue(defaultValue);
    setTransparency(true); setWantsFocus(true);
}
GuiKnob::GuiKnob(const GuiKnob& o):CKnobBase(o),style_(o.style_){}
void GuiKnob::valueChanged(){
    if(style_==Style::Machine){
        const float n=std::clamp(getValueNormalized(),0.0f,1.0f);
        const float snapped=std::round(n*5.0f)/5.0f;
        CKnobBase::setValueNormalized(snapped);
    }
    CKnobBase::valueChanged();
}
void GuiKnob::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize();
    const auto center=r.getCenter();
    const double v=std::clamp((double)getValueNormalized(),0.0,1.0);
    const double imageSize=
        style_==Style::Machine ? 250.0 :
        style_==Style::Scale ? 220.0 :
        style_==Style::Utility ? 104.0 : 125.0;
    const double imageRadius=imageSize*0.5;
    const VSTGUI::CRect imageRect{
        center.x-imageRadius,center.y-imageRadius,
        center.x+imageRadius,center.y+imageRadius};

    c->setDrawMode(VSTGUI::kAntiAliasing);

    // Amber under-light: deliberately offset downwards so the metal appears
    // illuminated from underneath rather than outlined with a neon border.
    struct GlowRing { double extra; double yOffset; double width; uint8_t alpha; };
    static constexpr GlowRing glow[]={
        {9.0,4.0,10.0,26},
        {6.0,3.0,6.0,62},
        {3.0,2.0,2.2,190}
    };
    for(const auto& g:glow){
        VSTGUI::CRect gr{
            center.x-imageRadius-g.extra,
            center.y-imageRadius-g.extra+g.yOffset,
            center.x+imageRadius+g.extra,
            center.y+imageRadius+g.extra+g.yOffset};
        c->setFrameColor({255,142,20,g.alpha});
        c->setLineWidth(g.width);
        c->drawEllipse(gr,VSTGUI::kDrawStroked);
    }

    bool bakedIndicator=false;
    if(auto* bitmap=getDrawBackground()){
        if(style_==Style::Main){
            if(auto* mfb=dynamic_cast<VSTGUI::CMultiFrameBitmap*>(bitmap)){
                const auto frame=mfb->normalizedValueToFrameIndex((float)v);
                const VSTGUI::CPoint pos{center.x-imageRadius,center.y-imageRadius};
                mfb->drawFrame(c,frame,pos);
                bakedIndicator=true;
            } else {
                bitmap->draw(c,imageRect,{0,0});
            }
        } else {
            bitmap->draw(c,imageRect,{0,0});
        }
    } else {
        radial(c,imageRect,{70,72,73,255},{18,20,21,255});
    }

    // MAIN uses the indicator baked into the JKnobMan filmstrip.
    // Other styles keep the separate VSTGUI indicator.
    if(!bakedIndicator){
        const double angle=(135.0+270.0*v)*kPi/180.0;
        const double p1=imageRadius*0.50;
        const double p2=imageRadius*0.82;
        const VSTGUI::CPoint a{
            center.x+std::cos(angle)*p1,
            center.y+std::sin(angle)*p1};
        const VSTGUI::CPoint b{
            center.x+std::cos(angle)*p2,
            center.y+std::sin(angle)*p2};

        c->setFrameColor({0,0,0,190});
        c->setLineWidth(style_==Style::Utility?4.0:5.0);
        c->drawLine(a,b);
        c->setFrameColor({255,145,18,255});
        c->setLineWidth(style_==Style::Utility?2.6:3.4);
        c->drawLine(a,b);
        c->setFrameColor({255,226,132,255});
        c->setLineWidth(style_==Style::Utility?1.0:1.25);
        c->drawLine(a,b);
    }

    setDirty(false);
}

GuiLabel::GuiLabel(const VSTGUI::CRect& s,std::string text,double fs,bool muted)
:CView(s),text_(std::move(text)),fontSize_(fs),muted_(muted){setMouseEnabled(false);}
GuiLabel::GuiLabel(const GuiLabel& o):CView(o),text_(o.text_),fontSize_(o.fontSize_),muted_(o.muted_){}
void GuiLabel::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize(); c->setDrawMode(VSTGUI::kAntiAliasing);
    c->setFont(VSTGUI::kNormalFont,fontSize_,VSTGUI::kBoldFace);
    VSTGUI::CRect sh=r; sh.offset(0,1); c->setFontColor({0,0,0,170});
    c->drawString(VSTGUI::UTF8String(text_.c_str()),sh,VSTGUI::kCenterText);
    c->setFontColor(muted_?VSTGUI::CColor{154,153,147,255}:kIvory);
    c->drawString(VSTGUI::UTF8String(text_.c_str()),r,VSTGUI::kCenterText);
    setDirty(false);
}

GuiStatusLamp::GuiStatusLamp(const VSTGUI::CRect& s,VSTGUI::IControlListener* l,int32_t tag,Kind k)
:CControl(s,l,tag,nullptr),kind_(k){setMouseEnabled(false); setTransparency(true);}
GuiStatusLamp::GuiStatusLamp(const GuiStatusLamp& o):CControl(o),kind_(o.kind_){}
void GuiStatusLamp::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize();
    const float on=std::clamp(getValueNormalized(),0.0f,1.0f);
    VSTGUI::CColor dim,bright;
    switch(kind_){
        case Kind::Pressure: dim={64,45,14,255}; bright={255,171,48,255}; break;
        case Kind::Friction: dim={54,52,47,255}; bright={232,225,207,255}; break;
        default: dim={58,19,17,255}; bright={244,55,43,255}; break;
    }
    c->setDrawMode(VSTGUI::kAntiAliasing);
    radial(c,r,{95,96,94,255},{20,22,23,255});
    auto lens=r; lens.inset(7,7);
    const auto lerp=[&](VSTGUI::CColor a,VSTGUI::CColor b){
        auto ch=[&](uint8_t x,uint8_t y){return static_cast<uint8_t>(std::lround(x+(y-x)*on));};
        return VSTGUI::CColor{ch(a.red,b.red),ch(a.green,b.green),ch(a.blue,b.blue),255};
    };
    const auto active=lerp(dim,bright);
    radial(c,lens,lerp({85,76,55,255},bright),active);
    c->setFrameColor({5,6,7,255}); c->setLineWidth(1.0); c->drawEllipse(r,VSTGUI::kDrawStroked);
    setDirty(false);
}

GuiScale::GuiScale(const VSTGUI::CRect& s,VSTGUI::VST3Editor* e):CView(s),editor_(e){
    setMouseEnabled(true); setWantsFocus(true);
}
GuiScale::GuiScale(const GuiScale& o):CView(o),editor_(o.editor_){}
void GuiScale::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize(); const int p=(int)std::lround((editor_?editor_->getZoomFactor():1.0)*100.0);
    char txt[20]{}; std::snprintf(txt,sizeof(txt),"UI %d%%",p);
    fillRound(c,r,5,{58,60,60,255},{24,26,27,255}); strokeRound(c,r,5,{102,103,100,180},1);
    c->setFont(VSTGUI::kNormalFont,10,VSTGUI::kBoldFace); c->setFontColor(kIvory);
    c->drawString(VSTGUI::UTF8String(txt),r,VSTGUI::kCenterText); setDirty(false);
}
VSTGUI::CMouseEventResult GuiScale::onMouseDown(VSTGUI::CPoint& where,const VSTGUI::CButtonState& buttons){
    if(!editor_||!buttons.isLeftButton()||!getViewSize().pointInside(where))return VSTGUI::kMouseEventNotHandled;
    constexpr double f[]={1.0,1.25,1.5,2.0};
    const double z=editor_->getZoomFactor(); size_t best=0; double d=std::abs(z-f[0]);
    for(size_t i=1;i<4;++i){const double q=std::abs(z-f[i]); if(q<d){d=q;best=i;}}
    editor_->setZoomFactor(f[(best+1)%4]); invalid();
    return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

} // namespace MechamorphMachine
