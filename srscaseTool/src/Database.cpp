#include "Database.h"
#include <stdexcept>
#include <sstream>
#include "common/Utils.h"
#include "common/stringutil.h"

Database::Database(const std::string& dbfile) {
    if (sqlite3_open(dbfile.c_str(), &db) != SQLITE_OK)
        throw std::runtime_error("Cannot open database.");
}
Database::~Database() { if (db) sqlite3_close(db); }
const std::vector<std::string> Database::GetAllModules() {
    // If cached, return immediately (document caching behavior!)
    if (!modules_.empty()) return modules_;
    sqlite3_stmt* stmt = nullptr;
	const char* sql = "SELECT DISTINCT COMPOSITONNAME FROM srscase;";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        // Log the error or throw an exception; don't silently fail
        // throw std::runtime_error(sqlite3_errmsg(db));
        return {};
    }

    try {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* ptxt = sqlite3_column_text(stmt, 0);
            modules_.emplace_back(ptxt ? reinterpret_cast<const char*>(ptxt) : "");
        }
    } catch (...) {
        sqlite3_finalize(stmt);
        throw;
    }
    sqlite3_finalize(stmt);

    return modules_;
}
 //const std::vector<std::string> Database::GetAllModules() {
 //    if(!modules_.empty()) return modules_;
 //    sqlite3_stmt* stmt = nullptr;
 //    const char* sql = "SELECT DISTINCT COMPOSITONNAME FROM srscase;";
 //    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
 //        while (sqlite3_step(stmt) == SQLITE_ROW) {
 //            const unsigned char* ptxt = sqlite3_column_text(stmt, 0);
 //            modules_.emplace_back(ptxt ? reinterpret_cast<const char*>(ptxt) : "");
 //        }
 //        sqlite3_finalize(stmt);
 //    }
 //    return modules_;
 //}

