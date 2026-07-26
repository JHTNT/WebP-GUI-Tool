#pragma once
#include <string>

enum class JobStatus { Pending, Running, Done, Failed, Cancelled };

struct FileItem {
    std::string path_utf8;
    bool checked = true;
    JobStatus status = JobStatus::Pending;
    std::string error_text;
};

struct CwebpOptions {
    int  quality = 90;
    bool lossless = false;
    int  near_lossless = -1;  // -1 = unused
    int  z = -1;              // -1 = unused; implies lossless, skips -q/-m
    int  method = 4;
    std::string preset;       // empty = unused
    bool resize = false;
    int  rw = 0, rh = 0;      // 0 = keep aspect ratio
    bool mt = true;
    std::string metadata = "none";
    int  alpha_q = -1;        // -1 = unused
    bool sharp_yuv = false;
    std::string extra;        // whitespace-separated extra args
};

struct OutputSettings {
    bool custom_dir = false;
    std::string dir_utf8;
};
