#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <memory>
#include "../common/Models.h"
#include "../common/Logger.h"

class CSVParser {
public:
    CSVParser(std::shared_ptr<Logger> logger);
    ~CSVParser();

    std::vector<AutomationTestCase> ParseAutomationCases(const std::wstring& filePath);
    std::vector<TestManagementCase> ParseManagementCases(const std::wstring& filePath);

private:
    std::vector<std::vector<std::string>> ReadCSV(const std::wstring& filePath);

    std::shared_ptr<Logger> m_logger;
};
