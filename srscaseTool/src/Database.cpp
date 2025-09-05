#include "Database.h"
#include <stdexcept>

#include "stringutil.h"

Database::Database(const std::string& dbfile) {
    if (sqlite3_open(dbfile.c_str(), &db) != SQLITE_OK)
        throw std::runtime_error("Cannot open database.");
}
Database::~Database() { if (db) sqlite3_close(db); }

std::vector<std::string> Database::GetAllModules() {
    std::vector<std::string> modules;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT DISTINCT COMPOSITONNAME FROM srscase;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* ptxt = sqlite3_column_text(stmt, 0);
            modules.emplace_back(ptxt ? reinterpret_cast<const char*>(ptxt) : "");
        }
        sqlite3_finalize(stmt);
    }
    return modules;
}

bool is_utf8(const std::string& str) {
    int i = 0, len = str.size();
    while (i < len) {
        unsigned char c = str[i];
        if ((c & 0x80) == 0) { // 0xxxxxxx, ASCII
            i += 1;
        }
        else if ((c & 0xE0) == 0xC0) { // 110xxxxx, 2 bytes
            if (i + 1 >= len || (str[i + 1] & 0xC0) != 0x80)
                return false;
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0) { // 1110xxxx, 3 bytes
            if (i + 2 >= len || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80)
                return false;
            i += 3;
        }
        else if ((c & 0xF8) == 0xF0) { // 11110xxx, 4 bytes
            if (i + 3 >= len || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80 || (str[i + 3] & 0xC0) != 0x80)
                return false;
            i += 4;
        }
        else {
            return false;
        }
    }
    return true;
}

std::vector<CaseRecord> Database::Search(const std::string& suite, const std::string& name, const std::string& id, const std::string& scriptid, const std::string& module, const std::string& text) {
    std::vector<CaseRecord> recs;
    std::string sql = "SELECT CASESUITE,CASENAME,CASEID,SCRIPTID,COMPOSITONNAME,REMARK,CASETXTCONTENT FROM srscase WHERE 1=1";
    if (!suite.empty()) sql += " AND CASESUITE LIKE ?";
    if (!name.empty()) sql += " AND CASENAME LIKE ?";
    if (!id.empty()) sql += " AND CASEID LIKE ?";
    if (!scriptid.empty()) sql += " AND SCRIPTID LIKE ?";
    if (!module.empty()) sql += " AND COMPOSITONNAME LIKE ?";
    if (!text.empty()) sql += " AND CASETXTCONTENT LIKE ?";

    //bool aa = is_utf8(suite);
    //sql = u8"SELECT CASESUITE,CASENAME,CASEID,SCRIPTID,COMPOSITONNAME,REMARK,CASETXTCONTENT FROM srscase WHERE 1=1 AND CASESUITE LIKE '%RG-MPLS域-mpls-GN-TP%'";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        int idx = 1;
        // todo SQLITE_STATIC -> SQLITE_TRANSIENT
        if (!suite.empty()) sqlite3_bind_text(stmt, idx++, suite.c_str(), -1, SQLITE_STATIC);
        if (!name.empty()) sqlite3_bind_text(stmt, idx++, name.c_str(), -1, SQLITE_STATIC);
        if (!id.empty()) sqlite3_bind_text(stmt, idx++, id.c_str(), -1, SQLITE_STATIC);
        if (!scriptid.empty()) sqlite3_bind_text(stmt, idx++, scriptid.c_str(), -1, SQLITE_STATIC);
        if (!module.empty()) sqlite3_bind_text(stmt, idx++, module.c_str(), -1, SQLITE_STATIC);
        if (!text.empty()) sqlite3_bind_text(stmt, idx++, text.c_str(), -1, SQLITE_STATIC);

        auto safe_get_text = [](sqlite3_stmt* s, int col) -> std::wstring {
            const char* content_utf8 = (const char*)sqlite3_column_text(s, col);
            if (content_utf8 == nullptr) return L"";
            return string_util::utf8_to_wstring(content_utf8);
        };
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            CaseRecord rec;
            rec.CASESUITE = safe_get_text(stmt, 0);
            rec.CASENAME = safe_get_text(stmt, 1);
            rec.CASEID = safe_get_text(stmt, 2);
            rec.SCRIPTID = safe_get_text(stmt, 3);
            rec.COMPOSITONNAME = safe_get_text(stmt, 4);
            rec.REMARK = safe_get_text(stmt, 5);
            rec.CASETXTCONTENT = safe_get_text(stmt, 6);
            recs.push_back(rec);
        }
        sqlite3_finalize(stmt);
    }
    return recs;
}
