// string_util.h
#pragma once
#include <Windows.h>
#include <string>
#include <cwctype>
#include <cctype>
#include <algorithm>
#include <locale>
#include <vector>

#include <functional>
#include <algorithm>

namespace string_util {

    // Convert UTF-8 (string) to UTF-16 (wstring)
    inline std::wstring utf8_to_wstring(const std::string& str) {
        if (str.empty()) return {};
        int sz = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
        std::wstring wstr(sz, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], sz);
        return wstr;
    }

    //inline std::string wstr_to_utf8(const std::wstring& wstr, UINT codePage = CP_UTF8) {
    inline std::string wstr_to_utf8(const std::wstring& wstr, UINT codePage = CP_UTF8) {
        // 计算需要多大的缓冲区来存储转换后的字符串
        int bufferSize = WideCharToMultiByte(codePage, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
        if (bufferSize == 0) {
            // 转换失败
            return "";
        }

        // 分配缓冲区
        std::vector<char> buffer(bufferSize);

        // 执行实际的转换
        int convertedSize = WideCharToMultiByte(codePage, 0, wstr.c_str(), (int)wstr.length(), buffer.data(), bufferSize, NULL, NULL);
        if (convertedSize == 0) {
            // 转换失败
            return "";
        }

        // 将缓冲区内容转换为std::string
        return std::string(buffer.data(), convertedSize);
    }
    //inline std::string wstr_to_utf8(const std::wstring& wstr) {
    //    if (wstr.empty()) return {};

    //    // Use -1 to include null terminator, then adjust
    //    int sz = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
    //        nullptr, 0, nullptr, nullptr);
    //    if (sz <= 1) return {}; // sz includes null terminator

    //    std::string str(sz - 1, 0); // -1 to exclude null terminator
    //    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
    //        str.data(), sz, nullptr, nullptr);
    //    return str;
    //}

    // Helper for tolower, handles wchar_t and char
    inline wchar_t to_lower(wchar_t ch) { return std::towlower(ch); }
    inline char    to_lower(char ch) { return std::tolower(static_cast<unsigned char>(ch)); }

    // Fuzzy match: all chars of pattern appear in order in str (case-insensitive)
    template<typename T>
    bool fuzzy_match(const T& pattern, const T& str) {
        if (pattern.empty()) return true;
        size_t pi = 0;
        for (auto c : str) {
            if (to_lower(c) == to_lower(pattern[pi])) {
                if (++pi == pattern.size())
                    return true;
            }
        }
        return false;
    }

    // Substring (case-insensitive)
    // Uses locale-independent lowercase comparison
    template<typename String>
    bool substring_match(const String& pattern, const String& str) {
        if (pattern.empty()) return true;
        auto it = std::search(
            str.begin(), str.end(),
            pattern.begin(), pattern.end(),
            [](typename String::value_type ch1, typename String::value_type ch2) {
                return to_lower(ch1) == to_lower(ch2);
            }
        );
        return it != str.end();
    }

    // Helper for isspace (normal or wide char)
    template<typename CharT>
    bool is_space(CharT ch);

    template<>
    inline bool is_space<char>(char ch) {
        return std::isspace(static_cast<unsigned char>(ch));
    }

    template<>
    inline bool is_space<wchar_t>(wchar_t ch) {
        return std::iswspace(ch);
    }

    // Template function for splitting
    template<typename StringT>
    std::vector<StringT> splitByFourSpaces(const StringT& str) {
        using CharT = typename StringT::value_type;
        std::vector<StringT> result;
        size_t i = 0, n = str.length();
        while (i < n) {
            // skip spaces
            while (i < n && is_space(str[i])) ++i;
            size_t j = i;
            // find next space
            while (j < n && !is_space(str[j])) ++j;
            if (i < j)
                result.emplace_back(str.substr(i, j - i));
            i = j;
        }
        return result;
    }


    std::vector<size_t> FindMatches(const std::wstring& content, const std::wstring& searchText);

    std::vector<std::wstring> splitByFourSpaces(const std::wstring& s);
    std::wstring joinWstrings(const std::vector<std::wstring>& lines);
    // std::deque<std::wstring> filterVectorWithPats(
    //     const std::deque<std::wstring>& items,
    //     const std::wstring& pat
    // );


} // namespace string_util
