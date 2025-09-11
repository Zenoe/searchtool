#include <windows.h>
#include "MainWindow.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize COM for the file dialogs
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBox(NULL, L"COM initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create and initialize the main window
    MainWindow mainWindow;
    if (!mainWindow.Initialize(hInstance, nCmdShow)) {
        MessageBox(NULL, L"Application initialization failed", L"Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return 1;
    }

    // Main message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}
