#pragma once

#include "common_types.h"
#include "tra_to_pm_converter.h"
#include "tra_updater.h"
#include "prism_verifier.h"
#include <string>

namespace TraProcessor {

class IntegratedTool {
public:
    // 完整验证结果
    struct ScenarioAResult {
        VerificationResult original_verification;  // 原始验证结果
        VerificationResult updated_verification;   // 更新后验证结果
        std::string simulation_path;               // 生成的模拟路径
        bool success;                              // 整体成功标志
        std::string error_message;                 // 错误信息
        
        ScenarioAResult() : success(false) {}
    };
    
    IntegratedTool() = default;
    ~IntegratedTool() = default;
    
    // 场景A主要功能：运行完整的离线单次模拟+自动化工具验证
    ScenarioAResult runScenarioA(const std::string& tra_file, 
                                const std::string& csl_file,
                                int simulation_steps = 1000,
                                bool handle_repeated_states = false,
                                const std::string& custom_path_file = "");
    
    // 显示结果对比
    void displayResults(const ScenarioAResult& result);
    
private:
    TraToPmConverter converter;
    TraUpdater updater;
    PrismVerifier verifier;
    
    // 生成临时文件名
    std::string generateTempFileName(const std::string& base_name, const std::string& suffix);
    
    // 清理临时文件
    void cleanupTempFiles(const std::vector<std::string>& temp_files);
    
    // 读取模拟路径摘要
    std::string readSimulationPathSummary(const std::string& path_file);
};

} // namespace TraProcessor 