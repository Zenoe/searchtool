#pragma once
#include <windows.h>
#include <string>
#include <memory>
#include <thread>
#include "resource.h"
#include "CSVParser.h"
#include "HttpClient.h"
#include "../common/Logger.h"

class UploadDialog {
public:
    UploadDialog(HWND hParent, HINSTANCE hInstance,
                const std::wstring& filePath,
                bool isAutomationCase,
                bool overwriteExisting);
    ~UploadDialog();

    bool ShowDialog();

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void InitializeDialog(HWND hwnd);
    void StartUpload(HWND hwnd);
    void UpdateProgress(int percentComplete, const std::wstring& status);
    void UploadComplete(bool success, const std::wstring& message);

    HWND m_hParent;
    HWND m_hDlg;
    HINSTANCE m_hInstance;

    std::wstring m_filePath;
    bool m_isAutomationCase;
    bool m_overwriteExisting;

    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<CSVParser> m_csvParser;
    std::shared_ptr<HttpClient> m_httpClient;

    std::thread m_uploadThread;
    bool m_uploadComplete;
    bool m_uploadSuccess;

    //static constexpr int IDD_UPLOAD_DIALOG = 100;
    //static constexpr int IDC_PROGRESS = 101;
    //static constexpr int IDC_STATUS = 102;
    //static constexpr int IDC_CANCEL = 103;
};
