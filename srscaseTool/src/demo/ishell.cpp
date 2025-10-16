// cl ishell.cpp /EHsc /DUNICODE /D_UNICODE /link Ole32.lib Shell32.lib Comctl32.lib user32.lib
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <string>
#include <commctrl.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

void invoke_folder_verb(HWND hwnd, const std::wstring& folder_path) {
    PIDLIST_ABSOLUTE pidlFolder = nullptr;
    if (SUCCEEDED(SHParseDisplayName(folder_path.c_str(), nullptr, &pidlFolder, 0, nullptr))) {
        IShellFolder* psfParent = nullptr;
        LPCITEMIDLIST pidlChild = nullptr;
        if (SUCCEEDED(SHBindToParent(pidlFolder, IID_IShellFolder, (void**)&psfParent, &pidlChild))) {
            IContextMenu* pcmFolder = nullptr;
            if (SUCCEEDED(psfParent->GetUIObjectOf(hwnd, 1, &pidlChild, IID_IContextMenu, nullptr, (void**)&pcmFolder))) {
                HMENU hMenu = CreatePopupMenu();
                if (hMenu) {
                    if (SUCCEEDED(pcmFolder->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL))) {
                        CMINVOKECOMMANDINFOEX cmi = { 0 };
                        cmi.cbSize = sizeof(cmi);
                        cmi.fMask = CMIC_MASK_UNICODE;
                        cmi.hwnd = hwnd;
                        cmi.lpVerb = "open";
                        cmi.lpVerbW = L"open";
                        cmi.nShow = SW_SHOWNORMAL;

                        // 使用完整路径而不是相对路径
                        std::wstring full_path = folder_path;
                        if (full_path.back() != L'\\') {
                            full_path += L'\\';
                        }

                        cmi.lpDirectoryW = full_path.c_str();
                        pcmFolder->InvokeCommand((LPCMINVOKECOMMANDINFO)&cmi);
                    }
                    DestroyMenu(hMenu);
                }
                pcmFolder->Release();
            }
            psfParent->Release();
        }
        CoTaskMemFree(pidlFolder);
    }
}

#define IDC_BUTTON_OPEN_FOLDER 1001

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowW(L"BUTTON", L"Open Folder",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 20, 150, 30,
            hwnd, (HMENU)IDC_BUTTON_OPEN_FOLDER, GetModuleHandle(NULL), NULL);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON_OPEN_FOLDER) {
            // 使用更可靠的路径，确保文件夹存在
            WCHAR path[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, 0, path))) {
                invoke_folder_verb(hwnd, path);
            } else {
                MessageBoxW(hwnd, L"Failed to get Windows folder path", L"Error", MB_ICONERROR);
            }
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    // 初始化 COM 为 STA
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(NULL, L"COM initialization failed", L"Error", MB_ICONERROR);
        return 1;
    }

    // 初始化通用控件
    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    const wchar_t CLASS_NAME[] = L"FolderVerbDemoClass";

    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(CLASS_NAME, L"Folder Verb Demo",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 120,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        CoUninitialize();
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}
