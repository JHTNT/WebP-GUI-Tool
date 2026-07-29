#include "theme.h"

#include <FL/Fl.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Tooltip.H>
#include <FL/fl_draw.H>

const Fl_Color kAccent = fl_rgb_color(37, 99, 235);

namespace {
const Fl_Color kBorder = fl_rgb_color(203, 208, 216);

void flat_box(int x, int y, int w, int h, Fl_Color fill, Fl_Color line) {
    fl_color(Fl::box_color(fill));
    fl_rectf(x, y, w, h);
    fl_color(Fl::box_color(line));
    fl_rect(x, y, w, h);
}
void up_box(int x, int y, int w, int h, Fl_Color c) {
    flat_box(x, y, w, h, c, fl_color_average(c, FL_BLACK, 0.82f));
}
void down_box(int x, int y, int w, int h, Fl_Color c) {
    flat_box(x, y, w, h, c, kBorder);
}
void thin_up(int x, int y, int w, int h, Fl_Color c) {
    flat_box(x, y, w, h, c, fl_color_average(c, FL_BLACK, 0.82f));
}
void thin_down(int x, int y, int w, int h, Fl_Color c) {
    flat_box(x, y, w, h, c, kBorder);
}
void up_frame(int x, int y, int w, int h, Fl_Color c) {
    fl_color(Fl::box_color(fl_color_average(c, FL_BLACK, 0.82f)));
    fl_rect(x, y, w, h);
}
void down_frame(int x, int y, int w, int h, Fl_Color) {
    fl_color(Fl::box_color(kBorder));
    fl_rect(x, y, w, h);
}
void round_down(int x, int y, int w, int h, Fl_Color c) {
    fl_color(Fl::box_color(c));
    fl_pie(x, y, w - 1, h - 1, 0, 360);
    fl_color(Fl::box_color(kBorder));
    fl_arc(x, y, w - 1, h - 1, 0, 360);
}
}  // namespace

void apply_modern_theme() {
    Fl::set_boxtype(FL_UP_BOX, up_box, 3, 3, 6, 6);
    Fl::set_boxtype(FL_DOWN_BOX, down_box, 3, 3, 6, 6);
    Fl::set_boxtype(FL_THIN_UP_BOX, thin_up, 2, 2, 4, 4);
    Fl::set_boxtype(FL_THIN_DOWN_BOX, thin_down, 2, 2, 4, 4);
    Fl::set_boxtype(FL_UP_FRAME, up_frame, 3, 3, 6, 6);
    Fl::set_boxtype(FL_DOWN_FRAME, down_frame, 3, 3, 6, 6);
    Fl::set_boxtype(FL_ROUND_DOWN_BOX, round_down, 2, 2, 4, 4);

    Fl::set_font(FL_HELVETICA, " Microsoft JhengHei UI");
    Fl::set_font(FL_HELVETICA_BOLD, "BMicrosoft JhengHei UI");
    FL_NORMAL_SIZE = 13;
    Fl::background(243, 244, 246);    // window
    Fl::background2(255, 255, 255);   // inputs / tree
    Fl::foreground(31, 41, 55);
    Fl::set_color(FL_SELECTION_COLOR, 37, 99, 235);
    Fl::visible_focus(0);

    Fl_Tooltip::color(fl_rgb_color(31, 41, 55));
    Fl_Tooltip::textcolor(FL_WHITE);
    Fl_Tooltip::size(12);
}

void style_modern(Fl_Group* root) {
    for (int i = 0; i < root->children(); ++i) {
        Fl_Widget* w = root->child(i);
        if (auto* g = dynamic_cast<Fl_Group*>(w)) style_modern(g);
        if (dynamic_cast<Fl_Light_Button*>(w))    // check & radio buttons
            w->selection_color(kAccent);
        else if (auto* c = dynamic_cast<Fl_Choice*>(w))
            c->color(FL_BACKGROUND2_COLOR);
    }
}

int HoverButton::handle(int e) {
    if (e == FL_ENTER || e == FL_LEAVE) {
        hover_ = e == FL_ENTER;
        redraw();
    }
    return Fl_Button::handle(e);
}

void HoverButton::draw() {
    Fl_Color saved = color();
    if (active_r()) {
        if (value()) color(fl_color_average(saved, FL_BLACK, 0.86f));
        else if (hover_) color(fl_color_average(saved, FL_BLACK, 0.93f));
    }
    Fl_Button::draw();
    color(saved);
}
