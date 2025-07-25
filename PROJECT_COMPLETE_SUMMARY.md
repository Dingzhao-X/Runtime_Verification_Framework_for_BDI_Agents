# 🎉 BDI智能体运行时验证项目 - 完成总结

## 📋 项目概述

本项目实现了基于**BDI智能体的运行时验证系统**，包含两个核心场景：
- **场景A**：离线单次模拟和自动化验证工具
- **场景B**：实时双进程运行时验证系统

### 核心贡献
1. ✅ **重复状态处理算法** - 理论创新点
2. ✅ **静态库架构** - 模块化和可重用性
3. ✅ **智能输出管理** - 文件自动组织
4. ✅ **实时验证系统** - 双进程协作

---

## 🏗️ 项目架构

### 文件结构
```
工作区域/
├── lib/                          # 静态库
│   ├── include/                  # 头文件 (#pragma once)
│   │   ├── common_types.h        # 通用数据结构
│   │   ├── tra_to_pm_converter.h # TRA→PM转换
│   │   ├── tra_updater.h         # 概率更新（含重复状态）
│   │   ├── prism_verifier.h      # PRISM验证接口
│   │   └── integrated_tool.h     # 场景A集成工具
│   ├── src/                      # 实现文件
│   ├── object/                   # 目标文件
│   ├── Makefile                  # 库构建脚本
│   └── libtra_processor.a        # 静态库文件
├── scenario_a/                   # 场景A源码
│   ├── scenario_a_demo.cpp       # 完整集成工具
│   └── simple_test.cpp           # 组件测试工具
├── scenario_b/                   # 场景B源码
│   ├── agent_simulator_v2.cpp    # 智能体模拟器
│   └── runtime_verifier_v2.cpp   # 运行时验证器
├── build/                        # 可执行文件输出
│   ├── Makefile                  # 应用程序构建脚本
│   ├── scenario_a_demo.exe       # 场景A工具
│   ├── simple_test.exe           # 测试工具
│   ├── agent_simulator.exe       # 场景B智能体
│   └── runtime_verifier.exe      # 场景B验证器
├── data/                         # 数据文件
│   ├── ProD_CUD_SMP.tra         # 原始转移矩阵
│   └── ProD_CUD_SMP.csl         # PCTL属性文件
└── build_complete.bat            # 一键构建脚本
```

### 技术栈
- **C++17** - 现代C++特性
- **静态库** - 代码重用和模块化
- **PRISM** - 概率模型检查器
- **Make** - 构建系统
- **多进程** - 实时通信

---

## 🎯 场景A：离线自动化工具

### 功能概述
**场景A**实现了一个**完整的5步自动化验证工具**：
1. 初始PRISM验证 → 获取原始概率
2. TRA到PM转换 → 生成PRISM模型
3. 路径模拟 → 生成观察序列
4. 概率更新 → 基于观察路径调整模型
5. 最终验证 → 对比概率变化

### 可用程序
```bash
# 完整集成工具
build/scenario_a_demo.exe --tra data/ProD_CUD_SMP.tra --csl data/ProD_CUD_SMP.csl

# 支持重复状态处理
build/scenario_a_demo.exe --repeated

# 组件测试工具
build/simple_test.exe
```

### 核心特性
- ✅ **一键自动化** - 无需手动干预
- ✅ **重复状态处理** - 理论贡献实现
- ✅ **概率对比可视化** - 清晰的结果展示
- ✅ **临时文件管理** - 自动清理

---

## 🔄 场景B：实时验证系统

### 功能概述
**场景B**实现了一个**双进程实时验证系统**：
- **Agent Process** - 模拟BDI智能体决策，记录路径
- **Verifier Process** - 监控路径变化，实时更新模型并验证

### 工作流程
```
Agent Simulator ----→ shared_path.txt ----→ Runtime Verifier
      |                                            |
      ↓                                            ↓
[根据模型决策]                               [监控路径变化]
      |                                            |
      ↓                                            ↓
[记录决策路径] ←--- updated.pm ←--- [更新模型并验证]
```

### 可用程序
```bash
# 启动验证器（后台监控）
build/runtime_verifier.exe --tra ../data/ProD_CUD_SMP.tra --csl ../data/ProD_CUD_SMP.csl

# 启动智能体（另一个窗口）  
build/agent_simulator.exe --reference ../data/ProD_CUD_SMP.tra --steps 30
```

### 核心特性  
- ✅ **实时双进程** - Agent + Verifier协作
- ✅ **智能文件管理** - 输出到原文件目录
- ✅ **动态模型重载** - 每3步更新模型
- ✅ **实时概率对比** - 持续监控变化

---

## 🛠️ 构建系统

### 一键构建
```bash
# 构建所有组件（库+场景A+场景B）
.\build_complete.bat
```

### Makefile支持
```bash
# 在build目录下
cd build

# 构建所有应用程序
make -f Makefile all

# 只构建场景A
make -f Makefile scenario_a

# 只构建场景B  
make -f Makefile scenario_b

# 检查构建状态
make -f Makefile status
```

