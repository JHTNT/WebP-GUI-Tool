#pragma once
#include <filesystem>
#include <string>
#include <vector>

std::wstring utf8_to_wide(const std::string& s);
std::string  wide_to_utf8(const std::wstring& s);
std::filesystem::path utf8_to_path(const std::string& s);
std::string  path_to_utf8(const std::filesystem::path& p);

bool is_supported_image(const std::filesystem::path& p);

// 輸入混合的檔案/資料夾路徑(來自 DnD 或 chooser),展開為支援的圖檔清單
std::vector<std::string> collect_images(const std::vector<std::string>& paths_utf8,
                                        bool recursive);
