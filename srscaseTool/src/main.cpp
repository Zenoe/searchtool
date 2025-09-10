#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <richedit.h>
#include <windowsx.h>
// #include <regex>
#include <re2/re2.h>
#include "stringutil.h"
#include "constant.h"
#include "helper.h"
#include "main.h"

#pragma comment(lib, "comctl32.lib")

void LayoutChildren(HWND hWnd, SplitterState& state) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    // Early exit for invalid dimensions
    if (w <= 0 || h <= 0) return;

    int availableWidth = w - 2 * xs;
    int contentHeight = h - topSectionHeight - eh - ys * 2;

    // Use DeferWindowPos for better performance and reduced flicker
    HDWP hdwp = BeginDeferWindowPos(16); // Estimate number of controls
    if (!hdwp) return;

    // Helper lambda to add window to deferred positioning
    auto deferMove = [&hdwp](HWND hwnd, int x, int y, int width, int height) {
        hdwp = DeferWindowPos(hdwp, hwnd, NULL, x, y, width, height,
                             SWP_NOZORDER | SWP_NOACTIVATE);
        return hdwp != NULL;
    };

    // First row controls - using struct for better organization
    struct ControlGroup {
        HWND label, control;
        int labelWidth, controlWidth;
    };
    RECT oldRect;
    GetWindowRect(hLblSuite, &oldRect);
    InvalidateRect(hWnd, &oldRect, TRUE);

    GetWindowRect(hBtnSearch, &oldRect);
    InvalidateRect(hWnd, &oldRect, TRUE);
    ControlGroup row1Controls[] = {
        {hLblSuite, hEditSuite, 80, 180},
        {hLblName, hEditName, 80, 180},
        {hLblID, hEditID, 80, 180},
        {hLblScriptID, hEditScriptID, 100, 160},
        {hLblModule, hComboModule, 60, 160},
        {hLblText, hEditText, 60, 220}
    };

    int x = xs;
    for (const auto& group : row1Controls) {
        if (!deferMove(group.label, x, row1Y, group.labelWidth, lh)) break;
        if (!deferMove(group.control, x + group.labelWidth, row1Y, group.controlWidth, eh)) break;
        x += group.labelWidth + group.controlWidth + xs;
    }

    deferMove(hCheckRegex, x-xs, row1Y, 90, 22);
    // Second row
    x = xs;
    deferMove(hBtnSearch, x, row2Y, 100, eh);
    deferMove(hBtnReset, x + 100 + padding, row2Y, 100, eh);

    // Table and content area
    int tableWidth = 0;
    if (state.splitterX == 0) {
        tableWidth = w * 1 / 2 - xs * 2;
        state.splitterX = xs + tableWidth + xs;
    } else {
        tableWidth = state.splitterX - xs - 6;
    }

    // Ensure minimum widths
    tableWidth = max(100, tableWidth);
    int contentX = state.splitterX + 6 + splitterW;
    int contentWidth = max(100, w - contentX - xs);

    int buttonY = topSectionHeight + contentHeight + ys;

    deferMove(hTable, xs, topSectionHeight, tableWidth, contentHeight);
    deferMove(hEditContent, contentX, topSectionHeight, contentWidth, contentHeight);
    deferMove(hEditSearchRichEdit,contentX, topSectionHeight + contentHeight, 200, eh );
    deferMove(hBtnPrevMatch,contentX + 200 + padding, topSectionHeight + contentHeight, 30, eh );
    deferMove(hBtnNextMatch,contentX + 200 +  padding  + 30, topSectionHeight + contentHeight, 30, eh );
    deferMove(hStatusBar,contentX + 200 +  ( padding *2  ) +  60 , topSectionHeight + contentHeight, 400, eh );

    // Pagination controls
    deferMove(hBtnPrev, xs, buttonY, 90, eh);
    deferMove(hBtnNext, xs + 100, buttonY, 90, eh);
    deferMove(hPageLabel, xs + 200, buttonY, pageLabelW, eh);

    // Apply all moves at once
    EndDeferWindowPos(hdwp);
}
// void LayoutChildren(HWND hWnd, SplitterState& state){
//     RECT rc;
//     GetClientRect(hWnd, &rc);
//     int w = rc.right - rc.left;
//     int h = rc.bottom - rc.top;

//     int availableWidth = w - 2 * xs;
//     // 表格和内容区域
//     int contentHeight = h - topSectionHeight - eh - ys * 2;

