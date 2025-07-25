#include "integrated_tool.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

namespace TraProcessor {

IntegratedTool::ScenarioAResult IntegratedTool::runScenarioA(const std::string& tra_file, 
                                                           const std::string& csl_file,
                                                           int simulation_steps,
                                                           bool handle_repeated_states,
                                                           const std::string& custom_path_file) {
    ScenarioAResult result;
    std::vector<std::string> temp_files;
    
    // 获取输入tra文件的目录路径
    std::string input_dir;
    size_t last_slash = tra_file.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        input_dir = tra_file.substr(0, last_slash + 1);
    }
    
    std::cout << "SCENARIO A: Integrated Verification" << std::endl;
    std::cout << std::string(40, '=') << std::endl;
    
    try {
        // Step 1: 初始验证 - 获取原始概率
        std::cout << "STEP 1: Running initial PRISM verification..." << std::endl;
        result.original_verification = verifier.runVerification(tra_file, csl_file);
        
        if (!result.original_verification.success) {
            result.error_message = "Initial verification failed: " + result.original_verification.error_message;
            return result;
        }
        
        std::cout << "✓ Initial verification completed with " 
                 << result.original_verification.probabilities.size() << " results" << std::endl << std::endl;
        
        // Step 2: 生成PRISM模型文件
        std::cout << "STEP 2: Converting TRA to PRISM model..." << std::endl;
        std::string pm_file = input_dir + generateTempFileName(tra_file, ".pm");
        temp_files.push_back(pm_file);
        
        if (!converter.convertFile(tra_file, pm_file)) {
            result.error_message = "Failed to convert TRA to PM file";
            cleanupTempFiles(temp_files);
            return result;
        }
        
        std::cout << "✓ PRISM model generated: " << pm_file << std::endl << std::endl;
        
        // Step 3: 生成或使用模拟路径
        std::string path_file;
        if (!custom_path_file.empty()) {
            // 检查自定义路径文件是否存在
            std::ifstream check_file(custom_path_file);
            if (check_file.good()) {
                std::cout << "STEP 3: Using provided simulation path..." << std::endl;
                path_file = custom_path_file;
                std::cout << "✓ Using simulation path: " << path_file << std::endl << std::endl;
                check_file.close();
            } else {
                std::cout << "STEP 3: Provided path file not found, generating new path..." << std::endl;
                path_file = custom_path_file;  // 使用指定的路径但生成新文件
                if (!verifier.generateSimulationPath(pm_file, simulation_steps, path_file)) {
                    result.error_message = "Failed to generate simulation path";
                    cleanupTempFiles(temp_files);
                    return result;
                }
                std::cout << "✓ Simulation path generated: " << path_file << std::endl << std::endl;
            }
        } else {
            std::cout << "STEP 3: Generating simulation path..." << std::endl;
            path_file = input_dir + generateTempFileName(tra_file, "_path.txt");
            temp_files.push_back(path_file);
            
            if (!verifier.generateSimulationPath(pm_file, simulation_steps, path_file)) {
                result.error_message = "Failed to generate simulation path";
                cleanupTempFiles(temp_files);
                return result;
            }
            
            std::cout << "✓ Simulation path generated: " << path_file << std::endl << std::endl;
        }
        
        // Step 4: 基于路径更新TRA文件
        std::cout << "STEP 4: Updating transition probabilities based on observed path..." << std::endl;
        std::string updated_tra = input_dir + generateTempFileName(tra_file, "_updated.tra");
        temp_files.push_back(updated_tra);
        
        if (!updater.updateTraFile(path_file, tra_file, updated_tra, handle_repeated_states)) {
            result.error_message = "Failed to update TRA file probabilities";
            cleanupTempFiles(temp_files);
            return result;
        }
        
        std::cout << "✓ Transition probabilities updated: " << updated_tra << std::endl << std::endl;
        
        // Step 5: 使用更新后的模型进行最终验证
        std::cout << "STEP 5: Running final verification with updated model..." << std::endl;
        result.updated_verification = verifier.runVerification(updated_tra, csl_file);
        
        if (!result.updated_verification.success) {
            result.error_message = "Final verification failed: " + result.updated_verification.error_message;
            cleanupTempFiles(temp_files);
            return result;
        }
        
        std::cout << "✓ Final verification completed with " 
                 << result.updated_verification.probabilities.size() << " results" << std::endl << std::endl;
        
        // 读取生成的路径作为结果的一部分
        result.simulation_path = readSimulationPathSummary(path_file);
        result.success = true;
        
        std::cout << "SCENARIO A COMPLETED SUCCESSFULLY" << std::endl;
        std::cout << std::string(40, '=') << std::endl;
        
    } catch (const std::exception& e) {
        result.error_message = std::string("Exception occurred: ") + e.what();
        cleanupTempFiles(temp_files);
        return result;
    }
    
    // 清理临时文件（可选）
    // cleanupTempFiles(temp_files);
    
