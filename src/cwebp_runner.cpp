#include "cwebp_runner.h"
#include "scanner.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <FL/Fl.H>

#include <chrono>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

std::string find_cwebp() {
    wchar_t buf[MAX_PATH];
    DWORD n = SearchPathW(nullptr, L"cwebp.exe", nullptr, MAX_PATH, buf, nullptr);
    if (n == 0 || n >= MAX_PATH) return {};
    return wide_to_utf8(buf);
}

std::vector<std::string> build_args(const CwebpOptions& o,
                                    const std::string& in_utf8,
                                    const std::string& out_utf8) {
    std::vector<std::string> a;
    // -preset must come first (it overrides earlier options)
    if (!o.preset.empty()) { a.push_back("-preset"); a.push_back(o.preset); }
    if (o.z >= 0) {
        a.push_back("-z"); a.push_back(std::to_string(o.z));   // implies lossless
    } else {
        if (o.lossless) a.push_back("-lossless");
        a.push_back("-q"); a.push_back(std::to_string(o.quality));
        a.push_back("-m"); a.push_back(std::to_string(o.method));
    }
    if (o.near_lossless >= 0) { a.push_back("-near_lossless"); a.push_back(std::to_string(o.near_lossless)); }
    if (o.resize) {
        a.push_back("-resize");
        a.push_back(std::to_string(o.rw));
        a.push_back(std::to_string(o.rh));
    }
    if (o.mt) a.push_back("-mt");
    if (o.sharp_yuv) a.push_back("-sharp_yuv");
    if (o.alpha_q >= 0) { a.push_back("-alpha_q"); a.push_back(std::to_string(o.alpha_q)); }
    if (!o.metadata.empty() && o.metadata != "none") { a.push_back("-metadata"); a.push_back(o.metadata); }
    std::istringstream iss(o.extra);
    for (std::string tok; iss >> tok;) a.push_back(tok);
    a.push_back(in_utf8);
    a.push_back("-o");
    a.push_back(out_utf8);
    return a;
}

// Windows quoting: double backslashes before a quote and at the end
static void append_quoted(std::wstring& cl, const std::wstring& arg) {
    if (!arg.empty() && arg.find_first_of(L" \t\"") == std::wstring::npos) {
        cl += arg;
        return;
    }
    cl += L'"';
    size_t bs = 0;
    for (wchar_t c : arg) {
        if (c == L'\\') { ++bs; cl += c; }
        else if (c == L'"') { cl.append(bs + 1, L'\\'); cl += c; bs = 0; }
        else { bs = 0; cl += c; }
    }
    cl.append(bs, L'\\');
    cl += L'"';
}

std::wstring build_cmdline_w(const std::string& exe_utf8,
                             const std::vector<std::string>& args_utf8) {
    std::wstring cl;
    append_quoted(cl, utf8_to_wide(exe_utf8));
    for (const auto& a : args_utf8) {
        cl += L' ';
        append_quoted(cl, utf8_to_wide(a));
    }
    return cl;
}

std::string make_output_path(const std::string& in_utf8, const OutputSettings& s) {
    fs::path in = utf8_to_path(in_utf8);
    fs::path dir = s.custom_dir ? utf8_to_path(s.dir_utf8) : in.parent_path();
    fs::path out = dir / (in.stem().wstring() + L".webp");
    if (_wcsicmp(out.c_str(), in.c_str()) == 0)
        out = dir / (in.stem().wstring() + L"_converted.webp");
    return path_to_utf8(out);
}

Converter::~Converter() {
    cancel();
    join();
}

void Converter::join() {
    if (th_.joinable()) th_.join();
}

void Converter::cancel() {
    cancel_ = true;
    std::lock_guard<std::mutex> lk(proc_mtx_);
    if (current_proc_) TerminateProcess((HANDLE)current_proc_, 1);
}

void Converter::post(void (*cb)(void*), Msg* m) {
    // FLTK's awake queue is bounded; retry when full
    while (Fl::awake(cb, m) != 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

Converter::RunResult Converter::run_one(const std::wstring& cmdline) {
    RunResult r;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE rd = nullptr, wr = nullptr;
    if (!CreatePipe(&rd, &wr, &sa, 0)) {
        r.output = "CreatePipe failed";
        return r;
    }
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError = wr;
    PROCESS_INFORMATION pi{};

    std::wstring buf = cmdline;   // CreateProcessW needs a writable buffer
    BOOL ok = CreateProcessW(nullptr, buf.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(wr);   // close parent's write end or ReadFile never sees EOF
    if (!ok) {
        CloseHandle(rd);
        r.output = "無法啟動 cwebp(CreateProcess 失敗,錯誤碼 " + std::to_string(GetLastError()) + ")";
        return r;
    }
    {
        std::lock_guard<std::mutex> lk(proc_mtx_);
        current_proc_ = pi.hProcess;
    }

    char cbuf[4096];
    DWORD got = 0;
    while (ReadFile(rd, cbuf, sizeof(cbuf), &got, nullptr) && got > 0)
        if (r.output.size() < 65536) r.output.append(cbuf, got);
    CloseHandle(rd);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = (DWORD)-1;
    GetExitCodeProcess(pi.hProcess, &code);
    {
        std::lock_guard<std::mutex> lk(proc_mtx_);
        current_proc_ = nullptr;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    r.exit_code = (int)code;
    return r;
}

void Converter::start(std::vector<Job> jobs, CwebpOptions opts, OutputSettings out,
                      std::string cwebp_path_utf8, void (*awake_cb)(void*), void* user) {
    join();
    cancel_ = false;
    running_ = true;
    th_ = std::thread([this, jobs = std::move(jobs), opts = std::move(opts),
                       out = std::move(out), exe = std::move(cwebp_path_utf8),
                       awake_cb, user]() mutable {
        int processed = 0, ok = 0, failed = 0;
        const int total = (int)jobs.size();
        size_t i = 0;
        for (; i < jobs.size(); ++i) {
            if (cancel_) break;
            const Job& j = jobs[i];
            post(awake_cb, new Msg{user, j.index, JobStatus::Running, "", processed, ok, failed, total, false});

            std::string out_path = make_output_path(j.in_utf8, out);
            std::wstring cl = build_cmdline_w(exe, build_args(opts, j.in_utf8, out_path));
            RunResult r = run_one(cl);
            ++processed;

            JobStatus st;
            std::string err;
            if (r.exit_code == 0) {
                st = JobStatus::Done;
                ++ok;
            } else if (cancel_) {
                st = JobStatus::Cancelled;
            } else {
                st = JobStatus::Failed;
                ++failed;
                err = r.output.substr(0, 2000);
            }
            post(awake_cb, new Msg{user, j.index, st, err, processed, ok, failed, total, false});
        }
        for (; i < jobs.size(); ++i)
            post(awake_cb, new Msg{user, jobs[i].index, JobStatus::Cancelled, "", processed, ok, failed, total, false});

        running_ = false;
        post(awake_cb, new Msg{user, -1, JobStatus::Pending, "", processed, ok, failed, total, true});
    });
}
