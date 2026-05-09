"""
裁判模块 - 管理斗地主游戏逻辑
负责发牌、叫分、出牌流程、胜负判定、与多个Bot通信
支持三个独立Bot实例互相对战
"""
import json
import time
from typing import List, Tuple, Optional
from .card import card_to_rank, card_to_str, create_deck, deal
from .bot_runner import BotRunner, BotResult
from .simple_bot import SimpleBot


class GameJudge:
    """
    斗地主游戏裁判
    管理一局完整的斗地主游戏：从发牌到胜负判定
    支持三个Bot实例各自独立运行，互相对战
    """

    def __init__(self, bot_runners: List[BotRunner], config: dict,
                 seed: int = None, game_id: int = 0, logger=None):
        """
        Args:
            bot_runners: 三个Bot执行器列表 [runner0, runner1, runner2]
            config: 配置字典
            seed: 随机种子
            game_id: 游戏编号
            logger: 日志器
        """
        self.bot_runners = bot_runners
        self.config = config
        self.seed = seed
        self.game_id = game_id
        self.logger = logger

        # 游戏状态
        self.hands = [[], [], []]       # 三个玩家的当前手牌（牌号）
        self.public_cards = []          # 底牌（牌号）
        self.landlord = -1              # 地主索引
        self.current_player = 0         # 当前出牌玩家
        self.last_move = []             # 最近一次非过牌出牌（牌号）
        self.last_move_player = -1      # 最后出牌的玩家
        self.pass_count = 0             # 连续过牌次数
        self.num_players = 3
        self.final_bid = 0              # 最终叫分
        self.bid_history = [0, 0, 0]    # 各玩家叫分
        self.round_count = 0            # 回合计数
        self.game_over = False
        self.winner = -1                # 赢家

        # *** 新增：每个玩家在本"出牌周期"内的最后一次出牌 ***
        # 当连续两人过牌（pass_count>=2）时，重置周期
        # 用于正确构建 history[0] 和 history[1]
        self._last_play_in_round = [[], [], []]  # 每个玩家本轮最后一次出牌

        # 每个Bot的独立通信数据
        self.bot_requests = [[], [], []]     # 每个Bot的请求列表
        self.bot_responses = [[], [], []]    # 每个Bot的响应列表
        self.bot_data = ["", "", ""]

        # 各Bot统计
        self.bot_total_calls = [0, 0, 0]
        self.bot_total_time = [0.0, 0.0, 0.0]
        self.bot_max_time = [0.0, 0.0, 0.0]
        self.bot_crashes = [0, 0, 0]
        self.bot_timeouts = [0, 0, 0]
        self.bot_played_cards = [[], [], []]

        # 各玩家崩溃/超时标记
        self.player_crashed = [False, False, False]
        self.simple_bots = [
            SimpleBot(seed),
            SimpleBot(seed + 1 if seed else 0),
            SimpleBot(seed + 2 if seed else 0)
        ]

        # 初始手牌（始终为17张，不含底牌）
        self.full_hands = [[], [], []]

    def _log(self, msg: str, level: str = "info"):
        if self.logger:
            getattr(self.logger, level)(msg, game_id=self.game_id)
        else:
            print(f"[游戏{self.game_id}] {msg}")

    def _hand_to_ranks(self, cards: list) -> list:
        return sorted(card_to_rank(c) for c in cards)

    def _player_name(self, player: int) -> str:
        status = ""
        if self.player_crashed[player]:
            status = "(已崩溃-简单Bot)"
        elif player == self.landlord:
            status = "(Bot-地主)"
        else:
            status = "(Bot-农民)"
        return f"玩家{player}{status}"

    def init_game(self):
        """初始化一局游戏：发牌"""
        deck = create_deck(self.seed)
        hands, self.public_cards = deal(deck)
        self.hands = [list(h) for h in hands]
        self.full_hands = [list(h) for h in hands]
        self.game_over = False
        self.winner = -1

        self._log(f"========== 第{self.game_id}局开始 ==========", "info")
        self._log(f"随机种子: {self.seed}", "debug")
        for p in range(3):
            self._log(f"玩家{p} 手牌({len(self.hands[p])}张): "
                      f"{', '.join(card_to_str(c) for c in sorted(self.hands[p]))}", "info")
        self._log(f"底牌: {', '.join(card_to_str(c) for c in sorted(self.public_cards))}", "info")

    def _call_bot_for_bid(self, player: int, turn_index: int) -> int:
        if self.player_crashed[player]:
            return 0

        bid_history = self.bid_history[:turn_index]
        bid_request = {"own": self.full_hands[player], "bid": bid_history}

        self.bot_requests[player].append(bid_request)
        runner = self.bot_runners[player]
        result = runner.call_bot(self.bot_requests[player], self.bot_responses[player], self.bot_data[player])

        self.bot_total_calls[player] += 1
        self.bot_total_time[player] += result.time_used
        self.bot_max_time[player] = max(self.bot_max_time[player], result.time_used)

        if result.success:
            self.bot_responses[player].append(result.action)
            if isinstance(result.action, int) and 0 <= result.action <= 3:
                return result.action
            else:
                self._log(f"玩家{player}叫分返回无效值: {result.action}，使用0", "warning")
                self.bot_responses[player][-1] = 0
                return 0
        else:
            if result.crashed:
                self.bot_crashes[player] += 1
                self._log(f"玩家{player}叫分时崩溃: {result.error_msg[:100]}", "error")
            if result.timed_out:
                self.bot_timeouts[player] += 1
                self._log(f"玩家{player}叫分时超时: {result.time_used:.2f}秒", "error")
            self.player_crashed[player] = True
            self.bot_responses[player].append(0)
            return 0

    def _build_first_play_request(self, player: int) -> dict:
        """构建首次出牌请求。own始终传17张（不含底牌）"""
        orig_hand = list(self.full_hands[player])
        if player == self.landlord and len(self.public_cards) > 0:
            for pc in self.public_cards:
                if pc in orig_hand:
                    orig_hand.remove(pc)
        return {
            "history": [[], []],
            "publiccard": self.public_cards,
            "own": orig_hand,
            "landlord": self.landlord,
            "pos": player,
            "finalbid": self.final_bid
        }

    def _get_history_for_bot(self, player: int) -> List[List[int]]:
        """
        构建 history 字段
        Botzone格式: history[0]=上上家, history[1]=上家
        传的是牌号(0-53)，不是点数！
        """
        prev1 = (player - 1) % 3  # 上家
        prev2 = (player - 2) % 3  # 上上家

        history = [[], []]

        # history[1] = 上家在本"出牌周期"内的最后一次出牌
        if self._last_play_in_round[prev1]:
            history[1] = list(self._last_play_in_round[prev1])

        # history[0] = 上上家在本"出牌周期"内的最后一次出牌
        if self._last_play_in_round[prev2]:
            history[0] = list(self._last_play_in_round[prev2])

        return history

    def _call_bot_for_play(self, player: int) -> Tuple[list, float]:
        if self.player_crashed[player]:
            return self._get_simple_bot_play(player), 0.0

        runner = self.bot_runners[player]

        # 判断是否首次出牌
        has_play_request = any("history" in r for r in self.bot_requests[player])

        if not has_play_request:
            request = self._build_first_play_request(player)
        else:
            request = {"history": self._get_history_for_bot(player)}

        self.bot_requests[player].append(request)
        result = runner.call_bot(self.bot_requests[player], self.bot_responses[player], self.bot_data[player])

        self.bot_total_calls[player] += 1
        self.bot_total_time[player] += result.time_used
        self.bot_max_time[player] = max(self.bot_max_time[player], result.time_used)

        if result.success:
            self.bot_responses[player].append(result.action)
            if isinstance(result.action, list):
                return result.action, result.time_used
            else:
                self._log(f"玩家{player}出牌返回无效格式: {result.action}，视为过牌", "warning")
                self.bot_responses[player].append([])
                return [], result.time_used
        else:
            if result.crashed:
                self.bot_crashes[player] += 1
                self._log(f"玩家{player}出牌时崩溃: {result.error_msg[:100]}", "error")
            if result.timed_out:
                self.bot_timeouts[player] += 1
                self._log(f"玩家{player}出牌时超时: {result.time_used:.2f}秒", "error")
            self.player_crashed[player] = True
            self.bot_responses[player].append([])
            return self._get_simple_bot_play(player), result.time_used

    def _get_simple_bot_play(self, player: int) -> list:
        is_leading = (self.last_move_player == player or not self.last_move)
        return self.simple_bots[player].decide_play(
            self.hands[player], self.last_move, is_leading
        )

    def _is_valid_play(self, player: int, cards: list) -> bool:
        if not cards:
            return True

        hand_copy = self.hands[player][:]
        for c in cards:
            if c in hand_copy:
                hand_copy.remove(c)
            else:
                return False

        if self.last_move and self.last_move_player != player and self.last_move_player != -1:
            if len(cards) != len(self.last_move):
                is_bomb = (len(cards) == 4 and len(set(card_to_rank(c) for c in cards)) == 1)
                is_rocket = (len(cards) == 2 and 52 in cards and 53 in cards)
                if not is_bomb and not is_rocket:
                    return False
                if is_rocket:
                    return True
                if is_bomb:
                    last_is_bomb = (len(self.last_move) == 4 and
                                    len(set(card_to_rank(c) for c in self.last_move)) == 1)
                    if not last_is_bomb:
                        return True
                    return card_to_rank(cards[0]) > card_to_rank(self.last_move[0])
        return True

    def _verify_and_fix_play(self, player: int, cards: list) -> list:
        if not self._is_valid_play(player, cards):
            self._log(f"玩家{player}出牌不合法，改用简单Bot代替", "warning")
            if not self.player_crashed[player]:
                self.bot_crashes[player] += 1
            self.player_crashed[player] = True
            return self._get_simple_bot_play(player)
        return cards

    def _apply_play(self, player: int, cards: list) -> bool:
        """执行出牌，并更新 _last_play_in_round"""
        if cards:
            for c in cards:
                self.hands[player].remove(c)

            # *** 更新本轮出牌记录 ***
            self._last_play_in_round[player] = list(cards)

            self.last_move = cards
            self.last_move_player = player
            self.pass_count = 0

            cards_str = ', '.join(card_to_str(c) for c in cards)
            self._log(f"玩家{player} ({self._player_name(player)}) "
                      f"出牌: {cards_str} | 剩余{len(self.hands[player])}张", "info")

            if not self.hands[player]:
                self.game_over = True
                self.winner = player
                self._log(f"玩家{player} ({self._player_name(player)}) 出完所有牌！", "info")
                return True
        else:
            self.pass_count += 1
            self._log(f"玩家{player} ({self._player_name(player)}) 过牌", "info")

            if self.pass_count >= 2:
                # *** 重置出牌周期 ***
                self.last_move = []
                self._last_play_in_round = [[], [], []]
                self.pass_count = 0
                self._log(f"连续两人过牌，玩家{self.last_move_player} 获得出牌权", "debug")

        return False

    def run_game(self) -> dict:
        """运行一局完整的三人Bot对战"""
        self.init_game()

        # ===== 叫分阶段 =====
        self._log("===== 开始叫分阶段 =====", "info")
        current_max_bid = 0
        landlord = -1

        for i in range(3):
            player = i
            bid_result = self._call_bot_for_bid(player, i)
            self.bid_history[player] = bid_result
            self._log(f"玩家{player} ({self._player_name(player)}) 叫分: {bid_result}", "info")
            if bid_result > current_max_bid:
                current_max_bid = bid_result
                landlord = player
            if bid_result == 3:
                break

        if landlord == -1:
            landlord = 0
            self._log("无人叫分，玩家0成为地主", "info")

        self.landlord = landlord
        self.final_bid = max(1, current_max_bid)

        # 地主获得底牌（仅在 hands 中增加，full_hands 保持17张不变）
        self.hands[landlord].extend(self.public_cards)

        self._log(f"地主: 玩家{landlord} ({self._player_name(landlord)})", "info")
        self._log(f"底分: {self.final_bid}", "info")

        # ===== 出牌阶段 =====
        self._log("===== 开始出牌阶段 =====", "info")
        self.current_player = self.landlord
        self.last_move_player = self.landlord
        self.pass_count = 0
        self.round_count = 0
        max_rounds = self.config.get("max_rounds", 100)

        while not self.game_over and self.round_count < max_rounds:
            self.round_count += 1
            player = self.current_player

            if self.player_crashed[player]:
                cards = self._get_simple_bot_play(player)
                time_used = 0.0
            else:
                cards, time_used = self._call_bot_for_play(player)
                cards = self._verify_and_fix_play(player, cards)

            if cards:
                self.bot_played_cards[player].extend(cards)

            game_ended = self._apply_play(player, cards)
            if game_ended:
                break

            self.current_player = (self.current_player + 1) % 3
            if self.pass_count == 0 and not self.last_move:
                self.current_player = self.last_move_player

        if self.round_count >= max_rounds:
            self._log(f"达到最大回合数{max_rounds}，游戏终止", "warning")
            self.game_over = True
            self.winner = -1

        return self._get_result()

    def _get_result(self, abnormal_reason: str = None) -> dict:
        result = {
            "game_id": self.game_id,
            "seed": self.seed,
            "winner": self.winner,
            "landlord": self.landlord,
            "final_bid": self.final_bid,
            "total_rounds": self.round_count,
        }
        for p in range(3):
            bot_calls = self.bot_total_calls[p]
            bot_time = round(self.bot_total_time[p], 3)
            avg_time = round(bot_time / bot_calls, 3) if bot_calls > 0 else 0
            result[f"bot{p}_calls"] = bot_calls
            result[f"bot{p}_total_time"] = bot_time
            result[f"bot{p}_max_time"] = round(self.bot_max_time[p], 3)
            result[f"bot{p}_avg_time"] = avg_time
            result[f"bot{p}_crashes"] = self.bot_crashes[p]
            result[f"bot{p}_timeouts"] = self.bot_timeouts[p]
            result[f"bot{p}_final_hand"] = len(self.hands[p])

        if self.game_over and self.winner != -1:
            if self.winner == self.landlord:
                result["result"] = f"地主(玩家{self.winner})胜利"
            else:
                result["result"] = f"农民(玩家{self.winner})胜利"
            for p in range(3):
                result[f"bot{p}_win"] = (p == self.winner)
        elif abnormal_reason:
            result["result"] = f"异常: {abnormal_reason}"
            for p in range(3):
                result[f"bot{p}_win"] = False
        else:
            result["result"] = "平局/未完成"
            for p in range(3):
                result[f"bot{p}_win"] = False

        self._log(f"===== 游戏结束: {result['result']} =====", "info")
        for p in range(3):
            self._log(f"玩家{p}: 调用{self.bot_total_calls[p]}次 | "
                      f"总耗时{result[f'bot{p}_total_time']:.2f}秒 | "
                      f"最大{result[f'bot{p}_max_time']:.2f}秒 | "
                      f"平均{result[f'bot{p}_avg_time']:.3f}秒 | "
                      f"崩溃{self.bot_crashes[p]}次 | "
                      f"超时{self.bot_timeouts[p]}次", "info")
        return result