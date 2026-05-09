#!/usr/bin/env python3
"""
斗地主Bot测试套件 - 主入口
=============================
运行方式：
    python run_testsuite.py                     # 使用默认配置
    python run_testsuite.py --games 20           # 运行20局
    python run_testsuite.py --bot my_bot.exe     # 指定Bot路径
    python run_testsuite.py --seed 12345         # 固定随机种子
    python run_testsuite.py --timeout 3.0        # 设置超时
"""
import os
import sys
import argparse

# 确保项目根目录在Python路径中
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
if PROJECT_ROOT not in sys.path:
    sys.path.insert(0, PROJECT_ROOT)

from testsuite.runner import TestRunner
from testsuite.config import DEFAULT_CONFIG


def main():
    parser = argparse.ArgumentParser(
        description="斗地主Bot测试套件",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例：
  python run_testsuite.py                         # 默认运行10局
  python run_testsuite.py --games 20              # 运行20局
  python run_testsuite.py --bot my_bot.exe        # 指定Bot
  python run_testsuite.py --seed 42               # 固定种子
  python run_testsuite.py --timeout 3.0           # 超时3秒
  python run_testsuite.py --no-console            # 不输出到控制台
        """
    )

    parser.add_argument("--games", "-n", type=int, default=None,
                        help="测试游戏局数（默认: 10）")
    parser.add_argument("--bot", "-b", type=str, default=None,
                        help="Bot可执行文件路径（默认: my_bot.exe）")
    parser.add_argument("--seed", "-s", type=int, default=None,
                        help="随机种子（指定后可复现测试）")
    parser.add_argument("--timeout", "-t", type=float, default=None,
                        help="单步超时时间（秒）（默认: 5.0）")
    parser.add_argument("--rounds", "-r", type=int, default=None,
                        help="每局最大回合数（默认: 100）")
    parser.add_argument("--output", "-o", type=str, default=None,
                        help="日志输出目录（默认: testsuite/output）")
    parser.add_argument("--no-console", action="store_true",
                        help="关闭控制台日志输出")
    parser.add_argument("--version", "-v", action="version",
                        version="斗地主Bot测试套件 v1.0.0")

    args = parser.parse_args()

    # 构建配置
    config = {}
    if args.games is not None:
        config["num_games"] = args.games
    if args.bot is not None:
        # 如果是相对路径，基于项目根目录
        bot_path = args.bot
        if not os.path.isabs(bot_path):
            bot_path = os.path.join(PROJECT_ROOT, bot_path)
        config["bot_path"] = bot_path
    if args.seed is not None:
        config["seed"] = args.seed
    if args.timeout is not None:
        config["bot_timeout"] = args.timeout
    if args.rounds is not None:
        config["max_rounds"] = args.rounds
    if args.output is not None:
        config["log_dir"] = args.output
    if args.no_console:
        config["log_to_console"] = False

    # 打印配置信息
    print("=" * 50)
    print("斗地主Bot测试套件")
    print("=" * 50)
    print(f"工作目录: {PROJECT_ROOT}")
    print(f"Bot路径: {config.get('bot_path', DEFAULT_CONFIG['bot_path'])}")
    print(f"测试局数: {config.get('num_games', DEFAULT_CONFIG['num_games'])}")
    print(f"超时时间: {config.get('bot_timeout', DEFAULT_CONFIG['bot_timeout'])}秒")
    print(f"随机种子: {config.get('seed', '随机')}")
    print("=" * 50)

    # 运行测试
    runner = TestRunner(config)
    runner.run_all()

    print("\n测试完成！")


if __name__ == "__main__":
    main()