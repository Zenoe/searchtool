#include "HttpClient.h"
#include <nlohmann/json.hpp>
#include <codecvt>
#include <locale>
#include <sstream>
#include "../common/stringutil.h"

using json = nlohmann::json;

HttpClient::HttpClient(std::shared_ptr<Logger> logger) : m_logger(logger) {
    m_logger->Info("HttpClient initialized");
}

HttpClient::~HttpClient() {
}

bool HttpClient::UploadAutomationCases(
    const std::string& serverUrl,
    const std::vector<AutomationTestCase>& cases,
    bool overwrite,
    ProgressCallback progressCallback)
{
    if (cases.empty()) {
        SetLastError("No automation cases to upload");
        return false;
    }

    m_logger->Info("Uploading " + std::to_string(cases.size()) + " automation cases to " + serverUrl);

    std::string requestBody = SerializeAutomationCases(cases, overwrite);
    std::string responseData;

    // Send request to server
    std::string url = serverUrl + "/api/upload/automation";
    bool success = SendHttpRequest(url, "POST", requestBody, responseData, progressCallback);

    if (!success) {
        return false;
    }

    // Parse response
    try {
        json response = json::parse(responseData);
        bool uploadSuccess = response["success"].get<bool>();

        if (uploadSuccess) {
            int processed = response["records_processed"].get<int>();
            int inserted = response["records_inserted"].get<int>();
            int updated = response["records_updated"].get<int>();
            int skipped = response["records_skipped"].get<int>();

            std::string message = "Upload completed: " +
                std::to_string(processed) + " records processed, " +
                std::to_string(inserted) + " inserted, " +
                std::to_string(updated) + " updated, " +
                std::to_string(skipped) + " skipped";

            m_logger->Info(message);

            if (progressCallback) {
                progressCallback(100, message);
            }

            return true;
        }
        else {
            std::string errorMsg = response["message"].get<std::string>();
            SetLastError("Server error: " + errorMsg);
            return false;
        }
    }
    catch (const std::exception& e) {
        SetLastError("Failed to parse server response: " + std::string(e.what()));
        return false;
    }
}

bool HttpClient::UploadManagementCases(
    const std::string& serverUrl,
    const std::vector<TestManagementCase>& cases,
    bool overwrite,
    ProgressCallback progressCallback)
{
    if (cases.empty()) {
        SetLastError("No management cases to upload");
        return false;
    }

    m_logger->Info("Uploading " + std::to_string(cases.size()) + " management cases to " + serverUrl);

    std::string requestBody = SerializeManagementCases(cases, overwrite);
    std::string responseData;

    // Send request to server
    std::string url = serverUrl + "/api/upload/management";
    bool success = SendHttpRequest(url, "POST", requestBody, responseData, progressCallback);

    if (!success) {
        return false;
    }

    // Parse response (similar to UploadAutomationCases)
    try {
        json response = json::parse(responseData);
        bool uploadSuccess = response["success"].get<bool>();

        if (uploadSuccess) {
            int processed = response["records_processed"].get<int>();
            int inserted = response["records_inserted"].get<int>();
            int updated = response["records_updated"].get<int>();
            int skipped = response["records_skipped"].get<int>();

            std::string message = "Upload completed: " +
                std::to_string(processed) + " records processed, " +
                std::to_string(inserted) + " inserted, " +
                std::to_string(updated) + " updated, " +
                std::to_string(skipped) + " skipped";

            m_logger->Info(message);

            if (progressCallback) {
                progressCallback(100, message);
            }

            return true;
        }
        else {
            std::string errorMsg = response["message"].get<std::string>();
            SetLastError("Server error: " + errorMsg);
            return false;
        }
    }
    catch (const std::exception& e) {
        SetLastError("Failed to parse server response: " + std::string(e.what()));
        return false;
    }
}

