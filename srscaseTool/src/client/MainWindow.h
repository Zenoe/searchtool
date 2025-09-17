#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include "../Database.h"
#include "../common/Logger.h"

#include "../constant.h"

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
private:
    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void CreateMenus(HWND hwnd);
    void HandleImportAutomationCase();
    void HandleImportManagementCase();
    LRESULT HandleSearch();
    void HandleReset();
    void HandleGotoPage();
    void HandleSearchContent(WPARAM wParam);


    void ShowListViewContextMenu(HWND hList, HWND hParentWnd, POINT pt);
    void LayoutChildren(HWND hWnd, SplitterState& state);
    void HandleCreate(HWND hWnd);
    LRESULT HandleSize();
    LRESULT HandlePaint();
    LRESULT HandleNotify(LPARAM lParam);
    bool UploadFile(const std::wstring& filePath, bool isAutomationCase, bool overwriteExisting);

    HWND m_hwnd;

    HWND hLblSuite, hEditSuite, hLblName, hEditName, hLblID, hEditID, hLblScriptID, hEditScriptID, hLblModule, hComboModule, hLblText, hEditText;
    HWND hTable, hEditContent, hEditContentInfo, hBtnPrev, hBtnNext, hBtnSearch, hPageLabel, hSplitter;
    HWND hEditSearchRichEdit, hBtnReset, hStatusBar;
    HWND hCheckRegex;
    HWND hBtnPrevMatch, hBtnNextMatch, hEditGotoPage, hBtnGotoPage;
    HWND hCheckCBG, hCheckExactSearch;
    HWND hBtnClearCasePkg;
    HWND hTool;

    HINSTANCE m_hInstance;
    std::shared_ptr<Logger> m_logger;

	int pageIndex = 0;      // 当前页码
	Database* g_db = nullptr;
	std::vector<CaseRecord> allRecords; // 搜索完整数据
    SplitterState state;

    std::wstring lastSearchText;
    int m_lastRow = -1;
    int m_lastCol = -1;
    bool m_searchExact = false;
    static constexpr int IDM_IMPORT_AUTOMATION = 101;
    static constexpr int IDM_IMPORT_MANAGEMENT = 102;
};
