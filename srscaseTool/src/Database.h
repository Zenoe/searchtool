#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>

struct CaseRecord {
    std::wstring CASESUITE, CASENAME, CASEID, SCRIPTID, COMPOSITONNAME, REMARK, CASETXTCONTENT;
};

class Database {
public:
    Database(const std::string& dbfile);
    ~Database();
    std::vector<std::string> GetAllModules();
    std::vector<CaseRecord> Search(const std::string&, const std::string&, const std::string&, const std::string&, const std::string&, const std::string&);

private:
    sqlite3* db = nullptr;
};
