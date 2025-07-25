#include "tra_to_pm_converter.h"
#include "file_lock.h"
#include "log_manager.h"
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
using namespace std::this_thread;
using namespace std::chrono;
using namespace TraProcessor;

class AgentSimulator {
private:
    int current_state;
    int step_counter;
    map<int, vector<pair<int, double>>> transition_model; // state -> [(next_state, prob), ...]
    string shared_path_file;
    string model_file;
    string output_directory;
    mt19937 random_gen;
    
    // 从文件路径提取目录路径
    string getDirectoryPath(const string& filepath) {
        size_t pos = filepath.find_last_of("/\\");
        if (pos != string::npos) {
            return filepath.substr(0, pos + 1);
        }
        return ""; // 当前目录
    }
    
    // 从模型文件读取转移概率，并尝试从updated版本更新模型（使用文件锁）
    void loadLatestModel() {
        // 如果model_file包含路径，就使用完整路径；否则使用输出目录
        string model_path = (model_file.find('/') != string::npos || model_file.find('\\') != string::npos) ? 
                           model_file : (output_directory + model_file);
        
        // 尝试从updated版本更新当前模型
        string updated_path = model_path;
        size_t dot_pos = updated_path.find_last_of(".");
        if (dot_pos != string::npos) {
            updated_path = updated_path.substr(0, dot_pos) + "_updated.pm";
            
            // 检查updated文件是否存在且比原文件新
            ifstream check_updated(updated_path);
            if (check_updated.good()) {
                check_updated.close();
                
                // 使用文件锁将updated文件覆盖到原文件
                FileLock updated_lock(updated_path);
                FileLock original_lock(model_path);
                try {
                    FileGuard updated_guard(updated_lock, 2000);
                    FileGuard original_guard(original_lock, 2000);
                    
                    // 复制updated文件内容到原文件
                    ifstream src(updated_path, ios::binary);
                    ofstream dst(model_path, ios::binary);
                    if (src.is_open() && dst.is_open()) {
                        dst << src.rdbuf();
                        cout << "[Agent] Model updated from " << updated_path << " to " << model_path << endl;
                    }
                    src.close();
                    dst.close();
                    
                } catch (const std::exception& e) {
                    cout << "[Agent] Failed to update model: " << e.what() << endl;
                }
            }
        }
        
        // 使用文件锁保护PM文件读取
        FileLock pm_lock(model_path);
        try {
            FileGuard guard(pm_lock, 2000);  // 2秒超时
            
            ifstream file(model_path);
            if (!file.is_open()) {
                cout << "[Agent] Warning: Cannot open model file " << model_path 
                     << ", using default transitions." << endl;
                loadDefaultTransitions();
                return;
            }
            
            cout << "[Agent] Successfully acquired lock for reading PM file: " << model_path << endl;
        
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
            cout << "[Agent] Model updated from " << model_path << ", loaded " 
                 << transition_model.size() << " states." << endl;
                 
        } catch (const std::exception& e) {
            cout << "[Agent] Failed to acquire lock for PM file: " << e.what() 
                 << ", using default transitions." << endl;
            loadDefaultTransitions();
        }
    }
    
    // 从行中提取状态号
    int extractState(const string& line) {
        size_t pos = line.find("s=") + 2;
        size_t end = line.find(" ", pos);
        if (end == string::npos) end = line.find("\t", pos);
        if (end == string::npos) end = line.length();
        
        string state_str = line.substr(pos, end - pos);
        return stoi(state_str);
    }
    
