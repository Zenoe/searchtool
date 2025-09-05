#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include "Database.h"
HWND ShowWaitSpinner(HWND hWndMain) {
    // Create overlay window (no border)
    RECT rc; GetWindowRect(hWndMain, &rc);
    int width = 260, height = 70;
    int x = rc.left + (rc.right-rc.left-width)/2;
    int y = rc.top + (rc.bottom-rc.top-height)/2;
    HWND hWait = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW, WC_DIALOG, L"",
        WS_POPUP | WS_VISIBLE | DS_MODALFRAME,
        x, y, width, height,
        hWndMain, NULL, GetModuleHandle(0), NULL);

    HWND hProg = CreateWindowEx(0, PROGRESS_CLASS, NULL,
        WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
        20, 16, 220, 24,
        hWait, NULL, GetModuleHandle(0), NULL);

    SendMessage(hProg, PBM_SETMARQUEE, TRUE, 60);
    SetWindowText(hWait, L"正在搜索，请稍等..."); // title bar

    HWND hStatic = CreateWindowEx(0, L"STATIC", L"Searching...",
        WS_CHILD | WS_VISIBLE, 70, 46, 120, 16,
        hWait, NULL, GetModuleHandle(0), NULL);

    UpdateWindow(hWait);
    return hWait;
}

HFONT GetModernFont()
{
    static HFONT font = nullptr;
    if (!font) {
        font = CreateFontW(
            -16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
            // 或：L"微软雅黑"
        );
    }
    return font;
}


void PopulateModules(HWND hCombo, const std::vector<std::string>& modules)
{
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"");// 首项为空
    for (const auto& m : modules) {
        // 假定m是UTF-8编码,转为std::wstring
        int wlen = MultiByteToWideChar(CP_UTF8, 0, m.c_str(), -1, NULL, 0);
        std::wstring ws(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, m.c_str(), -1, &ws[0], wlen);

        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)ws.c_str());
    }
}

void DrawSplitter(HWND hwnd, SplitterState& state)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    int cy = rc.bottom - rc.top;
    HDC hdc = GetDC(hwnd);

    // Draw splitter as a dark vertical bar
    RECT splitter = {state.splitterX, topSectionHeight, state.splitterX + SPLITTER_WIDTH, cy};
    HBRUSH hBrush = CreateSolidBrush(RGB(180,180,180));
    FillRect(hdc, &splitter, hBrush);
    DeleteObject(hBrush);

    ReleaseDC(hwnd, hdc);
}


// 工具: LABEL创建
HWND CreateLabel(HWND parent, LPCWSTR text, int x, int y, int w, int h, int id) {
    return CreateWindowEx(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, (HMENU)id, NULL, NULL);
}

void RefreshTable(HWND hList, const std::vector<CaseRecord>& data, int pageIndex, int pageSize)
{
    ListView_DeleteAllItems(hList);

    int start = pageIndex * pageSize;
    int end = (std::min)(start + pageSize, (int)data.size());
    for (int i = start; i < end; ++i) {
        LVITEM lvi{};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i - start;
        lvi.pszText = (LPWSTR)data[i].CASESUITE.c_str();
        ListView_InsertItem(hList, &lvi);

        ListView_SetItemText(hList, lvi.iItem, 1, (LPWSTR)data[i].CASENAME.c_str());
        ListView_SetItemText(hList, lvi.iItem, 2, (LPWSTR)data[i].CASEID.c_str());
        ListView_SetItemText(hList, lvi.iItem, 3, (LPWSTR)data[i].SCRIPTID.c_str());
        ListView_SetItemText(hList, lvi.iItem, 4, (LPWSTR)data[i].COMPOSITONNAME.c_str());
        ListView_SetItemText(hList, lvi.iItem, 5, (LPWSTR)data[i].REMARK.c_str());
    }
}

// 页码显示辅助（可在界面加STATIC控件并刷新文本）
void UpdatePageStatus(HWND hWndPage, int pageIndex, int total, int pageSize)
{
    int pageCount = (total + pageSize - 1) / pageSize;
    wchar_t buf[64] = {0};
    swprintf(buf, 64, L"%d / %d 页，%d条", pageIndex + 1, pageCount, total);
    SetWindowText(hWndPage, buf);
}