    return result;
}

void IntegratedTool::displayResults(const ScenarioAResult& result) {
    if (!result.success) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "                    ERROR OCCURRED" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        std::cout << "Error: " << result.error_message << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
        return;
    }
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "                    VERIFICATION RESULTS COMPARISON" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    // 显示路径摘要
    if (!result.simulation_path.empty()) {
        std::cout << "\nSimulation Path Summary:" << std::endl;
        std::cout << result.simulation_path << std::endl;
        std::cout << std::string(80, '-') << std::endl;
    }
    
    // 准备对比显示
    std::cout << "\nProbability Comparison:" << std::endl;
    std::cout << std::left << std::setw(45) << "PCTL Query" 
             << std::setw(25) << "Original" 
             << std::setw(25) << "Updated" 
             << "Change" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    // 遍历所有查询进行对比
    for (const auto& [query, original_prob] : result.original_verification.probabilities) {
        std::string updated_prob = "N/A";
        std::string change = "N/A";
        
        // 查找对应的更新后概率
        auto it = result.updated_verification.probabilities.find(query);
        if (it != result.updated_verification.probabilities.end()) {
            updated_prob = it->second;
            
            // 计算变化量（如果都是数字）
            try {
                double orig_val = std::stod(original_prob);
                double upd_val = std::stod(updated_prob);
                double diff = upd_val - orig_val;
                
                std::ostringstream change_stream;
                change_stream << std::fixed << std::setprecision(16);
                if (diff >= 0) change_stream << "+";
                change_stream << diff;
                change = change_stream.str();
            } catch (const std::exception&) {
                // 如果不能转换为数字，保持"N/A"
            }
        }
        
        // 截断过长的查询名称
        std::string display_query = query;
        if (display_query.length() > 47) {
            display_query = display_query.substr(0, 44) + "...";
        }
        
        std::cout << std::left << std::setw(45) << display_query
                 << std::setw(25) << original_prob
                 << std::setw(25) << updated_prob
                 << change << std::endl;
    }
    
    // 显示仅在更新后结果中出现的查询
    for (const auto& [query, updated_prob] : result.updated_verification.probabilities) {
        if (result.original_verification.probabilities.find(query) == 
            result.original_verification.probabilities.end()) {
            
            std::string display_query = query;
            if (display_query.length() > 47) {
                display_query = display_query.substr(0, 44) + "...";
            }
            
            std::cout << std::left << std::setw(45) << display_query
                     << std::setw(25) << "N/A"
                     << std::setw(25) << updated_prob
                     << "NEW" << std::endl;
        }
    }
    
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "Total original results: " << result.original_verification.probabilities.size() << std::endl;
    std::cout << "Total updated results: " << result.updated_verification.probabilities.size() << std::endl;
    std::cout << std::string(80, '=') << std::endl << std::endl;
}

std::string IntegratedTool::generateTempFileName(const std::string& base_name, const std::string& suffix) {
    // 简单提取文件名（不含路径和扩展名）
    size_t last_slash = base_name.find_last_of("/\\");
    size_t last_dot = base_name.find_last_of(".");
    
    std::string stem;
    if (last_slash != std::string::npos) {
        if (last_dot != std::string::npos && last_dot > last_slash) {
            stem = base_name.substr(last_slash + 1, last_dot - last_slash - 1);
        } else {
            stem = base_name.substr(last_slash + 1);
        }
    } else {
        if (last_dot != std::string::npos) {
            stem = base_name.substr(0, last_dot);
        } else {
            stem = base_name;
        }
    }
    
    return stem + suffix;
}

void IntegratedTool::cleanupTempFiles(const std::vector<std::string>& temp_files) {
    for (const auto& file : temp_files) {
        if (std::remove(file.c_str()) == 0) {
            std::cout << "Cleaned up temporary file: " << file << std::endl;
        }
    }
}

std::string IntegratedTool::readSimulationPathSummary(const std::string& path_file) {
    std::ifstream file(path_file);
    if (!file.is_open()) {
        return "Could not read simulation path file";
    }
    
    std::ostringstream summary;
    std::string line;
    std::vector<int> path_states;
    
    // 跳过表头
    std::getline(file, line);
    
    // 读取所有路径数据
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string action;
        int step, state;
        if (iss >> action >> step >> state) {
            path_states.push_back(state);
        }
    }
    
    file.close();
    
    if (path_states.empty()) {
        return "No valid path data found";
    }
    
    // 构建路径摘要 - 显示总状态数，但只显示前15个
    summary << "Path (" << path_states.size() << " states): ";
    for (size_t i = 0; i < std::min(path_states.size(), size_t(15)); ++i) {
        if (i > 0) summary << " → ";
        summary << path_states[i];
    }
    if (path_states.size() > 15) {
        summary << " → ... (+" << (path_states.size() - 15) << " more)";
    }
    
    return summary.str();
}

} // namespace TraProcessor 