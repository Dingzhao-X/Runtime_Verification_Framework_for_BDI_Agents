#include "tra_updater.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>

// 使用简单的文件操作，避免filesystem依赖问题
#include <fstream>

namespace TraProcessor {

bool TraUpdater::updateTraFile(const std::string& path_file, const std::string& original_tra, 
                              const std::string& output_tra, bool handle_repeated_states) {
    // 读取路径文件
    std::vector<int> path_states = extractPathStates(path_file);
    if (path_states.empty()) {
        std::cerr << "Error: No valid path data found in " << path_file << std::endl;
        return false;
    }
    
    // 无论是否处理重复状态，文件更新阶段都使用相同的逻辑
    // 只更新第一次出现状态的转移概率，重复状态的处理在运行时进行
    std::map<int, int> real_trans = generateFirstOccurrenceTransitions(path_states);
    std::set<int> from_in_path;
    for (const auto& pair : real_trans) {
        from_in_path.insert(pair.first);
    }
    
    return processTraFileStandard(original_tra, output_tra, real_trans, from_in_path);
}



std::vector<int> TraUpdater::extractPathStates(const std::string& path_file) {
    std::vector<int> path_states;
    std::ifstream fin(path_file);
    if (!fin.is_open()) {
        std::cerr << "Cannot open path file: " << path_file << std::endl;
        return path_states;
    }
    
    std::string line;
    std::getline(fin, line); // 跳过表头
    
    while (std::getline(fin, line)) {
        std::istringstream iss(line);
        std::string action;
        int step, state;
        if (iss >> action >> step >> state) {
            path_states.push_back(state);
        }
    }
    fin.close();
    
    std::cout << "[TraUpdater] Extracted " << path_states.size() << " states from path" << std::endl;
    return path_states;
}

std::map<int, int> TraUpdater::generateTransitionMap(const std::vector<int>& path_states) {
    std::map<int, int> real_trans;
    for (size_t i = 1; i < path_states.size(); ++i) {
        real_trans[path_states[i-1]] = path_states[i];
    }
    return real_trans;
}

std::map<int, int> TraUpdater::generateFirstOccurrenceTransitions(const std::vector<int>& path_states) {
    std::map<int, int> first_occurrence_trans;
    std::set<int> seen_states;
    
    for (size_t i = 1; i < path_states.size(); ++i) {
        int from_state = path_states[i-1];
        int to_state = path_states[i];
        
        // 只记录第一次出现的状态的转移
        if (seen_states.find(from_state) == seen_states.end()) {
            first_occurrence_trans[from_state] = to_state;
            seen_states.insert(from_state);
        }
    }
    
    std::cout << "[TraUpdater] First occurrence transitions: " << first_occurrence_trans.size() 
              << " (from " << path_states.size() << " total states)" << std::endl;
    return first_occurrence_trans;
}

bool TraUpdater::processTraFileStandard(const std::string& tra_file, const std::string& output_file,
                                       const std::map<int, int>& real_trans, const std::set<int>& from_in_path) {
    std::ifstream tra_in(tra_file);
    std::ofstream tra_out(output_file);
    
    if (!tra_in.is_open()) {
        std::cerr << "Cannot open TRA file: " << tra_file << std::endl;
        return false;
    }
    if (!tra_out.is_open()) {
        std::cerr << "Cannot create output file: " << output_file << std::endl;
        return false;
    }
    
    // 复制头部（第一行）
    std::string header;
    std::getline(tra_in, header);
    tra_out << header << std::endl;
    
    // 处理每一行转移
    std::string line;
    while (std::getline(tra_in, line)) {
        int from, to;
        double prob;
        if (!parseTraLine(line, from, to, prob)) {
            tra_out << line << std::endl; // 直接输出无法解析的行
            continue;
        }
        
        if (from_in_path.count(from)) {
            // 这个状态在路径中出现过，需要更新概率
            if (real_trans.at(from) == to) {
                tra_out << from << " " << to << " 1" << std::endl;
            } else {
                tra_out << from << " " << to << " 0" << std::endl;
            }
        } else {
            // 保持原始概率
            tra_out << from << " " << to << " " << prob << std::endl;
        }
    }
    
    tra_in.close();
    tra_out.close();
    
    std::cout << "[TraUpdater] Standard processing completed: " << output_file << std::endl;
    return true;
}

bool TraUpdater::processTraFileWithRepeatedStates(const std::string& tra_file, const std::string& backup_file,
                                                 const std::string& output_file, const std::vector<int>& path_states) {
    // 预分析：找出真正的重复状态（在路径中出现多次的状态）
    std::map<int, int> state_count;
    for (int state : path_states) {
        state_count[state]++;
    }
    
    std::set<int> truly_repeated_states;
    for (const auto& pair : state_count) {
        if (pair.second > 1) {
            truly_repeated_states.insert(pair.first);
        }
    }
    
    // 生成实际转移映射
    std::map<int, int> actual_transitions;
    for (size_t i = 1; i < path_states.size(); ++i) {
        actual_transitions[path_states[i-1]] = path_states[i];
    }
    
    // 加载原始概率
    auto original_probs = loadOriginalProbabilities(backup_file);
    if (original_probs.empty()) {
        std::cerr << "Error: Cannot load original probabilities from backup" << std::endl;
        return false;
    }
    
    std::ifstream tra_in(tra_file);
    std::ofstream tra_out(output_file);
    
    if (!tra_in.is_open()) {
        std::cerr << "Cannot open TRA file: " << tra_file << std::endl;
        return false;
    }
    if (!tra_out.is_open()) {
        std::cerr << "Cannot create output file: " << output_file << std::endl;
        return false;
    }
    
    // 复制头部
    std::string header;
    std::getline(tra_in, header);
    tra_out << header << std::endl;
    
    // 处理每一行转移
    std::string line;
    while (std::getline(tra_in, line)) {
        int from, to;
        double prob;
        if (!parseTraLine(line, from, to, prob)) {
            tra_out << line << std::endl;
            continue;
        }
        
        // 检查这个状态是否在路径中
        if (actual_transitions.count(from)) {
            // 状态在路径中
            if (truly_repeated_states.count(from)) {
                // 真正的重复状态：使用原始概率
                auto key = std::make_pair(from, to);
                if (original_probs.count(key)) {
                    tra_out << from << " " << to << " " << original_probs[key] << std::endl;
                } else {
                    tra_out << from << " " << to << " " << prob << std::endl;
                }
            } else {
                // 非重复状态：根据实际转移设为0/1
                int actual_to = actual_transitions[from];
                if (actual_to == to) {
                    tra_out << from << " " << to << " 1" << std::endl;
                } else {
                    tra_out << from << " " << to << " 0" << std::endl;
                }
            }
        } else {
            // 不在路径中的状态：保持原始概率
            tra_out << from << " " << to << " " << prob << std::endl;
        }
    }
    
    tra_in.close();
    tra_out.close();
    
    std::cout << "[TraUpdater] Repeated states processing completed: " << output_file << std::endl;
    std::cout << "[TraUpdater] Truly repeated states: " << truly_repeated_states.size() 
              << ", Path states: " << actual_transitions.size() << std::endl;
    return true;
}

std::map<std::pair<int, int>, double> TraUpdater::loadOriginalProbabilities(const std::string& backup_file) {
    std::map<std::pair<int, int>, double> probs;
    std::ifstream file(backup_file);
    
    if (!file.is_open()) {
        std::cerr << "Cannot open backup file: " << backup_file << std::endl;
        return probs;
    }
    
    std::string line;
    std::getline(file, line); // 跳过头部
    
    while (std::getline(file, line)) {
        int from, to;
        double prob;
        if (parseTraLine(line, from, to, prob)) {
            probs[{from, to}] = prob;
        }
    }
    
    file.close();
    std::cout << "[TraUpdater] Loaded " << probs.size() << " original probabilities" << std::endl;
    return probs;
}

bool TraUpdater::parseTraLine(const std::string& line, int& from, int& to, double& prob) {
    std::istringstream iss(line);
    return static_cast<bool>(iss >> from >> to >> prob);
}

} // namespace TraProcessor 