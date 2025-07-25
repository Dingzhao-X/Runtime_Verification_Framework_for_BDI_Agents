#pragma once

#include "common_types.h"
#include <string>
#include <map>

namespace TraProcessor {

class PrismVerifier {
public:
    PrismVerifier() = default;
    ~PrismVerifier() = default;
    
    // 主要功能：运行PRISM验证
    VerificationResult runVerification(const std::string& tra_file, const std::string& csl_file);
    
    // 生成模拟路径
    bool generateSimulationPath(const std::string& pm_file, int steps, const std::string& output_path);
    
private:
    // 调用PRISM命令
    bool callPrismCommand(const std::string& command, const std::string& output_file);
    
    // 解析PRISM输出文件
    std::map<std::string, std::string> parsePrismOutput(const std::string& output_file);
    
    // 简化PCTL查询显示
    std::string simplifyQueryDisplay(const std::string& query);
    
    // 检查PRISM是否可用
    bool isPrismAvailable();
    
    // 清理临时文件
    void cleanupTempFiles(const std::string& temp_output);
};

} // namespace TraProcessor 