### 静态库构建
```bash
# 在lib目录下
cd lib
make -f Makefile all
```

---

## 💡 关键技术创新

### 1. 重复状态处理算法 ⭐
**问题**：观察路径中出现重复状态时，如何正确更新转移概率？

**解决方案**：
- 创建原始TRA文件的备份
- 首次遇到状态：固定概率为0/1
- 再次遇到状态：恢复原始概率
- 确保概率更新的一致性

```cpp
// 核心算法在 TraUpdater::processTraFileWithRepeatedStates
if (fixed_states.count(from)) {
    // 重复状态：使用原始概率
    prob = original_probabilities[{from, to}];
} else {
    // 首次状态：固定为0或1
    fixed_states.insert(from);
    prob = (actual_transition == to) ? 1.0 : 0.0;
}
```

### 2. 智能输出管理 ⭐
**特性**：自动将输出文件放置在输入文件所在目录

**实现**：
```cpp
string getDirectoryPath(const string& filepath) {
    size_t pos = filepath.find_last_of("/\\");
    return (pos != string::npos) ? filepath.substr(0, pos + 1) : "";
}

// 例如：data/model.tra → 输出到 data/model_updated.tra
```

### 3. 模块化静态库设计 ⭐
**TraProcessor命名空间**包含4个核心组件：
- `TraToPmConverter` - TRA转PM转换
- `TraUpdater` - 概率更新（含重复状态处理）
- `PrismVerifier` - PRISM验证接口
- `IntegratedTool` - 场景A集成工具

### 4. 跨平台兼容性 ⭐
**特性**：
- 使用`-static-libgcc -static-libstdc++`避免运行时依赖
- 兼容Windows PowerShell和标准终端
- 路径处理支持`/`和`\`分隔符

---

## 📊 实验能力

### 场景A实验
- **自动化5步流程**
- **概率变化量化分析**  
- **重复状态理论验证**
- **大规模路径模拟**

### 场景B实验
- **实时验证性能测试**
- **双进程通信延迟分析**
- **动态模型更新效果**
- **长时间运行稳定性**

### 数据生成
- **概率对比表格** - 用于论文Results部分
- **系统架构图** - 已生成4个SVG图表
- **性能基准测试** - 可量化系统响应时间

---

## 🎓 论文贡献总结

### 理论贡献
1. **重复状态处理算法** - 新颖的概率更新方法
2. **实时验证架构** - BDI智能体运行时验证框架

### 技术贡献  
1. **模块化设计** - 可重用的静态库架构
2. **自动化工具链** - 端到端验证流程
3. **实时系统实现** - 双进程协作模式

### 实践贡献
1. **完整实现** - 从理论到可运行系统
2. **实验平台** - 支持多种验证场景
3. **可扩展架构** - 易于添加新功能

---

## 🚀 使用快速指南

### 场景A：一键验证
```bash
# 基本验证
build\scenario_a_demo.exe

# 使用重复状态处理
build\scenario_a_demo.exe --repeated

# 自定义文件
build\scenario_a_demo.exe --tra your_model.tra --csl your_props.csl
```

### 场景B：实时验证
```bash
# 窗口1：启动验证器
build\runtime_verifier.exe

# 窗口2：启动智能体  
build\agent_simulator.exe

# 观察实时概率变化输出
```

### 组件测试
```bash
# 测试静态库组件
build\simple_test.exe

# 检查构建状态
cd build && make -f Makefile status
```

---

## 📈 项目成果

### ✅ 完成的功能
- **场景A**：完整的5步自动化工具
- **场景B**：实时双进程验证系统  
- **静态库**：4个核心组件全部工作
- **构建系统**：一键构建和Makefile支持
- **文档**：详细的使用指南和技术说明

### ✅ 技术质量
- **代码规范**：现代C++17，#pragma once
- **错误处理**：完善的异常处理和状态检查
- **性能优化**：静态链接，避免运行时依赖
- **跨平台**：Windows/Linux兼容设计

### ✅ 实验就绪
- **数据生成**：支持概率对比和性能分析
- **图表素材**：4个系统架构和算法流程图
- **测试用例**：多种验证场景和参数组合
- **论文支持**：理论贡献的完整实现

---

## 🎊 项目状态

**🟢 场景A** - 完全实现并测试通过  
**🟢 场景B** - 完全实现并测试通过  
**🟢 静态库** - 4个组件全部工作  
**🟢 构建系统** - 多种构建方式支持  
**🟢 文档** - 完整的使用和技术文档  

**项目已完成！可以专注论文写作和实验数据生成！** 🎉

---

**下一步建议：**
1. 使用场景A生成论文所需的概率对比数据
2. 运行场景B进行实时验证演示
3. 基于完整技术实现撰写论文内容
4. 利用生成的图表和结果完善论文

**技术实现部分已完美完成！** ✨ 