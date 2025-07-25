#include "tra_to_pm_converter.h"
#include "tra_updater.h"
#include "prism_verifier.h"
#include "file_lock.h"
#include "log_manager.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace chrono;
using namespace TraProcessor;

class RuntimeVerifier {
private:
    string shared_path_file;
    string original_tra_file;
    string original_csl_file;
    string latest_tra_file;
    string latest_pm_file;
    string backup_tra_file;
    
    // 静态库组件
    TraProcessor::TraToPmConverter converter;
    TraProcessor::TraUpdater updater;
    TraProcessor::PrismVerifier verifier;
    
    map<string, string> last_probabilities; // PCTL性质 -> 概率值
    set<int> processed_steps;
    steady_clock::time_point last_check_time = steady_clock::now();
    int last_processed_step = 0;
    bool handle_repeated_states; // 是否处理重复状态
    
    // 从文件路径提取目录路径
    string getDirectoryPath(const string& filepath) {
        size_t pos = filepath.find_last_of("/\\");
        if (pos != string::npos) {
            return filepath.substr(0, pos + 1);
        }
        return ""; // 当前目录
    }
    
    // 从文件路径提取文件名（不含扩展名）
    string getFileStem(const string& filepath) {
        size_t last_slash = filepath.find_last_of("/\\");
        size_t last_dot = filepath.find_last_of(".");
        
        string filename = (last_slash != string::npos) ? 
                         filepath.substr(last_slash + 1) : filepath;
        
        if (last_dot != string::npos && last_dot > last_slash) {
            return filename.substr(0, last_dot - (last_slash + 1));
        }
        return filename;
    }
    
    // 检查是否有新的步数（使用文件锁）
    bool hasNewSteps() {
        FileLock path_lock(shared_path_file);
        try {
            FileGuard guard(path_lock, 500);  // 500ms超时
            
            ifstream file(shared_path_file);
            if (!file.is_open()) {
                return false;
            }
            
            int max_step = -1;
            string line;
            getline(file, line); // 跳过表头
            
            while (getline(file, line)) {
                istringstream iss(line);
                string action;
                int step, state;
                if (iss >> action >> step >> state) {
                    max_step = max(max_step, step);
                }
            }
            file.close();
            
            if (max_step > last_processed_step) {
                last_processed_step = max_step;
                return true;
            }
            return false;
            
        } catch (const std::exception& e) {
            // 如果无法获取锁，说明Agent正在写入，稍后再试
            return false;
        }
    }
    
    // 更新TRA文件
    bool updateTraFile() {
        cout << "[Verifier] Updating TRA file based on observed path..." << endl;
        
        // 输出到原文件夹
        string output_dir = getDirectoryPath(original_tra_file);
        string tra_stem = getFileStem(original_tra_file);
        latest_tra_file = output_dir + tra_stem + "_updated.tra";
        
        bool success = updater.updateTraFile(shared_path_file, original_tra_file, 
                                           latest_tra_file, handle_repeated_states);
        
        if (success) {
            cout << "[Verifier] TRA file updated: " << latest_tra_file << endl;
        } else {
            cout << "[Verifier] Failed to update TRA file!" << endl;
        }
        
        return success;
    }
    
    // 生成PM文件（使用文件锁）
    bool generatePmFile() {
        cout << "[Verifier] Generating PRISM model file..." << endl;
        
        // 输出到原文件夹
        string output_dir = getDirectoryPath(original_tra_file);
        string tra_stem = getFileStem(original_tra_file);
        
        // 生成用于验证的PM文件
        latest_pm_file = output_dir + tra_stem + "_updated.pm";
        
        // 使用文件锁保护PM文件写入
        FileLock pm_lock(latest_pm_file);
        try {
            FileGuard guard(pm_lock, 3000);  // 3秒超时
            
            bool success = converter.convertFile(latest_tra_file, latest_pm_file);
            
            if (success) {
                cout << "[Verifier] PRISM model file generated with lock: " << latest_pm_file << endl;
                
                // 如果启用了重复状态处理，还需要生成一个用于运行时的PM文件（使用原始名称）
                if (handle_repeated_states) {
                    string original_pm_file = output_dir + tra_stem + ".pm";
                    FileLock original_lock(original_pm_file);
                    try {
                        FileGuard original_guard(original_lock, 3000);
                        if (converter.convertFile(original_tra_file, original_pm_file)) {
                            cout << "[Verifier] Original PRISM model file generated with lock: " << original_pm_file << endl;
                        } else {
                            cout << "[Verifier] Failed to generate original PM file!" << endl;
                            success = false;
                        }
                    } catch (const std::exception& e) {
                        cout << "[Verifier] Failed to acquire lock for original PM file generation: " << e.what() << endl;
                        success = false;
                    }
                }
            } else {
                cout << "[Verifier] Failed to generate PM file!" << endl;
            }
            
            return success;
            
        } catch (const std::exception& e) {
            cout << "[Verifier] Failed to acquire lock for PM file generation: " << e.what() << endl;
            return false;
        }
    }
    
