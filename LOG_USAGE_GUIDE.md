# 日志功能使用指南

## 概述

所有场景（Scenario A、Agent Simulator、Runtime Verifier）现在都支持自动日志记录功能。程序运行时的所有输出（包括 `cout` 和 `cerr`）都会自动重定向到指定目录的日志文件中。

## 日志输出位置

### Scenario A (scenario_a_demo.exe)
- **日志位置**: `--tra` 参数指定文件所在目录的 `out_log.txt`
- **示例**: 如果使用 `--tra ../data/loop_test/loop_test.tra`，日志将保存到 `../data/loop_test/out_log.txt`

### Agent Simulator (agent_simulator.exe)
- **日志位置**: `--model` 参数指定文件所在目录的 `out_log_agent.txt`
- **示例**: 如果使用 `--model ../data/loop_test/loop_test.pm`，日志将保存到 `../data/loop_test/out_log_agent.txt`
- **PM文件选择**: 
  - 默认使用 `xxx_updated.pm` 文件
  - 如果存在 `xxx.pm` 文件（由带 `--repeated` 参数的 verifier 生成），则优先使用它

### Runtime Verifier (runtime_verifier.exe)
- **日志位置**: `--tra` 参数指定文件所在目录的 `out_log_verifier.txt`
- **示例**: 如果使用 `--tra ../data/loop_test/loop_test.tra`，日志将保存到 `../data/loop_test/out_log_verifier.txt`
- **路径文件自动设置**: 如果指定了 `--tra` 但没有指定 `--path`，会自动在 tra 文件所在目录生成对应的路径文件
- **重复状态处理**:
  - 使用 `--repeated` 参数时会生成两个PM文件：
    1. `xxx_updated.pm`: 用于验证的更新后模型
    2. `xxx.pm`: 用于 agent 运行时的原始模型（保留重复状态的原始概率）
  - 不使用 `--repeated` 参数时只生成 `xxx_updated.pm`

## 使用示例

### 场景A示例
```bash
# 运行场景A，日志将保存到 ../data/loop_test/out_log.txt
./scenario_a_demo.exe --tra ../data/loop_test/loop_test.tra --steps 10

# 程序结束后会显示：
# [LogManager] Log saved to: ../data/loop_test/out_log.txt
```

### Agent Simulator示例
```bash
# 运行Agent模拟器，日志将保存到 ../data/loop_test/out_log_agent.txt
./agent_simulator.exe --model ../data/loop_test/loop_test.pm --steps 5 --interval 1000

# 程序结束后会显示：
# [LogManager] Log saved to: ../data/loop_test/out_log_agent.txt
```

### Runtime Verifier示例
```bash
# 运行实时验证器，日志将保存到 ../data/loop_test/out_log_verifier.txt
# 使用--repeated参数会生成两个PM文件
./runtime_verifier.exe --tra ../data/loop_test/loop_test.tra --interval 2000 --repeated

# 程序结束后会显示：
# [LogManager] Log saved to: ../data/loop_test/out_log_verifier.txt

# 如果没有指定--path，会自动使用 ../data/loop_test/loop_test_path.txt
```

## 特性

1. **自动目录创建**: 如果指定的目录不存在，系统会自动创建
2. **完整记录**: 记录程序从开始到结束的所有输出
3. **跨平台支持**: 支持 Windows 和 Linux 系统
4. **文件覆盖**: 每次运行都会覆盖之前的日志文件
5. **进程结束记录**: 记录到进程完全结束
6. **并发安全**: 场景B中 Agent 和 Verifier 使用不同的日志文件名，避免并发冲突
7. **路径文件自动设置**: Runtime Verifier 支持根据 tra 文件自动设置路径文件位置
8. **智能PM文件选择**: Agent 会根据 Verifier 的 `--repeated` 参数自动选择合适的PM文件

## 注意事项

1. **文件权限**: 确保程序对目标目录有写入权限
2. **磁盘空间**: 长时间运行的程序可能产生较大的日志文件
3. **同时运行**: 如果多个程序同时运行并输出到同一目录，后启动的程序会覆盖之前的日志
4. **错误处理**: 如果无法创建日志文件，程序会继续运行但输出到控制台
5. **PM文件生成**: 
   - 使用 `--repeated` 参数时会生成两个PM文件
   - Agent 会优先使用原始名称的 `.pm` 文件（如果存在）
   - 如果原始名称的 `.pm` 文件不存在，则使用 `_updated.pm`
   - 原始名称的 `.pm` 文件保留了重复状态的原始概率

## 技术实现

- 使用 `LogManager`