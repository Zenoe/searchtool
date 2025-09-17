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
    // ���������ļ�
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
            // ����ʧ��ʱ���ؿ�����
            return json();
        }
    }

    // ��ȡӦ�ó�������Ŀ¼
    static std::string GetAppDataDir() {
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path))) {
            std::wstring wpath(path);
            wpath += L"\\TestCaseUploader";

            // ȷ��Ŀ¼����
            CreateDirectoryW(wpath.c_str(), NULL);

            // ת��ΪUTF-8
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            return converter.to_bytes(wpath);
        }

        return "";
    }

    // �ַ���ת������
    static std::wstring Utf8ToWide(const std::string& str) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(str);
    }

    static std::string WideToUtf8(const std::wstring& wstr) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }
};
