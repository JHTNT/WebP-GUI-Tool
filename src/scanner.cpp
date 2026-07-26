#include "scanner.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cwctype>

namespace fs = std::filesystem;

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

std::string wide_to_utf8(const std::wstring& s) {
    if (s.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    std::string u(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.data(), (int)s.size(), u.data(), n, nullptr, nullptr);
    return u;
}

fs::path utf8_to_path(const std::string& s) { return fs::path(utf8_to_wide(s)); }
std::string path_to_utf8(const fs::path& p) { return wide_to_utf8(p.native()); }

bool is_supported_image(const fs::path& p) {
    std::wstring ext = p.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](wchar_t c) { return (wchar_t)std::towlower(c); });
    return ext == L".png" || ext == L".jpg" || ext == L".jpeg" ||
           ext == L".tif" || ext == L".tiff" || ext == L".webp";
}

std::vector<std::string> collect_images(const std::vector<std::string>& paths_utf8,
                                        bool recursive) {
    std::vector<std::string> out;
    std::error_code ec;
    const auto opts = fs::directory_options::skip_permission_denied;
    for (const auto& s : paths_utf8) {
        fs::path p = utf8_to_path(s);
        if (fs::is_directory(p, ec)) {
            try {
                if (recursive) {
                    for (const auto& e : fs::recursive_directory_iterator(p, opts))
                        if (e.is_regular_file(ec) && is_supported_image(e.path()))
                            out.push_back(path_to_utf8(e.path()));
                } else {
                    for (const auto& e : fs::directory_iterator(p, opts))
                        if (e.is_regular_file(ec) && is_supported_image(e.path()))
                            out.push_back(path_to_utf8(e.path()));
                }
            } catch (const fs::filesystem_error&) {
                // skip unreadable directories
            }
        } else if (fs::is_regular_file(p, ec) && is_supported_image(p)) {
            out.push_back(path_to_utf8(p));
        }
    }
    return out;
}