//     // 第一行控件重排
//     int x = xs;
//     MoveWindow(hLblSuite, x, row1Y, 80, lh, TRUE);
//     MoveWindow(hEditSuite, x + 80, row1Y, 180, eh, TRUE);
//     x += 80 + 180 + xs;

//     MoveWindow(hLblName, x, row1Y, 80, lh, TRUE);
//     MoveWindow(hEditName, x + 80, row1Y, 180, eh, TRUE);
//     x += 80 + 180 + xs;

//     MoveWindow(hLblID, x, row1Y, 80, lh, TRUE);
//     MoveWindow(hEditID, x + 80, row1Y, 180, eh, TRUE);
//     x += 80 + 180 + xs;

//     MoveWindow(hLblScriptID, x, row1Y, 100, lh, TRUE);
//     MoveWindow(hEditScriptID, x + 100, row1Y, 160, eh, TRUE);
//     x += 100 + 160 + xs;

//     MoveWindow(hLblModule, x, row1Y, 60, lh, TRUE);
//     MoveWindow(hComboModule, x + 60, row1Y, 160, eh, TRUE); // 下拉框高度保持正常
//     x += 60 + 160 + xs;

//     MoveWindow(hLblText, x, row1Y, 60, lh, TRUE);
//     MoveWindow(hEditText, x + 60, row1Y, 220, eh, TRUE);
//     x += 60 + 220 + xs;

//     // 第二行控件重排
//     x = xs;
//     MoveWindow(hBtnSearch, x, row2Y, 100, eh, TRUE);

//     int tableWidth = 0;
//     // ListView表格
//     // MoveWindow(hSplitter, xs+tableWidth+6, topSectionHeight, splitterW, contentHeight, TRUE);
//     // 内容编辑区
//     if(state.splitterX == 0){
//         tableWidth = w * 2 / 3 - xs * 2;
//         state.splitterX = xs + tableWidth + xs;
//     }else{
//         tableWidth = state.splitterX -xs -6 ;
//     }
//     MoveWindow(hTable, xs, topSectionHeight, tableWidth, contentHeight, TRUE);
//     int contentX = state.splitterX + 6 + splitterW;
//     int contentWidth = w - contentX - xs;
//     MoveWindow(hEditContent, contentX, topSectionHeight, contentWidth, contentHeight, TRUE);
//     // MyDebug(contentX, " ", topSectionHeight," ", contentWidth," ", contentHeight);

