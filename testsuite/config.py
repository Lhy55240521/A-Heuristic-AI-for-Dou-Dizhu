"""
配置管理模块
"""
import os

# 当前工作目录
WORK_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 默认配置
DEFAULT_CONFIG = {
    # Bot 相关
    "bot_path": os.path.join(WORK_DIR, "my_bot2.exe"),   # Bot 可执行文件路径
    "bot_timeout": 5.0,                                   # 单步超时时间（秒）
    
    # 比赛相关
    "seed": None,                                         # 随机种子（None=随机）
    "max_rounds": 100,                                    # 最大回合数（防止死循环）
    "num_games": 10,                                      # 测试游戏局数
    
    # 日志相关
    "log_dir": os.path.join(WORK_DIR, "testsuite", "output"),  # 日志输出目录
    "log_level": "INFO",                                       # 日志级别
    "log_to_console": True,                                    # 是否输出到控制台
    
    # 简单Bot相关（崩溃时替补）
    "simple_bot_pass_rate": 0.3,                          # 简单Bot过牌概率（用于测试）
    
    # 崩溃处理
    "crash_retry": 1,                                     # 崩溃后重试次数
    "use_simple_bot_on_crash": True,                      # 崩溃后是否用简单Bot代替
}