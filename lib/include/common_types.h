#pragma once

#include <vector>
#include <map>
#include <string>

namespace TraProcessor {

// 转移结构体
struct Transition {
    int src;
    int tgt;
    double prob;
    
    Transition(int s, int t, double p) : src(s), tgt(t), prob(p) {}
};

// 验证结果结构体
struct VerificationResult {
    std::map<std::string, std::string> probabilities;
    bool success;
    std::string error_message;
    
    VerificationResult() : success(false) {}
};

// 常量定义
namespace Constants {
    const double EPSILON = 0.0001;  // 概率误差范围
    const int DEFAULT_PRECISION = 4; // 默认小数位数
}

} // namespace TraProcessor 