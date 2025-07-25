#pragma once

#include "common_types.h"
#include <string>
#include <vector>
#include <map>

namespace TraProcessor {

class TraToPmConverter {
public:
    TraToPmConverter() = default;
    ~TraToPmConverter() = default;
    
    // 主要功能：将TRA文件转换为PRISM PM文件
    bool convertFile(const std::string& tra_file, const std::string& pm_file);
    
    // 工具函数：格式化概率值
    std::string formatProbability(double prob, int precision = Constants::DEFAULT_PRECISION);
    
private:
    // 读取TRA文件
    bool readTraFile(const std::string& tra_file, std::vector<Transition>& transitions, 
                     int& state_num, int& trans_num);
    
    // 按源状态分组转移
    void groupTransitionsBySource(const std::vector<Transition>& transitions,
                                  std::map<int, std::vector<std::pair<int, double>>>& grouped);
    
    // 归一化概率
    void normalizeProbabilities(std::map<int, std::vector<std::pair<int, double>>>& grouped);
    
    // 写入PRISM模型文件
    bool writePrismModel(const std::string& pm_file, 
                        const std::map<int, std::vector<std::pair<int, double>>>& grouped,
                        int state_num);
};

} // namespace TraProcessor 