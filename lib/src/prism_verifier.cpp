#include "prism_verifier.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace TraProcessor {

VerificationResult PrismVerifier::runVerification(const std::string& tra_file, const std::string& csl_file) {
    VerificationResult result;
    
    if (!isPrismAvailable()) {
        result.error_message = "PRISM is not available or not in PATH";
        return result;
    }
    
    // 构建PRISM命令
    std::string temp_output = "prism_output_temp.txt";
    std::string command = "prism -dtmc -importtrans \"" + tra_file + "\" \"" + csl_file + "\"";
    
    std::cout << "[PrismVerifier] Running PRISM verification..." << std::endl;
    
    if (callPrismCommand(command, temp_output)) {
        result.probabilities = parsePrismOutput(temp_output);
        result.success = !result.probabilities.empty();
        
        if (result.success) {
            std::cout << "[PrismVerifier] Verification completed successfully with " 
                     << result.probabilities.size() << " results" << std::endl;
        } else {
            result.error_message = "No valid results found in PRISM output";
        }
    } else {
        result.error_message = "Failed to execute PRISM command";
    }
    
    // 清理临时文件
    cleanupTempFiles(temp_output);
    
    return result;
}

bool PrismVerifier::generateSimulationPath(const std::string& pm_file, int steps, const std::string& output_path) {
    if (!isPrismAvailable()) {
        std::cerr << "[PrismVerifier] PRISM is not available" << std::endl;
        return false;
    }
    
    std::string temp_output = "prism_sim_temp.txt";
    std::string command = "prism -dtmc \"" + pm_file + "\" -simpath " + std::to_string(steps) + " \"" + output_path + "\"";
    
    std::cout << "[PrismVerifier] Generating simulation path..." << std::endl;
    
    bool success = callPrismCommand(command, temp_output);
    cleanupTempFiles(temp_output);
    
    // 检查输出文件是否存在
    std::ifstream check_output(output_path);
    bool file_exists = check_output.good();
    check_output.close();
    
    if (success && file_exists) {
        std::cout << "[PrismVerifier] Simulation path generated: " << output_path << std::endl;
        return true;
    } else {
        std::cerr << "[PrismVerifier] Failed to generate simulation path" << std::endl;
        return false;
    }
}

bool PrismVerifier::callPrismCommand(const std::string& command, const std::string& output_file) {
    std::string full_command = command + " > \"" + output_file + "\" 2>&1";
    int result = std::system(full_command.c_str());
    return result == 0;
}

std::map<std::string, std::string> PrismVerifier::parsePrismOutput(const std::string& output_file) {
    std::map<std::string, std::string> results;
    std::ifstream file(output_file);
    
    if (!file.is_open()) {
        std::cerr << "[PrismVerifier] Cannot read PRISM output file: " << output_file << std::endl;
        return results;
    }
    
    std::string line;
    std::string current_query;
    
    while (std::getline(file, line)) {
        // 查找查询行 (P=?)
        if (line.find("P=?") != std::string::npos) {
            current_query = simplifyQueryDisplay(line);
        }
        
        // 查找结果行
        if (line.find("Result:") != std::string::npos && !current_query.empty()) {
            size_t pos = line.find("Result:") + 7;
            std::string result = line.substr(pos);
            
            // 清理结果字符串
            result.erase(0, result.find_first_not_of(" \t"));
            result.erase(result.find('('));
            
            // 检查是否是有效的数值结果
            if (!result.empty() && result != "false" && result != "true") {
                results[current_query] = result;
            }
            
            current_query.clear();
        }
    }
    
    file.close();
    return results;
}

std::string PrismVerifier::simplifyQueryDisplay(const std::string& query) {
    // 简化PCTL查询的显示
    if (query.find("P=?") != std::string::npos) {
        size_t start = query.find("[");
        size_t end = query.find("]");
        if (start != std::string::npos && end != std::string::npos && end > start) {
            std::string formula = query.substr(start + 1, end - start - 1);
            return "P[" + formula + "]";
        }
    }
    
    // 如果无法简化，返回原始查询（截取合理长度）
    std::string simplified = query;
    if (simplified.length() > 100) {
        simplified = simplified.substr(0, 100) + "...";
    }
    return simplified;
}

bool PrismVerifier::isPrismAvailable() {
    // 尝试调用PRISM -help来检查是否可用
    std::string test_output = "prism_test_temp.txt";
    std::string test_command = "prism -help > \"" + test_output + "\" 2>&1";
    
    std::system(test_command.c_str());
    
    // 检查文件是否存在且有内容
    bool available = false;
    std::ifstream file(test_output);
    if (file.is_open()) {
        std::string line;
        if (std::getline(file, line) && !line.empty()) {
            available = true;
        }
        file.close();
    }
    
    cleanupTempFiles(test_output);
    
    if (!available) {
        std::cerr << "[PrismVerifier] Warning: PRISM not found in PATH. Please ensure PRISM is installed and accessible." << std::endl;
    }
    
    return available;
}

void PrismVerifier::cleanupTempFiles(const std::string& temp_output) {
    // 简单的文件删除，避免filesystem依赖
    std::remove(temp_output.c_str());
}

} // namespace TraProcessor 