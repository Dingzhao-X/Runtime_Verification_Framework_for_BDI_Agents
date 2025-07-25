#include "log_manager.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define MKDIR(path) _mkdir(path)
#define ACCESS(path, mode) _access(path, mode)
#else
#include <sys/stat.h>
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0755)
#define ACCESS(path, mode) access(path, mode)
#endif

namespace TraProcessor {

// 静态成员变量定义
std::unique_ptr<std::ofstream> LogManager::log_file = nullptr;
std::streambuf* LogManager::original_cout_buf = nullptr;
std::streambuf* LogManager::original_cerr_buf = nullptr;
bool LogManager::is_redirected = false;
std::string LogManager::current_log_path = "";

bool LogManager::startLogging(const std::string& directory_path) {
    return startLogging(directory_path, "out_log.txt");
}

bool LogManager::startLogging(const std::string& directory_path, const std::string& log_filename) {
    if (is_redirected) {
        stopLogging(); // 如果已经在记录，先停止
    }
    
    // 确保目录存在
    if (!ensureDirectoryExists(directory_path)) {
        std::cerr << "[LogManager] Failed to create directory: " << directory_path << std::endl;
        return false;
    }
    
    // 构建日志文件路径
    std::string log_path = directory_path;
    if (!log_path.empty() && log_path.back() != '/' && log_path.back() != '\\') {
        log_path += "/";
    }
    log_path += log_filename;
    
    // 创建日志文件
    log_file = std::make_unique<std::ofstream>(log_path, std::ios::out | std::ios::trunc);
    if (!log_file->is_open()) {
        std::cerr << "[LogManager] Failed to create log file: " << log_path << std::endl;
        log_file.reset();
        return false;
    }
    
    // 保存原始输出缓冲区
    original_cout_buf = std::cout.rdbuf();
    original_cerr_buf = std::cerr.rdbuf();
    
    // 重定向 cout 和 cerr 到日志文件
    std::cout.rdbuf(log_file->rdbuf());
    std::cerr.rdbuf(log_file->rdbuf());
    
    is_redirected = true;
    current_log_path = log_path;
    
    // 写入日志开始标记
    std::cout << "=== Log Started ===" << std::endl;
    std::cout << "Log file: " << log_path << std::endl;
    std::cout << "===================" << std::endl << std::endl;
    
    return true;
}

void LogManager::stopLogging() {
    if (!is_redirected) {
        return;
    }
    
    // 写入日志结束标记
    std::cout << std::endl << "=== Log Ended ===" << std::endl;
    
    // 恢复原始输出缓冲区
    if (original_cout_buf) {
        std::cout.rdbuf(original_cout_buf);
        original_cout_buf = nullptr;
    }
    
    if (original_cerr_buf) {
        std::cerr.rdbuf(original_cerr_buf);
        original_cerr_buf = nullptr;
    }
    
    // 关闭日志文件
    if (log_file) {
        log_file->close();
        log_file.reset();
    }
    
    is_redirected = false;
    
    // 输出到控制台，告知日志文件位置
    std::cout << "[LogManager] Log saved to: " << current_log_path << std::endl;
    current_log_path = "";
}

std::string LogManager::getCurrentLogPath() {
    return current_log_path;
}

bool LogManager::isLogging() {
    return is_redirected;
}

std::string LogManager::extractDirectoryPath(const std::string& file_path) {
    size_t last_slash = file_path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        return file_path.substr(0, last_slash);
    }
    return "."; // 当前目录
}

bool LogManager::ensureDirectoryExists(const std::string& directory_path) {
    if (directory_path.empty() || directory_path == ".") {
        return true; // 当前目录总是存在
    }
    
    // 检查目录是否已存在
    if (ACCESS(directory_path.c_str(), 0) == 0) {
        return true;
    }
    
    // 尝试创建目录
    if (MKDIR(directory_path.c_str()) == 0) {
        return true;
    }
    
    // 如果创建失败，可能是因为父目录不存在，尝试递归创建
    std::string parent_dir = extractDirectoryPath(directory_path);
    if (parent_dir != directory_path && ensureDirectoryExists(parent_dir)) {
        return MKDIR(directory_path.c_str()) == 0;
    }
    
    return false;
}

} // namespace TraProcessor 