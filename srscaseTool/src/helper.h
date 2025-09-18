#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include "Database.h"
#include "common/stringutil.h"

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

HWND CreateLabel(HWND parent, LPCWSTR text, int x, int y, int w, int h, UINT_PTR id) {
    //  64 位 Windows 上，HMENU 实际为指针型，强制转换低位 int 可能丢失高位数据，id 不要为int 类型 
    return CreateWindowEx(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, (HMENU)id, NULL, NULL);
}

void RefreshTable(HWND hList, const std::vector<CaseRecord>& data, int pageIndex, int pageSize, bool showExtra = false)
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
        if(showExtra){
            // std::wstring joined = string_util::joinWstrings(data[i].matched_lines);
            // SetWindowTextW(hInfo, joined.c_str());
            // LVI expects pointer: use joined.c_str(), joined will be alive during the call.
            // ListView_SetItemText(hList, lvi.iItem, 5, (LPWSTR)joined.c_str());
            // auto tmp = string_util::joinWstrings( data[i].matched_lines ).c_str();
            // ListView_SetItemText(hList, lvi.iItem, 5, (LPWSTR)tmp);
        }else{
            // ListView_SetItemText(hList, lvi.iItem, 5, (LPWSTR)data[i].REMARK.c_str());
        }
    }
}

void UpdatePageStatus(HWND hWndPage, int pageIndex, int total, int pageSize)
{
    int pageCount = (total + pageSize - 1) / pageSize;
    wchar_t buf[64] = {0};
    swprintf(buf, 64, L"%d / %d 页，%d条", pageIndex + 1, pageCount, total);
    SetWindowText(hWndPage, buf);
}

void ClearRichEditFormat(HWND hRichEdit) {
    // Get the text length in characters (not bytes)
    LONG textLen = GetWindowTextLengthW(hRichEdit);

    // Select all text
    SendMessageW(hRichEdit, EM_SETSEL, 0, textLen);

    // Reset character formatting to default
    CHARFORMAT2W cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_WEIGHT | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT;
    cf.crTextColor = RGB(0, 0, 0); // Black text
    cf.crBackColor = RGB(255, 255, 255); // White background
    cf.dwEffects = 0; // Remove all effects (bold, italic, etc.)
    cf.wWeight = FW_NORMAL; // Normal weight

    SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));

    // Deselect
    SendMessageW(hRichEdit, EM_SETSEL, -1, 0);
}

// Get plain text from RichEdit control without formatting
std::wstring GetPlainTextFromRichEdit(HWND hRichEdit) {
    // Get the text length in characters
    int textLen = GetWindowTextLengthW(hRichEdit);

    if (textLen <= 0) {
        return L"";
    }

    // Allocate buffer for the text (plus null terminator)
    std::wstring plainText(textLen + 1, L'\0');

    // Use EM_GETTEXT to get plain text (simpler approach)
    GetWindowTextW(hRichEdit, &plainText[0], textLen + 1);

    // Resize to actual length (remove null terminator)
    plainText.resize(textLen);

    return plainText;
}

static int currentMatchIndex = 0;
static std::vector<size_t> lastMatches;
void ScrollToMatch(HWND hRichEdit, int matchIndex, size_t matchLen) {
    if (matchIndex < 0 || matchIndex >= (int)lastMatches.size()) return;

    LONG start = (LONG)lastMatches[matchIndex];
    LONG end = start + (LONG)matchLen;

    // Select the match (use EX version for >64K safety)
    CHARRANGE cr{ start, end };
    SendMessageW(hRichEdit, EM_EXSETSEL, 0, (LPARAM)&cr);

    // Show selection even when the control doesn't have focus
    SendMessageW(hRichEdit, EM_HIDESELECTION, FALSE, 0);

    // Scroll vertically to make the start line visible
    LRESULT targetLine = SendMessageW(hRichEdit, EM_EXLINEFROMCHAR, 0, start);
    LRESULT firstVisible = SendMessageW(hRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0);

    int delta = (int)targetLine - (int)firstVisible;
    if (delta != 0) {
        SendMessageW(hRichEdit, EM_LINESCROLL, 0, delta);
    }

    currentMatchIndex = matchIndex;
}