    // 调用PRISM进行验证
    map<string, string> runPrismVerification() {
        cout << "[Verifier] Running PRISM verification..." << endl;
        
        auto result = verifier.runVerification(latest_tra_file, original_csl_file);
        
        if (result.success) {
            cout << "[Verifier] Verification completed with " 
                 << result.probabilities.size() << " results" << endl;
            return result.probabilities;
        } else {
            cout << "[Verifier] Verification failed: " << result.error_message << endl;
            return map<string, string>();
        }
    }
    
    // 显示概率更新对比
    void displayProbabilityComparison(const map<string, string>& new_probabilities) {
        cout << "\n" << string(60, '=') << endl;
        cout << "       PROBABILITY UPDATE COMPARISON" << endl;
        cout << string(60, '=') << endl;
        
        for (const auto& [query, new_prob] : new_probabilities) {
            cout << "Query: " << query << endl;
            
            if (last_probabilities.find(query) != last_probabilities.end()) {
                cout << "  Before: " << last_probabilities[query] << endl;
                cout << "  After:  " << new_prob << endl;
                
                // 尝试比较数值变化
                try {
                    double old_val = stod(last_probabilities[query]);
                    double new_val = stod(new_prob);
                    double change = new_val - old_val;
                    cout << "  Change: " << fixed << setprecision(16) << (change >= 0 ? "+" : "") << change << endl;
                } catch (const exception&) {
                    // 如果不能转换为数字，就跳过数值比较
                }
            } else {
                cout << "  Current: " << new_prob << " (first calculation)" << endl;
            }
            cout << endl;
        }
        
        cout << string(60, '=') << endl << endl;
        
        // 更新记录
        last_probabilities = new_probabilities;
    }
    
public:
    RuntimeVerifier(const string& path_file, const string& tra_file, const string& csl_file, 
                   bool handle_repeated = false) 
        : shared_path_file(path_file), original_tra_file(tra_file), original_csl_file(csl_file),
          handle_repeated_states(handle_repeated) {
        
        // 准备备份文件路径
        string output_dir = getDirectoryPath(original_tra_file);
        string tra_stem = getFileStem(original_tra_file);
        backup_tra_file = output_dir + tra_stem + "_backup.tra";
        
        cout << "[Verifier] Runtime Verifier initialized." << endl;
        cout << "[Verifier] Monitoring: " << shared_path_file << endl;
        cout << "[Verifier] Original TRA: " << original_tra_file << endl;
        cout << "[Verifier] CSL Properties: " << original_csl_file << endl;
        cout << "[Verifier] Output directory: " << output_dir << endl;
        
        // 进行初始验证以避免遗漏任何已存在的路径数据
        cout << "[Verifier] Performing initial verification..." << endl;
        if (hasNewSteps()) {
            cout << "[Verifier] Found existing path data, processing..." << endl;
            if (updateTraFile() && generatePmFile()) {
                auto initial_probabilities = runPrismVerification();
                if (!initial_probabilities.empty()) {
                    displayProbabilityComparison(initial_probabilities);
                }
            } else {
                cout << "[Verifier] Failed to process initial data!" << endl;
            }
        } else {
            // 即使没有路径数据，也进行一次基准验证
            cout << "[Verifier] No existing path data found, running baseline verification..." << endl;
            string baseline_tra = output_dir + tra_stem + "_updated.tra";
            // 复制原始文件作为baseline
            ifstream src(original_tra_file, ios::binary);
            ofstream dst(baseline_tra, ios::binary);
            if (src && dst) {
                dst << src.rdbuf();
                src.close();
                dst.close();
                latest_tra_file = baseline_tra;
                
                if (generatePmFile()) {
                    auto baseline_probabilities = runPrismVerification();
                    if (!baseline_probabilities.empty()) {
                        displayProbabilityComparison(baseline_probabilities);
                    }
                }
            }
        }
        cout << "[Verifier] Initial verification completed." << endl << endl;
    }
    
