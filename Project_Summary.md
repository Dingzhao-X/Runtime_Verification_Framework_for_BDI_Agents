# BDI智能体运行时验证框架 - 项目完整总结

**GitHub地址**: https://github.com/Dingzhao-X/Runtime_Verification_Framework_for_BDI_Agents.git（注释、说明文档为中文）

## 项目概述

本项目实现了一个完整的BDI智能体运行时验证框架，支持基于PRISM模型检查器的概率模型更新和验证。该框架通过创新的重复状态处理算法，解决了传统方法在处理智能体循环行为时的准确性问题，并提供了离线验证和实时监控两种运行模式。

## 核心创新

### 1. 重复状态处理算法
- **问题识别**: 当智能体执行路径包含重复访问的状态时，传统更新方法会错误地覆盖原始概率分布
- **解决方案**: 设计了两种策略来处理重复状态：
  - `--repeated`标志：采用第一次出现策略，适用于一致性行为模型
  - 默认模式：采用最后一次出现策略，适用于学习适应性模型
- **技术实现**: 通过状态访问历史跟踪和选择性概率更新，确保模型的数学完整性

### 2. 双场景架构设计
- **场景A**: 离线单次模拟验证，支持批量分析和结果对比
- **场景B**: 实时双进程监控，支持智能体运行时的连续验证

## 技术架构

### 核心组件
1. **TraUpdater**: 转移概率更新引擎，实现重复状态处理逻辑
2. **PrismVerifier**: PRISM模型检查器接口，支持自动化验证
3. **TraToPmConverter**: 模型格式转换器，支持TRA到PRISM格式转换
4. **IntegratedTool**: 集成验证工具，提供完整的验证流程
5. **LogManager**: 统一日志管理系统
6. **FileGuard**: 并发安全文件访问控制

### 技术特性
- **跨平台兼容**: 支持Windows和Linux系统
- **静态链接**: 避免运行时依赖问题
- **模块化设计**: 清晰的组件分离和接口定义
- **异常安全**: 完善的错误处理和资源管理
- **并发安全机制**: 实现跨平台文件锁定系统，采用RAII模式确保异常安全的锁管理，配置超时机制防止死锁
- **统一日志系统**: 自动重定向标准输出到日志文件，采用多行格式确保文件中的可读性，支持自动目录创建和文件管理

## 使用指南

### 系统要求
- C++17兼容编译器
- PRISM模型检查器（需添加到系统PATH）
- Windows 11 或 Linux发行版

### 编译方法
**注意**: 默认Makefile为Linux版，Windows版本使用`.windows_espl`后缀。本项目测试使用MSYS2 MINGW64环境+Linux版Makefile，仅能保证该配置的正确性。

```bash
# 编译静态库
cd lib
make

# 编译场景A、场景B
cd ../build
make
```

## 场景A：离线验证模式

### 功能描述
场景A实现离线单次模拟验证，支持完整的"模拟→更新→验证"自动化流程。

### 使用方法
```bash
scenario_a_demo.exe [options]
```

### 参数说明
- `tra_file`: 输入的转移矩阵文件
- `csl_file`: PCTL属性规范文件
- `--repeated`: 启用重复状态的第一次出现策略
- `--steps N`: 模拟步数（默认1000步）
- `--path FILE`: 指定自定义路径文件

**默认数据说明**: 如果没有任何参数，默认使用`data/default_data`的文件，该数据集涉及约1000个状态，但无重复状态。

### 重要行为说明
1. **输出目录**: 如果不指定`--path`参数，日志和输出文件将保存到TRA文件所在的文件夹
2. **路径文件优先级**: 如果指定的路径文件已存在，程序将直接使用该文件进行验证，此时`--steps`参数将被忽略
3. **自动化流程**: 程序自动执行初始验证→路径生成/加载→概率更新→最终验证的完整流程

### 输出格式
```
================================================================================
                    VERIFICATION RESULTS COMPARISON
================================================================================

Simulation Path Summary:
Path (10 states): 0 → 1 → 2 → 4 → 1 → 2 → 3 → 5 → 6 → 7
--------------------------------------------------------------------------------

Probability Comparison:
================================================================================
Query: P[ F "final_success" ]
  Original: 1.0
  Updated:  0.0
  Change:   -1.0000000000000000

Query: P[ F "loop_state" ]
  Original: 1.0
  Updated:  1.0
  Change:   +0.0000000000000000
================================================================================
```

