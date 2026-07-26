cmake_minimum_required(VERSION 3.15)
project(webp_gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The installed FLTKConfig.cmake is broken (references uninstalled demo
# targets), so locate the static lib and headers directly.
find_path(FLTK_INCLUDE_DIR FL/Fl.H PATHS "C:/Program Files (x86)/FLTK/include")
find_library(FLTK_LIBRARY fltk PATHS "C:/Program Files (x86)/FLTK/lib")
if(NOT FLTK_INCLUDE_DIR OR NOT FLTK_LIBRARY)
    message(FATAL_ERROR "FLTK not found (FLTK_INCLUDE_DIR / FLTK_LIBRARY)")
endif()

add_executable(webp_gui WIN32
    src/main.cpp
    src/scanner.cpp
    src/cwebp_runner.cpp
    src/ui_main_window.cpp)
target_include_directories(webp_gui PRIVATE ${FLTK_INCLUDE_DIR})
target_link_libraries(webp_gui PRIVATE ${FLTK_LIBRARY}
    gdi32 gdiplus comctl32 comdlg32 ole32 uuid ws2_32 winspool shell32 advapi32 imm32 shlwapi)