    // 开始监控
    void startMonitoring(int check_interval_ms = 2000) {
        cout << "[Verifier] Starting real-time monitoring (initial verification already completed)..." << endl;
        cout << "[Verifier] Check interval: " << check_interval_ms << "ms" << endl;
        cout << "[Verifier] Press Ctrl+C to stop." << endl << endl;
        
        while (true) {
            if (hasNewSteps()) {
                cout << "[Verifier] New path data detected! Processing..." << endl;
                
                // 更新TRA文件
                if (updateTraFile() && generatePmFile()) {
                    // 运行验证
                    auto new_probabilities = runPrismVerification();
                    
                    if (!new_probabilities.empty()) {
                        displayProbabilityComparison(new_probabilities);
                    }
                } else {
                    cout << "[Verifier] Failed to process updates!" << endl;
                }
            }
            
            // 等待下一次检查
            this_thread::sleep_for(chrono::milliseconds(check_interval_ms));
        }
    }
};

int main(int argc, char* argv[]) {
    cout << "=== BDI Agent Runtime Verifier ===" << endl;
    
    string path_file = "../data/default_data/shared_path.txt";
    string tra_file = "../data/default_data/ProD_CUD_SMP.tra";
    string csl_file = "../data/default_data/ProD_CUD_SMP.csl";
    int check_interval = 2000; // 2秒
    bool handle_repeated_states = false;
    
    // 处理命令行参数
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--path" && i + 1 < argc) {
            path_file = argv[++i];
        } else if (arg == "--tra" && i + 1 < argc) {
            tra_file = argv[++i];
        } else if (arg == "--csl" && i + 1 < argc) {
            csl_file = argv[++i];
        } else if (arg == "--interval" && i + 1 < argc) {
            check_interval = stoi(argv[++i]);
        } else if (arg == "--repeated") {
            handle_repeated_states = true;
        } else if (arg == "--help") {
            cout << "Usage: " << argv[0] << " [options]" << endl;
            cout << "Options:" << endl;
            cout << "  --path FILE     Shared path file (default: ../data/default_data/shared_path.txt)" << endl;
            cout << "  --tra FILE      Original TRA file (default: ../data/default_data/ProD_CUD_SMP.tra)" << endl;
            cout << "  --csl FILE      CSL properties file (default: ../data/default_data/ProD_CUD_SMP.csl)" << endl;
            cout << "  --interval MS   Check interval in ms (default: 2000)" << endl;
            cout << "  --repeated      Handle repeated states (default: false)" << endl;
            cout << "  --help          Show this help" << endl;
            return 0;
        }
    }
    
    // 如果指定了tra文件但没有指定path文件，则在tra文件所在目录生成path文件
    if (tra_file != "../data/default_data/ProD_CUD_SMP.tra" && path_file == "../data/default_data/shared_path.txt") {
        // 获取tra文件的目录路径
        size_t last_slash = tra_file.find_last_of("/\\");
        string tra_dir = (last_slash != string::npos) ? tra_file.substr(0, last_slash + 1) : "./";
        // 获取tra文件的基本名称（不含扩展名）
        string tra_base = tra_file.substr(last_slash + 1);
        size_t last_dot = tra_base.find_last_of(".");
        if (last_dot != string::npos) {
            tra_base = tra_base.substr(0, last_dot);
        }
        // 设置path文件路径
        path_file = tra_dir + tra_base + "_path.txt";
    }

 
    // 启动日志记录到tra文件所在目录
    string log_directory = LogManager::extractDirectoryPath(tra_file);
    if (!LogManager::startLogging(log_directory, "out_log_verifier.txt")) {
        cerr << "Warning: Failed to start logging to " << log_directory << endl;
    }
    
    cout << "Configuration:" << endl;
    cout << "  Path file: " << path_file << endl;
    cout << "  TRA file: " << tra_file << endl;
    cout << "  CSL file: " << csl_file << endl;
    cout << "  Check interval: " << check_interval << "ms" << endl;
    cout << "  Handle repeated states: " << (handle_repeated_states ? "Yes" : "No") << endl << endl;

    RuntimeVerifier verifier(path_file, tra_file, csl_file, handle_repeated_states);
    verifier.startMonitoring(check_interval);
    
    // 停止日志记录
    LogManager::stopLogging();
    
    return 0;
} 