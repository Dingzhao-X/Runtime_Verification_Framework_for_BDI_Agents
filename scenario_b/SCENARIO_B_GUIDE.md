# 场景B：实时运行时验证系统 - 使用指南

## 🎯 场景B概述

场景B实现了一个**实时的BDI智能体运行时验证系统**，包含两个独立但协作的程序：
1. **Agent Simulator** - 智能体模拟器：模拟BDI智能体的实时决策过程
2. **Runtime Verifier** - 运行时验证器：监控智能体行为并进行实时验证

### 系统架构
```
Agent Process          Verifier Process
     |                        |
     v                        v
[Decision Making]       [Path Monitoring]
     |                        |
     v                        v
[Record Path] <=====> [Update Model]
     |                        |
     v                        v
[shared_path.txt]       [PRISM Verification]
```

---

## 🚀 快速开始

### 1. 构建所有程序
```bash
# 方法1：使用build目录的Makefile（推荐）
cd build
make -f Makefile scenario_b

# 方法2：一次构建所有
make -f Makefile all
```

### 2. 检查构建状态
```bash
cd build
make -f Makefile status
```

### 3. 简单测试
```bash
# 在build目录下

# 测试Agent Simulator
.\agent_simulator.exe --help

# 测试Runtime Verifier  
.\runtime_verifier.exe --help
```

---

## 📋 程序详细说明

### Agent Simulator (agent_simulator.exe)

**功能：** 模拟BDI智能体的实时决策过程，根据当前模型做出决策并记录路径。

**特性：**
- ✅ 使用静态库（无外部依赖）
- ✅ 智能输出目录管理（输出到原文件所在目录）
- ✅ 动态模型重载（每3步重新加载模型）
- ✅ 随机决策引擎（基于概率分布）
- ✅ 实时路径记录

**命令行参数：**
```bash
.\agent_simulator.exe [options]

Options:
  --path FILE       共享路径文件 (默认: ../data/shared_path.txt)
  --model FILE      最新模型文件 (默认: ../data/ProD_CUD_SMP_updated.pm)  
  --reference FILE  参考文件用于确定输出目录 (默认: ../data/ProD_CUD_SMP.tra)
  --steps N         最大步数 (默认: 30)
  --interval MS     决策间隔毫秒数 (默认: 3000)
  --help            显示帮助
```

**使用示例：**
```bash
# 基本使用
.\agent_simulator.exe

# 自定义参数
.\agent_simulator.exe --steps 50 --interval 1000 --path my_path.txt

# 指定模型和输出位置
.\agent_simulator.exe --model data/custom.pm --reference data/base.tra
```

---

### Runtime Verifier (runtime_verifier.exe)

**功能：** 监控智能体的路径文件，实时更新概率模型并进行PRISM验证。

**特性：**
- ✅ 使用静态库进行TRA更新和PRISM验证
- ✅ 智能输出管理（在原文件目录生成更新文件）
- ✅ 实时路径监控
- ✅ 概率变化对比显示
- ✅ 自动化验证流程

**命令行参数：**
```bash
.\runtime_verifier.exe [options]

Options:
  --path FILE     共享路径文件 (默认: ../data/shared_path.txt)
  --tra FILE      原始TRA文件 (默认: ../data/ProD_CUD_SMP.tra)
  --csl FILE      CSL属性文件 (默认: ../data/ProD_CUD_SMP.csl)
  --interval MS   检查间隔毫秒数 (默认: 2000)
  --help          显示帮助
```

**使用示例：**
```bash
# 基本使用
.\runtime_verifier.exe

# 自定义检查间隔
.\runtime_verifier.exe --interval 1000

# 指定文件
.\runtime_verifier.exe --tra data/model.tra --csl data/props.csl
```

---

## 💡 输出文件管理

### 智能输出目录
场景B的程序会**自动将输出文件放在原始文件所在的目录**，而不是当前工作目录：

```bash
# 例如，如果原始TRA文件是 data/ProD_CUD_SMP.tra
# 那么输出文件会是：
data/ProD_CUD_SMP_updated.tra    # 更新后的TRA文件
data/ProD_CUD_SMP_updated.pm     # 更新后的PM文件  
data/shared_path.txt             # 共享路径文件（如果没有指定完整路径）
```

### 典型的输出文件
- `*_updated.tra` - 基于观察路径更新的TRA文件
- `*_updated.pm` - 转换后的PRISM模型文件
- `shared_path.txt` - 智能体的决策路径记录

---

## 🔄 实时运行演示

### 双进程模式运行

**步骤1：启动Runtime Verifier（监控进程）**
```bash
cd build
.\runtime_verifier.exe --tra ../data/ProD_CUD_SMP.tra --csl ../data/ProD_CUD_SMP.csl
```
输出：
```
=== BDI Agent Runtime Verifier ===
Configuration:
  Path file: shared_path.txt
  TRA file: ../data/ProD_CUD_SMP.tra
  CSL file: ../data/ProD_CUD_SMP.csl
  Check interval: 2000ms

[Verifier] Runtime Verifier initialized.
[Verifier] Output directory: ../data/
[Verifier] Starting real-time monitoring...
[Verifier] Press Ctrl+C to stop.
```

