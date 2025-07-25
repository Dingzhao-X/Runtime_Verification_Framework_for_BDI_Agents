#include "tra_to_pm_converter.h"
#include "tra_updater.h"
#include "prism_verifier.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "   Static Library Components Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string tra_file = "../data/ProD_CUD_SMP.tra";
    std::string csl_file = "../data/ProD_CUD_SMP.csl";
    std::string pm_file = "test_output.pm";
    std::string updated_tra = "test_updated.tra";
    std::string path_file = "../data/path.txt";
    
    if (argc >= 2) {
        tra_file = argv[1];
    }
    if (argc >= 3) {
        csl_file = argv[2];
    }
    
    std::cout << "\nTesting individual components:\n" << std::endl;
    
    // 测试1: TRA到PM转换
    std::cout << "1. Testing TRA to PM conversion..." << std::endl;
    TraProcessor::TraToPmConverter converter;
    if (converter.convertFile(tra_file, pm_file)) {
        std::cout << "   ✓ TRA to PM conversion successful: " << pm_file << std::endl;
    } else {
        std::cout << "   ✗ TRA to PM conversion failed" << std::endl;
        return 1;
    }
    
    // 测试2: 概率更新
    std::cout << "\n2. Testing probability update..." << std::endl;
    TraProcessor::TraUpdater updater;
    if (updater.updateTraFile(path_file, tra_file, updated_tra, false)) {
        std::cout << "   ✓ Probability update successful: " << updated_tra << std::endl;
    } else {
        std::cout << "   ✗ Probability update failed" << std::endl;
        return 1;
    }
    
    // 测试3: PRISM验证（可能失败如果没有安装PRISM）
    std::cout << "\n3. Testing PRISM verification..." << std::endl;
    TraProcessor::PrismVerifier verifier;
    auto result = verifier.runVerification(tra_file, csl_file);
    if (result.success) {
        std::cout << "   ✓ PRISM verification successful with " 
                 << result.probabilities.size() << " results" << std::endl;
        
        // 显示结果
        for (const auto& [query, prob] : result.probabilities) {
            std::cout << "     " << query << " = " << prob << std::endl;
        }
    } else {
        std::cout << "   ⚠ PRISM verification failed: " << result.error_message << std::endl;
        std::cout << "   (This is expected if PRISM is not installed)" << std::endl;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "       Component Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
} 