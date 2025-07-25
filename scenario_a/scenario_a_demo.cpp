#include "integrated_tool.h"
#include "log_manager.h"
#include <iostream>
#include <string>
#include <filesystem>

int main(int argc, char *argv[]) {

    // 默认参数
    std::string tra_file = "../data/default_data/ProD_CUD_SMP.tra";
    std::string csl_file = "../data/default_data/ProD_CUD_SMP.csl";
    std::string path_file = "../data/default_data/ProD_CUD_SMP_path.txt"; 
    int simulation_steps = 1000;
    bool handle_repeated_states = false;

    // 处理命令行参数
    for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
        if (arg == "--tra" && i + 1 < argc) {
            tra_file = argv[++i];
        } else if (arg == "--csl" && i + 1 < argc) {
            csl_file = argv[++i];
        } else if (arg == "--path" && i + 1 < argc) {
            path_file = argv[++i];
        } else if (arg == "--steps" && i + 1 < argc) {
            simulation_steps = std::stoi(argv[++i]);
        } else if (arg == "--repeated") {
            handle_repeated_states = true;
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --tra FILE       TRA file (default: ../data/default_data/ProD_CUD_SMP.tra)" << std::endl;
            std::cout << "  --csl FILE       CSL file (default: ../data/default_data/ProD_CUD_SMP.csl)" << std::endl;
            std::cout << "  --path FILE      Path file (default: ../data/default_data/ProD_CUD_SMP_path.txt)" << std::endl;
            std::cout << "  --steps N        Simulation steps (default: 1000)" << std::endl;
            std::cout << "  --repeated       Handle repeated states (default: false)" << std::endl;
            std::cout << "  --help           Show this help" << std::endl;
            return 0;
        }
    }

    // 如果指定了tra文件但没有指定path文件，则在tra文件所在目录生成path文件
    if (tra_file != "../data/default_data/ProD_CUD_SMP.tra" && path_file == "../data/default_data/ProD_CUD_SMP_path.txt") {
        // 获取tra文件的目录路径
        size_t last_slash = tra_file.find_last_of("/\\");
        std::string tra_dir = (last_slash != std::string::npos) ? tra_file.substr(0, last_slash + 1) : "./";
        // 获取tra文件的基本名称（不含扩展名）
        std::string tra_base = tra_file.substr(last_slash + 1);
        size_t last_dot = tra_base.find_last_of(".");
        if (last_dot != std::string::npos) {
            tra_base = tra_base.substr(0, last_dot);
        }
        // 设置path文件路径
        path_file = tra_dir + tra_base + "_path.txt";
    }

    // 启动日志记录到tra文件所在目录
    std::string log_directory = TraProcessor::LogManager::extractDirectoryPath(tra_file);
     if (!TraProcessor::LogManager::startLogging(log_directory)) {
         std::cerr << "Warning: Failed to start logging to " << log_directory << std::endl;
     }

    // 创建集成工具并运行场景A
    TraProcessor::IntegratedTool tool;
    auto result = tool.runScenarioA(tra_file, csl_file, simulation_steps, handle_repeated_states, path_file);

    // 显示结果
    tool.displayResults(result);

    // 停止日志记录
    TraProcessor::LogManager::stopLogging();

    return result.success ? 0 : 1;
}