#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <windows.h>
#include <winhttp.h>
#include "../common/Models.h"
#include "../common/Logger.h"

#pragma comment(lib, "winhttp.lib")

class HttpClient {
public:
    HttpClient(std::shared_ptr<Logger> logger);
    ~HttpClient();

    // Callback for progress updates
    using ProgressCallback = std::function<void(int percentComplete, const std::string& status)>;

    // Upload methods
    bool UploadAutomationCases(
        const std::string& serverUrl,
        const std::vector<AutomationTestCase>& cases,
        bool overwrite,
        ProgressCallback progressCallback);

    bool UploadManagementCases(
        const std::string& serverUrl,
        const std::vector<TestManagementCase>& cases,
        bool overwrite,
        ProgressCallback progressCallback);

    // Get last error message
    std::string GetLastError() const { return m_lastError; }

private:
    // Internal HTTP request helper
    bool SendHttpRequest(
        const std::string& url,
        const std::string& method,
        const std::string& body,
        std::string& response,
        ProgressCallback progressCallback);

    // JSON serialization
    std::string SerializeAutomationCases(const std::vector<AutomationTestCase>& cases, bool overwrite);
    std::string SerializeManagementCases(const std::vector<TestManagementCase>& cases, bool overwrite);

    // String conversion utilities

    // Error handling
    void SetLastError(const std::string& error);

    std::shared_ptr<Logger> m_logger;
    std::string m_lastError;
};
