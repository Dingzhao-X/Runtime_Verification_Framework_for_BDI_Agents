#include "tra_to_pm_converter.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace TraProcessor {

bool TraToPmConverter::convertFile(const std::string& tra_file, const std::string& pm_file) {
    std::vector<Transition> transitions;
    int state_num, trans_num;
    
    // 读取TRA文件
    if (!readTraFile(tra_file, transitions, state_num, trans_num)) {
        return false;
    }
    
    // 按源状态分组
    std::map<int, std::vector<std::pair<int, double>>> grouped;
    groupTransitionsBySource(transitions, grouped);
    
    // 归一化概率
    normalizeProbabilities(grouped);
    
    // 写入PRISM模型
    return writePrismModel(pm_file, grouped, state_num);
}

std::string TraToPmConverter::formatProbability(double prob, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << prob;
    return ss.str();
}

bool TraToPmConverter::readTraFile(const std::string& tra_file, 
                                  std::vector<Transition>& transitions, 
                                  int& state_num, int& trans_num) {
    std::ifstream infile(tra_file);
    if (!infile.is_open()) {
        std::cerr << "Cannot open TRA file: " << tra_file << std::endl;
        return false;
    }
    
    // 读取头部
    infile >> state_num >> trans_num;
    transitions.reserve(trans_num);
    
    // 读取所有转移
    int src, tgt;
    double prob;
    while (infile >> src >> tgt >> prob) {
        transitions.emplace_back(src, tgt, prob);
    }
    
    infile.close();
    return true;
}

void TraToPmConverter::groupTransitionsBySource(const std::vector<Transition>& transitions,
                                               std::map<int, std::vector<std::pair<int, double>>>& grouped) {
    for (const auto& t : transitions) {
        grouped[t.src].emplace_back(t.tgt, t.prob);
    }
}

void TraToPmConverter::normalizeProbabilities(std::map<int, std::vector<std::pair<int, double>>>& grouped) {
    for (auto& [src, transitions] : grouped) {
        double sum = 0.0;
        for (const auto& [tgt, prob] : transitions) {
            sum += prob;
        }
        
        // 如果总和与1的差值在允许范围内，进行归一化
        if (std::abs(sum - 1.0) < Constants::EPSILON) {
            for (auto& [tgt, prob] : transitions) {
                prob /= sum;
            }
        }
    }
}

bool TraToPmConverter::writePrismModel(const std::string& pm_file, 
                                      const std::map<int, std::vector<std::pair<int, double>>>& grouped,
                                      int state_num) {
    std::ofstream outfile(pm_file);
    if (!outfile.is_open()) {
        std::cerr << "Cannot create PM file: " << pm_file << std::endl;
        return false;
    }
    
    // 写入PRISM模型头部
    outfile << "dtmc\n\n";
    outfile << "module tra_model\n";
    outfile << "    s : [0.." << (state_num-1) << "] init 0;\n\n";
    
    // 写入转移规则
    for (int i = 0; i < state_num; ++i) {
        if (grouped.find(i) != grouped.end()) {
            outfile << "    [] s=" << i << " -> ";
            const auto& outs = grouped.at(i);
            for (size_t j = 0; j < outs.size(); ++j) {
                outfile << formatProbability(outs[j].second) << ":(s'=" << outs[j].first << ")";
                if (j != outs.size() - 1) outfile << " + ";
            }
            outfile << ";\n";
        }
    }
    outfile << "endmodule\n";
    
    outfile.close();
    return true;
}

} // namespace TraProcessor 