bool is_utf8(const std::string& str) {
    size_t i = 0, len = str.size();
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
std::vector<size_t> preprocessBadChar(const std::wstring& pattern) {
    constexpr size_t ALPHABET_SIZE = 65536; // Unicode BMP
    std::vector<size_t> badChar(ALPHABET_SIZE, -1);
    for (size_t i = 0; i < pattern.length(); ++i) {
        badChar[static_cast<size_t>(pattern[i])] = i;
    }
    return badChar;
}

// Good suffix preprocessing
std::vector<size_t> preprocessGoodSuffix(const std::wstring& pattern) {
    size_t m = pattern.length();
    std::vector<size_t> goodSuffix(m + 1, m);
    std::vector<size_t> suffix(m + 1, 0);

    suffix[m] = m;
    int f = 0, g = m - 1;
    for (int i = static_cast<int>(m) - 1; i >= 0; --i) {
        if (i > g && suffix[i + m - f] < i - g) {
            suffix[i] = suffix[i + m - f];
        }
        else {
            g = i;
            f = i;
            while (g >= 0 && pattern[g] == pattern[g + m - f]) {
                --g;
            }
            suffix[i] = f - g;
        }
    }

    for (size_t i = 0; i < m; ++i)
        goodSuffix[m - suffix[i]] = m - i - 1;
    for (size_t i = 1; i <= m; ++i)
        if (goodSuffix[i] == m)
            goodSuffix[i] = goodSuffix[i - 1];

    return goodSuffix;
}

std::vector<size_t> boyerMooreSearch(const std::wstring& text, const std::wstring& pattern) {
    std::vector<size_t> result;
    size_t n = text.length();
    size_t m = pattern.length();
    if (m == 0 || n == 0 || m > n)
        return result;

    auto badChar = preprocessBadChar(pattern);
    auto goodSuffix = preprocessGoodSuffix(pattern);

    size_t s = 0;
    while (s <= n - m) {
        int j = static_cast<int>(m) - 1;
        while (j >= 0 && pattern[j] == text[s + j])
            --j;

        if (j < 0) {
            result.push_back(s); // match found
            s += goodSuffix[0];
        }
        else {
            size_t bcShift = static_cast<size_t>(j) - badChar[static_cast<size_t>(text[s + j])];
            size_t gsShift = goodSuffix[j + 1];
            s += (std::max<size_t>)(1, (std::max)(bcShift, gsShift));
        }
    }
    return result;
}
std::vector<CaseRecord> Database::Search(
    const std::string& suite,
    const std::string& name,
    const std::string& id,
    const std::string& scriptid,
    const std::string& module,
    const std::string& text,
    const bool cbg,
    const bool isExact)
{
    std::vector<CaseRecord> recs;
    std::string sql = "SELECT CASESUITE,CASENAME,CASEID,SCRIPTID,COMPOSITONNAME,REMARK,CASETXTCONTENT FROM srscase WHERE 1=1";

    auto add_cond = [&](const std::string& val, const std::string& colname) {
        if (!val.empty()) {
            sql += isExact
                ? " AND " + colname + " = ?"
                : " AND " + colname + " LIKE ?" ;
        }
    };
    add_cond(suite,      "CASESUITE");
    add_cond(name,       "CASENAME");
    add_cond(id,         "CASEID");
    add_cond(scriptid,   "SCRIPTID");
    add_cond(module,     "COMPOSITONNAME");
    //add_cond(text,       "CASETXTCONTENT");

    if (cbg) {
        sql += " AND EXISTS (SELECT 1 FROM tcmgmt WHERE CASEDESCRIPTION = srscase.SCRIPTID )";
    }

    if (!text.empty()) {
        if(isExact)
            // sql += " AND CASETXTCONTENT LIKE ? COLLATE BINARY";
            // case sensitive
            sql += " AND CASETXTCONTENT GLOB ? COLLATE BINARY";
        else
            sql += " AND CASETXTCONTENT LIKE ?";
    }
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        int idx = 1;

        auto bind_cond = [&](const std::string& val) {
            if (!val.empty()) {
                if (isExact) {
                    sqlite3_bind_text(stmt, idx++, val.c_str(), -1, SQLITE_STATIC);
                } else {
                    std::string fuzzy = "%" + val + "%";
                    // fuzzy.c_str() is not guaranteed to live after the call. must use SQLITE_TRANSIENT
                    sqlite3_bind_text(stmt, idx++, fuzzy.c_str(), -1, SQLITE_TRANSIENT);
                }
            }
        };
        bind_cond(suite);
        bind_cond(name);
        bind_cond(id);
        bind_cond(scriptid);
        bind_cond(module);
        // bind_cond(text);
        if (!text.empty()) {
            if(isExact){
                std::string fuzzy = "*" + text + "*";
                sqlite3_bind_text(stmt, idx++, fuzzy.c_str(), -1, SQLITE_TRANSIENT);
            }else{
                std::string fuzzy = "%" + text + "%";
                sqlite3_bind_text(stmt, idx++, fuzzy.c_str(), -1, SQLITE_TRANSIENT);
            }
        }
        auto safe_get_text = [](sqlite3_stmt* s, int col) -> std::wstring {
            const char* content_utf8 = (const char*)sqlite3_column_text(s, col);
            if (content_utf8 == nullptr) return L"";
            return string_util::utf8_to_wstring(content_utf8);
        };

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            CaseRecord rec;
            rec.CASESUITE         = safe_get_text(stmt, 0);
            rec.CASENAME          = safe_get_text(stmt, 1);
            rec.CASEID            = safe_get_text(stmt, 2);
            rec.SCRIPTID          = safe_get_text(stmt, 3);
            rec.COMPOSITONNAME    = safe_get_text(stmt, 4);
            rec.REMARK            = safe_get_text(stmt, 5);
            rec.CASETXTCONTENT    = safe_get_text(stmt, 6);

            if (!text.empty() && isExact && !rec.CASETXTCONTENT.empty()) {
                std::wstring target = string_util::utf8_to_wstring(text);
                std::wistringstream iss(rec.CASETXTCONTENT);
                std::wstring line;
                int line_no = 1;
                while (std::getline(iss, line)) {
                    if (line.find(target) == std::wstring::npos) {
                        ++line_no;
                        continue;
                    }
                    std::vector<std::wstring> units = string_util::splitByFourSpaces(line);
                    for(const auto & unit: units){
                        if (!boyerMooreSearch( unit, target).empty()) {
                            rec.matched_lines.emplace_back(std::to_wstring(line_no) + L": " + unit);
						}
                    // for(auto unit: units){
                         //if(unit.find(target) != std::wstring::npos){
                         //    // rec.matched_lines.push_back(std::to_wstring(line_no) + L": " + unit);
                         //    rec.matched_lines.emplace_back(std::to_wstring(line_no) + L": " + unit);
                         //}
                    }

                    // push word
                    // std::wstringstream linestream(line);
                    // std::wstring word;
                    // while (linestream >> word) { // splits on whitespace
                    //     if (word.find(target) != std::wstring::npos) {
                    //         rec.matched_lines.push_back(std::to_wstring(line_no) + L":" + word);
                    //     }
                    // }


                    // push tokens (very slow)
                    // std::vector<std::wstring> tokens;
                    // std::wstringstream linestream(line);
                    // std::wstring word;
                    // while (linestream >> word) tokens.push_back(word);
                    // std::vector<std::wstring> target_tokens;
                    // // Split target by whitespace (can use wstringstream too)
                    // std::wstringstream targetstream(target);
                    // while (targetstream >> word) target_tokens.push_back(word);
                    // // Now, for each group in the line, compare target tokens with line tokens
                    // for (size_t i = 0; i + target_tokens.size() <= tokens.size(); ++i) {
                    //     bool match = true;
                    //     for (size_t j = 0; j < target_tokens.size(); ++j) {
                    //         if (tokens[i+j].find(target_tokens[j]) == std::wstring::npos) {
                    //             match = false;
                    //             break;
                    //         }
                    //     }
                    //     if (match) {
                    //         // Join the group tokens and push!
                    //         std::wstring matched;
                    //         for (size_t j = 0; j < target_tokens.size(); ++j) {
                    //             if (j) matched += L"    ";
                    //             matched += tokens[i+j];
                    //         }
                    //         rec.matched_lines.push_back(std::to_wstring(line_no) + L":" + matched);
                    //     }
                    // }

                    // push whole line
                    // if (line.find(target) != std::wstring::npos) {
                    //     rec.matched_lines.push_back(std::to_wstring(line_no) + L":" + line);
                    // }
                    ++line_no;
                }
            }
            recs.push_back(rec);
        }
        sqlite3_finalize(stmt);
    }
    return recs;
}
// std::vector<CaseRecord> Database::Search(const bool isfuzzy, const std::string& suite, const std::string& name, const std::string& id, const std::string& scriptid, const std::string& module, const std::string& text, const bool cbg) {
//     std::vector<CaseRecord> recs;
//     std::string sql = "SELECT CASESUITE,CASENAME,CASEID,SCRIPTID,COMPOSITONNAME,REMARK,CASETXTCONTENT FROM srscase WHERE 1=1";
//     if (!suite.empty()) sql += " AND CASESUITE LIKE ?";
//     if (!name.empty()) sql += " AND CASENAME LIKE ?";
//     if (!id.empty()) sql += " AND CASEID LIKE ?";
//     if (!scriptid.empty()) sql += " AND SCRIPTID LIKE ?";
//     if (!module.empty()) sql += " AND COMPOSITONNAME LIKE ?";
//     if (!text.empty()) sql += " AND CASETXTCONTENT LIKE ?";
//     if (cbg) {
//         // 只查主表用 exists
//         sql += " AND EXISTS (SELECT 1 FROM tcmgmt WHERE CASEDESCRIPTION = srscase.SCRIPTID )";
//         // sql += " JOIN tcmgmt ON tcmgmt.CASEDESCRIPTION = srscase.SCRIPTID";
//     }
//     //bool aa = is_utf8(suite);
//     //sql = u8"SELECT CASESUITE,CASENAME,CASEID,SCRIPTID,COMPOSITONNAME,REMARK,CASETXTCONTENT FROM srscase WHERE 1=1 AND CASESUITE LIKE '%RG-MPLS域-mpls-GN-TP%'";
//     sqlite3_stmt* stmt;
//     if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
//         int idx = 1;
//         // todo SQLITE_STATIC -> SQLITE_TRANSIENT
//         if (!suite.empty()) sqlite3_bind_text(stmt, idx++, suite.c_str(), -1, SQLITE_STATIC);
//         if (!name.empty()) sqlite3_bind_text(stmt, idx++, name.c_str(), -1, SQLITE_STATIC);
//         if (!id.empty()) sqlite3_bind_text(stmt, idx++, id.c_str(), -1, SQLITE_STATIC);
//         if (!scriptid.empty()) sqlite3_bind_text(stmt, idx++, scriptid.c_str(), -1, SQLITE_STATIC);
//         if (!module.empty()) sqlite3_bind_text(stmt, idx++, module.c_str(), -1, SQLITE_STATIC);
//         if (!text.empty()) sqlite3_bind_text(stmt, idx++, text.c_str(), -1, SQLITE_STATIC);

//         auto safe_get_text = [](sqlite3_stmt* s, int col) -> std::wstring {
//             const char* content_utf8 = (const char*)sqlite3_column_text(s, col);
//             if (content_utf8 == nullptr) return L"";
//             return string_util::utf8_to_wstring(content_utf8);
//         };
//         while (sqlite3_step(stmt) == SQLITE_ROW) {
//             CaseRecord rec;
//             rec.CASESUITE = safe_get_text(stmt, 0);
//             rec.CASENAME = safe_get_text(stmt, 1);
//             rec.CASEID = safe_get_text(stmt, 2);
//             rec.SCRIPTID = safe_get_text(stmt, 3);
//             rec.COMPOSITONNAME = safe_get_text(stmt, 4);
//             rec.REMARK = safe_get_text(stmt, 5);
//             rec.CASETXTCONTENT = safe_get_text(stmt, 6);
//             recs.push_back(rec);
//         }
//         sqlite3_finalize(stmt);
//     }
//     return recs;
// }
