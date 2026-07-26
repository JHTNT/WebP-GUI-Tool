#pragma once
#include "job_model.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Locate cwebp.exe via PATH; empty string if not found
std::string find_cwebp();

// Build cwebp argv (without the executable itself)
std::vector<std::string> build_args(const CwebpOptions& o,
                                    const std::string& in_utf8,
                                    const std::string& out_utf8);

// Compose full command line (UTF-16) with Windows quoting rules
std::wstring build_cmdline_w(const std::string& exe_utf8,
                             const std::vector<std::string>& args_utf8);

// Output path; appends _converted when it would overwrite the input
std::string make_output_path(const std::string& in_utf8, const OutputSettings& s);

class Converter {
public:
    struct Job {
        int index;            // index into the UI list
        std::string in_utf8;
    };
    // Sent to the UI thread via Fl::awake; receiver deletes it
    struct Msg {
        void* user = nullptr;
        int index = -1;               // -1 = summary message
        JobStatus status = JobStatus::Pending;
        std::string error;
        int processed = 0, ok = 0, failed = 0, total = 0;
        bool all_done = false;
    };

    ~Converter();

    void start(std::vector<Job> jobs, CwebpOptions opts, OutputSettings out,
               std::string cwebp_path_utf8, void (*awake_cb)(void*), void* user);
    void cancel();   // set flag and terminate the current child process
    void join();
    bool running() const { return running_; }

private:
    struct RunResult {
        int exit_code = -1;
        std::string output;   // merged stdout + stderr
    };
    RunResult run_one(const std::wstring& cmdline);
    void post(void (*cb)(void*), Msg* m);

    std::thread th_;
    std::atomic<bool> cancel_{false};
    std::atomic<bool> running_{false};
    std::mutex proc_mtx_;
    void* current_proc_ = nullptr;   // HANDLE; void* avoids windows.h in header
};
