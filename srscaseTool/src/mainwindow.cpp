#include <windows.h>
#include <commctrl.h>
#include <string>
#include "Database.h"
#include <vector>

#pragma comment(lib, "comctl32.lib")
Database* g_db = nullptr;

// 分页按钮和标签控件ID
enum {
    IDC_STATIC_SUITE = 2000,
    IDC_STATIC_NAME,
    IDC_STATIC_ID,
    IDC_STATIC_SCRIPTID,
    IDC_STATIC_MODULE,
    IDC_STATIC_TEXT,
    IDC_BTN_PREV,
    IDC_BTN_NEXT,
    IDC_BTN_SEARCH,
    IDC_EDIT_SUITE,
    IDC_EDIT_NAME,
    IDC_EDIT_ID,
    IDC_EDIT_SCRIPTID,
    IDC_COMBO_MODULE,
    IDC_EDIT_TEXT,
    IDC_LISTVIEW,
    IDC_EDIT_CONTENT,
};

HFONT GetModernFont()
{
    static HFONT font = nullptr;
    if (!font) {
        font = CreateFontW(
            -18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
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



// 分页参数
static int pageSize = 20;      // 每页记录数
static int pageIndex = 0;      // 当前页码
static std::vector<CaseRecord> allRecords; // 搜索完整数据

// 工具: LABEL创建
HWND CreateLabel(HWND parent, LPCWSTR text, int x, int y, int w, int h, int id) {
    return CreateWindowEx(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, parent, (HMENU)id, NULL, NULL);
}

// ListView分页刷新
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
    wchar_t buf[64];
    swprintf(buf, 64, L"%d / %d 页，%d条", pageIndex + 1, pageCount, total);
    SetWindowTextW(hWndPage, buf);
}

inline std::string wstr_to_utf8(const wchar_t* wstr)
{
    if (!wstr) return "";
    int bufSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (bufSize <= 1) return "";
    std::string result(bufSize - 1, '\0'); // Exclude null terminator
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], bufSize, nullptr, nullptr);
    return result;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 所有控件
    static HWND hLblSuite, hEditSuite, hLblName, hEditName, hLblID, hEditID, hLblScriptID, hEditScriptID, hLblModule, hComboModule, hLblText, hEditText;
    static HWND hTable, hEditContent, hBtnPrev, hBtnNext, hBtnSearch, hPageLabel;

    switch (msg)
    {
    case WM_CREATE:
    {
        InitCommonControlsEx(nullptr);

        // 位置参数
        int yt = 12;   // 顶部Y
        int lh = 22;   // label高度
        int eh = 22;   // 输入框高度
        int xs = 10;   // 间距
        int x = xs;

        // 标签和输入框（依次排列）
        hLblSuite = CreateLabel(hWnd, L"用例包：", x,    yt, 56, lh, IDC_STATIC_SUITE);
        hEditSuite = CreateWindowEx(0, L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, x+56, yt, 90, eh, hWnd, (HMENU)IDC_EDIT_SUITE, nullptr, nullptr);
        SendMessage(hEditSuite, EM_LIMITTEXT, 20, 0); x += 56+90+xs;

        hLblName = CreateLabel(hWnd, L"用例名：", x,    yt, 56, lh, IDC_STATIC_NAME);
        hEditName = CreateWindowEx(0, L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, x+56, yt, 90, eh, hWnd, (HMENU)IDC_EDIT_NAME, nullptr, nullptr);
        SendMessage(hEditName, EM_LIMITTEXT, 24, 0); x += 56+90+xs;

        hLblID = CreateLabel(hWnd, L"用例ID：", x,    yt, 56, lh, IDC_STATIC_ID);
        hEditID = CreateWindowEx(0, L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, x+56, yt, 90, eh, hWnd, (HMENU)IDC_EDIT_ID, nullptr, nullptr);
        SendMessage(hEditID, EM_LIMITTEXT, 16, 0); x += 56+90+xs;

        hLblScriptID = CreateLabel(hWnd, L"脚本编号：", x, yt, 70, lh, IDC_STATIC_SCRIPTID);
        hEditScriptID = CreateWindowEx(0, L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, x+70, yt, 90, eh, hWnd, (HMENU)IDC_EDIT_SCRIPTID, nullptr, nullptr);
        SendMessage(hEditScriptID, EM_LIMITTEXT, 20, 0); x += 70+90+xs;

        hLblModule = CreateLabel(hWnd, L"模块：", x, yt, 44, lh, IDC_STATIC_MODULE);
        hComboModule = CreateWindowEx(0, L"COMBOBOX", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER, x+44, yt, 100, eh, hWnd, (HMENU)IDC_COMBO_MODULE, nullptr, nullptr);
        x += 44+100+xs;

        hLblText = CreateLabel(hWnd, L"内容：", x, yt, 44, lh, IDC_STATIC_TEXT);
        hEditText = CreateWindowEx(0, L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, x+44, yt, 160, eh, hWnd, (HMENU)IDC_EDIT_TEXT, nullptr, nullptr);
        SendMessage(hEditText, EM_LIMITTEXT, 80, 0); x += 44+160+xs;

        hBtnSearch = CreateWindowEx(0, L"BUTTON", L"搜索", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, yt, 80, eh, hWnd, (HMENU)IDC_BTN_SEARCH, nullptr, nullptr);
        x += 80+xs;

        // 表格（LISTVIEW）
        int top_height = yt + lh + xs;
        hTable      = CreateWindowEx(0, WC_LISTVIEW, nullptr, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                     xs, top_height, 600, 340, hWnd, (HMENU)IDC_LISTVIEW, nullptr, nullptr);
        ListView_SetExtendedListViewStyle(hTable, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        // 表头
        LVCOLUMN lvc{};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;
        const wchar_t* headers[] = { L"用例包", L"用例名", L"用例ID", L"脚本编号", L"模块", L"备注", nullptr };
        int widths[] = { 98,98,98,86,86,90 };
        for (int i = 0; i < 6; ++i) {
            lvc.pszText = (LPWSTR)headers[i];
            lvc.cx = widths[i];
            ListView_InsertColumn(hTable, i, &lvc);
        }

        // 右侧内容区
        hEditContent = CreateWindowEx(0, L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
                        610, top_height, 340, 340, hWnd, (HMENU)IDC_EDIT_CONTENT, nullptr, nullptr);

        // 分页控件+按钮
        int bt_y = top_height + 350 + 8;
        hBtnPrev = CreateWindowEx(0, L"BUTTON", L"上一页", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, bt_y, 80, eh, hWnd, (HMENU)IDC_BTN_PREV, nullptr, nullptr);
        hBtnNext = CreateWindowEx(0, L"BUTTON", L"下一页", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 120, bt_y, 80, eh, hWnd, (HMENU)IDC_BTN_NEXT, nullptr, nullptr);
        hPageLabel = CreateWindowEx(0, L"STATIC", L"0/0页", WS_CHILD | WS_VISIBLE, 220, bt_y, 120, eh, hWnd, NULL, nullptr, nullptr);

        // Font
        HFONT modernFont = GetModernFont();
        for (HWND h : {hLblSuite, hEditSuite, hLblName, hEditName, hLblID, hEditID, hLblScriptID, hEditScriptID,
                       hLblModule, hComboModule, hLblText, hEditText, hBtnSearch,
                       hBtnPrev, hBtnNext, hPageLabel, hTable, hEditContent}) {
            SendMessage(h, WM_SETFONT, (WPARAM)modernFont, TRUE);
        }

        // Load module options
        auto modules = g_db->GetAllModules();
        PopulateModules(hComboModule, modules);

        // Initial 查询
        allRecords = g_db->Search("", "", "", "", "", "");
        pageIndex = 0;
        RefreshTable(hTable, allRecords, pageIndex, pageSize);
        UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);

        break;
    }
    case WM_COMMAND:
        // 搜索按钮
        if (LOWORD(wParam) == IDC_BTN_SEARCH)
        {
            wchar_t wsuite[64], wname[64], wid[64], wscriptid[64], wtext[128], wmodule[64];
            GetWindowTextW(hEditSuite, wsuite, 64);
            GetWindowTextW(hEditName, wname, 64);
            GetWindowTextW(hEditID, wid, 64);
            GetWindowTextW(hEditScriptID, wscriptid, 64);
            GetWindowTextW(hEditText, wtext, 128);
            SendMessageW(hComboModule, CB_GETLBTEXT, SendMessageW(hComboModule, CB_GETCURSEL, 0, 0), (LPARAM)wmodule);

            std::string suite    = wstr_to_utf8(wsuite);
            std::string name     = wstr_to_utf8(wname);
            std::string id       = wstr_to_utf8(wid);
            std::string scriptid = wstr_to_utf8(wscriptid);
            std::string module   = wstr_to_utf8(wmodule);
            std::string text     = wstr_to_utf8(wtext);

            // SQL LIKE %
            allRecords = g_db->Search(
                suite.empty() ? "" : "%" + suite + "%",
                name.empty() ? "" : "%" + name + "%",
                id.empty() ? "" : "%" + id + "%",
                scriptid.empty() ? "" : "%" + scriptid + "%",
                module.empty() ? "" : "%" + module + "%",
                text.empty() ? "" : "%" + text + "%"
            );
            pageIndex = 0;
            RefreshTable(hTable, allRecords, pageIndex, pageSize);
            UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
        }
        // 下拉框筛选也可触发搜索（可选）
        else if (LOWORD(wParam) == IDC_COMBO_MODULE && HIWORD(wParam) == CBN_SELCHANGE) {
            SendMessage(hWnd, WM_COMMAND, (WPARAM)IDC_BTN_SEARCH, 0); // 触发搜索
        }
        // 翻页
        else if (LOWORD(wParam) == IDC_BTN_PREV && pageIndex > 0) {
            pageIndex--;
            RefreshTable(hTable, allRecords, pageIndex, pageSize);
            UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
        }
        else if (LOWORD(wParam) == IDC_BTN_NEXT &&
                 (pageIndex+1)*pageSize < (int)allRecords.size()) {
            pageIndex++;
            RefreshTable(hTable, allRecords, pageIndex, pageSize);
            UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
        }
        break;
    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == IDC_LISTVIEW && ((LPNMHDR)lParam)->code == LVN_ITEMCHANGED) {
            NMLISTVIEW* pnm = (NMLISTVIEW*)lParam;
            if (pnm->uNewState & LVIS_SELECTED) {
                int idx = pageIndex * pageSize + pnm->iItem;
                if (idx >= 0 && idx < (int)allRecords.size()) {
                    SetWindowTextW(hEditContent, std::wstring(allRecords[idx].CASETXTCONTENT.begin(), allRecords[idx].CASETXTCONTENT.end()).c_str());
                }
            }
        }
        break;
    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);

        int xs = 10, top_height = 44;
        int left_width = w * 2 / 3;
        int right_width = w - left_width - xs;
        int bottom_height = h - top_height - 98;

        // 顶部行重排（只调整宽度）
        MoveWindow(hLblSuite, xs, 12, 56, 22, TRUE);
        MoveWindow(hEditSuite, xs+56, 12, 90, 22, TRUE);

        MoveWindow(hLblName, 162, 12, 56, 22, TRUE);
        MoveWindow(hEditName, 218, 12, 90, 22, TRUE);

        MoveWindow(hLblID, 314, 12, 56, 22, TRUE);
        MoveWindow(hEditID, 370, 12, 90, 22, TRUE);

        MoveWindow(hLblScriptID, 466, 12, 70, 22, TRUE);
        MoveWindow(hEditScriptID, 536, 12, 90, 22, TRUE);

        MoveWindow(hLblModule, 632, 12, 44, 22, TRUE);
        MoveWindow(hComboModule, 676, 12, 100, 22, TRUE);

        MoveWindow(hLblText, 782, 12, 44, 22, TRUE);
        MoveWindow(hEditText, 826, 12, 160, 22, TRUE);

        MoveWindow(hBtnSearch, 996, 12, 80, 22, TRUE);

        // ListView和内容区
        MoveWindow(hTable, xs, top_height, left_width-xs, bottom_height, TRUE);
        MoveWindow(hEditContent, left_width, top_height, right_width-xs, bottom_height, TRUE);

        // 分页按钮和页码显示label
        int bt_y = top_height + bottom_height + xs;
        MoveWindow(hBtnPrev, 20, bt_y, 80, 22, TRUE);
        MoveWindow(hBtnNext, 120, bt_y, 80, 22, TRUE);
        MoveWindow(hPageLabel, 220, bt_y, 120, 22, TRUE);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
