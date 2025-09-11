#pragma once
#include <string>

struct AutomationTestCase {
    std::string script_id;            // 脚本序号 (主键)
    std::string module_name;          // 模块名
    std::string case_description;     // 用例描述
    std::string tc_rf_submit_time;    // TC_RF提交时间
    std::string case_package_name;    // 用例包名称
    std::string case_id;              // 用例编号
    std::string case_name;            // 用例名称
    std::string case_level;           // 用例等级
    std::string test_requirement;     // 测试需求
    std::string physical_interface_type; // 物理接口类型
    std::string initial_port_status;  // 初始端口状态
    std::string test_topology_name;   // 测试拓扑名称
    std::string dut_count;            // DUT数量
    std::string tester_type;          // 测试仪类型
    std::string port_count;           // 使用端口数
    std::string test_duration;        // 测试用时
    std::string department;           // 归属部门
    std::string designer;             // 设计人
    std::string component;            // 组件
    std::string component_domain;     // 组件域
    std::string script_type;          // 脚本类型
    std::string process_name;         // 进程名
    std::string fixed_testbed;        // 固化测试床
    std::string source_project;       // 来源项目
    std::string audit_result;         // 审核结果
    std::string execution_status;     // 执行已入库
    std::string instock_process_status; // 入库流程状态
    std::string instock_process_time; // 入库流程时间
};

struct TestManagementCase {
    std::string case_id;              // 用例编号 (主键)
    std::string test_step;            // 测试步骤
    std::string test_type;            // 测试类型
    std::string case_package_name;    // 用例包名称
    std::string case_name;            // 用例名称
    std::string case_description;     // 用例描述
    std::string case_tag;             // 用例标签
    std::string case_owner;           // 用例负责人
    std::string script_name;          // 脚本名称
    std::string script_id;            // 脚本序号
    std::string script_status;        // 脚本状态
};

// Request/Response 结构体，用于客户端和服务器之间通信
struct UploadRequest {
    bool overwrite;                   // 是否覆盖现有数据
    std::string data_type;            // 数据类型: "automation" 或 "management"
};

struct UploadResponse {
    bool success;                     // 操作是否成功
    std::string message;              // 响应消息
    int records_processed;            // 处理的记录数
    int records_inserted;             // 插入的记录数
    int records_updated;              // 更新的记录数
    int records_skipped;              // 跳过的记录数
    std::vector<std::string> errors;  // 错误消息列表
};
