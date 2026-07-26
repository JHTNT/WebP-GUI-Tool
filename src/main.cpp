#include <FL/Fl.H>

#include "ui_main_window.h"

int main() {
    Fl::lock();   // enable FLTK multithread support for Fl::awake
    MainWindow win;
    win.show();
    return Fl::run();
}
