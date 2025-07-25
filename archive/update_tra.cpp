#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>
using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "用法: " << argv[0] << " 路径txt文件名 原始tra文件名 输出tra文件名" << endl;
        return 1;
    }

    string path_file = argv[1];
    string tra_file = argv[2];
    string out_file = argv[3];

    // 1. 读取路径文件，提取状态序列
    vector<int> path_states;
    ifstream fin(path_file);
    if (!fin) {
        cout << "无法打开路径文件: " << path_file << endl;
        return 1;
    }
    string line;
    getline(fin, line); // 跳过表头
    while (getline(fin, line)) {
        istringstream iss(line);
        string action;
        int step, state;
        if (!(iss >> action >> step >> state)) continue;
        path_states.push_back(state);
    }
    fin.close();

    // 2. 生成实际出现的转移集合和from->to的唯一映射
    map<int, int> real_trans; // from -> to
    set<int> from_in_path;
    for (size_t i = 1; i < path_states.size(); ++i) {
        real_trans[path_states[i-1]] = path_states[i];
        from_in_path.insert(path_states[i-1]);
    }

    // 3. 处理tra文件
    ifstream tra_in(tra_file);
    ofstream tra_out(out_file);
    if (!tra_in) {
        cout << "无法打开tra文件: " << tra_file << endl;
        return 1;
    }
    if (!tra_out) {
        cout << "无法创建输出文件: " << out_file << endl;
        return 1;
    }

    // 复制头部（第一行）
    string header;
    getline(tra_in, header);
    tra_out << header << endl;

    // 处理每一行转移
    while (getline(tra_in, line)) {
        istringstream iss(line);
        int from, to;
        double prob;
        if (!(iss >> from >> to >> prob)) {
            tra_out << line << endl; // 直接输出无法解析的行
            continue;
        }
        if (from_in_path.count(from)) {
            if (real_trans[from] == to) {
                tra_out << from << " " << to << " 1" << endl;
            } else {
                tra_out << from << " " << to << " 0" << endl;
            }
        } else {
            tra_out << from << " " << to << " " << prob << endl;
        }
    }

    tra_in.close();
    tra_out.close();
    cout << "处理完成，输出文件: " << out_file << endl;
    return 0;
}