# 斗地主Bot测试套件使用指南

## 目录

1. [快速开始](#1-快速开始)
2. [目录结构](#2-目录结构)
3. [命令行参数](#3-命令行参数)
4. [日志系统](#4-日志系统)
5. [模块说明](#5-模块说明)
6. [常见问题](#6-常见问题)

---

## 1. 快速开始

### 环境准备

```bash
# 激活虚拟环境
testsuite_env\Scripts\activate
```

### 运行测试（10局，默认 my_bot2.exe）

```bash
python run_testsuite.py
```

### 指定Bot和局数

```bash
# 用 my_bot.exe 测试20局
python run_testsuite.py --bot my_bot.exe --games 20

# 固定随机种子（可复现）
python run_testsuite.py --seed 42 --games 5

# 设置单步超时3秒
python run_testsuite.py --timeout 3.0

# 不输出到控制台（仅写日志文件）
python run_testsuite.py --no-console
```

### 100局分析测试

```bash
python testsuite\analyze_100.py
```

### 验证导入是否正常

```bash
python testsuite\test_import.py
```

---

## 2. 目录结构

```
课设--暂定/
├── run_testsuite.py                       # 主入口脚本（命令行启动）
├── testsuite/
│   ├── __init__.py                        # 包初始化
│   ├── USAGE.md                           # 本文件（使用指南）
│   ├── config.py                          # 配置管理（默认Bot路径、超时等）
│   ├── card.py                            # 牌面工具（54张牌映射、发牌）
│   ├── bot_runner.py                      # Bot子进程管理（JSON通信、超时检测）
│   ├── simple_bot.py                      # 简单Bot（崩溃时替补出牌）
│   ├── judge.py                           # 裁判（游戏规则、叫分、出牌、胜负判定）
│   ├── logger.py                          # 日志系统（中文输出）
│   ├── runner.py                          # 测试运行器（多局统筹、统计报告）
│   ├── debug_crash.py                     # 崩溃调试脚本
│   ├── verify_json.py                     # JSON协议验证脚本
│   ├── analyze_100.py                     # 多局分析报告脚本
│   ├── test_import.py                     # 导入验证脚本
│   ├── .gitignore
│   └── output/                            # 测试输出目录（自动生成）
│       ├── games/                         # 每局详细日志
│       ├── summary/                       # 汇总报告
│       └── test_*.log                     # 全局日志
└── testsuite_env/                         # Python虚拟环境
```

---

## 3. 命令行参数

`run_testsuite.py` 支持以下参数：

| 参数 | 简写 | 说明 | 默认值 |
|------|------|------|--------|
| `--games` | `-n` | 测试局数 | 10 |
| `--bot` | `-b` | Bot可执行文件路径 | my_bot2.exe |
| `--seed` | `-s` | 随机种子（指定后可复现） | 随机 |
| `--timeout` | `-t` | 单步超时时间（秒） | 5.0 |
| `--rounds` | `-r` | 每局最大回合数 | 100 |
| `--output` | `-o` | 日志输出目录 | testsuite/output |
| `--no-console` | — | 关闭控制台日志 | False |
| `--version` | `-v` | 显示版本号 | — |

### 示例组合

```bash
# 复现测试：固定种子+指定Bot
python run_testsuite.py --seed 12345 --bot my_bot2.exe --games 10

# 压力测试：加快速度（关闭日志）
python run_testsuite.py --games 50 --timeout 3.0 --no-console
```

---

## 4. 日志系统

### 日志目录结构

```
testsuite/output/
├── games/
│   ├── game_0001.log       # 第1局详细日志
│   ├── game_0002.log       # 第2局详细日志
│   └── ...
├── summary/
│   └── summary_20260509_153635.txt  # 汇总报告
└── test_20260509_153635.log         # 全局运行日志
```

### 日志内容示例

```
[游戏0001] 玩家0 (Bot-地主) 出牌: ♣4♣5♦6♥7♠8♥9♦10♥J♥Q♠K | 剩余10张
[游戏0001] 玩家1 (Bot-农民) 过牌
[游戏0001] 玩家2 (Bot-农民) 出牌: ♠2♥2♣2 (三带二) 剩余12张
```

### 汇总报告示例

```
============================================================
斗地主Bot测试报告
生成时间: 2026-05-09 15:36:35
============================================================

总游戏局数: 10
平局/异常局数: 0

各Bot统计:
  玩家0: 胜5局 (胜率50.0%) | 崩溃0次 | 超时0次 | 平均0.876秒 | 最大0.970秒
  玩家1: 胜2局 (胜率20.0%) | 崩溃0次 | 超时0次 | 平均0.872秒 | 最大0.960秒
  玩家2: 胜3局 (胜率30.0%) | 崩溃0次 | 超时0次 | 平均0.870秒 | 最大0.960秒
```

---

## 5. 模块说明

### 5.1 裁判（judge.py）

裁判负责管理一局完整的斗地主对战：

1. **发牌**：使用可指定种子的随机数生成器
2. **叫分**：按Botzone规则（0→1→2号玩家轮流叫分，3分立即结束）
3. **出牌**：地主先出，轮流跟牌，连续两人过牌则出牌权重置
4. **胜负判定**：有人出完所有牌即结束
5. **崩溃处理**：Bot崩溃/超时后用简单Bot代替，游戏不会卡死

### 5.2 Bot执行器（bot_runner.py）

负责与被测Bot可执行文件通信：

- 通过 `subprocess.Popen` 启动Bot子进程
- 输入：`{"requests": [...], "responses": [...], "data": ""}`
- 输出：`{"response": ..., "data": ""}`
- 支持超时检测（`subprocess.TimeoutExpired`）
- 支持崩溃检测（非0返回码、JSON解析失败）

### 5.3 简单Bot（simple_bot.py）

当被测Bot崩溃或超时时的替补方案：

- **叫分**：总是叫0分（不叫）
- **主动出牌**：出点数最小的单张
- **跟牌**：有更大的单张就出，否则过牌

### 5.4 牌面工具（card.py）

提供54张牌的完整映射和基本操作：

- 牌号0~53 → 点数3~17
- 花色显示（♥♦♠♣）
- 发牌（每人17张+3张底牌）

### 5.5 配置管理（config.py）

默认配置：

```python
DEFAULT_CONFIG = {
    "bot_path": "my_bot2.exe",       # Bot可执行文件
    "bot_timeout": 5.0,               # 单步超时（秒）
    "seed": None,                      # 随机种子
    "max_rounds": 100,                 # 每局最大回合数
    "num_games": 10,                   # 测试局数
    "log_dir": "testsuite/output",     # 日志目录
    "log_level": "INFO",               # 日志级别
    "log_to_console": True,            # 控制台输出
}
```

---

## 6. 常见问题

### 6.1 Bot一直崩溃怎么办？

检查以下几点：

1. **JSON协议**：`history` 字段传的是**牌号(0-53)**而非点数(3-17)，Bot内部自己转
2. **首次出牌**：`own` 字段始终传最初的17张牌（不含底牌），即使Bot是地主
3. **连续调用**：每次调用时 `requests` 数组包含所有历史request，不能只传当前这个

### 6.2 三个Bot都跑完了但玩家2几乎不出牌？

这是 `history` 字段构建不正确导致的。裁判已经修复，原因：

- `history[0]` = 上上家出牌
- `history[1]` = 上家出牌
- 两者要**各自独立记录**，不能只填最后一个出牌的人

### 6.3 怎么看Bot的MCTS搜索信息？

Bot的 `stderr` 输出会被测试框架捕获，显示在日志中：

```
stderr=Total simulations: 32458, Elapsed time: 899 ms
```

### 6.4 如何调试单次调用？

使用 `debug_crash.py` 脚本手动输入JSON并查看完整stderr：

```bash
python testsuite\debug_crash.py
```

### 6.5 如何验证发送的JSON格式正确？

使用 `verify_json.py` 对比 `lianxu.json` 的协议格式：

```bash
python testsuite\verify_json.py