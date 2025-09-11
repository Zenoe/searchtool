#ifndef SYSUTIL_H_
#define SYSUTIL_H_
#include <windows.h>
void CopyTextToClipboard(HWND hWnd, const wchar_t* text) {
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


#endif // SYSUTIL_H_