// Function to navigate to next match
void GoToNextMatch(HWND hRichEdit, size_t matchLen) {
    if (lastMatches.empty()) return;

    int nextIndex = (currentMatchIndex + 1) % (int)lastMatches.size();
    ScrollToMatch(hRichEdit, nextIndex, matchLen);
}

// Function to navigate to previous match
void GoToPreviousMatch(HWND hRichEdit, size_t matchLen) {
    if (lastMatches.empty()) return;

    int prevIndex = (currentMatchIndex - 1 + (int)lastMatches.size()) % (int)lastMatches.size();
    ScrollToMatch(hRichEdit, prevIndex, matchLen);
}

// Modified HighlightMatches function with auto-scroll to first match
void HighlightMatches(HWND hRichEdit, const std::vector<size_t>& matchPositions, size_t matchLen) {
    // Store matches for navigation
    lastMatches = matchPositions;
    currentMatchIndex = 0;

    for (size_t pos : matchPositions) {
        CHARRANGE range = { (LONG)pos, (LONG)(pos + matchLen) };

        // Select the match
        SendMessageW(hRichEdit, EM_SETSEL, range.cpMin, range.cpMax);

        // Apply red color
        CHARFORMAT2W cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR | CFM_BACKCOLOR;
        cf.crTextColor = RGB(255, 0, 0); // Red text
        cf.crBackColor = RGB(255, 255, 0); // Yellow background for better visibility

        SendMessageW(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
    }

    // Auto-scroll to first match if any matches found
    if (!matchPositions.empty()) {
        ScrollToMatch(hRichEdit, 0, matchLen);
    }
    else {
        // Deselect if no matches
        SendMessageW(hRichEdit, EM_SETSEL, -1, 0);
    }
}


void FilterComboBox(HWND hCombo, const std::vector<std::string>& itemList, const std::wstring& filter) {
    // Save current selection (edit text) and cursor position
    TCHAR currText[256] = {};
    GetWindowText(hCombo, currText, 256);
    DWORD startPos, endPos;
    SendMessage(hCombo, CB_GETEDITSEL, (WPARAM)&startPos, (LPARAM)&endPos);

    // Prevent flicker during update
    SendMessage(hCombo, WM_SETREDRAW, FALSE, 0);

    // Remove all items
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);

    // Add items matching filter
    for (const auto& item : itemList) {
        const std::wstring witem = string_util::utf8_to_wstring(item);
        // Case insensitive search - check if filter is substring of item
        if (filter.empty()) {
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)witem.c_str());
        } else {
            // Convert both strings to lowercase for case-insensitive comparison
            std::wstring itemLower = witem;
            std::wstring filterLower = filter;
            std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), towlower);
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), towlower);

            if (itemLower.find(filterLower) != std::wstring::npos) {
                SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)witem.c_str());
            }
        }
    }

    // Restore edit text and cursor position
    SetWindowText(hCombo, currText);
    SendMessage(hCombo, CB_SETEDITSEL, 0, MAKELPARAM(startPos, endPos));

    SendMessage(hCombo, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hCombo, nullptr, TRUE); // Redraw

    // Show dropdown list if there are matching items
    LRESULT itemCount = SendMessage(hCombo, CB_GETCOUNT, 0, 0);
    if (itemCount > 0) {
        // Show the dropdown list
        SendMessage(hCombo, CB_SHOWDROPDOWN, TRUE, 0);
    } else {
        // Hide dropdown if no items match
        SendMessage(hCombo, CB_SHOWDROPDOWN, FALSE, 0);
    }
}
