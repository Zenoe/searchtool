// cl /EHsc /D_UNICODE /DUNICODE /W4 /Zi /MD Win32ComboFilter.cpp user32.lib gdi32.lib

#include <windows.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype> // For towlower

constexpr int IDC_COMBO_MODULE = 1001;

// Sample items
std::vector<std::wstring> itemList = {
    L"Apple", L"Orange", L"Grape", L"Banana", L"Avocado",
    L"Pineapple", L"Pear", L"Peach", L"Plum", L"Kiwi", L"Strawberry", L"Watermelon", L"Mango", L"Lemon", L"Lime"
};

HWND hComboModule = nullptr;

void FilterComboBox(HWND hCombo, const std::wstring& filter) {
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
        // Case insensitive search - check if filter is substring of item
        if (filter.empty()) {
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)item.c_str());
        } else {
            // Convert both strings to lowercase for case-insensitive comparison
            std::wstring itemLower = item;
            std::wstring filterLower = filter;
            std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), towlower);
            std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), towlower);

            if (itemLower.find(filterLower) != std::wstring::npos) {
                SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)item.c_str());
            }
        }
    }

    // Restore edit text and cursor position
    SetWindowText(hCombo, currText);
    SendMessage(hCombo, CB_SETEDITSEL, 0, MAKELPARAM(startPos, endPos));

    SendMessage(hCombo, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hCombo, nullptr, TRUE); // Redraw

    // Show dropdown list if there are matching items
    int itemCount = SendMessage(hCombo, CB_GETCOUNT, 0, 0);
    if (itemCount > 0) {
        // Show the dropdown list
        SendMessage(hCombo, CB_SHOWDROPDOWN, TRUE, 0);
    } else {
        // Hide dropdown if no items match
        SendMessage(hCombo, CB_SHOWDROPDOWN, FALSE, 0);
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Create editable combo box (CBS_DROPDOWN)
        int x = 10, y = 10, width = 200, height = 100;
        hComboModule = CreateWindowEx(
            WS_EX_CLIENTEDGE, L"COMBOBOX", nullptr,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL,
            x, y, width, height,
            hwnd, (HMENU)IDC_COMBO_MODULE,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), nullptr
        );

        // Set font
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(hComboModule, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Add all items initially
        for (const auto& item : itemList) {
            SendMessage(hComboModule, CB_ADDSTRING, 0, (LPARAM)item.c_str());
        }

        break;
    }
    case WM_SIZE: {
        // Resize combo box when window is resized
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        SetWindowPos(hComboModule, nullptr, 10, 10, rcClient.right - 20, 100, SWP_NOZORDER);
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_COMBO_MODULE) {
            switch (HIWORD(wParam)) {
            case CBN_EDITCHANGE: {
                // Get current text from combo edit field
                TCHAR text[256] = {};
                GetWindowText(hComboModule, text, 256);

                // Filter items based on text
                FilterComboBox(hComboModule, text);
                break;
            }
            case CBN_SETFOCUS: {
                // Show dropdown when combo gets focus
                int itemCount = SendMessage(hComboModule, CB_GETCOUNT, 0, 0);
                if (itemCount > 0) {
                    SendMessage(hComboModule, CB_SHOWDROPDOWN, TRUE, 0);
                }
                break;
            }
            case CBN_KILLFOCUS: {
                // Hide dropdown when losing focus
                SendMessage(hComboModule, CB_SHOWDROPDOWN, FALSE, 0);
                break;
            }
            }
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"ComboFilterWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc)) {
        MessageBox(nullptr, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"Combo Box Type-To-Filter Example",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
        nullptr, nullptr, hInst, nullptr
    );

    if (hwnd == nullptr) {
        MessageBox(nullptr, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
