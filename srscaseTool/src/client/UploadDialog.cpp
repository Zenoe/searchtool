#include "UploadDialog.h"
#include <commctrl.h>
#include <sstream>
#include "../common/stringutil.h"
// 对话框模板资源ID (定义在resource.h中)
// 实际项目中需要在资源文件中定义对话框模板

UploadDialog::UploadDialog(HWND hParent, HINSTANCE hInstance,
                          const std::wstring& filePath,
                          bool isAutomationCase,
                          bool overwriteExisting)
    : m_hParent(hParent),
      m_hDlg(nullptr),
      m_hInstance(hInstance),
      m_filePath(filePath),
      m_isAutomationCase(isAutomationCase),
      m_overwriteExisting(overwriteExisting),
      m_uploadComplete(false),
      m_uploadSuccess(false)
{
    m_logger = std::make_shared<Logger>("client.log");
    m_csvParser = std::make_shared<CSVParser>(m_logger);
    m_httpClient = std::make_shared<HttpClient>(m_logger);
}

UploadDialog::~UploadDialog() {
    // 如果上传线程仍在运行，等待其完成
    if (m_uploadThread.joinable()) {
        m_uploadThread.join();
    }
}

bool UploadDialog::ShowDialog() {
    // 使用DialogBoxParam创建模态对话框
    // 注意：IDD_UPLOAD_DIALOG应在资源文件中定义

    // 模拟对话框的创建和显示（在实际应用中，使用DialogBoxParam）
    m_logger->Info("Showing upload dialog for file: " + std::string(m_filePath.begin(), m_filePath.end()));

    // 创建对话框，使用DialogBoxParam传递this指针
    INT_PTR result = DialogBoxParam(
        m_hInstance,
        MAKEINTRESOURCE(IDD_UPLOAD_DIALOG),
        m_hParent,
        DialogProc,
        reinterpret_cast<LPARAM>(this)
    );

    return (result == IDOK);
}

INT_PTR CALLBACK UploadDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    UploadDialog* pThis = nullptr;

    if (uMsg == WM_INITDIALOG) {
        // 存储this指针以供后续消息使用
        pThis = reinterpret_cast<UploadDialog*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));

        // 初始化对话框
        pThis->m_hDlg = hwnd;
        pThis->InitializeDialog(hwnd);
        return TRUE;
    }

    pThis = reinterpret_cast<UploadDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (pThis) {
        return pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR UploadDialog::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // 确认按钮 - 开始上传
            StartUpload(hwnd);
            return TRUE;

        case IDCANCEL:
        case IDC_CANCEL:
            // 取消按钮 - 关闭对话框
            if (!m_uploadComplete) {
                // 如果上传正在进行，询问是否真的要取消
                if (MessageBox(hwnd, L"确定要取消上传吗？", L"确认", MB_YESNO | MB_ICONQUESTION) == IDNO) {
                    return TRUE;
                }

                // 这里应该有代码来安全停止上传线程
                // ...
            }

            EndDialog(hwnd, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        // 窗口关闭事件
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

void UploadDialog::InitializeDialog(HWND hwnd) {
    // 设置对话框标题
    std::wstring title = m_isAutomationCase ? L"上传自动化用例" : L"上传测试管理系统用例";
    SetWindowText(hwnd, title.c_str());

    // 获取控件句柄
    HWND hProgress = GetDlgItem(hwnd, IDC_PROGRESS);
    HWND hStatus = GetDlgItem(hwnd, IDC_STATUS);

    // 初始化进度条
    SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hProgress, PBM_SETSTEP, 1, 0);
    SendMessage(hProgress, PBM_SETPOS, 0, 0);

    // 设置初始状态文本
    SetWindowText(hStatus, L"准备上传文件...");

    // 禁用确定按钮，直到上传完成
    EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);

    // 显示文件信息
    std::wstring fileInfo = L"文件: " + m_filePath;
    SetDlgItemText(hwnd, IDC_STATUS, fileInfo.c_str());

    // 自动开始上传
    StartUpload(hwnd);
}

