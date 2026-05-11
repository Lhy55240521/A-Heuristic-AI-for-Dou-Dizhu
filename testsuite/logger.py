"""
日志系统模块
支持控制台输出和文件输出，中文日志
"""
import os
import sys
import logging
from datetime import datetime
from .config import DEFAULT_CONFIG

# 花色符号映射（用于处理终端编码问题）
SUIT_MAP = {
    '\u2660': '[黑桃]',  # ♠
    '\u2665': '[红桃]',  # ♥
    '\u2666': '[方块]',  # ♦
    '\u2663': '[梅花]',  # ♣
}

def safe_console_output(msg):
    """安全输出到控制台，处理编码问题"""
    if sys.platform.startswith('win'):
        try:
            # 尝试直接输出
            return msg
        except UnicodeEncodeError:
            # 如果编码失败，替换花色符号
            result = msg
            for char, replacement in SUIT_MAP.items():
                result = result.replace(char, replacement)
            return result
    return msg


class TestLogger:
    """测试日志管理器"""

    def __init__(self, config: dict = None):
        self.config = config or DEFAULT_CONFIG
        self.log_dir = self.config.get("log_dir", "testsuite/output")
        self.game_loggers = {}  # 每局游戏一个独立日志文件

        # 创建日志目录
        os.makedirs(self.log_dir, exist_ok=True)
        os.makedirs(os.path.join(self.log_dir, "games"), exist_ok=True)
        os.makedirs(os.path.join(self.log_dir, "summary"), exist_ok=True)

        # 初始化全局日志
        self.global_logger = self._create_global_logger()

    def _create_global_logger(self):
        """创建全局摘要日志"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        logger = logging.getLogger(f"global_{timestamp}")
        logger.setLevel(logging.DEBUG)

        # 清除已有处理器
        logger.handlers.clear()

        # 文件处理器（全局日志）
        log_file = os.path.join(self.log_dir, f"test_{timestamp}.log")
        fh = logging.FileHandler(log_file, encoding="utf-8")
        fh.setLevel(logging.DEBUG)
        formatter = logging.Formatter("%(asctime)s | %(levelname)s | %(message)s",
                                      datefmt="%Y-%m-%d %H:%M:%S")
        fh.setFormatter(formatter)
        logger.addHandler(fh)

        # 控制台处理器
        if self.config.get("log_to_console", True):
            ch = logging.StreamHandler(sys.stdout)
            ch.setLevel(getattr(logging, self.config.get("log_level", "INFO")))
            ch.setFormatter(formatter)
            
            # 添加过滤器处理Windows终端编码问题
            class SafeConsoleFilter(logging.Filter):
                def filter(self, record):
                    record.msg = self.safe_console_output(record.msg)
                    return True
                
                def safe_console_output(self, msg):
                    """安全输出到控制台，处理Windows终端编码问题"""
                    if sys.platform.startswith('win'):
                        SUIT_MAP = {
                            '\u2660': '[黑桃]',
                            '\u2665': '[红桃]',
                            '\u2666': '[方块]',
                            '\u2663': '[梅花]',
                        }
                        result = msg
                        for char, replacement in SUIT_MAP.items():
                            result = result.replace(char, replacement)
                        return result
                    return msg
            
            ch.addFilter(SafeConsoleFilter())
            logger.addHandler(ch)

        return logger

    def get_game_logger(self, game_id: int):
        """获取或创建某局游戏的详细日志"""
        if game_id not in self.game_loggers:
            logger = logging.getLogger(f"game_{game_id:04d}")
            logger.setLevel(logging.DEBUG)
            logger.handlers.clear()

            # 每局独立日志文件
            log_file = os.path.join(self.log_dir, "games", f"game_{game_id:04d}.log")
            fh = logging.FileHandler(log_file, encoding="utf-8")
            fh.setLevel(logging.DEBUG)
            formatter = logging.Formatter("%(asctime)s | %(message)s",
                                          datefmt="%H:%M:%S")
            fh.setFormatter(formatter)
            logger.addHandler(fh)

            # 控制台也显示简要信息
            if self.config.get("log_to_console", True):
                ch = logging.StreamHandler(sys.stdout)
                ch.setLevel(logging.INFO)
                # 设置控制台编码
                if hasattr(ch.stream, 'buffer'):
                    ch.stream = sys.stdout
                ch.setFormatter(logging.Formatter("[游戏%(game_id)04d] %(message)s"))
                logger.addHandler(ch)

            self.game_loggers[game_id] = logger
        return self.game_loggers[game_id]

    def info(self, msg: str, game_id: int = None):
        """记录信息级别日志"""
        if game_id is not None:
            logger = self.get_game_logger(game_id)
            logger.info(msg, extra={"game_id": game_id})
        else:
            self.global_logger.info(msg)

    def debug(self, msg: str, game_id: int = None):
        """记录调试级别日志"""
        if game_id is not None:
            logger = self.get_game_logger(game_id)
            logger.debug(msg, extra={"game_id": game_id})
        else:
            self.global_logger.debug(msg)

    def warning(self, msg: str, game_id: int = None):
        """记录警告级别日志"""
        if game_id is not None:
            logger = self.get_game_logger(game_id)
            logger.warning(msg, extra={"game_id": game_id})
        else:
            self.global_logger.warning(msg)

    def error(self, msg: str, game_id: int = None):
        """记录错误级别日志"""
        if game_id is not None:
            logger = self.get_game_logger(game_id)
            logger.error(msg, extra={"game_id": game_id})
        else:
            self.global_logger.error(msg)

    def write_summary(self, summary_data: dict):
        """写入测试摘要报告"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        summary_file = os.path.join(self.log_dir, "summary", f"summary_{timestamp}.txt")
        with open(summary_file, "w", encoding="utf-8") as f:
            f.write("=" * 60 + "\n")
            f.write(f"斗地主Bot测试报告\n")
            f.write(f"生成时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write("=" * 60 + "\n\n")
            f.write(f"总游戏局数: {summary_data.get('total_games', 0)}\n")
            f.write(f"平局/异常局数: {summary_data.get('abnormal', 0)}\n\n")

            # 各玩家统计
            f.write("-" * 60 + "\n")
            f.write("各Bot统计:\n")
            for p in range(3):
                wins = summary_data.get(f"玩家{p}_胜局", 0)
                win_rate = summary_data.get(f"玩家{p}_胜率", "N/A")
                crashes = summary_data.get(f"玩家{p}_崩溃次数", 0)
                timeouts = summary_data.get(f"玩家{p}_超时次数", 0)
                avg_t = summary_data.get(f"玩家{p}_平均耗时", 0)
                max_t = summary_data.get(f"玩家{p}_最大耗时", 0)
                f.write(f"  玩家{p}: 胜{wins}局 (胜率{win_rate}) | "
                        f"崩溃{crashes}次 | 超时{timeouts}次 | "
                        f"平均{avg_t:.3f}秒 | 最大{max_t:.3f}秒\n")
            f.write("\n")

            f.write("-" * 60 + "\n")
            f.write("各局详情:\n")
            for g in summary_data.get("games", []):
                line = f"  第{g['id']}局: {g.get('result', '未知')} | " \
                       f"地主: 玩家{g.get('landlord', -1)} | " \
                       f"步数: {g.get('steps', 0)}"
                # 各玩家耗时
                for p in range(3):
                    t = g.get(f"bot{p}_time", 0)
                    c = g.get(f"bot{p}_crashes", 0)
                    line += f" | 玩家{p}: {t:.2f}秒"
                    if c:
                        line += f"(崩{c})"
                f.write(line + "\n")
        return summary_file
