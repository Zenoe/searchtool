#ifndef SYSUTIL_H_
#define SYSUTIL_H_
#include <windows.h>
#include <sstream>
#include <vector>
 void inline CopyTextToClipboard(HWND hWnd, const wchar_t* text) {
    if (!OpenClipboard(hWnd)) return;
    EmptyClipboard();

    size_t len = (wcslen(text) + 1) * sizeof(wchar_t);
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, len);
    if (hg) {
        void* ptr = GlobalLock(hg);
        memcpy(ptr, text, len);
        GlobalUnlock(hg);

        SetClipboardData(CF_UNICODETEXT, hg);
    }
    CloseClipboard();
    // hg不用free，系统会管理
}

template <typename... Args>
void OutputDebugPrint(Args&&... args) {
#ifdef _DEBUG
    std::wstringstream ss;
    (ss << ... << args);     // Fold expression (C++17)
    ss << L"\n";
    OutputDebugStringW(ss.str().c_str());
#endif
}

// 防止多重定义，要么inline，要么在cpp中定义。 .h 文件负责声明
// 模板函数不受此限制
//inline void OutputDebugPrintVev(const std::vector<FileEntry>& entries) {
//    OutputDebugPrint("vev begin");
//    for (auto e : entries) {
//        OutputDebugPrint(e.fullpath, " ", e.label);
//    }
//    OutputDebugPrint("vev end");
//}

#endif // SYSUTIL_H_
