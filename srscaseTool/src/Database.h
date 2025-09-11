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
    std::vector<CaseRecord> Search(
                                   const std::string& arg1 = "",
                                   const std::string& arg2 = "",
                                   const std::string& arg3 = "",
                                   const std::string& arg4 = "",
                                   const std::string& arg5 = "",
                                   const std::string& arg6 = "",
                                   const bool cbg = false
                                  );

private:
    sqlite3* db = nullptr;
};
