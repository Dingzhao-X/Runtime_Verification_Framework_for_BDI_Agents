#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <thread>
#include <chrono>
#include <filesystem>
#include <sstream>

using namespace std;
using namespace chrono;
namespace fs = filesystem;

#ifdef _WIN32
static constexpr const string UPDATE_CMD = "update_tra.exe";
static constexpr const string TRA2PM_CMD = "tra2pm.exe";
#else
static constexpr const string UPDATE_CMD = "./update_tra";
static constexpr const string TRA2PM_CMD = "./tra2pm";
#endif

class RuntimeVerifier {
private:
    string shared_path_file;
    string original_tra_file;
    string original_csl_file;
    string latest_tra_file;
    string latest_pm_file;
    string backup_tra_file;
    
    map<string, string> last_probabilities; // PCTL性质 -> 概率值
    set<int> processed_steps;
    steady_clock::time_point last_check_time = steady_clock::now();
    int last_processed_step = 0;
    
    // 检查文件是否有更新
    bool hasFileChanged() {
        if (!fs::exists(shared_path_file)) return false;
        
        auto current_time = fs::last_write_time(shared_path_file);
        auto file_time = steady_clock::now() - (steady_clock::now() - steady_clock::now()) + 
                        duration_cast<steady_clock::duration>(current_time.time_since_epoch());
        
        if (file_time > last_check_time) {
            last_check_time = steady_clock::now();
            return true;
        }
        return false;
    }
    
    // 检查是否有新的步骤
    bool hasNewSteps() {
        ifstream file(shared_path_file);
        if (!file.is_open()) return false;
        
        string line;
        int max_step = -1;
        
        while (getline(file, line)) {
            if (line.find("tra_model") != string::npos) {
                istringstream iss(line);
                string action;
                int step, state;
                if (iss >> action >> step >> state) {
                    max_step = max(max_step, step);
                }
            }
        }
        file.close();
        
        if (max_step > last_processed_step) {
            last_processed_step = max_step;
            return true;
        }
        return false;
    }
    
    // 调用外部脚本更新tra文件
    bool updateTraFile() {
        cout << "[Verifier] Updating transition probabilities..." << endl;
        
        // 构建更新命令
        string update_cmd = UPDATE_CMD + " " + shared_path_file + " " + original_tra_file + " " + latest_tra_file;
        
        cout << "[Verifier] Executing: " << update_cmd << endl;
        int result = system(update_cmd.c_str());
        
        if (result != 0) {
            cout << "[Verifier] Warning: update_tra command failed with code " << result << endl;
            return false;
        }
        
        cout << "[Verifier] Transition probabilities updated successfully." << endl;
        return true;
    }
    
    // 调用外部脚本生成pm文件
    bool generatePmFile() {
        cout << "[Verifier] Generating PRISM model file..." << endl;
        
        string convert_cmd = TRA2PM_CMD + " " + latest_tra_file + " " + latest_pm_file;
        
        cout << "[Verifier] Executing: " << convert_cmd << endl;
        int result = system(convert_cmd.c_str());
        
        if (result != 0) {
            cout << "[Verifier] Warning: tra2pm command failed with code " << result << endl;
            return false;
        }
        
        cout << "[Verifier] PRISM model file generated successfully." << endl;
        return true;
    }
    
    // 调用PRISM进行验证
    map<string, string> runPrismVerification() {
        cout << "[Verifier] Running PRISM verification..." << endl;
        
        string prism_cmd = "prism -dtmc -importtrans " + latest_tra_file + " " + original_csl_file + " > prism_output.txt 2>&1";
        
        cout << "[Verifier] Executing: " << prism_cmd << endl;
        system(prism_cmd.c_str());
        
        // 解析PRISM输出
        return parsePrismOutput("prism_output.txt");
    }
    
    // 解析PRISM输出文件
    map<string, string> parsePrismOutput(const string& output_file) {
        map<string, string> results;
        ifstream file(output_file);
        
        if (!file.is_open()) {
            cout << "[Verifier] Warning: Cannot read PRISM output file." << endl;
            return results;
        }
        
        string line;
        string current_query;
        
        while (getline(file, line)) {
            // 查找查询行
            if (line.find("P=?") != string::npos) {
                current_query = line;
                // 简化查询显示
                size_t start = line.find("[") + 1;
                size_t end = line.find("]");
                if (start != string::npos && end != string::npos) {
                    string formula = line.substr(start, end - start);
                    current_query = "P[" + formula + "]";
                }
            }
            
            // 查找结果行
            if (line.find("Result:") != string::npos && !current_query.empty()) {
                size_t pos = line.find("Result:") + 7;
                string result = line.substr(pos);
                // 清理空格
                result.erase(0, result.find_first_not_of(" \t"));
                result.erase(result.find_last_not_of(" \t\n\r") + 1);
                
                results[current_query] = result;
                current_query.clear();
            }
        }
        
        file.close();
        return results;
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
                    cout << "  Change: " << (change >= 0 ? "+" : "") << change << endl;
                } catch (const exception&) {
                    // 如果不能转换为数字，就跳过数值比较
                }
            } else {
                cout << "  Current: " << new_prob << " (first calculation)" << endl;
            }
            cout << endl;
        }
        
        cout << string(60, '=') << endl << endl;
        
        // 更新历史记录
        last_probabilities = new_probabilities;
    }
    
    // 创建备份文件
    void createBackupFiles() {
        if (!fs::exists(backup_tra_file) && fs::exists(original_tra_file)) {
            fs::copy_file(original_tra_file, backup_tra_file);
            cout << "[Verifier] Created backup file: " << backup_tra_file << endl;
        }
        
        // 初始化latest文件
        if (fs::exists(original_tra_file)) {
            fs::copy_file(original_tra_file, latest_tra_file, fs::copy_options::overwrite_existing);
        }
    }