    // 从行中提取转移概率
    vector<pair<int, double>> extractTransitions(const string& line) {
        vector<pair<int, double>> transitions;
        
        size_t start = line.find("->") + 2;
        string transitions_part = line.substr(start);
        
        // 处理多个转移，用'+'分隔
        stringstream ss(transitions_part);
        string transition;
        
        while (getline(ss, transition, '+')) {
            // 清理空格
            transition.erase(0, transition.find_first_not_of(" \t"));
            transition.erase(transition.find_last_not_of(" \t;") + 1);
            
            // 解析格式：0.7000:(s'=1)
            size_t colon_pos = transition.find(':');
            size_t state_start = transition.find("s'=") + 3;
            size_t state_end = transition.find(')', state_start);
            
            if (colon_pos != string::npos && state_start != string::npos && state_end != string::npos) {
                double prob = stod(transition.substr(0, colon_pos));
                int next_state = stoi(transition.substr(state_start, state_end - state_start));
                transitions.push_back({next_state, prob});
            }
        }
        
        return transitions;
    }
    
    // 加载默认转移（简单的顺序转移）
    void loadDefaultTransitions() {
        transition_model.clear();
        for (int i = 0; i < 50; ++i) {
            if (i < 49) {
                transition_model[i] = {{i + 1, 1.0}};
            }
            // 状态49作为终止状态
        }
        cout << "[Agent] Loaded default transitions (sequential 0->1->...->49)" << endl;
    }
    
    // 根据当前模型选择下一个状态
    int selectNextState() {
        if (transition_model.find(current_state) == transition_model.end() ||
            transition_model[current_state].empty()) {
            cout << "[Agent] No transitions available from state " << current_state << endl;
            return -1; // 终止
        }
        
        const auto& transitions = transition_model[current_state];
        
        // 生成随机数
        uniform_real_distribution<double> dist(0.0, 1.0);
        double rand_val = dist(random_gen);
        
        // 根据概率选择
        double cumulative = 0.0;
        for (const auto& [next_state, prob] : transitions) {
            cumulative += prob;
            if (rand_val <= cumulative) {
                return next_state;
            }
        }
        
        // 如果没有选中（概率不完整），选择最后一个
        return transitions.back().first;
    }
    
    // 记录转移到共享文件，输出到正确的目录（使用文件锁）
    void recordTransition(int next_state) {
        // 确保输出到正确的目录
        string output_path = (shared_path_file.find('/') != string::npos || 
                             shared_path_file.find('\\') != string::npos) ? 
                            shared_path_file : (output_directory + shared_path_file);
        
        // 使用文件锁保护path.txt文件写入
        FileLock path_lock(output_path);
        try {
            FileGuard guard(path_lock, 1000);  // 1秒超时
            
            // 如果是第一步，创建文件并写入表头
            if (step_counter == 1) {
                ofstream file(output_path, ios::out);
                if (file.is_open()) {
                    file << "Action Step State" << endl;
                    file << "act " << (step_counter - 1) << " " << current_state << endl;
                    file.close();
                }
            }
            
            // 追加新的转移
            ofstream file(output_path, ios::app);
            if (file.is_open()) {
                file << "act " << step_counter << " " << next_state << endl;
                file.close();
                
                cout << "[Agent] Step " << step_counter << ": " << current_state 
                     << " -> " << next_state << endl;
            } else {
                cout << "[Agent] Warning: Cannot write to path file " << output_path << endl;
            }
            
        } catch (const std::exception& e) {
            cout << "[Agent] Failed to acquire lock for path file: " << e.what() << endl;
        }
    }
    
public:
    AgentSimulator(const string& path_file, const string& model_file_name) 
        : current_state(0), step_counter(0), shared_path_file(path_file), 
          model_file(model_file_name), random_gen(random_device{}()) {
        
        // 使用model_file的目录作为输出目录
        output_directory = getDirectoryPath(model_file_name);
        
        cout << "[Agent] Agent Simulator initialized." << endl;
        cout << "[Agent] Shared path file: " << shared_path_file << endl;
        cout << "[Agent] Model file: " << model_file << endl;
        cout << "[Agent] Output directory: " << (output_directory.empty() ? "current" : output_directory) << endl;
        cout << "[Agent] Starting from state " << current_state << endl;
        
        loadLatestModel();
    }
    
