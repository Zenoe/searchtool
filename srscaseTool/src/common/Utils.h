#pragma once
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <codecvt>
#include <locale>
#include <windows.h>
#include <shlobj.h>


#include <string>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

class Utils {
public:
    // 加载配置文件
    static json LoadConfig(const std::string& configFile) {
        try {
            std::ifstream file(configFile);
            if (!file.is_open()) {
                return json();
            }

            json config;
            file >> config;
            return config;
        }
        catch (const std::exception& e) {
            // 加载失败时返回空配置
            return json();
        }
    }

    // static std::string GetAppDataDir() {
    //     wchar_t path[MAX_PATH];
    //     if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
    //         std::wstring wpath(path);
    //         wpath += L"\\TestCaseUploader";

    //         // 确保目录存在
    //         CreateDirectoryW(wpath.c_str(), NULL);

    //         // 转换为UTF-8
    //         std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    //         return converter.to_bytes(wpath);
    //     }

    //     return "";
    // }


    // Returns first position of pattern in text, or npos if not found
    // not sure if it works
    static size_t boyer_moore_search(const std::wstring& text, const std::wstring& pattern) {
        auto build_bad = [](const std::wstring& pattern) -> std::vector<size_t> {
            if (pattern.empty()) {
                return std::vector<size_t>(65536, 0);
            }

            size_t m = pattern.length();
            std::vector<size_t> table(65536, m);

            for (size_t i = 0; i < m - 1; ++i) {
                wchar_t c = pattern[i];
                if (static_cast<size_t>(c) < 65536) {
                    table[static_cast<size_t>(c)] = m - 1 - i;
                }
            }
            return table;
        };
		if (pattern.empty() || text.empty() || pattern.size() > text.size())
			return std::wstring::npos;

		size_t n = text.length();
		size_t m = pattern.length();

		auto bad_char_table = build_bad(pattern);

		size_t offset = 0;
		while (offset <= n - m) {
			size_t i = m - 1;
            while (i != static_cast<size_t>(-1) && pattern[i] == text[offset + i]) {
                --i;
            }
            if (i == static_cast<size_t>(-1)) {
                return offset; // Found
            }
            wchar_t bad_char = text[offset + i];
            size_t shift = bad_char_table[static_cast<size_t>(bad_char)];
            offset += std::max<size_t>(1, shift);
        }
        return std::wstring::npos; // Not found
    }
};
