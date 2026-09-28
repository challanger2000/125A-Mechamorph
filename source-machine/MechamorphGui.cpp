#include "MechamorphGui.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/lib/cfont.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace MechamorphMachine {
namespace {
constexpr double kPi=3.14159265358979323846;
constexpr VSTGUI::CColor kPanelTop{67,68,68,255};
constexpr VSTGUI::CColor kPanelBottom{20,22,23,255};
constexpr VSTGUI::CColor kIvory{224,215,196,255};

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
}

GuiFaceplate::GuiFaceplate(const VSTGUI::CRect& s):CView(s){setMouseEnabled(false);}
GuiFaceplate::GuiFaceplate(const GuiFaceplate& o):CView(o){}
void GuiFaceplate::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize();
    c->setDrawMode(VSTGUI::kAntiAliasing);
    c->setFillColor({8,9,10,255}); c->drawRect(r,VSTGUI::kDrawFilled);
    auto chassis=r; chassis.inset(8,8);
    fillRound(c,chassis,12,kPanelTop,kPanelBottom);
    strokeRound(c,chassis,12,{4,5,6,255},2.0);
    auto inner=chassis; inner.inset(5,5); strokeRound(c,inner,9,{188,186,178,45},1.0);

    // Upper structure: branding bay, MACHINE bridge and status bay.
    fillRound(c,{28,28,520,344},8,{82,83,82,255},{30,31,31,255});
    strokeRound(c,{28,28,520,344},8,{6,7,8,255},2);
    fillRound(c,{548,20,892,366},16,{41,43,44,255},{17,19,20,255});
    strokeRound(c,{548,20,892,366},16,{6,7,8,255},2);
    fillRound(c,{920,28,1412,344},8,{53,54,54,255},{19,21,22,255});
    strokeRound(c,{920,28,1412,344},8,{6,7,8,255},2);

    // Lower module bank. These dimensions are the permanent layout contract.
    const double xs[]={28,270,512,754,996};
    const double ws[]={230,230,230,230,316};
    for(int i=0;i<5;++i){
        const VSTGUI::CRect bay{xs[i],386,xs[i]+ws[i],872};
        fillRound(c,bay,8,{39,41,42,255},{20,22,23,255});
        strokeRound(c,bay,8,{6,7,8,255},1.6);
    }
    // Utility column.
    for(const auto& y : {398.0,554.0,710.0}){
        VSTGUI::CRect bay{1324,y,1412,y+146};
        fillRound(c,bay,7,{40,42,43,255},{18,20,21,255});
        strokeRound(c,bay,7,{6,7,8,255},1.4);
    }

    // Engraved label plates: text itself is a separate VSTGUI layer.
    const double labelX[]={56,298,540,782,1028};
    const double labelW[]={174,174,174,174,252};
    for(int i=0;i<5;++i){
        VSTGUI::CRect plate{labelX[i],806,labelX[i]+labelW[i],848};
        fillRound(c,plate,5,{78,77,73,255},{36,36,35,255});
        strokeRound(c,plate,5,{8,9,10,255},1);
    }
    // Status label plates.
    for(const auto& x : {1000.0,1140.0,1280.0}){
        VSTGUI::CRect plate{x,246,x+104,282};
        fillRound(c,plate,4,{70,69,66,255},{32,32,31,255});
        strokeRound(c,plate,4,{8,9,10,255},1);
    }

    // restrained brushed-metal texture
    c->setFrameColor({235,235,229,10}); c->setLineWidth(1.0);
    for(int y=18;y<884;y+=6)c->drawLine({14.0,(double)y},{1426.0,(double)y});
    setDirty(false);
}

GuiKnob::GuiKnob(const VSTGUI::CRect& s,VSTGUI::IControlListener* l,int32_t tag,Style st)
:CAnimKnob(s,l,tag,nullptr),style_(st){
    setStartAngle((float)(135.0/180.0*kPi)); setRangeAngle((float)(270.0/180.0*kPi));
    setTransparency(true); setWantsFocus(true);
}
GuiKnob::GuiKnob(const GuiKnob& o):CAnimKnob(o),style_(o.style_){}
void GuiKnob::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize(); const auto center=r.getCenter();
    const double v=std::clamp((double)getValueNormalized(),0.0,1.0);
    const double radius=std::min(r.getWidth(),r.getHeight())*.36;
    c->setDrawMode(VSTGUI::kAntiAliasing);

    // Scale/ticks remain visible even before final filmstrips are installed.
    const int ticks=style_==Style::Machine?6:(style_==Style::Utility?9:13);
    for(int i=0;i<ticks;++i){
        const double t=(ticks==1)?0.0:(double)i/(ticks-1);
        const double a=(135.0+270.0*t)*kPi/180.0;
        const double ro=radius+15,ri=radius+8;
        c->setFrameColor({200,195,181,(uint8_t)((i==0||i==ticks-1)?220:120)});
        c->setLineWidth((i==0||i==ticks-1)?1.5:1.0);
        c->drawLine({center.x+std::cos(a)*ri,center.y+std::sin(a)*ri},
                    {center.x+std::cos(a)*ro,center.y+std::sin(a)*ro});
    }

    const VSTGUI::CRect shadow{center.x-radius-5,center.y-radius,center.x+radius+5,center.y+radius+10};
    c->setFillColor({0,0,0,125}); c->drawEllipse(shadow,VSTGUI::kDrawFilled);
    const VSTGUI::CRect skirt{center.x-radius,center.y-radius,center.x+radius,center.y+radius};
    radial(c,skirt,{105,108,109,255},{24,26,27,255});
    c->setFrameColor({5,6,7,255}); c->setLineWidth(1.5); c->drawEllipse(skirt,VSTGUI::kDrawStroked);
    auto cap=skirt; cap.inset(radius*.18,radius*.18);
    radial(c,cap,{52,54,55,255},{13,14,15,255});

    const double a=(135.0+270.0*v)*kPi/180.0;
    const double p1=radius*.25,p2=radius*.78;
    c->setFrameColor(kIvory); c->setLineWidth(style_==Style::Utility?2.0:2.6);
    c->drawLine({center.x+std::cos(a)*p1,center.y+std::sin(a)*p1},
                {center.x+std::cos(a)*p2,center.y+std::sin(a)*p2});
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

GuiStatusLamp::GuiStatusLamp(const VSTGUI::CRect& s,Kind k):CView(s),kind_(k){setMouseEnabled(false);}
GuiStatusLamp::GuiStatusLamp(const GuiStatusLamp& o):CView(o),kind_(o.kind_){}
void GuiStatusLamp::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize();
    VSTGUI::CColor dim;
    switch(kind_){
        case Kind::Pressure: dim={64,45,14,255}; break;
        case Kind::Friction: dim={54,52,47,255}; break;
        default: dim={58,19,17,255}; break;
    }
    c->setDrawMode(VSTGUI::kAntiAliasing);
    radial(c,r,{95,96,94,255},{20,22,23,255});
    auto lens=r; lens.inset(7,7); radial(c,lens,{85,76,55,255},dim);
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
