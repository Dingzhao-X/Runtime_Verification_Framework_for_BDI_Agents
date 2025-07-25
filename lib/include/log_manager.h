#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <memory>

namespace TraProcessor {

class LogManager {
private:
    static std::unique_ptr<std::ofstream> log_file;
    static std::streambuf* original_cout_buf;
    static std::streambuf* original_cerr_buf;
    static bool is_redirected;
    static std::string current_log_path;
    
public:
    // 开始日志重定向到指定目录的out_log.txt
    static bool startLogging(const std::string& directory_path);
    
    // 开始日志重定向到指定目录的自定义日志文件
    static bool startLogging(const std::string& directory_path, const std::string& log_filename);
    
    // 停止日志重定向，恢复到标准输出
    static void stopLogging();
    
    // 获取当前日志文件路径
    static std::string getCurrentLogPath();
    
    // 检查是否正在记录日志
    static bool isLogging();
    
    // 从文件路径提取目录路径的工具函数
    static std::string extractDirectoryPath(const std::string& file_path);
    
    // 确保目录存在的工具函数
    static bool ensureDirectoryExists(const std::string& directory_path);
};

} // namespace TraProcessor 