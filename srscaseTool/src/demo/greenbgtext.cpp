#define UNICODE
#define _UNICODE
#include <windows.h>

const COLORREF EDIT_BK_COLOR = RGB(144, 238, 144);
HBRUSH hEditBkBrush;
HWND hEdit;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
        {
            // Create Edit Control
            hEdit = CreateWindowEx(
                0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER,
                10, 10, 300, 25,
                hwnd, (HMENU)100, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            return 0;
        }

        case WM_CTLCOLOREDIT:
        {
            HDC hdcEdit = (HDC)wParam;
            HWND hwndEdit = (HWND)lParam;
            if(hwndEdit == hEdit)
            {
                SetBkColor(hdcEdit, EDIT_BK_COLOR);
                // Set the text color if you want, for visibility:
                // SetTextColor(hdcEdit, RGB(0,0,0));
                return (INT_PTR)hEditBkBrush;
            }
            break;
        }

        case WM_DESTROY:
            if(hEditBkBrush) DeleteObject(hEditBkBrush);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow)
{
    // Prepare custom brush
    hEditBkBrush = CreateSolidBrush(EDIT_BK_COLOR);

    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"MainClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        wc.lpszClassName, L"Edit Control Custom BG",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 340, 80, 0, 0, hInst, 0
    );

    MSG msg;
    while(GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