public:
    RuntimeVerifier(const string& path_file, const string& tra_file, const string& csl_file) 
        : shared_path_file(path_file), original_tra_file(tra_file), original_csl_file(csl_file) {
        
        string tra_file_name = fs::path(tra_file).stem().string();
        latest_tra_file = tra_file_name + "_latest.tra";
        latest_pm_file = tra_file_name + "_latest.pm";
        backup_tra_file = tra_file_name + "_backup.tra";
        
        cout << "[Verifier] Runtime Verifier initialized." << endl;
        cout << "[Verifier] Monitoring file: " << shared_path_file << endl;
        cout << "[Verifier] Original TRA: " << original_tra_file << endl;
        cout << "[Verifier] Original CSL: " << original_csl_file << endl;
        cout << "[Verifier] Latest TRA: " << latest_tra_file << endl;
        cout << "[Verifier] Latest PM: " << latest_pm_file << endl << endl;
        
        createBackupFiles();
        
        // 初始验证
        cout << "[Verifier] Running initial verification..." << endl;
        if (generatePmFile()) {
            auto initial_probs = runPrismVerification();
            displayProbabilityComparison(initial_probs);
        }
    }
    
    // 主监控循环
    void startMonitoring(int check_interval_ms = 2000) {
        cout << "[Verifier] Starting file monitoring. Check interval: " << check_interval_ms << "ms" << endl;
        cout << "[Verifier] Press Ctrl+C to stop monitoring." << endl << endl;
        
        while (true) {
            if (hasNewSteps()) {
                cout << "[Verifier] New steps detected! Processing updates..." << endl;
                
                // 执行更新流程
                if (updateTraFile() && generatePmFile()) {
                    auto new_probabilities = runPrismVerification();
                    displayProbabilityComparison(new_probabilities);
                } else {
                    cout << "[Verifier] Error in update process, skipping verification." << endl;
                }
                
                cout << "[Verifier] Waiting for next changes..." << endl << endl;
            }
            
            // 等待下次检查
            this_thread::sleep_for(chrono::milliseconds(check_interval_ms));
        }
    }
};

int main(int argc, char* argv[]) {
    cout << "=== Runtime Verification Monitor for BDI Agents ===" << endl;
    
    string path_file = "shared_path.txt";
    string tra_file = "ProD_CUD_SMP.tra";
    string csl_file = "ProD_CUD_SMP.csl";
    int interval = 2000; // 2秒
    
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
            interval = stoi(argv[++i]);
        } else if (arg == "--help") {
            cout << "Usage: " << argv[0] << " [options]" << endl;
            cout << "Options:" << endl;
            cout << "  --path FILE     Shared path file to monitor (default: shared_path.txt)" << endl;
            cout << "  --tra FILE      Original TRA file (default: ProD_CUD_SMP.tra)" << endl;
            cout << "  --csl FILE      CSL properties file (default: ProD_CUD_SMP.csl)" << endl;
            cout << "  --interval MS   Check interval in ms (default: 2000)" << endl;
            cout << "  --help          Show this help" << endl;
            return 0;
        }
    }
    
    // 检查必需文件是否存在
    if (!fs::exists(tra_file)) {
        cout << "Error: Original TRA file not found: " << tra_file << endl;
        return 1;
    }
    if (!fs::exists(csl_file)) {
        cout << "Error: CSL file not found: " << csl_file << endl;
        return 1;
    }
    
    cout << "Configuration:" << endl;
    cout << "  Monitoring: " << path_file << endl;
    cout << "  Original TRA: " << tra_file << endl;
    cout << "  CSL Properties: " << csl_file << endl;
    cout << "  Check interval: " << interval << "ms" << endl << endl;
    
    RuntimeVerifier verifier(path_file, tra_file, csl_file);
    verifier.startMonitoring(interval);
    
    return 0;
} 