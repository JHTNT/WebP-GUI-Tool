#include <FL/Fl.H>

#include "theme.h"
#include "ui_main_window.h"

int main() {
    Fl::lock();   // enable FLTK multithread support for Fl::awake
    apply_modern_theme();
    MainWindow win;
    win.show();
    return Fl::run();
}
