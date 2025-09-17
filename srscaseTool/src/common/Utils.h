// src/common/Utils.h
#pragma once
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>
#include <codecvt>
#include <locale>
#include <windows.h>
#include <shlobj.h>

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

    // 获取应用程序数据目录
    static std::string GetAppDataDir() {
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
            std::wstring wpath(path);
            wpath += L"\\TestCaseUploader";

            // 确保目录存在
            CreateDirectoryW(wpath.c_str(), NULL);

            // 转换为UTF-8
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            return converter.to_bytes(wpath);
        }

        return "";
    }

    // 字符串转换工具
    static std::wstring Utf8ToWide(const std::string& str) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(str);
    }

    static std::string WideToUtf8(const std::wstring& wstr) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }
};
