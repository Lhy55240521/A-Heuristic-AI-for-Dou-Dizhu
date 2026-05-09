"""快速验证测试套件是否可导入"""
import sys
import os

# 将项目根目录加入路径
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if project_root not in sys.path:
    sys.path.insert(0, project_root)

try:
    from testsuite import *
    print("testsuite包导入成功")
    from testsuite.card import *
    print("card模块    导入成功")
    from testsuite.config import DEFAULT_CONFIG
    print("config模块  导入成功")
    from testsuite.bot_runner import BotRunner
    print("bot_runner  导入成功")
    from testsuite.simple_bot import SimpleBot
    print("simple_bot  导入成功")
    from testsuite.logger import TestLogger
    print("logger模块  导入成功")
    from testsuite.judge import GameJudge
    print("judge模块   导入成功")
    from testsuite.runner import TestRunner
    print("runner模块  导入成功")
    print("=" * 40)
    print("所有模块导入成功！测试套件已准备就绪。")
except Exception as e:
    print(f"导入失败: {e}")
    import traceback
    traceback.print_exc()

input("按回车键退出...")