**步骤2：启动Agent Simulator（另一个命令行窗口）**
```bash
cd build  
.\agent_simulator.exe --reference ../data/ProD_CUD_SMP.tra --steps 20 --interval 2000
```
输出：
```
=== BDI Agent Real-time Simulator (v2) ===
Configuration:
  Shared path file: shared_path.txt
  Reference file: ../data/ProD_CUD_SMP.tra
  Max steps: 20
  Interval: 2000ms

[Agent] Agent Simulator initialized.
[Agent] Output directory: ../data/
[Agent] Starting from state 0

[Agent] Starting simulation...
[Agent] Step 1: 0 -> 1 (recorded to ../data/shared_path.txt)
[Agent] Step 2: 1 -> 3 (recorded to ../data/shared_path.txt)  
[Agent] Reloading latest model...
...
```

**同时，Verifier会检测到变化：**
```
[Verifier] New path data detected! Processing...
[Verifier] Updating TRA file based on observed path...
[Verifier] TRA file updated: ../data/ProD_CUD_SMP_updated.tra
[Verifier] Generating PRISM model file...
[Verifier] PRISM model file generated: ../data/ProD_CUD_SMP_updated.pm
[Verifier] Running PRISM verification...
[Verifier] Verification completed with 1 results

============================================================
       PROBABILITY UPDATE COMPARISON
============================================================
Query: P[F "box_1_success"&"box_2_success"]
  Before: 0.7543210  
  After:  0.8123456
  Change: +0.0580246
============================================================
```

---

## 🛠️ 构建系统

### Makefile目标
在`build/`目录下：

```bash
# 显示帮助
make -f Makefile help

# 构建所有程序
make -f Makefile all

# 只构建场景A
make -f Makefile scenario_a  

# 只构建场景B
make -f Makefile scenario_b

# 清理可执行文件
make -f Makefile clean

# 显示构建状态
make -f Makefile status
```

### 可用的可执行文件
构建完成后，`build/`目录包含：
- ✅ `scenario_a_demo.exe` - 场景A集成工具
- ✅ `simple_test.exe` - 组件测试工具
- ✅ `agent_simulator.exe` - 场景B智能体模拟器
- ✅ `runtime_verifier.exe` - 场景B运行时验证器

---

## 🎯 关键技术特性

### 1. 静态库集成 ✅
- 不再依赖外部`tra2pm.exe`和`update_tra.exe`
- 直接使用`TraProcessor`命名空间中的类
- 统一的错误处理和状态管理

### 2. 智能文件管理 ✅  
- 自动识别文件路径并输出到合适目录
- 避免文件分散在不同位置
- 支持相对路径和绝对路径

### 3. 实时通信 ✅
- 通过共享文件系统进行进程间通信
- Agent写入路径，Verifier读取并处理
- 无需复杂的IPC机制

### 4. 概率更新可视化 ✅
- 实时显示概率变化
- 支持数值变化量计算
- 清晰的对比格式

---

## 🔍 故障排除

### 常见问题

**问题1：静态库未找到**
```
ERROR: Static library not found at ../lib/libtra_processor.a
```
**解决：** 先运行`build_complete.bat`构建静态库

**问题2：PRISM未安装**
```
[Verifier] Verification failed: PRISM is not available or not in PATH
```
**解决：** 安装PRISM并确保在PATH中，或者只测试TRA更新功能

**问题3：权限问题**
```
[Agent] Warning: Cannot write to path file
```
**解决：** 确保对输出目录有写权限

**问题4：模型文件未找到**
```
[Agent] Warning: Cannot open model file, using default transitions
```
**解决：** 确保模型文件路径正确，或让程序使用默认转移

---

## 🚀 高级用法

### 自定义实验设置
```bash
# 快速测试（短间隔，少步数）
.\agent_simulator.exe --steps 10 --interval 500 &
.\runtime_verifier.exe --interval 500

# 长期运行（更真实的场景）
.\agent_simulator.exe --steps 100 --interval 5000 &  
.\runtime_verifier.exe --interval 3000

# 使用自定义数据文件
.\agent_simulator.exe --reference data/custom.tra --path data/custom_path.txt &
.\runtime_verifier.exe --tra data/custom.tra --csl data/custom.csl --path data/custom_path.txt
```

### 批量实验脚本
可以创建批处理脚本来运行多个实验：

```batch
@echo off
echo Running Scenario B Experiment...

start "Agent" .\agent_simulator.exe --steps 30 --interval 2000
timeout /t 2
start "Verifier" .\runtime_verifier.exe --interval 1500

echo Both processes started. Press any key to stop all...
pause > nul
taskkill /f /im agent_simulator.exe 2>nul
taskkill /f /im runtime_verifier.exe 2>nul
```

---

**场景B已完全实现！享受实时BDI智能体运行时验证系统！** 🎉 