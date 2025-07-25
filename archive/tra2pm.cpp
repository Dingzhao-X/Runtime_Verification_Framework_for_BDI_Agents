#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <iomanip>

using namespace std;

struct Transition {
    int src;
    int tgt;
    double prob;
};

//格式化概率值，保留4位小数
string formatProbability(double prob) {
    stringstream ss;
    ss << fixed << setprecision(4) << prob;
    return ss.str();
}

// 归一化概率函数
void normalizeProbabilities(map<int, vector<pair<int, double>>>& grouped) {
    const double EPSILON = 0.0001;  // 允许的误差范围
    
    for (auto& [src, transitions] : grouped) {
        double sum = 0.0;
        for (const auto& [tgt, prob] : transitions) {
            sum += prob;
        }
        
        // 如果总和与1的差值在允许范围内，进行归一化
        if (std::abs(sum - 1.0) < EPSILON) {
            for (auto& [tgt, prob] : transitions) {
                prob /= sum;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "用法: " << argv[0] << " 输入.tra 输出.pm" << endl;
        return 1;
    }

    ifstream infile(argv[1]);
    ofstream outfile(argv[2]);
    if (!infile.is_open()) {
        cerr << "无法打开输入文件: " << argv[1] << endl;
        return 1;
    }
    if (!outfile.is_open()) {
        cerr << "无法打开输出文件: " << argv[2] << endl;
        return 1;
    }

    int state_num, trans_num;
    infile >> state_num >> trans_num;
    vector<Transition> transitions;
    int src, tgt;
    double prob;

    // 读取所有转移
    while (infile >> src >> tgt >> prob) {
        transitions.push_back({src, tgt, prob});
    }

    // 按源状态分组
    map<int, vector<pair<int, double>>> grouped;
    for (const auto& t : transitions) {
        grouped[t.src].push_back({t.tgt, t.prob});
    }

    // 归一化概率
    normalizeProbabilities(grouped);

    // 写入PRISM模型
    outfile << "dtmc\n\n";
    outfile << "module tra_model\n";
    outfile << "    s : [0.." << (state_num-1) << "] init 0;\n\n";

    for (int i = 0; i < state_num; ++i) {
        if (grouped.count(i)) {
            outfile << "    [] s=" << i << " -> ";
            const auto& outs = grouped[i];
            for (size_t j = 0; j < outs.size(); ++j) {
                outfile << formatProbability(outs[j].second) << ":(s'=" << outs[j].first << ")";
                if (j != outs.size() - 1) outfile << " + ";
            }
            outfile << ";\n";
        }
    }
    outfile << "endmodule\n";

    infile.close();
    outfile.close();
    cout << "转换完成，已生成 " << argv[2] << endl;
    return 0;
}