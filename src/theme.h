#pragma once
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>

extern const Fl_Color kAccent;

void apply_modern_theme();           // call before creating any widget
void style_modern(Fl_Group* root);   // call after a window's widgets exist

// Fl_Button with hover/press shading
class HoverButton : public Fl_Button {
public:
    using Fl_Button::Fl_Button;
    int handle(int e) override;
    void draw() override;

private:
    bool hover_ = false;
};
