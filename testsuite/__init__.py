"""
斗地主Bot测试套件
=================
包含裁判、Bot执行器、日志系统、简单Bot等模块，
可用于对斗地主Bot进行完整的端到端测试。
"""

from .config import DEFAULT_CONFIG
from .card import card_to_rank, card_to_str, hand_to_str, create_deck, deal, ALL_CARDS
from .bot_runner import BotRunner, BotResult
from .simple_bot import SimpleBot
from .logger import TestLogger
from .judge import GameJudge
from .runner import TestRunner

__version__ = "1.0.0"