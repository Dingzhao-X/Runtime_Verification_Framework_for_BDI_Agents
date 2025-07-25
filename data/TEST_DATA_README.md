# 实验测试数据说明

本目录包含了三组测试数据，用于验证BDI Agent运行时验证系统的不同功能。

## 1. Simple Test (简单测试)
**文件：** `simple_test.tra`, `simple_test.csl`, `simple_test_path.txt`

**特点：**
- 6个状态，8个转移
- **无重复状态**
- 线性执行路径：0→1→3→4→5
- 用于基本功能测试

**状态转移图：**
```
0 → 1 (0.8) / 2 (0.2)
1 → 3 (0.6) / 4 (0.4)
2 → 3 (0.7) / 5 (0.3)
3 → 4 (1.0)
4 → 5 (1.0)
```

## 2. Loop Test (循环测试) ⭐**重复状态**
**文件：** `loop_test.tra`, `loop_test.csl`, `loop_test_path.txt`

**特点：**
- 8个状态，11个转移
- **包含重复状态（状态1被多次访问）**
- 执行路径：0→1→2→3→1→2→4→1→7
- 用于测试重复状态处理功能

**关键循环：**
```
1 ← 3 (0.5)  # 状态1可以从状态3返回
1 ← 4 (0.4)  # 状态1可以从状态4返回
```

**重复状态分析：**
- 状态1在路径中出现3次（步骤1、4、7）
- 状态2在路径中出现2次（步骤2、5）
- 这正是测试`--repeated`功能的理想数据

## 3. Complex Test (复杂测试)
**文件：** `complex_test.tra`, `complex_test.csl`, `complex_test_path.txt`

**特点：**
- 15个状态，22个转移
- **包含多个循环路径**
- 左右分支结构 + 循环反馈
- 执行路径包含重复：0→2→5→2→6→11→6→11→14

**复杂结构：**
```
左分支循环：1 ← 3 (0.2)
右分支循环：2 ← 5 (0.4), 6 ← 11 (0.2)
```

## 使用方法

### Scenario A 测试
```bash
# 简单测试（无重复状态）
build\scenario_a_demo.exe --tra data/simple_test.tra --csl data/simple_test.csl --path data/simple_test_path.txt

# 循环测试（有重复状态）
build\scenario_a_demo.exe --tra data/loop_test.tra --csl data/loop_test.csl --path data/loop_test_path.txt --repeated

# 复杂测试
build\scenario_a_demo.exe --tra data/complex_test.tra --csl data/complex_test.csl --path data/complex_test_path.txt --repeated
```

### Scenario B 测试
```bash
# 启动验证器（有重复状态处理）
build\runtime_verifier.exe --tra data/loop_test.tra --csl data/loop_test.csl --repeated

# 启动智能体模拟器
build\agent_simulator.exe --reference data/loop_test.tra
```

## 预期结果

### Simple Test
- 原始概率：P[F "success"] ≈ 某个值
- 更新后概率：基于观察路径调整

### Loop Test (重点)
- **重复状态处理测试**
- 状态1的转移概率应该基于**原始值**恢复
- 非重复状态使用观察值更新

### Complex Test
- 多分支概率调整
- 复杂循环结构验证

---
**注意：** Loop Test是专门设计用来测试重复状态处理功能的数据集，请确保使用`--repeated`参数！ 