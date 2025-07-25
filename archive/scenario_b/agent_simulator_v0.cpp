#include "../lib/include/tra_to_pm_converter.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <random>
#include <thread>
#include <chrono>
#include <algorithm>
#include <sstream>

using namespace std;

class AgentSimulator {
private:
    int current_state;
    int step_counter;
    map<int, vector<pair<int, double>>> transition_model; // state -> [(next_state, prob), ...]
    string shared_path_file;
    string model_file;
    mt19937 random_gen;
    
    // 从最新的.pm文件读取转移概率
    void loadLatestModel() {
        ifstream file(model_file);
        if (!file.is_open()) {
            cout << "[Agent] Warning: Cannot open model file " << model_file << ", using default transitions." << endl;
            loadDefaultTransitions();
            return;
        }
        
        transition_model.clear();
        string line;
        while (getline(file, line)) {
            // 解析PRISM模型格式: [] s=0 -> 1.0000:(s'=1);
            if (line.find("[] s=") != string::npos && line.find("->") != string::npos) {
                int state = extractState(line);
                vector<pair<int, double>> transitions = extractTransitions(line);
                if (!transitions.empty()) {
                    transition_model[state] = transitions;
                }
            }
        }
        file.close();
        cout << "[Agent] Model updated from " << model_file << ", loaded " << transition_model.size() << " states." << endl;
    }
    
    // 从行中提取状态号
    int extractState(const string& line) {
        size_t pos = line.find("s=") + 2;
        size_t end = line.find(" ", pos);
        return stoi(line.substr(pos, end - pos));
    }
    
    // 从行中提取转移信息
    vector<pair<int, double>> extractTransitions(const string& line) {
        vector<pair<int, double>> transitions;
        size_t start = line.find("-> ") + 3;
        size_t end = line.find(";");
        string transition_part = line.substr(start, end - start);
        
        // 解析多个转移，如: 0.7489:(s'=2) + 0.2511:(s'=3)
        stringstream ss(transition_part);
        string token;
        while (getline(ss, token, '+')) {
            // 清理空格
            token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
            
            size_t colon_pos = token.find(':');
            if (colon_pos != string::npos) {
                double prob = stod(token.substr(0, colon_pos));
                size_t state_start = token.find("s'=") + 3;
                size_t state_end = token.find(")", state_start);
                int next_state = stoi(token.substr(state_start, state_end - state_start));
                transitions.push_back({next_state, prob});
            }
        }
        return transitions;
    }
    
    // 加载默认转移（基于原始数据）
    void loadDefaultTransitions() {
        // 基于用户提供的path.txt路径，创建一些默认转移
        transition_model[0] = {{1, 1.0}};
        transition_model[1] = {{2, 0.7489}, {3, 0.2511}};
        transition_model[3] = {{4, 0.5976}, {5, 0.4024}};
        transition_model[5] = {{8, 1.0}};
        transition_model[8] = {{11, 0.4274}, {12, 0.5726}};
        transition_model[12] = {{16, 1.0}};
        transition_model[16] = {{20, 1.0}};
        transition_model[20] = {{24, 0.3326}, {25, 0.6674}};
        transition_model[25] = {{31, 1.0}};
        transition_model[31] = {{37, 1.0}};
        // 添加一些循环和终止状态
        transition_model[37] = {{44, 0.6}, {50, 0.4}};
        transition_model[44] = {{50, 0.7}, {59, 0.3}};
        transition_model[50] = {{67, 1.0}};
        cout << "[Agent] Loaded default transitions with " << transition_model.size() << " states." << endl;
    }
    
    // 基于概率选择下一个状态
    int selectNextState() {
        if (transition_model.find(current_state) == transition_model.end()) {
            cout << "[Agent] No transitions available from state " << current_state << ", terminating." << endl;
            return -1; // 终止标记
        }
        
        auto& transitions = transition_model[current_state];
        uniform_real_distribution<double> dist(0.0, 1.0);
        double rand_val = dist(random_gen);
        double cumulative = 0.0;
        
        for (auto& [next_state, prob] : transitions) {
            cumulative += prob;
            if (rand_val <= cumulative) {
                return next_state;
            }
        }
        
        // 如果因为精度问题没有选中，返回最后一个状态
        return transitions.back().first;
    }
    
