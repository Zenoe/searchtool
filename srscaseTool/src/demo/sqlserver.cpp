// cpp file should be saved in utf-16
// cl.exe /EHsc /std:c++17 sqlserver.cpp odbc32.lib
#include <windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Unicode ODBC diagnostic
void checkRet(SQLRETURN ret, SQLHANDLE handle, SQLSMALLINT type, const std::wstring& msg) {
    if(ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        std::wcerr << L"Error: " << msg << std::endl;
        SQLWCHAR sqlState[256], msgText[1024];
        SQLINTEGER nativeError;
        SQLSMALLINT textLength;
        SQLGetDiagRecW(type, handle, 1, sqlState, &nativeError, msgText, 1024, &textLength);
        std::wcerr << L"SQLState: " << sqlState << L", Msg: " << msgText << std::endl;
        exit(1);
    }
}

int wmain() {
    SQLHENV henv = nullptr;
    SQLHDBC hdbc = nullptr;
    SQLHSTMT hstmt = nullptr;
    SQLRETURN ret;

    // Init ODBC environment
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
    checkRet(ret, henv, SQL_HANDLE_ENV, L"Alloc ENV");

    ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    checkRet(ret, henv, SQL_HANDLE_ENV, L"Set Env Attr");

    // Init connection
    ret = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    checkRet(ret, hdbc, SQL_HANDLE_DBC, L"Alloc DBC");

    std::wstring connStr =
        L"DRIVER={ODBC Driver 17 for SQL Server};"
        L"SERVER=172.29.106.90,1433;"
        L"DATABASE=SWProDebug;"
        L"UID=sa;"
        L"PWD=·1234567890-=autotest;"
        L"Encrypt=no;"
        L"TrustServerCertificate=yes;"
        L"Connection Timeout=15;";

    SQLWCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen;
    ret = SQLDriverConnectW(hdbc, NULL,
        (SQLWCHAR*)connStr.c_str(), SQL_NTS,
        outConnStr, sizeof(outConnStr)/sizeof(SQLWCHAR), &outConnStrLen,
        SQL_DRIVER_NOPROMPT);
    checkRet(ret, hdbc, SQL_HANDLE_DBC, L"Driver Connect");

    // Open files (wifstream/wofstream for Unicode)
    std::wifstream infile(L"data/PJ43CaseId.txt");
    std::wofstream outfile(L"output.csv", std::ios::app);

    if(!infile || !outfile) {
        std::wcerr << L"File open error." << std::endl;
        return 1;
    }

    // For each line in PJ43CaseId.txt
    std::wstring caseid, idstr;
    while (std::getline(infile, caseid)) {
        // Trim whitespace
        caseid.erase(0, caseid.find_first_not_of(L" \t\r\n"));
        caseid.erase(caseid.find_last_not_of(L" \t\r\n")+1);

        if(caseid.empty())
            continue;
// /12.5SPJ43-客户规格测试-OLAF/X86-AC/RG-三层路由域-BGP4+-ADMINDIST-93293-GN-001(BGP自动发现路由管理距离配置验证)_X86_结果(FAIL)_用时(6)_20250802093047_851007415.txt
        // Prepare the SQL query (wide version!)
        std::wstringstream ss;
        ss << L"SELECT TOP 1 ID FROM TEST_CASE_DATA_LOG "
              L"WHERE PROJECT_ID = '7AFAEA50-BCF2-F190-6CA3-3A1AC2ED5BC9' "
              // L"AND ANALYST_LOG LIKE '%93293%'"
              // L"AND ANALYST_LOG LIKE N'%RG-三层路由域-BGP4+-ADMINDIST-93293-GN-001%'"
            L"AND ANALYST_LOG LIKE N'%RG-三层路由域-BGP4+-ADMINDIST-93293-GN-001%' "
              // L"AND ANALYST_LOG LIKE '%" << caseid << L"%' "
              L"ORDER BY CREATE_TIME DESC";

        std::wstring sql = ss.str();
        std::wcout << L"searching: " << ss.str() << std::endl;
        // Allocate statement
        if (hstmt) SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        ret = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
        checkRet(ret, hstmt, SQL_HANDLE_STMT, L"Alloc STMT");

        // Execute (wide version)
        ret = SQLExecDirectW(hstmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if(ret != SQL_SUCCESS && ret != SQL_NO_DATA) {
            std::wcerr << L"SQL error: " << sql << " " << ret << std::endl;
            continue; // skip error
        }

        // Fetch result (wide)
        SQLWCHAR idbuf[128];
        SQLLEN ind = 0;
        idstr.clear();

        ret = SQLFetch(hstmt);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            ret = SQLGetData(hstmt, 1, SQL_C_WCHAR, idbuf, sizeof(idbuf), &ind);
            if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
                idstr = idbuf;
            }
        }
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        hstmt = nullptr;
        std::wcout << L"result: " << idstr.c_str()<< std::endl;

        // Write to CSV (quoted, wide)
        outfile << L"\"" << caseid << L"\",\"" << idstr << L"\"\n";
    }

    // Cleanup
    if (hstmt) SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    if (hdbc) SQLDisconnect(hdbc);
    if (hdbc) SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    if (henv) SQLFreeHandle(SQL_HANDLE_ENV, henv);

    return 0;
}
