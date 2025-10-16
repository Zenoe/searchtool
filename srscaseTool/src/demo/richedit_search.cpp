#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <strsafe.h>
#include <random>
#include <string>
#include <vector>
#include <fstream>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comctl32.lib")
// #pragma comment(lib, "Msftedit.lib")

#define IDC_RICHEDIT 1001
#define IDC_EDIT    1002

const wchar_t szClassName[] = L"RichEditSearchApp";

// Read whole file into std::string (raw bytes)
std::string read_file_to_string(const std::wstring& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return {}; // error
    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    return std::string(buffer.begin(), buffer.end());
}

// Convert UTF-8 string to wstring (UTF-16) using MultiByteToWideChar
std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) return {};
    int sz = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
    std::wstring wstr(sz, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], sz);
    return wstr;
}

// Complete function:
std::wstring read_text_file_to_wstring(const std::wstring& filename) {
    std::string buffer = read_file_to_string(filename);
    return utf8_to_wstring(buffer);
}
std::wstring GenerateRandomText(size_t lines, size_t linelen) {
    std::wstring result;
    std::mt19937 rng(static_cast<unsigned>(time(0)));
    std::uniform_int_distribution<int> charDist('a', 'z');
    std::uniform_int_distribution<int> spaceDist(0, 8);

    for (size_t l = 0; l < lines; ++l) {
        for (size_t k = 0; k < linelen; ++k) {
            if (spaceDist(rng) == 0)
                result += L' ';
            else
                result += static_cast<wchar_t>(charDist(rng));
        }
        result += L"\r\n";
    }
    return result;
}

// Find all (possibly overlapping) matches of searchText in content.
std::vector<size_t> FindMatches(const std::wstring& content, const std::wstring& searchText) {
    std::vector<size_t> matches;
    if (searchText.empty())
        return matches;
    size_t pos = 0;
    while ((pos = content.find(searchText, pos)) != std::wstring::npos) {
        matches.push_back(pos);
        ++pos; // For overlapping matches
    }
    return matches;
}

// Remove all character formatting (restore default).
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

// Highlight all matches in red
void HighlightMatches(HWND hRichEdit, const std::vector<size_t>& matchPositions, size_t matchLen) {
    for (size_t pos : matchPositions) {
        CHARRANGE range = {(LONG)pos, (LONG)(pos + matchLen)};

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hRichEdit = nullptr;
    static HWND hEdit = nullptr;
    static std::wstring richText;

    switch (msg) {
    case WM_CREATE: {
        // Load RichEdit library
        HMODULE hRichEditLib = LoadLibraryW(L"Msftedit.dll");
        if (!hRichEditLib) {
            MessageBoxW(hWnd, L"Failed to load Msftedit.dll", L"Error", MB_ICONERROR);
            return -1;
        }

        RECT rc;
        GetClientRect(hWnd, &rc);

        // Create search edit box
        hEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                rc.left + 10, rc.top + 10, rc.right - 20, 24,
                                hWnd, (HMENU)IDC_EDIT, nullptr, nullptr);

        // Create RichEdit control
        hRichEdit = CreateWindowExW(0, MSFTEDIT_CLASS, L"",
                                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
                                    ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, // Read-only to prevent formatting changes
                                    rc.left + 10, rc.top + 40, rc.right - 20, rc.bottom - 50,
                                    hWnd, (HMENU)IDC_RICHEDIT, nullptr, nullptr);

        if (!hRichEdit) {
            MessageBoxW(hWnd, L"Failed to create RichEdit control", L"Error", MB_ICONERROR);
            return -1;
        }

        // Set font
        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
        if (hFont) {
            SendMessageW(hRichEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        // Generate and set random text
        // richText = GenerateRandomText(50, 60);
        richText = read_text_file_to_wstring(L"text.txt");
        SetWindowTextW(hRichEdit, richText.c_str());

        return 0;
    }
    case WM_SIZE: {
        if (hEdit && hRichEdit) {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            MoveWindow(hEdit, 10, 10, w-20, 24, TRUE);
            MoveWindow(hRichEdit, 10, 40, w-20, h-50, TRUE);
        }
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDC_EDIT && HIWORD(wParam) == EN_CHANGE) {
            // User changed input
            int len = GetWindowTextLengthW(hEdit);
            std::wstring searchText(len, L'\0');
            if (len > 0) {
                GetWindowTextW(hEdit, &searchText[0], len + 1);
            } else {
                searchText.clear();
            }

            // Get plain text from RichEdit (without formatting characters)
            std::wstring currText = GetPlainTextFromRichEdit(hRichEdit);

            // Remove old formatting
            ClearRichEditFormat(hRichEdit);

            // Find and highlight matches if search text is not empty
            if (!searchText.empty()) {
                auto matches = FindMatches(currText, searchText);
                HighlightMatches(hRichEdit, matches, searchText.length());
            }
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = szClassName;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(0, szClassName, L"RichEdit Incremental Search (C++17)",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
                                nullptr, nullptr, hInst, nullptr);

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