bool HttpClient::SendHttpRequest(
    const std::string& url,
    const std::string& method,
    const std::string& body,
    std::string& response,
    ProgressCallback progressCallback)
{
    // Parse URL
    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(URL_COMPONENTS);
    urlComp.dwSchemeLength = -1;
    urlComp.dwHostNameLength = -1;
    urlComp.dwUrlPathLength = -1;

    std::wstring wideUrl = string_util::utf8_to_wstring(url);
    if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.length()), 0, &urlComp)) {
        SetLastError("Failed to parse URL: " + url);
        return false;
    }

    // Extract components
    std::wstring scheme(urlComp.lpszScheme, urlComp.dwSchemeLength);
    std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);
    std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);

    // Initialize WinHTTP
    HINTERNET hSession = WinHttpOpen(
        L"TestCaseUploader/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession) {
        SetLastError("Failed to initialize HTTP session");
        return false;
    }

    // Connect to server
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        host.c_str(),
        urlComp.nPort,
        0);

    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        SetLastError("Failed to connect to server: " + std::string(host.begin(), host.end()));
        return false;
    }

    // Create request
    DWORD flags = (scheme == L"https") ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        string_util::utf8_to_wstring(method).c_str(),
        path.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        SetLastError("Failed to create HTTP request");
        return false;
    }

    // Set headers
    if (!WinHttpAddRequestHeaders(
        hRequest,
        L"Content-Type: application/json",
        -1,
        WINHTTP_ADDREQ_FLAG_ADD)) {

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        SetLastError("Failed to set request headers");
        return false;
    }

    // Send request
    if (progressCallback) {
        progressCallback(10, "Connecting to server...");
    }

    BOOL result = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        body.empty() ? NULL : const_cast<char*>(body.c_str()),
        body.empty() ? 0 : static_cast<DWORD>(body.length()),
        body.empty() ? 0 : static_cast<DWORD>(body.length()),
        0);

    if (!result) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        SetLastError("Failed to send HTTP request");
        return false;
    }

    if (progressCallback) {
        progressCallback(30, "Request sent, waiting for response...");
    }

    // Receive response
    result = WinHttpReceiveResponse(hRequest, NULL);
    if (!result) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        SetLastError("Failed to receive HTTP response");
        return false;
    }

    if (progressCallback) {
        progressCallback(50, "Response received, processing data...");
    }

    // Check status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(DWORD);

    if (!WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        NULL,
        &statusCode,
        &statusCodeSize,
        NULL)) {

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        SetLastError("Failed to query HTTP status code");
        return false;
    }

    if (statusCode != 200) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        SetLastError("HTTP error: " + std::to_string(statusCode));
        return false;
    }

    // Read response data
    DWORD bytesAvailable = 0;
    DWORD bytesRead = 0;
    std::string responseBuffer;

    do {
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            SetLastError("Error querying data available");
            return false;
        }

        if (bytesAvailable == 0) {
            break;
        }

        char* buffer = new char[bytesAvailable + 1];
        if (!buffer) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            SetLastError("Out of memory");
            return false;
        }

        ZeroMemory(buffer, bytesAvailable + 1);

        if (!WinHttpReadData(
            hRequest,
            buffer,
            bytesAvailable,
            &bytesRead)) {

            delete[] buffer;
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            SetLastError("Error reading HTTP response");
            return false;
        }

        responseBuffer.append(buffer, bytesRead);
        delete[] buffer;

        if (progressCallback) {
            progressCallback(50 + (responseBuffer.length() * 40 / (responseBuffer.length() + bytesAvailable)),
                           "Downloading response data...");
        }

    } while (bytesAvailable > 0);

    // Clean up
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (progressCallback) {
        progressCallback(90, "Processing server response...");
    }

    response = responseBuffer;
    return true;
}

std::string HttpClient::SerializeAutomationCases(const std::vector<AutomationTestCase>& cases, bool overwrite) {
    json root;
    root["overwrite"] = overwrite;
    root["data_type"] = "automation";

    json casesArray = json::array();
    for (const auto& testCase : cases) {
        json caseObj;
        caseObj["script_id"] = testCase.script_id;
        caseObj["module_name"] = testCase.module_name;
        caseObj["case_description"] = testCase.case_description;
        caseObj["tc_rf_submit_time"] = testCase.tc_rf_submit_time;
        caseObj["case_package_name"] = testCase.case_package_name;
        caseObj["case_id"] = testCase.case_id;
        caseObj["case_name"] = testCase.case_name;
        caseObj["case_level"] = testCase.case_level;
        caseObj["test_requirement"] = testCase.test_requirement;
        caseObj["physical_interface_type"] = testCase.physical_interface_type;
        caseObj["initial_port_status"] = testCase.initial_port_status;
        caseObj["test_topology_name"] = testCase.test_topology_name;
        caseObj["dut_count"] = testCase.dut_count;
        caseObj["tester_type"] = testCase.tester_type;
        caseObj["port_count"] = testCase.port_count;
        caseObj["test_duration"] = testCase.test_duration;
        caseObj["department"] = testCase.department;
        caseObj["designer"] = testCase.designer;
        caseObj["component"] = testCase.component;
        caseObj["component_domain"] = testCase.component_domain;
        caseObj["script_type"] = testCase.script_type;
        caseObj["process_name"] = testCase.process_name;
        caseObj["fixed_testbed"] = testCase.fixed_testbed;
        caseObj["source_project"] = testCase.source_project;
        caseObj["audit_result"] = testCase.audit_result;
        caseObj["execution_status"] = testCase.execution_status;
        caseObj["instock_process_status"] = testCase.instock_process_status;
        caseObj["instock_process_time"] = testCase.instock_process_time;

        casesArray.push_back(caseObj);
    }

    root["cases"] = casesArray;
    return root.dump();
}

std::string HttpClient::SerializeManagementCases(const std::vector<TestManagementCase>& cases, bool overwrite) {
    json root;
    root["overwrite"] = overwrite;
    root["data_type"] = "management";

    json casesArray = json::array();
    for (const auto& testCase : cases) {
        json caseObj;
        caseObj["case_id"] = testCase.case_id;
        caseObj["test_step"] = testCase.test_step;
        caseObj["test_type"] = testCase.test_type;
        caseObj["case_package_name"] = testCase.case_package_name;
        caseObj["case_name"] = testCase.case_name;
        caseObj["case_description"] = testCase.case_description;
        caseObj["case_tag"] = testCase.case_tag;
        caseObj["case_owner"] = testCase.case_owner;
        caseObj["script_name"] = testCase.script_name;
        caseObj["script_id"] = testCase.script_id;
        caseObj["script_status"] = testCase.script_status;

        casesArray.push_back(caseObj);
    }

    root["cases"] = casesArray;
    return root.dump();
}


void HttpClient::SetLastError(const std::string& error) {
    m_lastError = error;
    m_logger->Error(error);
}