    // 主运行循环
    void run(int max_steps = 50, int interval_ms = 3000) {
        cout << "[Agent] Starting simulation. Max steps: " << max_steps 
             << ", Interval: " << interval_ms << "ms" << endl;
        cout << "[Agent] Press Ctrl+C to stop gracefully." << endl << endl;
        
        while (step_counter < max_steps) {
            // 每3步重新加载模型（模拟实时更新）
            if (step_counter % 3 == 0 && step_counter > 0) {
                cout << "[Agent] Reloading latest model..." << endl;
                loadLatestModel();
            }
            
            // 选择下一个状态
            int next_state = selectNextState();
            if (next_state == -1) {
                cout << "[Agent] Reached terminal state, stopping." << endl;
                break; // 终止条件
            }
            
            step_counter++;
            recordTransition(next_state);
            current_state = next_state;
            
            // 等待间隔
            this_thread::sleep_for(chrono::milliseconds(interval_ms));
        }
        
        cout << endl << "[Agent] Simulation completed after " << step_counter << " steps." << endl;
        cout << "[Agent] Final state: " << current_state << endl;
        
        // 显示完整路径文件信息
        string final_path = (shared_path_file.find('/') != string::npos || 
                           shared_path_file.find('\\') != string::npos) ? 
                          shared_path_file : (output_directory + shared_path_file);
        cout << "[Agent] Complete path recorded in: " << final_path << endl;
    }
};

int main(int argc, char* argv[]) {
    cout << "=== BDI Agent Real-time Simulator ===" << endl;
    
    string path_file = "../data/default_data/shared_path.txt";
    string model_file = "../data/default_data/ProD_CUD_SMP.pm";  // 默认使用原始模型
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
            cout << "  --path FILE       Shared path file (default: ../data/default_data/shared_path.txt)" << endl;
            cout << "  --model FILE      Model file (default: ../data/default_data/ProD_CUD_SMP.pm)" << endl;
            cout << "  --steps N         Maximum steps (default: 30)" << endl;
            cout << "  --interval MS     Interval between decisions in ms (default: 3000)" << endl;
            cout << "  --help            Show this help" << endl;
            return 0;
        }
    }
    
    // 如果指定了pm文件但没有指定path文件，则在pm文件所在目录生成path文件
    if (model_file != "../data/default_data/ProD_CUD_SMP.pm" && path_file == "../data/default_data/shared_path.txt") {
        // 获取pm文件的目录路径
        size_t last_slash = model_file.find_last_of("/\\");
        string model_dir = (last_slash != string::npos) ? model_file.substr(0, last_slash + 1) : "./";
        // 获取pm文件的基本名称（不含扩展名）
        string model_base = model_file.substr(last_slash + 1);
        size_t last_dot = model_base.find_last_of(".");
        if (last_dot != string::npos) {
            model_base = model_base.substr(0, last_dot);
        }
        // 设置path文件路径
        path_file = model_dir + model_base + "_path.txt";
        
        // 如果path文件已存在，删除它以确保覆盖
        ifstream check_file(path_file);
        if (check_file.good()) {
            check_file.close();
            if (remove(path_file.c_str()) == 0) {
                cout << "[Agent] Removed existing path file: " << path_file << endl;
            } else {
                cout << "[Agent] Warning: Failed to remove existing path file: " << path_file << endl;
            }
        }
    }
    
    // 启动日志记录到model文件所在目录
    string log_directory = LogManager::extractDirectoryPath(model_file);
    if (!LogManager::startLogging(log_directory, "out_log_agent.txt")) {
        cerr << "Warning: Failed to start logging to " << log_directory << endl;
    }
    
    cout << "Configuration:" << endl;
    cout << "  Shared path file: " << path_file << endl;
    cout << "  Model file: " << model_file << endl;
    cout << "  Max steps: " << max_steps << endl;
    cout << "  Interval: " << interval << "ms" << endl << endl;
    
    AgentSimulator simulator(path_file, model_file);
    simulator.run(max_steps, interval);
    
    // 停止日志记录
    LogManager::stopLogging();
    
    return 0;
} 