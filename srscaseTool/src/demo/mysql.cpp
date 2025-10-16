// cl.exe /EHsc /std:c++17 /I"C:\portable\mysql-connector-c++-8.0.20-winx64\include" /I"C:\cpplib\boost_1_88_0" mysql.cpp /link /LIBPATH:"C:\portable\mysql-connector-c++-8.0.20-winx64\lib64\vs14" mysqlcppconn.lib
#include <mysql/jdbc.h>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <regex>

int main() {
    const std::string host = "10.110.185.45:3306";
    const std::string user = "root";
    const std::string passwd = "Password123@mysq";
    const std::string db = "rfcbb";

    try {
        sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
        std::unique_ptr<sql::Connection> con(driver->connect(host, user, passwd));
        con->setSchema(db);

        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT name, arguments FROM testcase_keyword_table"));

        std::ofstream outfile("output.txt");

        std::string current_key;
        std::vector<std::string> values;
        std::regex cli_regex("^cli.*");
        bool first_log_encountered = false;

        while (res->next()) {
            std::string name = res->getString("name");
            std::string arguments = res->getString("arguments");

            if (name == "log") {
                // Output previous key-value pair if any
                if (first_log_encountered && !current_key.empty() && !values.empty()) {
                    outfile << current_key << "\n";
                    for (size_t i = 0; i < values.size(); ++i) {
                        if (i != 0) outfile << ",";
                        outfile << values[i];
                    }
                    outfile << "\n\n";
                    // outfile << std::endl;
                }
                // Prepare for new key
                current_key = arguments;
                values.clear();
                first_log_encountered = true;
            } else {
                if (std::regex_match(name, cli_regex)) {
                    values.push_back(arguments);
                } else if (name == "dc_conf") {
                    values.push_back("end");
                    values.push_back("enable");
                    values.push_back("configure terminal");
                    values.push_back(arguments);
                }
            }
        }

        // Write the last key-value pair if any
        if (!current_key.empty() && !values.empty()) {
            outfile << current_key << "\n";
            for (size_t i = 0; i < values.size(); ++i) {
                if (i != 0) outfile << ",";
                outfile << values[i];
            }
            outfile << "\n\n";
        }

        outfile.close();
        std::cout << "Data extraction complete. Output written to output.txt" << std::endl;

    } catch (sql::SQLException &e) {
        std::cerr << "SQL Error: " << e.what() << std::endl;
        return 1;
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