    // 写入路径到共享文件
    void recordTransition(int next_state) {
        ofstream file(shared_path_file, ios::app);
        if (!file.is_open()) {
            cout << "[Agent] Error: Cannot write to " << shared_path_file << endl;
            return;
        }
        
        file << "tra_model " << step_counter << " " << next_state << endl;
        file.close();
        
        cout << "[Agent] Step " << step_counter << ": " << current_state << " -> " << next_state << endl;
    }

public:
    AgentSimulator(const string& path_file, const string& latest_model) 
        : current_state(0), step_counter(0), shared_path_file(path_file), 
          model_file(latest_model), random_gen(random_device{}()) {
        
        // 初始化共享路径文件
        ofstream file(shared_path_file);
        if (file.is_open()) {
            file << "action step s" << endl;
            file << "- 0 0" << endl;
            file.close();
        }
        
        loadLatestModel();
        cout << "[Agent] Agent Simulator initialized. Starting from state " << current_state << endl;
    }
    
    // 主运行循环
    void run(int max_steps = 50, int interval_ms = 3000) {
        cout << "[Agent] Starting simulation. Max steps: " << max_steps << ", Interval: " << interval_ms << "ms" << endl;
        cout << "[Agent] Press Ctrl+C to stop gracefully." << endl << endl;
        
        while (step_counter < max_steps) {
            // 每3步重新加载模型（模拟实时更新）
            if (step_counter % 3 == 0 && step_counter > 0) {
                cout << "[Agent] Reloading latest model..." << endl;
                loadLatestModel();
            }
            
            // 选择下一个状态
            int next_state = selectNextState();
            if (next_state == -1) break; // 终止条件
            
            step_counter++;
            recordTransition(next_state);
            current_state = next_state;
            
            // 等待间隔
            this_thread::sleep_for(chrono::milliseconds(interval_ms));
        }
        
        cout << endl << "[Agent] Simulation completed after " << step_counter << " steps." << endl;
        cout << "[Agent] Final state: " << current_state << endl;
    }
};

int main(int argc, char* argv[]) {
    cout << "=== BDI Agent Real-time Simulator ===" << endl;
    
    string path_file = "../data/shared_path.txt";
    string model_file = "../data/latest.pm";
    int max_steps = 30;
    int interval = 3000; // 3秒
    
    // 处理命令行参数
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--path" && i + 1 < argc) {
            path_file = argv[++i];
        } else if (arg == "--model" && i + 1 < argc) {
            model_file = argv[++i];
        } else if (arg == "--steps" && i + 1 < argc) {
            max_steps = stoi(argv[++i]);
        } else if (arg == "--interval" && i + 1 < argc) {
            interval = stoi(argv[++i]);
        } else if (arg == "--help") {
            cout << "Usage: " << argv[0] << " [options]" << endl;
            cout << "Options:" << endl;
            cout << "  --path FILE     Shared path file (default: ../data/shared_path.txt)" << endl;
            cout << "  --model FILE    Latest model file (default: ../data/latest.pm)" << endl;
            cout << "  --steps N       Maximum steps (default: 30)" << endl;
            cout << "  --interval MS   Interval between decisions in ms (default: 3000)" << endl;
            cout << "  --help          Show this help" << endl;
            return 0;
        }
    }
    
    cout << "Configuration:" << endl;
    cout << "  Shared path file: " << path_file << endl;
    cout << "  Model file: " << model_file << endl;
    cout << "  Max steps: " << max_steps << endl;
    cout << "  Interval: " << interval << "ms" << endl << endl;
    
    AgentSimulator simulator(path_file, model_file);
    simulator.run(max_steps, interval);
    
    return 0;
} 