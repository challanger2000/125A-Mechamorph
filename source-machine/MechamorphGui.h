#pragma once

#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/cview.h"
#include <string>

namespace VSTGUI { class VST3Editor; }

namespace MechamorphMachine {

class GuiFaceplate final : public VSTGUI::CView {
public:
    explicit GuiFaceplate(const VSTGUI::CRect& size);
    GuiFaceplate(const GuiFaceplate& other);
    VSTGUI::CBaseObject* newCopy() const override { return new GuiFaceplate(*this); }
    void draw(VSTGUI::CDrawContext* context) override;
};

class GuiLogo final : public VSTGUI::CView {
public:
    explicit GuiLogo(const VSTGUI::CRect& size);
    GuiLogo(const GuiLogo& other);
    VSTGUI::CBaseObject* newCopy() const override { return new GuiLogo(*this); }
    void draw(VSTGUI::CDrawContext* context) override;
};

class GuiKnob final : public VSTGUI::CAnimKnob {
public:
    enum class Style { Main, Scale, Utility, Machine };
    GuiKnob(const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag,
            Style style, VSTGUI::CBitmap* background, float defaultValue);
    GuiKnob(const GuiKnob& other);
    VSTGUI::CBaseObject* newCopy() const override { return new GuiKnob(*this); }
    void draw(VSTGUI::CDrawContext* context) override;
private:
    Style style_;
};

class GuiLabel final : public VSTGUI::CView {
public:
    GuiLabel(const VSTGUI::CRect& size, std::string text, double fontSize, bool muted=false);
    GuiLabel(const GuiLabel& other);
    VSTGUI::CBaseObject* newCopy() const override { return new GuiLabel(*this); }
    void draw(VSTGUI::CDrawContext* context) override;
private:
    std::string text_;
    double fontSize_;
    bool muted_;
};

class GuiStatusLamp final : public VSTGUI::CView {
public:
    enum class Kind { Pressure, Friction, Stall };
    GuiStatusLamp(const VSTGUI::CRect& size, Kind kind);
    GuiStatusLamp(const GuiStatusLamp& other);
    VSTGUI::CBaseObject* newCopy() const override { return new GuiStatusLamp(*this); }
    void draw(VSTGUI::CDrawContext* context) override;
private:
    Kind kind_;
};

class GuiScale final : public VSTGUI::CView {
public:
    GuiScale(const VSTGUI::CRect& size, VSTGUI::VST3Editor* editor);
    GuiScale(const GuiScale& other);
    VSTGUI::CBaseObject* newCopy() const override { return new GuiScale(*this); }
    void draw(VSTGUI::CDrawContext* context) override;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where, const VSTGUI::CButtonState& buttons) override;
private:
    VSTGUI::VST3Editor* editor_;
};

} // namespace MechamorphMachine
