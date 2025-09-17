#include "MainWindow.h"
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <richedit.h>
#include <windowsx.h>
// #include <regex>
#include <re2/re2.h>
#include "../common/stringutil.h"
#include "../helper.h"
#include "../common/sysutil.h"

#include "resource.h"
 #include "UploadDialog.h"


#pragma comment(lib, "comctl32.lib")

MainWindow::MainWindow() : m_hwnd(nullptr), m_hInstance(nullptr) {
    m_logger = std::make_shared<Logger>("client.log");
    m_logger->Info("Application starting...");
}

MainWindow::~MainWindow() {
    m_logger->Info("Application shutting down...");
    if(g_db){
        delete g_db;
        g_db = nullptr;
    }
}

bool MainWindow::Initialize(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    LoadLibraryW(L"Msftedit.dll"); // RichEdit 4/5

    if(!g_db) g_db = new Database("srsautocase.db");
    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    // Register window class
    const wchar_t CLASS_NAME[] = L"TestCaseUploaderWindowClass";

    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = CLASS_NAME;
    wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex)) {
        m_logger->Error("Window registration failed");
        return false;
    }
    m_hwnd = CreateWindow(wcex.lpszClassName, L"用例查找", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN |WS_CLIPSIBLINGS,
                         20, 20, ww, wh, nullptr, nullptr, hInstance, this); // this:: aditional data, so pThis can be initialized

    if (!m_hwnd) {
        m_logger->Error("Window creation failed");
        return false;
    }

    // Create menus
    CreateMenus(m_hwnd);

    // Show window
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    m_logger->Info("Window initialized successfully");
    return true;
}

