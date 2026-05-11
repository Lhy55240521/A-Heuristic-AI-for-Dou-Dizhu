"""
测试运行器主模块
统筹多局三人Bot对战的执行、统计和报告生成
"""
import os
import sys
import json
import random
import time
from datetime import datetime
from .config import DEFAULT_CONFIG
from .bot_runner import BotRunner
from .logger import TestLogger
from .judge import GameJudge


class TestRunner:
    """测试运行器 - 管理三个Bot互相进行斗地主对战"""

    def __init__(self, config: dict = None):
        """
        Args:
            config: 配置字典，覆盖默认配置
        """
        self.config = DEFAULT_CONFIG.copy()
        if config:
            self.config.update(config)

        # 初始化日志器
        self.logger = TestLogger(self.config)

        # 初始化三个Bot执行器（每个玩家一个独立实例）
        bot_path = self.config.get("bot_path")
        bot_paths = self.config.get("bot_paths")
        timeout = self.config.get("bot_timeout", 5.0)

        if bot_paths is None:
            bot_paths = [bot_path, bot_path, bot_path]
        else:
            bot_paths = list(bot_paths)
            if len(bot_paths) != 3:
                raise ValueError("bot_paths 必须包含 3 个路径")

        for idx, path in enumerate(bot_paths):
            if not os.path.isfile(path):
                self.logger.warning(f"玩家{idx}的Bot可执行文件不存在: {path}")

        self.bot_runners = [
            BotRunner(bot_paths[0], timeout),  # 玩家0
            BotRunner(bot_paths[1], timeout),  # 玩家1
            BotRunner(bot_paths[2], timeout),  # 玩家2
        ]

        # 提取bot简化名（不含路径和扩展名）
        bot_names_config = self.config.get("bot_names")
        if bot_names_config and len(bot_names_config) == 3:
            self.bot_names = list(bot_names_config)
        else:
            base_names = [
                os.path.splitext(os.path.basename(p))[0] for p in bot_paths
            ]
            model_name = os.environ.get("DZZERO_MODEL", "")
            use_deep = os.environ.get("DZZERO_USE_DEEP") == "1"
            if use_deep and model_name:
                self.bot_names = [
                    base_names[0],
                    model_name,
                    model_name
                ]
            else:
                self.bot_names = base_names

        # 统计信息
        self.results = []
        self.total_games = 0
        self.abnormal = 0

    def run_single_game(self, game_id: int, seed: int = None) -> dict:
        """
        运行一局游戏

        Args:
            game_id: 游戏编号
            seed: 随机种子

        Returns:
            游戏结果字典
        """
        judge = GameJudge(
            bot_runners=self.bot_runners,
            config=self.config,
            seed=seed,
            game_id=game_id,
            logger=self.logger,
            bot_names=self.bot_names
        )

        result = judge.run_game()
        return result

    def run_all(self):
        """运行所有测试局"""
        num_games = self.config.get("num_games", 10)
        base_seed = self.config.get("seed")

        self.logger.info(f"开始测试：共{num_games}局")
        bot_paths = self.config.get("bot_paths")
        if bot_paths is None:
            self.logger.info(f"Bot路径: {self.config.get('bot_path')}")
        else:
            self.logger.info(f"Bot路径: {bot_paths}")
        self.logger.info(f"每步超时: {self.config.get('bot_timeout')}秒")
        if base_seed is not None:
            self.logger.info(f"基准种子: {base_seed}")
        self.logger.info("=" * 50)

        # 创建随机数生成器用于生成每局种子
        rng = random.Random(base_seed)
        start_time = time.time()

        # 累加器
        player_wins = [0, 0, 0]
        player_crashes = [0, 0, 0]
        player_timeouts = [0, 0, 0]
        player_total_time = [0.0, 0.0, 0.0]
        player_max_time = [0.0, 0.0, 0.0]

        for gid in range(num_games):
            if base_seed is not None:
                game_seed = rng.randint(0, 2**31 - 1)
            else:
                game_seed = None

            try:
                result = self.run_single_game(gid + 1, seed=game_seed)
                self.results.append(result)
                self.total_games += 1

                # 累加各玩家统计
                for p in range(3):
                    if result.get(f"bot{p}_win"):
                        player_wins[p] += 1
                    player_crashes[p] += result.get(f"bot{p}_crashes", 0)
                    player_timeouts[p] += result.get(f"bot{p}_timeouts", 0)
                    player_total_time[p] += result.get(f"bot{p}_total_time", 0)
                    player_max_time[p] = max(
                        player_max_time[p],
                        result.get(f"bot{p}_max_time", 0)
                    )

                if "异常" in result.get("result", ""):
                    self.abnormal += 1

                # 输出进度
                elapsed = time.time() - start_time
                eta = (elapsed / (gid + 1)) * (num_games - gid - 1) if gid > 0 else 0
                self.logger.info(
                    f"进度: [{gid + 1}/{num_games}] | "
                    f"已用{elapsed:.0f}秒 | "
                    f"预计剩余{eta:.0f}秒 | "
                    f"胜率: 玩家0={player_wins[0]} 玩家1={player_wins[1]} 玩家2={player_wins[2]}"
                )

            except Exception as e:
                self.logger.error(f"第{gid + 1}局执行异常: {str(e)}")
                import traceback
                traceback.print_exc()
                self.abnormal += 1
                self.results.append({
                    "game_id": gid + 1,
                    "seed": game_seed,
                    "result": f"异常: {str(e)}",
                })

        # 生成报告
        self._generate_report(player_wins, player_crashes, player_timeouts,
                              player_total_time, player_max_time)

    def _generate_report(self, player_wins, player_crashes, player_timeouts,
                         player_total_time, player_max_time):
        """生成测试报告"""
        summary = {
            "total_games": self.total_games,
            "abnormal": self.abnormal,
            "games": []
        }

        for p in range(3):
            total_calls = sum(
                r.get(f"bot{p}_calls", 0) for r in self.results
            )
            avg_time = (player_total_time[p] / total_calls
                       if total_calls > 0 else 0)
            summary[f"玩家{p}_胜局"] = player_wins[p]
            summary[f"玩家{p}_胜率"] = (f"{player_wins[p]/self.total_games*100:.1f}%"
                                      if self.total_games > 0 else "N/A")
            summary[f"玩家{p}_崩溃次数"] = player_crashes[p]
            summary[f"玩家{p}_超时次数"] = player_timeouts[p]
            summary[f"玩家{p}_平均耗时"] = round(avg_time, 4)
            summary[f"玩家{p}_最大耗时"] = round(player_max_time[p], 4)

        for r in self.results:
            entry = {
                "id": r.get("game_id"),
                "result": r.get("result", "未知"),
                "landlord": r.get("landlord", -1),
                "steps": r.get("total_rounds", 0),
            }
            for p in range(3):
                entry[f"bot{p}_time"] = round(r.get(f"bot{p}_total_time", 0), 2)
                entry[f"bot{p}_crashes"] = r.get(f"bot{p}_crashes", 0)
            summary["games"].append(entry)

        # 写入摘要文件
        summary_file = self.logger.write_summary(summary)

        # 控制台输出汇总
        self.logger.info("=" * 50)
        self.logger.info("测试完成！汇总报告：")
        self.logger.info(f"总游戏局数: {self.total_games}")
        self.logger.info(f"平局/异常: {self.abnormal}")
        for p in range(3):
            self.logger.info(
                f"玩家{p}: 胜{player_wins[p]}局 "
                f"(胜率{player_wins[p]/self.total_games*100:.1f}%) | "
                f"崩溃{player_crashes[p]}次 | "
                f"超时{player_timeouts[p]}次 | "
                f"平均耗时{summary[f'玩家{p}_平均耗时']:.3f}秒"
            )
        self.logger.info(f"报告已保存到: {summary_file}")
        self.logger.info("=" * 50)