void UploadDialog::StartUpload(HWND hwnd) {
    // 禁用取消按钮，改为"关闭"
    SetDlgItemText(hwnd, IDCANCEL, L"关闭");

    // 启动上传线程
    m_uploadThread = std::thread([this]() {
        try {
            // 更新UI状态
            UpdateProgress(5, L"正在解析CSV文件...");

            // 解析CSV文件
            if (m_isAutomationCase) {
                auto cases = m_csvParser->ParseAutomationCases(m_filePath);

                // 检查是否有有效数据
                if (cases.empty()) {
                    UploadComplete(false, L"CSV文件中没有有效的自动化用例数据");
                    return;
                }

                UpdateProgress(30, L"正在上传 " + string_util::utf8_to_wstring(std::to_string(cases.size())) + L" 条自动化用例...");

                // 上传数据
                std::string serverUrl = "http://localhost:8080"; // 实际应用中应从配置中读取
                bool success = m_httpClient->UploadAutomationCases(
                    serverUrl,
                    cases,
                    m_overwriteExisting,
                    [this](int percent, const std::string& status) {
                        // 传递上传进度回调
                        UpdateProgress(30 + (percent * 70 / 100), string_util::utf8_to_wstring(status));
                    }
                );

                if (success) {
                    UploadComplete(true, L"自动化用例上传成功");
                } else {
                    UploadComplete(false, L"上传失败: " + string_util::utf8_to_wstring(m_httpClient->GetLastError()));
                }
            }
            else {
                auto cases = m_csvParser->ParseManagementCases(m_filePath);

                // 检查是否有有效数据
                if (cases.empty()) {
                    UploadComplete(false, L"CSV文件中没有有效的测试管理系统用例数据");
                    return;
                }

                UpdateProgress(30, L"正在上传 " + string_util::utf8_to_wstring(std::to_string(cases.size())) + L" 条测试管理系统用例...");

                // 上传数据
                std::string serverUrl = "http://localhost:8080"; // 实际应用中应从配置中读取
                bool success = m_httpClient->UploadManagementCases(
                    serverUrl,
                    cases,
                    m_overwriteExisting,
                    [this](int percent, const std::string& status) {
                        // 传递上传进度回调
                        UpdateProgress(30 + (percent * 70 / 100), string_util::utf8_to_wstring(status));
                    }
                );

                if (success) {
                    UploadComplete(true, L"测试管理系统用例上传成功");
                } else {
                    UploadComplete(false, L"上传失败: " + string_util::utf8_to_wstring(m_httpClient->GetLastError()));
                }
            }
        }
        catch (const std::exception& e) {
            UploadComplete(false, L"上传过程中发生错误: " + string_util::utf8_to_wstring(e.what()));
        }
    });
}

void UploadDialog::UpdateProgress(int percentComplete, const std::wstring& status) {
    // 由于这个函数会从工作线程调用，需要使用PostMessage或SendMessage来更新UI

    // 创建一个带有状态消息的堆分配字符串，将在消息处理中释放
    wchar_t* statusCopy = _wcsdup(status.c_str());

    // 使用PostMessage发送自定义消息更新UI
    // 注：WM_APP+1是自定义消息ID，实际应用中应定义常量
PostMessage(m_hDlg, WM_UPDATE_PROGRESS, percentComplete, reinterpret_cast<LPARAM>(statusCopy));

    // 实际更新UI的代码
    if (m_hDlg) {
        // 更新进度条
        SendMessage(GetDlgItem(m_hDlg, IDC_PROGRESS), PBM_SETPOS, percentComplete, 0);

        // 更新状态文本
        std::wstring wideStatus;
        wideStatus.assign(status.begin(), status.end());
        SetDlgItemText(m_hDlg, IDC_STATUS, wideStatus.c_str());
    }
}

void UploadDialog::UploadComplete(bool success, const std::wstring& message) {
    m_uploadComplete = true;
    m_uploadSuccess = success;

    if (m_hDlg) {
        // 使用 _wcsdup 正确复制宽字符
        wchar_t* messageCopy = _wcsdup(message.c_str());
        PostMessage(m_hDlg, WM_UPLOAD_COMPLETE, success ? 1 : 0, reinterpret_cast<LPARAM>(messageCopy));

        // 更新UI
        UpdateProgress(100, message);

        // 更新按钮状态
        EnableWindow(GetDlgItem(m_hDlg, IDOK), FALSE);
        SetDlgItemText(m_hDlg, IDCANCEL, L"关闭");

        // 显示消息框通知用户
        // 不用重复赋值，直接用 message
        if (success) {
            MessageBox(m_hDlg, message.c_str(), L"上传成功", MB_OK | MB_ICONINFORMATION);
        }
        else {
            MessageBox(m_hDlg, message.c_str(), L"上传失败", MB_OK | MB_ICONERROR);
        }
    }
}
//void UploadDialog::UploadComplete(bool success, const std::wstring& message) {
//    m_uploadComplete = true;
//    m_uploadSuccess = success;
//
//    if (m_hDlg) {
//        // 使用PostMessage发送完成消息
//        wchar* messageCopy = _strdup(message.c_str());
//PostMessage(m_hDlg, WM_UPLOAD_COMPLETE, success ? 1 : 0, reinterpret_cast<LPARAM>(messageCopy));
//
//        // 更新UI
//        UpdateProgress(100, message);
//
//        // 更新按钮状态
//        EnableWindow(GetDlgItem(m_hDlg, IDOK), FALSE);
//        SetDlgItemText(m_hDlg, IDCANCEL, L"关闭");
//
//        // 显示消息框通知用户
//        std::wstring wideMessage;
//        wideMessage.assign(message.begin(), message.end());
//
//        if (success) {
//            MessageBox(m_hDlg, wideMessage.c_str(), L"上传成功", MB_OK | MB_ICONINFORMATION);
//        } else {
//            MessageBox(m_hDlg, wideMessage.c_str(), L"上传失败", MB_OK | MB_ICONERROR);
//        }
//    }
//}
