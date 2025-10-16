// SplitView.cpp: Win32 App with ListView, Splitter, RichEdit (drag splitter)
// C++17, Unicode
// cl.exe /EHsc /std:c++17 splitter.cpp  /D_UNICODE /link Comctl32.lib user32.lib gdi32.lib
#define UNICODE
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <string>

#pragma comment(lib, "comctl32.lib")

const int SPLITTER_WIDTH = 4;
const int MIN_PANEL_WIDTH = 80;

enum ChildIDs
{
    IDC_LISTVIEW = 101,
    IDC_RICHEDIT = 102
};

struct SplitterState
{
    int splitterX = 250;        // Initial splitter position
    bool dragging = false;
    int dragOffset = 0;         // Mouse offset from edge
};

// Forward Prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void LayoutChildren(HWND hwnd, SplitterState& state)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    int cx = rc.right - rc.left;
    int cy = rc.bottom - rc.top;

    int splitterX = state.splitterX;
    // Bounds restrict
    if (splitterX < MIN_PANEL_WIDTH) splitterX = MIN_PANEL_WIDTH;
    if (splitterX > cx - MIN_PANEL_WIDTH - SPLITTER_WIDTH)
        splitterX = cx - MIN_PANEL_WIDTH - SPLITTER_WIDTH;

    HWND hList = GetDlgItem(hwnd, IDC_LISTVIEW);
    HWND hEdit = GetDlgItem(hwnd, IDC_RICHEDIT);

    MoveWindow(hList, 0, 0, splitterX, cy, TRUE);
    MoveWindow(hEdit, splitterX + SPLITTER_WIDTH, 0, cx - splitterX - SPLITTER_WIDTH, cy, TRUE);
}

void DrawSplitter(HWND hwnd, SplitterState& state)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    int cy = rc.bottom - rc.top;
    HDC hdc = GetDC(hwnd);

    // Draw splitter as a dark vertical bar
    RECT splitter = {state.splitterX, 0, state.splitterX + SPLITTER_WIDTH, cy};
    HBRUSH hBrush = CreateSolidBrush(RGB(180,180,180));
    FillRect(hdc, &splitter, hBrush);
    DeleteObject(hBrush);

    ReleaseDC(hwnd, hdc);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    // Common controls: ListView
    INITCOMMONCONTROLSEX icce = { sizeof(icce), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icce);

    // Load Richedit DLL
    LoadLibraryW(L"Msftedit.dll");

    // Register window class
    const wchar_t CLASS_NAME[] = L"SplitViewWin32CXX";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"SplitView: ListView | Splitter | RichEdit",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        nullptr, nullptr, hInst, nullptr);

    if (!hwnd) return 0;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return int(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static SplitterState state;
    switch (msg)
    {
    case WM_CREATE:
    {
        RECT rc;
        GetClientRect(hwnd, &rc);

        // ListView
        // ListView
        HWND hList = CreateWindowEx(
                                    0, WC_LISTVIEW, L"",
                                    WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_EDITLABELS,
                                    0, 0, state.splitterX, rc.bottom - rc.top,
                                    hwnd, (HMENU)IDC_LISTVIEW, GetModuleHandleW(nullptr), nullptr);

        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.pszText = (LPWSTR)L"Column";
        col.cx = 100;
        ListView_InsertColumn(hList, 0, &col);

        // Add some items
        for (int i = 0; i < 20; ++i)
          {
            wchar_t buf[40];
            wsprintfW(buf, L"Item #%d", i+1);
            LVITEMW item = {};
            item.mask = LVIF_TEXT;
            item.iItem = i;
            item.pszText = buf;
            ListView_InsertItem(hList, &item);
          }

        // RichEdit
        HWND hEdit = CreateWindowEx(
            0, MSFTEDIT_CLASS, L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_SAVESEL | ES_WANTRETURN,
            state.splitterX + SPLITTER_WIDTH, 0,
            rc.right - state.splitterX - SPLITTER_WIDTH, rc.bottom - rc.top,
            hwnd, (HMENU)IDC_RICHEDIT, GetModuleHandleW(nullptr), nullptr);

        SetWindowTextW(hEdit, L"This is a Microsoft RichEdit control.\r\n\nDrag the splitter to resize panels.");

        // Set font for RichEdit
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

        return 0;
    }
    case WM_SIZE:
        LayoutChildren(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        DrawSplitter(hwnd, state);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        int mx = GET_X_LPARAM(lp);
        int my = GET_Y_LPARAM(lp);
        RECT rc;
        GetClientRect(hwnd, &rc);

        // Check if in splitter area
        if (mx >= state.splitterX && mx < state.splitterX + SPLITTER_WIDTH)
        {
            state.dragging = true;
            SetCapture(hwnd);
            state.dragOffset = mx - state.splitterX;
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        int mx = GET_X_LPARAM(lp);

        if (state.dragging)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int cx = rc.right - rc.left;

            int newX = mx - state.dragOffset;
            if (newX < MIN_PANEL_WIDTH) newX = MIN_PANEL_WIDTH;
            if (newX > cx - MIN_PANEL_WIDTH - SPLITTER_WIDTH)
                newX = cx - MIN_PANEL_WIDTH - SPLITTER_WIDTH;

            state.splitterX = newX;
            LayoutChildren(hwnd, state);

            // Visual feedback
            InvalidateRect(hwnd, nullptr, FALSE);
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
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
}