//     // 分页按钮和页码显示
//     int buttonY = topSectionHeight + contentHeight + ys;
//     MoveWindow(hBtnPrev, xs, buttonY, 90, eh, TRUE);
//     MoveWindow(hBtnNext, xs + 100, buttonY, 90, eh, TRUE);
//     MoveWindow(hPageLabel, xs + 200, buttonY, pageLabelW, eh, TRUE);
// }

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 所有控件
    static SplitterState state;
    static std::wstring lastSearchText;

    switch (msg)
        {
        case WM_CREATE:
            {
                InitCommonControlsEx(nullptr);
                int x = xs;
                int row1Y = yt;

                hLblSuite = CreateLabel(hWnd, L"用例包：", x, row1Y, 70, lh, IDC_STATIC_SUITE);
                hEditSuite = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                             searchInputStyle, 0, row1Y, 240, eh, hWnd, (HMENU)IDC_EDIT_SUITE, nullptr, nullptr);


                hLblName = CreateLabel(hWnd, L"用例名：", x, row1Y, 70, lh, IDC_STATIC_NAME);
                hEditName = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
                                           searchInputStyle, 0, row1Y, 140, eh, hWnd, (HMENU)IDC_EDIT_NAME, nullptr, nullptr);

                hLblID = CreateLabel(hWnd, L"用例ID：", x, row1Y, 70, lh, IDC_STATIC_ID);
                hEditID = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
                                         searchInputStyle, 0, row1Y, 140, eh, hWnd, (HMENU)IDC_EDIT_ID, nullptr, nullptr);
                SendMessage(hEditID, EM_LIMITTEXT, 80, 0);


                hLblScriptID = CreateLabel(hWnd, L"脚本编号：", x, 0, 80, lh, IDC_STATIC_SCRIPTID);
                hEditScriptID = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
                                               searchInputStyle, 0, 0, 160, eh, hWnd, (HMENU)IDC_EDIT_SCRIPTID, nullptr, nullptr);
                SendMessage(hEditScriptID, EM_LIMITTEXT, 80, 0);

                hLblModule = CreateLabel(hWnd, L"模块：", x, 0, 60, lh, IDC_STATIC_MODULE);
                hComboModule = CreateWindowEx(WS_EX_CLIENTEDGE, L"COMBOBOX", nullptr,
                                              searchInputStyle | CBS_DROPDOWNLIST, x + 60, 0, 160, eh * 8, hWnd, (HMENU)IDC_COMBO_MODULE, nullptr, nullptr);

                hLblText = CreateLabel(hWnd, L"内容：", x, 0, 60, lh, IDC_STATIC_TEXT);
                hEditText = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
                                           searchInputStyle, x + 60, 0, 220, eh, hWnd, (HMENU)IDC_EDIT_TEXT, nullptr, nullptr);
                hCheckRegex = CreateWindowEx(0, L"BUTTON", L"正则匹配",
                                                  WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                                  0, 0, 80, 22, hWnd, (HMENU)IDC_CHECK_REGEX, nullptr, nullptr);
                hEditSearchRichEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
                                                     searchInputStyle, 0, 0, 220, eh, hWnd, (HMENU)IDC_EDIT_SEARCH_RICH_CONTENT, nullptr, nullptr);
                if (hEditSearchRichEdit) {
                    SetWindowSubclass(hEditSearchRichEdit, EditSubclassProc, 0, 0);
                }


                hBtnPrevMatch = CreateWindowEx(0, L"BUTTON", L"<",
                                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0,0, 90, eh, hWnd, (HMENU)IDC_BTN_PREVTEXT, nullptr, nullptr);
                hBtnNextMatch = CreateWindowEx(0, L"BUTTON", L">",
                                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 90, eh, hWnd, (HMENU)IDC_BTN_NEXTTEXT, nullptr, nullptr);
                int row2Y = row1Y + eh + ys;
                x = xs;
                hBtnSearch = CreateWindowEx(0, L"BUTTON", L"搜索",
                                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, row2Y, 100, eh, hWnd, (HMENU)IDC_BTN_SEARCH, nullptr, nullptr);

                hBtnReset = CreateWindowEx(0, L"BUTTON", L"重置",
                                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, row2Y, 100, eh, hWnd, (HMENU)IDC_BTN_RESET, nullptr, nullptr);
                // 表格（LISTVIEW） - 调整位置和大小
                int top_height = row2Y + eh + ys + 10;  // 增加更多间距
                int tableWidth = 700;  // 增加表格宽度
                int tableHeight = 360; // 增加表格高度

                hTable = CreateWindowEx(0, WC_LISTVIEW, nullptr,
                                        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                        xs, top_height, tableWidth, tableHeight, hWnd, (HMENU)IDC_LISTVIEW, nullptr, nullptr);
                ListView_SetExtendedListViewStyle(hTable, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

                // 表头 - 调整列宽
                LVCOLUMN lvc{};
                lvc.mask = LVCF_TEXT | LVCF_WIDTH;
                const wchar_t* headers[] = { L"用例包", L"用例名", L"用例ID", L"脚本编号", L"模块", L"备注", nullptr };
                int widths[] = { 120, 120, 120, 100, 100, 120 };  // 增加列宽
                for (int i = 0; i < 6; ++i) {
                    lvc.pszText = (LPWSTR)headers[i];
                    lvc.cx = widths[i];
                    ListView_InsertColumn(hTable, i, &lvc);
                }

                // 右侧内容区 - 调整位置和大小
                int contentX = xs + tableWidth + xs;  // 与表格保持间距
                int contentWidth = 340;
                hEditContent = CreateWindowExW(0, MSFTEDIT_CLASS, L"",
                                            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
                                            ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                                            contentX, top_height, contentWidth, tableHeight,
                                            hWnd, (HMENU)IDC_EDIT_CONTENT, nullptr, nullptr);

                // hEditContent = CreateWindowEx(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, nullptr,
                //                               WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                //                               contentX, top_height, contentWidth, tableHeight, hWnd, (HMENU)IDC_EDIT_CONTENT, nullptr, nullptr);

                // 分页控件+按钮 - 调整位置
                int bt_y = top_height + tableHeight + 15;  // 增加间距
                hBtnPrev = CreateWindowEx(0, L"BUTTON", L"上一页",
                                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, xs, bt_y, 90, eh, hWnd, (HMENU)IDC_BTN_PREV, nullptr, nullptr);
                hBtnNext = CreateWindowEx(0, L"BUTTON", L"下一页",
                                          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, xs + 100, bt_y, 90, eh, hWnd, (HMENU)IDC_BTN_NEXT, nullptr, nullptr);
                hPageLabel = CreateWindowEx(0, L"STATIC", L"0/0页",
                                            WS_CHILD | WS_VISIBLE | SS_CENTER, xs + 200, bt_y, pageLabelW, eh, hWnd, NULL, nullptr, nullptr);

                hStatusBar = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                             0, 0, 0, 0,
                                             hWnd, nullptr, nullptr, nullptr);
                // Font
                HFONT modernFont = GetModernFont();
                for (HWND h : {hLblSuite, hEditSuite, hLblName, hEditName, hLblID, hEditID, hLblScriptID, hEditScriptID,
                               hLblModule, hComboModule, hLblText, hEditText,hCheckRegex, hBtnSearch, hBtnReset,
                               hBtnPrev, hBtnNext, hPageLabel, hTable, hEditContent, hBtnNextMatch, hBtnPrevMatch, hStatusBar}) {
                    SendMessage(h, WM_SETFONT, (WPARAM)modernFont, TRUE);
                }
                for (HWND h : { hEditSuite, hEditName,  hEditID,  hEditScriptID, hEditText, hEditSearchRichEdit}) {
                    SetWindowSubclass(h, EditProc, 0, 0);
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

        case WM_LBUTTONDOWN:
            {
                int mx = GET_X_LPARAM(lParam);
                int my = GET_Y_LPARAM(lParam);
                RECT rc;
                GetClientRect(hWnd, &rc);

                // Check if in splitter area
                if (mx >= state.splitterX && mx < state.splitterX + SPLITTER_WIDTH)
                    {
                        state.dragging = true;
                        SetCapture(hWnd);
                        state.dragOffset = mx - state.splitterX;
                    }
                return 0;
            }
        case WM_MOUSEMOVE:
            {
                int mx = GET_X_LPARAM(lParam);

                if (state.dragging)
                    {
                        RECT rc;
                        GetClientRect(hWnd, &rc);
                        int cx = rc.right - rc.left;

                        int newX = mx - state.dragOffset;
                        if (newX < MIN_PANEL_WIDTH) newX = MIN_PANEL_WIDTH;
                        if (newX > cx - MIN_PANEL_WIDTH - SPLITTER_WIDTH)
                            newX = cx - MIN_PANEL_WIDTH - SPLITTER_WIDTH;

                        state.splitterX = newX;
                        LayoutChildren(hWnd, state);
                        // Visual feedback
                        InvalidateRect(hWnd, nullptr, FALSE);
                    }
                else
                    {
                        // Change cursor if near splitter
                        if (mx >= state.splitterX && mx < state.splitterX + SPLITTER_WIDTH)
                            SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                    }
                return 0;
            }
        case WM_LBUTTONUP:
            {
                if (state.dragging)
                    {
                        state.dragging = false;
                        ReleaseCapture();
                    }
                return 0;
            }
        case WM_COMMAND:
            // 搜索按钮
            if (LOWORD(wParam) == IDC_BTN_SEARCH)
                {
                    BOOL useRegex = (SendMessage(hCheckRegex, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    wchar_t wsuite[64], wname[64], wid[64], wscriptid[64], wtext[128];
                    wchar_t wmodule[64] = { 0 };
                    GetWindowTextW(hEditSuite, wsuite, 64);
                    GetWindowTextW(hEditName, wname, 64);
                    GetWindowTextW(hEditID, wid, 64);
                    GetWindowTextW(hEditScriptID, wscriptid, 64);
                    GetWindowTextW(hEditText, wtext, 128);
                    SendMessageW(hComboModule, CB_GETLBTEXT, SendMessageW(hComboModule, CB_GETCURSEL, 0, 0), (LPARAM)wmodule);
                    std::string suite = string_util::wstr_to_utf8(wsuite);
                    std::string name = string_util::wstr_to_utf8(wname);
                    std::string id = string_util::wstr_to_utf8(wid);
                    std::string scriptid = string_util::wstr_to_utf8(wscriptid);
                    std::string module = string_util::wstr_to_utf8(wmodule);
                    std::string text = string_util::wstr_to_utf8(wtext);
                    std::vector<CaseRecord> records;

                    // SQL LIKE %
                    EnableWindow(hWnd, FALSE);
                    HWND hWait = ShowWaitSpinner(hWnd);
                    if(!useRegex){
                        records = g_db->Search(
                                                  suite.empty() ? "" : "%" + suite + "%",
                                                  name.empty() ? "" : "%" + name + "%",
                                                  id.empty() ? "" : "%" + id + "%",
                                                  scriptid.empty() ? "" : "%" + scriptid + "%",
                                                  module.empty() ? "" : "%" + module + "%",
                                                  text.empty() ? "" : "%" + text + "%"
                                                 );

					}
					else {
                        RE2 reg(text);
                        if(!reg.ok()){
                            MessageBoxW(hWnd, L"正则表达式语法错误", L"错误", MB_ICONERROR);
                        }else{
                            auto rawRecords = g_db->Search(
                                                           suite.empty() ? "" : "%" + suite + "%",
                                                           name.empty() ? "" : "%" + name + "%",
                                                           id.empty() ? "" : "%" + id + "%",
                                                           scriptid.empty() ? "" : "%" + scriptid + "%",
                                                           module.empty() ? "" : "%" + module + "%",
                                                           "");// 内容字段不先筛
                            records.clear();
                            //try {
                                    //std::wregex re(wtext, std::regex_constants::ECMAScript | std::regex_constants::icase);
                                    for (const auto& rec : rawRecords) {
                                        int clen = WideCharToMultiByte(CP_UTF8, 0, rec.CASETXTCONTENT.c_str(), -1, NULL, 0, NULL, NULL);
                                        if (clen <= 1) continue;
                                        std::string content_utf8(clen - 1, 0);
                                        WideCharToMultiByte(CP_UTF8, 0, rec.CASETXTCONTENT.c_str(), -1, &content_utf8[0], clen, NULL, NULL);

                                        if (RE2::PartialMatch(content_utf8, reg)) {
                                            records.push_back(rec);
    }
                                        // if (std::regex_search(rec.CASETXTCONTENT, re)) {
                                        //     records.push_back(rec);
                                        // }
                                    }
                            //}
                        }
						// catch (...) {
						// 	MessageBoxW(hWnd, L"正则表达式语法错误", L"错误", MB_ICONERROR);
						// }
					}
                    DestroyWindow(hWait);
                    EnableWindow(hWnd, TRUE);
                    SetActiveWindow(hWnd);

                    allRecords = records;
                    pageIndex = 0;
                    RefreshTable(hTable, allRecords, pageIndex, pageSize);
                    UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
                }else if (LOWORD(wParam) == IDC_BTN_RESET){
                SetWindowTextW(hEditSuite, L"");
                SetWindowTextW(hEditName, L"");
                SetWindowTextW(hEditID, L"");
                SetWindowTextW(hEditScriptID, L"");
                SetWindowTextW(hEditText, L"");

                // Set combo box selection to none (-1 means no selection)
                SendMessageW(hComboModule, CB_SETCURSEL, (WPARAM)-1, 0);

                    EnableWindow(hWnd, FALSE);
                    HWND hWait = ShowWaitSpinner(hWnd);
                    allRecords = g_db->Search();
                    DestroyWindow(hWait);
                    EnableWindow(hWnd, TRUE);
                    SetActiveWindow(hWnd);
                    pageIndex = 0;
                    RefreshTable(hTable, allRecords, pageIndex, pageSize);
                    UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
            }

            else if (LOWORD(wParam) == IDC_EDIT_SEARCH_RICH_CONTENT) {
                if (HIWORD(wParam) == EN_CHANGE) {
                    // User changed input
                    int len = GetWindowTextLengthW(hEditSearchRichEdit);
                    std::wstring searchText(len, L'\0');
                    if (len > 0) {
                        GetWindowTextW(hEditSearchRichEdit, &searchText[0], len + 1);
                    }
                    else {
                        searchText.clear();
                    }

                    lastSearchText = searchText;

                    // Get plain text from RichEdit (without formatting characters)
                    std::wstring currText = GetPlainTextFromRichEdit(hEditContent);

                    // Remove old formatting
                    ClearRichEditFormat(hEditContent);

                    // Find and highlight matches if search text is not empty
                    if (!searchText.empty()) {
                        auto matches = string_util::FindMatches(currText, searchText);
                        HighlightMatches(hEditContent, matches, searchText.length());

                        // Update status bar
                        if (hStatusBar) {
                            std::wstring statusText;
                            if (matches.empty()) {
                                statusText = L"未找到";
                            }
                            else {
                                // statusText = std::to_wstring(matches.size()) + L" F3/Shift+F3 搜索下/上一个";
                                statusText = L"找到 " + std::to_wstring(matches.size()) + L" 个匹配项";
                            }
                            SetWindowTextW(hStatusBar, statusText.c_str());
                        }
                    }
                    else {
                        lastMatches.clear();
                        currentMatchIndex = 0;
                        if (hStatusBar) {
                            SetWindowTextW(hStatusBar, L"Ready");
                        }
                    }
                }
            }

            else if(LOWORD(wParam) == IDC_BTN_NEXTTEXT){
                GoToNextMatch(hEditContent, lastSearchText.length());
            }
            else if(LOWORD(wParam) == IDC_BTN_PREVTEXT){
                GoToPreviousMatch(hEditContent, lastSearchText.length());
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
                        int len = GetWindowTextLengthW(hEditSearchRichEdit);
                        std::wstring searchText(len, L'\0');
                        if (len > 0) {
                            GetWindowTextW(hEditSearchRichEdit, &searchText[0], len + 1);
                            SendMessage(hWnd,
                                        WM_COMMAND,
                                        MAKEWPARAM(IDC_EDIT_SEARCH_RICH_CONTENT, EN_CHANGE),
                                        (LPARAM)searchText.c_str() );

                        }

                    }
                }
            }
            break;
        case WM_CTLCOLORSTATIC: {
            // Uniform background coloring - make labels blend with main window
            // Default static control background appears as light gray and needs adjustment
            HDC hdc = (HDC)wParam;
            HWND hwndStatic = (HWND)lParam;

            // 设置文本颜色（可选）
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

            // 设置背景模式为透明
            SetBkMode(hdc, TRANSPARENT);

            // 返回空画刷实现透明背景
            // return (LRESULT)GetStockObject(NULL_BRUSH);
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        }

        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);
                RECT rc;
                GetClientRect(hWnd, &rc);
                // 填充背景为白色（或你需要的颜色）
                FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
                DrawSplitter(hWnd, state);
                EndPaint(hWnd, &ps);
                return 0;
            }

        case WM_SIZE: {
            LayoutChildren(hWnd, state);
            InvalidateRect(hWnd, nullptr, TRUE);
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

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{


    setlocale(LC_ALL, "chs");
    LoadLibraryW(L"Msftedit.dll"); // RichEdit 4/5

    g_db = new Database("srsautocase.db");

    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);    // 这里加这句，窗口背景为白色
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"myWndClass";
    wc.hIconSm = NULL;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wc);

    // Ensure your parent window (hWnd) is created with these styles. They prevent child windows from painting over each other and minimize flickering.
    // WS_CLIPCHILDREN and WS_CLIPSIBLINGS : 防止拖动splitter时左右界面闪动
    hwnd_ = CreateWindow(wc.lpszClassName, L"用例查找", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN |WS_CLIPSIBLINGS,
        20, 20, ww, wh, nullptr, nullptr, hInst, nullptr);

    ShowWindow(hwnd_, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    delete g_db;
    return 0;
}

LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (msg == WM_KEYDOWN)
    {
        if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
        {
            SendMessage(hwnd, EM_SETSEL, 0, (LPARAM)-1);
            return 0;
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (msg) {
    case WM_KEYDOWN: {
        // Handle F3 for next match, Shift+F3 for previous match
        if (wParam == VK_F3) {
            // Get parent window and retrieve the static variables
            HWND hParent = GetParent(hWnd);
            HWND hRichEdit = GetDlgItem(hParent, IDC_EDIT_CONTENT);

            // Get current search text length
            int len = GetWindowTextLengthW(hWnd);

            if (hRichEdit && len > 0) {
                if (GetKeyState(VK_SHIFT) & 0x8000) {
                    GoToPreviousMatch(hRichEdit, len);
                }
                else {
                    GoToNextMatch(hRichEdit, len);
                }
            }
            return 0;
        }
        break;
    }
    case WM_NCDESTROY: {
        // Remove subclass when control is destroyed
        RemoveWindowSubclass(hWnd, EditSubclassProc, uIdSubclass);
        break;
    }
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}