void MainWindow::CreateMenus(HWND hwnd) {
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();

    AppendMenu(hSubMenu, MF_STRING, IDM_IMPORT_AUTOMATION, L"导入自动化用例");
    AppendMenu(hSubMenu, MF_STRING, IDM_IMPORT_MANAGEMENT, L"导入测试管理系统用例");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, L"数据");

    SetMenu(hwnd, hMenu);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<MainWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } 
    else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
    } else {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

LRESULT MainWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
        case IDC_CHECK_EXACT_SEARCH: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            int contentX = state.splitterX + 6 + splitterW;
            int contentWidth = max(100, w - contentX - xs);
            int contentHeight = h - topSectionHeight - eh - ys * 2;
            if (HIWORD(wParam) == BN_CLICKED) {
                BOOL checked = (SendMessage(hCheckExactSearch, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (checked) {
                    // shows up
                    MoveWindow(hEditContentInfo, contentX, topSectionHeight, contentWidth, contentInfoH, false);
                    MoveWindow(hEditContent, contentX, topSectionHeight + contentInfoH, contentWidth, contentHeight - contentInfoH, false);
                }
                else {
                    MoveWindow(hEditContent, contentX, topSectionHeight, contentWidth, contentHeight, true);
                }
                ShowWindow(hEditContentInfo, checked ? SW_SHOW : SW_HIDE);
            }
            break;
        }
		case IDM_IMPORT_AUTOMATION:
			HandleImportAutomationCase();
			return 0;
		case IDM_IMPORT_MANAGEMENT:
			HandleImportManagementCase();
			return 0;
		case IDC_BTN_SEARCH:
			return HandleSearch();
		case IDC_BTN_RESET:
			HandleReset();
			return 0;
		case IDC_BTN_GOTO_PAGE:
			HandleGotoPage();
			return 0;
		case IDC_EDIT_SEARCH_RICH_CONTENT:
			HandleSearchContent(wParam);
			return 0;
		case IDC_BTN_NEXTTEXT:
			GoToNextMatch(hEditContent, lastSearchText.length());
			break;

		case IDC_BTN_PREVTEXT:
			GoToPreviousMatch(hEditContent, lastSearchText.length());
			break;

		case IDC_COMBO_MODULE:
            switch(HIWORD(wParam)){
                case CBN_SELCHANGE:
                    // SendMessage(m_hwnd, WM_COMMAND, (WPARAM)IDC_BTN_SEARCH, 0); // 触发搜索
                    break;
                case CBN_EDITCHANGE:
                    // Get current text from combo edit field
                    TCHAR text[256] = {};
                    GetWindowText(hComboModule, text, 256);
                    // Filter items based on text
                    FilterComboBox(hComboModule,g_db->GetAllModules(), text);
                    break;
            }
			break;

		case IDC_BTN_PREV:
			if (pageIndex > 0) {
				pageIndex--;
				RefreshTable(hTable, allRecords, pageIndex, pageSize,m_searchExact);
				UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
			}
			break;

		case IDC_BTN_NEXT:
			if ((pageIndex + 1) * pageSize < (int)allRecords.size()) {
				pageIndex++;
				RefreshTable(hTable, allRecords, pageIndex, pageSize,m_searchExact);
				UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
			}
			break;
		case IDC_COPY_LISTVIEW: {
			LVITEM lvi = { 0 };
			wchar_t buf[512] = { 0 };
			lvi.iItem = m_lastRow;
			lvi.iSubItem = m_lastCol;
			lvi.mask = LVIF_TEXT;
			lvi.pszText = buf;
			lvi.cchTextMax = 511;

			ListView_GetItem(hTable, &lvi);
			CopyTextToClipboard(m_hwnd, buf);
            break;
		}
		case IDC_COPY_LISTVIEW_ROW: {
			// 获取整行内容
			HWND hHeader = ListView_GetHeader(hTable);
			int colCount = Header_GetItemCount(hHeader);

			std::wstring rowText;
			for (int i = 0; i < colCount; ++i) {
				LVITEM lvi = { 0 };
				wchar_t buf[512] = { 0 };
				lvi.iItem = m_lastRow;
				lvi.iSubItem = i;
				lvi.mask = LVIF_TEXT;
				lvi.pszText = buf;
				lvi.cchTextMax = 511;
				ListView_GetItem(hTable, &lvi);

				if (i > 0) rowText += L"\t"; // tab分隔，或可改为","
				rowText += buf;
			}
			CopyTextToClipboard(m_hwnd, rowText.c_str());
		}

		}
		break;
	case WM_CREATE:
		HandleCreate(hwnd);
		return 0;
    case WM_SIZE:
        return HandleSize();
    case WM_NOTIFY:
        return HandleNotify(lParam);

    case WM_PAINT:
        return HandlePaint();
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
    case WM_LBUTTONDOWN:
        {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            RECT rc;
            GetClientRect(m_hwnd, &rc);

            // Check if in splitter area
            if (mx >= state.splitterX && mx < state.splitterX + SPLITTER_WIDTH)
                {
                    state.dragging = true;
                    SetCapture(m_hwnd);
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
                    GetClientRect(m_hwnd, &rc);
                    int cx = rc.right - rc.left;

                    int newX = mx - state.dragOffset;
                    if (newX < MIN_PANEL_WIDTH) newX = MIN_PANEL_WIDTH;
                    if (newX > cx - MIN_PANEL_WIDTH - SPLITTER_WIDTH)
                        newX = cx - MIN_PANEL_WIDTH - SPLITTER_WIDTH;

                    state.splitterX = newX;
                    LayoutChildren(m_hwnd, state);
                    // Visual feedback
                    InvalidateRect(m_hwnd, nullptr, FALSE);
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
    case WM_UPDATE_PROGRESS:
        {
            // 处理进度更新消息
            int percentComplete = static_cast<int>(wParam);
            char* statusMessage = reinterpret_cast<char*>(lParam);

            // 这里可以更新主窗口中的状态栏或其他进度指示器

            // 释放消息中分配的字符串
            free(statusMessage);
            return 0;
        }

    case WM_UPLOAD_COMPLETE:
        {
            // 处理上传完成消息
            bool success = (wParam != 0);
            char* resultMessage = reinterpret_cast<char*>(lParam);

            // 这里可以更新主窗口的状态

            // 释放消息中分配的字符串
            free(resultMessage);
            return 0;
        }
    
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
void MainWindow::HandleImportAutomationCase() {
	m_logger->Info("Import automation case menu selected");

    // Open file dialog
    OPENFILENAME ofn;
    wchar_t szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        // Show dialog asking if user wants to overwrite existing data
        int result = MessageBox(m_hwnd,
                               L"是否覆盖已存在的相同脚本序号的记录？",
                               L"导入确认",
                               MB_YESNOCANCEL | MB_ICONQUESTION);

        if (result == IDCANCEL) {
            return;
        }

        bool overwrite = (result == IDYES);

        // Upload the file
        UploadFile(szFile, true, overwrite);
    }
}

void MainWindow::HandleImportManagementCase() {
    m_logger->Info("Import management case menu selected");

    // Open file dialog (same as above)
    OPENFILENAME ofn;
    wchar_t szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        // Show dialog asking if user wants to overwrite existing data
        int result = MessageBox(m_hwnd,
                               L"是否覆盖已存在的相同用例编号的记录？",
                               L"导入确认",
                               MB_YESNOCANCEL | MB_ICONQUESTION);

        if (result == IDCANCEL) {
            return;
        }

        bool overwrite = (result == IDYES);

        // Upload the file
        UploadFile(szFile, false, overwrite);
    }
}

LRESULT MainWindow::HandleSearch(){

    BOOL useRegex = (SendMessage(hCheckRegex, BM_GETCHECK, 0, 0) == BST_CHECKED);
    BOOL searchCbg = (SendMessage(hCheckCBG, BM_GETCHECK, 0, 0) == BST_CHECKED);
    m_searchExact = (SendMessage(hCheckExactSearch, BM_GETCHECK, 0, 0) == BST_CHECKED);
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

    HWND hWait = nullptr;
    // SQL LIKE %
    if(!useRegex){
        EnableWindow(m_hwnd, FALSE);
        hWait = ShowWaitSpinner(m_hwnd);

        records = g_db->Search(
            suite.empty() ? "" : suite,
            name.empty() ? "" : name,
            id.empty() ? "" : id,
            scriptid.empty() ? "" : scriptid,
            module.empty() ? "" : module,
            text.empty() ? "" : text,
            searchCbg,
            m_searchExact
        );
	}
	else {
		RE2 reg(text); // accept utf8 param
		if (!reg.ok()) {
			MessageBoxW(m_hwnd, L"正则表达式语法错误", L"错误", MB_ICONERROR);
		}
		else {
            EnableWindow(m_hwnd, FALSE);
            HWND hWait = ShowWaitSpinner(m_hwnd);

            auto rawRecords = g_db->Search(
                                           suite.empty() ? "" : "%" + suite + "%",
                                           name.empty() ? "" : "%" + name + "%",
                                           id.empty() ? "" : "%" + id + "%",
                                           scriptid.empty() ? "" : "%" + scriptid + "%",
                                           module.empty() ? "" : "%" + module + "%",
                                           "", searchCbg, m_searchExact);// 内容字段不先筛
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
        // 	MessageBoxW(m_hwnd, L"正则表达式语法错误", L"错误", MB_ICONERROR);
        // }
    }
    if(hWait) DestroyWindow(hWait);
    EnableWindow(m_hwnd, TRUE);
    SetActiveWindow(m_hwnd);

    allRecords = records;
    pageIndex = 0;
    RefreshTable(hTable, allRecords, pageIndex, pageSize, m_searchExact);
    UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
    return 0;
}

void MainWindow::HandleReset(){
    SetWindowTextW(hEditSuite, L"");
    SetWindowTextW(hEditName, L"");
    SetWindowTextW(hEditID, L"");
    SetWindowTextW(hEditScriptID, L"");
    SetWindowTextW(hEditText, L"");
    SendMessage(hCheckCBG, BM_SETCHECK, BST_UNCHECKED, 0);
    SendMessage(hCheckRegex, BM_SETCHECK, BST_UNCHECKED, 0);
    // Set combo box selection to none (-1 means no selection)
    SendMessageW(hComboModule, CB_SETCURSEL, (WPARAM)-1, 0);

    EnableWindow(m_hwnd, FALSE);
    HWND hWait = ShowWaitSpinner(m_hwnd);
    allRecords = g_db->Search();
    DestroyWindow(hWait);
    EnableWindow(m_hwnd, TRUE);
    SetActiveWindow(m_hwnd);
    pageIndex = 0;
    RefreshTable(hTable, allRecords, pageIndex, pageSize);
    UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
}
void MainWindow::HandleGotoPage(){
                wchar_t buf[12];
                GetWindowTextW(hEditGotoPage, buf, 12);
                int pageTarget = _wtoi(buf);

                int totalPages = (int)allRecords.size() / pageSize + ((int)allRecords.size() % pageSize ? 1 : 0);

                // 合法性判断
                if (pageTarget < 1 || pageTarget > totalPages) {
                    MessageBoxW(m_hwnd, L"页码无效！", L"提示", MB_ICONWARNING);
                    return;
                }
                pageIndex = pageTarget - 1;
                RefreshTable(hTable, allRecords, pageIndex, pageSize, m_searchExact);
                UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
            }
void MainWindow::HandleSearchContent(WPARAM wParam){
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
void MainWindow::HandleCreate(HWND phwnd) {
	InitCommonControlsEx(nullptr);
	int x = xs;
	int row1Y = yt;

	hLblSuite = CreateLabel(phwnd, L"用例包：", x, row1Y, 70, lh, IDC_STATIC_SUITE);
	hEditSuite = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
		searchInputStyle, 0, row1Y, 240, eh, phwnd, (HMENU)IDC_EDIT_SUITE, nullptr, nullptr);
	// hBtnClearCasePkg = CreateWindowExW(0, L"BUTTON", L"X",
	// 	WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_FLAT,
	// 	0, 0, CLEARBTN_W, CLEARBTN_H,
	// 	phwnd, nullptr, GetModuleHandle(nullptr), nullptr);
	//ShowWindow(hBtnClearCasePkg, SW_HIDE);

	hLblName = CreateLabel(phwnd, L"用例名：", x, row1Y, 70, lh, IDC_STATIC_NAME);
	hEditName = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		searchInputStyle, 0, row1Y, 140, eh, phwnd, (HMENU)IDC_EDIT_NAME, nullptr, nullptr);

	hLblID = CreateLabel(phwnd, L"用例ID：", x, row1Y, 70, lh, IDC_STATIC_ID);
	hEditID = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		searchInputStyle, 0, row1Y, 140, eh, phwnd, (HMENU)IDC_EDIT_ID, nullptr, nullptr);
	SendMessage(hEditID, EM_LIMITTEXT, 80, 0);


	hLblScriptID = CreateLabel(phwnd, L"脚本编号：", x, 0, 80, lh, IDC_STATIC_SCRIPTID);
	hEditScriptID = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		searchInputStyle, 0, 0, 160, eh, phwnd, (HMENU)IDC_EDIT_SCRIPTID, nullptr, nullptr);
	SendMessage(hEditScriptID, EM_LIMITTEXT, 80, 0);

	hLblModule = CreateLabel(phwnd, L"模块：", x, 0, 60, lh, IDC_STATIC_MODULE);
    int visible_items = 10; // Only allow 10 items visible in the drop-down
    int combo_height = eh;  // Usually the height of the non-drop part (edit/static)
    int dropdown_height = (visible_items + 1) * eh; // +1 because the combo box part needs its own space

    hComboModule = CreateWindowEx(WS_EX_CLIENTEDGE, L"COMBOBOX", nullptr,
                                  WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL,
                                  // searchInputStyle | CBS_DROPDOWNLIST | WS_VSCROLL |CBS_DROPDOWN,
                                  x + 60, 0, 160, dropdown_height,
                                  phwnd, (HMENU)IDC_COMBO_MODULE, (HINSTANCE)GetWindowLongPtr(phwnd, GWLP_HINSTANCE), nullptr);
	// hComboModule = CreateWindowEx(WS_EX_CLIENTEDGE, L"COMBOBOX", nullptr,
	// 	searchInputStyle | CBS_DROPDOWNLIST, x + 60, 0, 160, eh * 8, phwnd, (HMENU)IDC_COMBO_MODULE, nullptr, nullptr);

	hLblText = CreateLabel(phwnd, L"内容：", x, 0, 60, lh, IDC_STATIC_TEXT);
	hEditText = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		searchInputStyle, x + 60, 0, 220, eh, phwnd, (HMENU)IDC_EDIT_TEXT, nullptr, nullptr);
	hCheckRegex = CreateWindowEx(0, L"BUTTON", L"正则匹配",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		0, 0, 80, 22, phwnd, (HMENU)IDC_CHECK_REGEX, nullptr, nullptr);
	hEditSearchRichEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		searchInputStyle, 0, 0, 220, eh, phwnd, (HMENU)IDC_EDIT_SEARCH_RICH_CONTENT, nullptr, nullptr);
	if (hEditSearchRichEdit) {
		SetWindowSubclass(hEditSearchRichEdit, EditSubclassProc, 0, 0);
	}


	hBtnPrevMatch = CreateWindowEx(0, L"BUTTON", L"<",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 90, eh, phwnd, (HMENU)IDC_BTN_PREVTEXT, nullptr, nullptr);
	hBtnNextMatch = CreateWindowEx(0, L"BUTTON", L">",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 90, eh, phwnd, (HMENU)IDC_BTN_NEXTTEXT, nullptr, nullptr);
	int row2Y = row1Y + eh + ys;
	x = xs;
	hBtnSearch = CreateWindowEx(0, L"BUTTON", L"搜索",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, row2Y, 100, eh, phwnd, (HMENU)IDC_BTN_SEARCH, nullptr, nullptr);

	hBtnReset = CreateWindowEx(0, L"BUTTON", L"重置",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, row2Y, 100, eh, phwnd, (HMENU)IDC_BTN_RESET, nullptr, nullptr);
    hCheckCBG = CreateWindowEx(0, L"BUTTON", L"CBG SRS",
                                 WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                 0, 0, 80, 22, phwnd, (HMENU)IDC_CHECK_CBG, nullptr, nullptr);
    hCheckExactSearch = CreateWindowEx(0, L"BUTTON", L"精确查找",
                                 WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                 0, 0, 80, 22, phwnd, (HMENU)IDC_CHECK_EXACT_SEARCH, nullptr, nullptr);

	// 表格（LISTVIEW） - 调整位置和大小
	int top_height = row2Y + eh + ys + 10;  // 增加更多间距
	int tableWidth = 700;  // 增加表格宽度
	int tableHeight = 360; // 增加表格高度

	hTable = CreateWindowEx(0, WC_LISTVIEW, nullptr,
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		xs, top_height, tableWidth, tableHeight, phwnd, (HMENU)IDC_LISTVIEW, nullptr, nullptr);
	ListView_SetExtendedListViewStyle(hTable, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    hTool = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
              WS_POPUP | TTS_ALWAYSTIP,
              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        phwnd, nullptr, nullptr, nullptr);
    TOOLINFO ti = { sizeof(ti) };
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = phwnd;
    ti.uId  = (UINT_PTR)hTable;
    ti.lpszText = LPSTR_TEXTCALLBACK; // 动态文本
    SendMessage(hTool, TTM_ADDTOOL, 0, (LPARAM)&ti);

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

    ListView_SetColumnWidth(hTable, 3, 248);
	// 右侧内容区 - 调整位置和大小
	int contentX = xs + tableWidth + xs;  // 与表格保持间距
	int contentWidth = 340;

	hEditContentInfo = CreateWindowExW(0, MSFTEDIT_CLASS, L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
		ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
		contentX, top_height, contentWidth, tableHeight,
		phwnd, (HMENU)IDC_EDIT_CONTENTINFO, nullptr, nullptr);

    ShowWindow(hEditContentInfo, m_searchExact ? SW_SHOW : SW_HIDE);
	hEditContent = CreateWindowExW(0, MSFTEDIT_CLASS, L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
		ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
		contentX, top_height, contentWidth, tableHeight,
		phwnd, (HMENU)IDC_EDIT_CONTENT, nullptr, nullptr);

	// hEditContent = CreateWindowEx(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, nullptr,
	//                               WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
	//                               contentX, top_height, contentWidth, tableHeight, phwnd, (HMENU)IDC_EDIT_CONTENT, nullptr, nullptr);

	// 分页控件+按钮 - 调整位置
	int bt_y = top_height + tableHeight + 15;  // 增加间距
	hBtnPrev = CreateWindowEx(0, L"BUTTON", L"上一页",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, xs, bt_y, 90, eh, phwnd, (HMENU)IDC_BTN_PREV, nullptr, nullptr);
	hBtnNext = CreateWindowEx(0, L"BUTTON", L"下一页",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, xs + 100, bt_y, 90, eh, phwnd, (HMENU)IDC_BTN_NEXT, nullptr, nullptr);

	hEditGotoPage = CreateWindowEx(0, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
		0, 0, 40, 22, phwnd, (HMENU)IDC_EDIT_GOTO_PAGE, nullptr, nullptr);

	SendMessage(hEditGotoPage, EM_LIMITTEXT, 6, 0); // 限制最大6位数字
	SendMessage(hEditGotoPage, WM_SETFONT, (WPARAM)GetModernFont(), TRUE);

	hBtnGotoPage = CreateWindowEx(0, L"BUTTON", L"跳页",
		WS_CHILD | WS_VISIBLE,
		0, 0, 48, 22, phwnd, (HMENU)IDC_BTN_GOTO_PAGE, nullptr, nullptr);


	hPageLabel = CreateWindowEx(0, L"STATIC", L"0/0页",
		WS_CHILD | WS_VISIBLE | SS_CENTER, xs + 200, bt_y, pageLabelW, eh, phwnd, NULL, nullptr, nullptr);

	hStatusBar = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT,
		0, 0, 0, 0,
		phwnd, nullptr, nullptr, nullptr);
	// Font
	HFONT modernFont = GetModernFont();
	for (HWND h : {hLblSuite, hEditSuite, hLblName, hEditName, hLblID, hEditID, hLblScriptID, hEditScriptID,
		hLblModule, hComboModule, hLblText, hEditText, hCheckRegex, hCheckCBG, hCheckExactSearch, hBtnSearch, hBtnReset,
		hBtnPrev, hBtnNext, hPageLabel, hTable, hEditContent, hEditContentInfo, hBtnNextMatch, hBtnPrevMatch, hStatusBar, hBtnGotoPage, hEditGotoPage}) {
		SendMessage(h, WM_SETFONT, (WPARAM)modernFont, TRUE);
	}
	for (HWND h : { hEditSuite, hEditName, hEditID, hEditScriptID, hEditText, hEditSearchRichEdit}) {
		SetWindowSubclass(h, MainWindow::EditProc, 0, 0);
	}
	// Load module options
	auto modules = g_db->GetAllModules();
	PopulateModules(hComboModule, modules);

	// Initial 查询
	pageIndex = 0;
	// allRecords = g_db->Search("", "", "", "", "", "");
	// RefreshTable(hTable, allRecords, pageIndex, pageSize);
	// UpdatePageStatus(hPageLabel, pageIndex, (int)allRecords.size(), pageSize);
}