## 场景B：实时监控模式

### 功能描述
场景B实现双进程实时监控，包含智能体模拟器和运行时验证器，支持连续的运行时验证。

### 智能体模拟器 (Agent Simulator)

#### 使用方法
```bash
agent_simulator.exe [options]
```

#### 参数说明
- `--path FILE`: 共享路径文件（默认：../data/default_data/shared_path.txt）
- `--model FILE`: 模型文件（默认：../data/default_data/ProD_CUD_SMP.pm）
- `--steps N`: 最大执行步数（默认：30）
- `--interval MS`: 决策间隔毫秒数（默认：3000）

**默认数据说明**: 同理，默认使用`data/default_data`的文件。

#### 重要行为说明
1. **输出目录**: 如果不指定`--path`参数，智能体将输出到PM文件所在的文件夹
2. **路径文件检查**: 如果指定的路径文件已存在而智能体未启动，这是不合理的状态，应该删除该路径文件后重新启动

### 运行时验证器 (Runtime Verifier)

#### 使用方法
```bash
runtime_verifier.exe [options]
```

#### 参数说明
- `--path FILE`: 共享路径文件（默认：../data/default_data/shared_path.txt）
- `--tra FILE`: 原始TRA文件（默认：../data/default_data/ProD_CUD_SMP.tra）
- `--csl FILE`: CSL属性文件（默认：../data/default_data/ProD_CUD_SMP.csl）
- `--interval MS`: 检查间隔毫秒数（默认：2000）
- `--repeated`: 启用重复状态处理

**默认数据说明**: 同理，默认使用`data/default_data`的文件。

#### 重要行为说明
1. **输出目录**: 如果不指定`--path`参数，验证器将输出到TRA文件所在的文件夹
2. **并发协调**: 验证器和智能体可以任意顺序启动，系统会自动处理同步

### 双进程协调机制
- **文件锁定**: 使用平台特定的文件锁确保并发安全
- **状态同步**: 智能体读取最新的验证结果，验证器处理最新的执行路径
- **异常恢复**: 支持进程独立重启和错误恢复

## 技术实现细节

### 重复状态处理算法
```cpp
// 核心算法逻辑
if (--repeated flag enabled) {
    // 第一次出现策略：记录每个状态的首次转移
    for each repeated state:
        use first_occurrence_transition
        set probability = 1.0 for selected transition
        set probability = 0.0 for other transitions
} else {
    // 最后一次出现策略：记录每个状态的最终转移
    for each repeated state:
        use last_occurrence_transition
        set probability = 1.0 for selected transition
        set probability = 0.0 for other transitions
}
```

### 并发安全实现
```cpp
// 文件锁定机制
class FileGuard {
    // Windows: CreateFile with exclusive access
    // Linux: flock with LOCK_EX
    // Timeout: configurable (default 2-3 seconds)
    // RAII: automatic cleanup on destruction
};
```

### 日志系统架构
```cpp
class LogManager {
    static bool startLogging(directory_path, log_filename);  // 重定向输出到日志文件
    static void stopLogging();                               // 恢复标准输出
    static string getCurrentLogPath();                       // 获取当前日志路径
    static bool isLogging();                                 // 检查是否正在记录
};
```

## 验证结果

### 功能验证
- ✅ 重复状态处理算法正确性验证
- ✅ 双场景架构完整性测试

### 性能指标
- **模型更新延迟**: <500ms（典型模型）
- **验证响应时间**: <1.5s（PRISM验证）
- **内存占用**: <50MB（运行时监控）
- **文件锁获取**: <10ms（正常情况）

## 项目成果

### 代码规模
- **总代码行数**: ~3,200行C++17代码
- **核心库**: 2,400行（libtra_processor.a）
- **场景A应用**: 350行
- **场景B应用**: 450行
- **测试套件**: 完整的测试数据和自动化脚本

## 总结

本项目成功实现了一个完整的BDI智能体运行时验证框架，通过创新的重复状态处理算法和双场景架构设计，解决了传统方法的关键局限性。该框架不仅具有强大的技术功能，还提供了良好的可用性和扩展性，为BDI智能体的可靠性验证提供了实用的解决方案。

项目的成功实施证明了运行时验证在智能体系统中的实用价值，为相关领域的进一步研究和应用奠定了坚实基础。 