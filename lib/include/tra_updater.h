#pragma once

#include "common_types.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace TraProcessor {

class TraUpdater {
public:
    TraUpdater() = default;
    ~TraUpdater() = default;
    
    // 主要功能：根据观察路径更新TRA文件
    bool updateTraFile(const std::string& path_file, const std::string& original_tra, 
                      const std::string& output_tra, bool handle_repeated_states = false);
    

    
private:
    // 从路径文件中提取状态序列
    std::vector<int> extractPathStates(const std::string& path_file);
    
    // 生成实际转移映射 (from_state -> to_state)
    std::map<int, int> generateTransitionMap(const std::vector<int>& path_states);
    
    // 生成第一次出现状态的转移映射（用于重复状态处理）
    std::map<int, int> generateFirstOccurrenceTransitions(const std::vector<int>& path_states);
    
    // 处理TRA文件更新（标准方式，不处理重复状态）
    bool processTraFileStandard(const std::string& tra_file, const std::string& output_file,
                               const std::map<int, int>& real_trans, const std::set<int>& from_in_path);
    
    // 处理TRA文件更新（处理重复状态）
    bool processTraFileWithRepeatedStates(const std::string& tra_file, const std::string& backup_file,
                                         const std::string& output_file, const std::vector<int>& path_states);
    
    // 跟踪已固定的状态（用于重复状态处理）
    std::set<int> fixed_states;
    
    // 读取备份文件中的原始概率
    std::map<std::pair<int, int>, double> loadOriginalProbabilities(const std::string& backup_file);
    
    // 工具函数：解析TRA文件行
    bool parseTraLine(const std::string& line, int& from, int& to, double& prob);
};

} // namespace TraProcessor 