LRESULT MainWindow::HandleSize(){
    LayoutChildren(m_hwnd, state);
    InvalidateRect(m_hwnd, nullptr, TRUE);
    return 0;
}

LRESULT MainWindow::HandleNotify(LPARAM lParam){
    LPNMHDR pnmh = (LPNMHDR)lParam;
    if (pnmh->idFrom == IDC_LISTVIEW) {
        // ------- 处理选中变更 ----------
        if (pnmh->code == LVN_ITEMCHANGED) {
            NMLISTVIEW* pnm = (NMLISTVIEW*)lParam;
            if (pnm->uNewState & LVIS_SELECTED) {
                int idx = pageIndex * pageSize + pnm->iItem;
                if (idx >= 0 && idx < (int)allRecords.size()) {
                    SetWindowTextW(hEditContent, std::wstring(
                                                              allRecords[idx].CASETXTCONTENT.begin(), allRecords[idx].CASETXTCONTENT.end()).c_str());

                    std::wstring joined = string_util::joinWstrings(allRecords[idx].matched_lines);
                    SetWindowTextW(hEditContentInfo, joined.c_str());
                    int len = GetWindowTextLengthW(hEditSearchRichEdit);
                    std::wstring searchText(len, L'\0');
                    if (len > 0) {
                        GetWindowTextW(hEditSearchRichEdit, &searchText[0], len + 1);
                        SendMessage(m_hwnd, WM_COMMAND,
                                    MAKEWPARAM(IDC_EDIT_SEARCH_RICH_CONTENT, EN_CHANGE),
                                    (LPARAM)searchText.c_str());
                    }
                }
            }
        }else if(pnmh->code == NM_RCLICK){
            // ListView 右键点击通知

            // 获取点击坐标
            DWORD pos = GetMessagePos();
            POINT pt;
            pt.x = GET_X_LPARAM(pos);
            pt.y = GET_Y_LPARAM(pos);

            // 触发弹菜单函数
            ShowListViewContextMenu(hTable, m_hwnd, pt);
        }else if(pnmh->code == NM_CUSTOMDRAW){
            // give listview alternating row colors
            NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
            switch(cd->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                // Request item painting notifications
                return CDRF_NOTIFYITEMDRAW;
            case CDDS_ITEMPREPAINT: {
                // Alternate color per row
                int idx = (int)cd->nmcd.dwItemSpec;
                COLORREF bk = (idx & 1) ? RGB(255, 255, 255) : RGB(245, 245, 240); // Change colors as desired
                cd->clrTextBk = bk;
                // Optional: darker text for selected rows
                if (cd->nmcd.uItemState & CDIS_SELECTED) {
                    // cd->clrTextBk = RGB(200, 220, 255);      // selected background
                    cd->clrText   = RGB(0, 0, 80);           // darker text
                } else {
                    cd->clrText   = RGB(20, 20, 20);         // normal text
                }
                return CDRF_NEWFONT; // Use new background
            }
            }
        }

        // ------- 处理高亮自定义 ---------- 无效
        // else if (pnmh->code == NM_CUSTOMDRAW) {
        //     LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lParam;
        //     switch (cd->nmcd.dwDrawStage) {
        //     case CDDS_PREPAINT:
        //         return CDRF_NOTIFYITEMDRAW;
        //     case CDDS_ITEMPREPAINT: {
        //         BOOL selected = (ListView_GetItemState(cd->nmcd.hdr.hwndFrom, cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED);
        //         if (selected)
        //             cd->clrTextBk = RGB(80,160,250); // 深色高亮
        //         else
        //             cd->clrTextBk = RGB(255,255,255); // 普通底色
        //         return CDRF_DODEFAULT;
        //     }
        //     }
        // }
    }
    if (((LPNMHDR)lParam)->code == TTN_GETDISPINFO || ((LPNMHDR)lParam)->code == TTN_NEEDTEXT) {
        NMTTDISPINFO* pdi = (NMTTDISPINFO*)lParam;
        // 取鼠标当前项和列
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hTable, &pt);
        LVHITTESTINFO hit = {0};
        hit.pt = pt;
        int iItem = ListView_HitTest(hTable, &hit);
        if (iItem >= 0) {
            wchar_t buf[1024];
            ListView_GetItemText(hTable, iItem, hit.iSubItem, buf, ARRAYSIZE(buf));
            pdi->lpszText = buf; // 可以包含'\n'，ToolTip自动多行
            return TRUE;
        }
    }
    return 0;
}
LRESULT MainWindow::HandlePaint(){
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    // 填充背景为白色（或你需要的颜色）
    FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
    DrawSplitter(m_hwnd, state);
    EndPaint(m_hwnd, &ps);
    return 0;
}
void MainWindow::ShowListViewContextMenu(HWND hList, HWND hParentWnd, POINT ptScreen) {
    // 屏幕坐标 => 客户坐标
    POINT ptClient = ptScreen;
    ScreenToClient(hList, &ptClient);

    // 找到行
    LVHITTESTINFO ht = {0};
    ht.pt = ptClient;
    int row = ListView_HitTest(hList, &ht);
    if (row < 0) return; // 没命中行

    // 找列
    int col = 0;
    int x = ptClient.x;
    HWND hHeader = ListView_GetHeader(hList);
    int colCount = Header_GetItemCount(hHeader);
    int xStart = 0;

    for (int i = 0; i < colCount; ++i) {
        int colWidth = ListView_GetColumnWidth(hList, i);
        if (x < xStart + colWidth) {
            col = i;
            break;
        }
        xStart += colWidth;
    }

    // 记录当前点中行列（建议加到全局，或静态变量，或窗口成员变量）
    m_lastRow = row;
    m_lastCol = col;

    // 弹唯一菜单
    HMENU menu = CreatePopupMenu();
    InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING, IDC_COPY_LISTVIEW, L"复制");
    InsertMenu(menu, 1, MF_BYPOSITION | MF_STRING, IDC_COPY_LISTVIEW_ROW, L"复制行");

    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, ptScreen.x, ptScreen.y, 0, hParentWnd, NULL);
    DestroyMenu(menu);
}
void MainWindow::LayoutChildren(HWND hWnd, SplitterState& state) {
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
        HWND label, control, clearBtn;
        int labelWidth, controlWidth;
    };
    RECT oldRect;
    GetWindowRect(hLblSuite, &oldRect);
    InvalidateRect(hWnd, &oldRect, TRUE);

    GetWindowRect(hBtnSearch, &oldRect);
    InvalidateRect(hWnd, &oldRect, TRUE);
    ControlGroup row1Controls[] = {
        // {hLblSuite, hEditSuite, hBtnClearCasePkg, 80, 180},
        {hLblSuite, hEditSuite, nullptr, 80, 180},
        {hLblName, hEditName, nullptr, 80, 180},
        {hLblID, hEditID, nullptr, 80, 180},
        {hLblScriptID, hEditScriptID,nullptr,  100, 160},
        {hLblModule, hComboModule,nullptr,  60, 160},
        {hLblText, hEditText,nullptr,  60, 220}
    };

    int x = xs;
    for (const auto& group : row1Controls) {
        if (!deferMove(group.label, x, row1Y, group.labelWidth, lh)) break;
        if (!deferMove(group.control, x + group.labelWidth, row1Y, group.controlWidth, eh)) break;
        if(group.clearBtn){
            int btnx = x + group.labelWidth + group.controlWidth - CLEARBTN_W;
            if (!deferMove(group.clearBtn, btnx, row1Y, CLEARBTN_W, CLEARBTN_H)) break;
            SetWindowPos(group.clearBtn, HWND_TOP, btnx, row1Y, CLEARBTN_W, CLEARBTN_H, SWP_SHOWWINDOW);
        }
        x += group.labelWidth + group.controlWidth + xs;
    }

    deferMove(hCheckRegex, x-xs, row1Y, 90, 22);
    // Second row
    x = xs;
    deferMove(hBtnSearch, x, row2Y, 100, eh);
    deferMove(hBtnReset, x + 100 + padding, row2Y, 100, eh);
    deferMove(hCheckCBG, x + 200 + ( padding << 1 ), row2Y, 100, eh);
    deferMove(hCheckExactSearch, x + 300 + (padding * 3), row2Y, 100, eh);

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
    if (m_searchExact) {
		deferMove(hEditContentInfo, contentX, topSectionHeight, contentWidth, contentInfoH);
		deferMove(hEditContent, contentX, topSectionHeight + contentInfoH, contentWidth, contentHeight- contentInfoH);
    }
    else {
		deferMove(hEditContent, contentX, topSectionHeight, contentWidth, contentHeight);
    }


    deferMove(hEditSearchRichEdit,contentX, topSectionHeight + contentHeight, 200, eh );
    deferMove(hBtnPrevMatch,contentX + 200 + padding, topSectionHeight + contentHeight, 30, eh );
    deferMove(hBtnNextMatch,contentX + 200 +  padding  + 30, topSectionHeight + contentHeight, 30, eh );
    deferMove(hStatusBar,contentX + 200 +  ( padding *2  ) +  60 , topSectionHeight + contentHeight, 400, eh );

    // Pagination controls
    deferMove(hBtnPrev, xs, buttonY, 90, eh);
    deferMove(hBtnNext, xs + 100, buttonY, 90, eh);
    deferMove(hEditGotoPage, xs + 100 + 90, buttonY, 40, eh);
    deferMove(hBtnGotoPage, xs + 190 + 40, buttonY, 48, eh);
    deferMove(hPageLabel, xs + 230+48+padding, buttonY, pageLabelW, eh);

    // Apply all moves at once
    EndDeferWindowPos(hdwp);
}
bool MainWindow::UploadFile(const std::wstring& filePath, bool isAutomationCase, bool overwriteExisting) {
    // Create and show upload dialog
    UploadDialog uploadDlg(m_hwnd, m_hInstance, filePath, isAutomationCase, overwriteExisting);
    return uploadDlg.ShowDialog();
}
LRESULT CALLBACK MainWindow::EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (msg == WM_KEYDOWN)
    {
        if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
        {
            SendMessage(hwnd, EM_SETSEL, 0, (LPARAM)-1);
            return 0;
        }
    }
    else if (msg == WM_LBUTTONDBLCLK)
        {
            SendMessage(hwnd, EM_SETSEL, 0, (LPARAM)-1);      // Select all
            return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK MainWindow::EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
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
