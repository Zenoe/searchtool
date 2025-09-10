#pragma once

#include <windows.h>

#include <sstream>
#include <vector>
#include "Database.h"

static HWND hwnd_, hLblSuite, hEditSuite, hLblName, hEditName, hLblID, hEditID, hLblScriptID, hEditScriptID, hLblModule, hComboModule, hLblText, hEditText;
static HWND hTable, hEditContent, hBtnPrev, hBtnNext, hBtnSearch, hPageLabel, hSplitter;
static HWND hEditSearchRichEdit, hBtnReset, hStatusBar;
static HWND hCheckRegex;
static HWND hBtnPrevMatch, hBtnNextMatch;
Database* g_db = nullptr;
static std::vector<CaseRecord> allRecords; // 搜索完整数据

// 分页按钮和标签控件ID
int pageIndex = 0;      // 当前页码

static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

template <typename... Args>
void MyDebug(Args&&... args) {
#ifdef _DEBUG
    std::wstringstream ss;
    (ss << ... << args);     // Fold expression (C++17)
    ss << L"\n";
    OutputDebugStringW(ss.str().c_str());
#endif
}

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
