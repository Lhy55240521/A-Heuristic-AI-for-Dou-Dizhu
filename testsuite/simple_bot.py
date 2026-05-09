"""
简单Bot：当被测Bot崩溃或超时时，用简单策略代替出牌
"""
import random
from .card import card_to_rank


class SimpleBot:
    """
    简单打牌机器人
    策略：如果有牌可出就出最小的牌，否则过牌
    模拟Botzone托管策略
    """

    def __init__(self, seed=None):
        self.rng = random.Random(seed)

    def decide_bid(self, hand, bid_history, current_max_bid):
        """
        叫分决策 - 简单策略：总是叫0分
        Returns:
            叫分 (0)
        """
        return 0

    def decide_play(self, hand, last_move, is_leading):
        """
        出牌决策 - 简单策略：
        1. 如果可以过牌，则过牌
        2. 否则出点数最小的牌（单张）
        
        Args:
            hand: 当前手牌（牌号列表）
            last_move: 上家出的牌（牌号列表），空表示主动出牌
            is_leading: 是否主动出牌
        
        Returns:
            要出的牌列表，空列表表示过牌
        """
        # 主动出牌：出最小的单张
        if is_leading:
            if hand:
                # 找点数最小的牌
                min_card = min(hand, key=card_to_rank)
                return [min_card]
            return []
        
        # 跟牌：如果能过则过
        # 如果上家出的牌是火箭，直接过
        if len(last_move) == 2 and 52 in last_move and 53 in last_move:
            return []
        
        # 如果能找到更大的单张，就出
        last_rank = card_to_rank(last_move[0]) if last_move else 0
        
        # 尝试出单张
        if len(last_move) == 1:
            # 找更大的单张
            bigger = [c for c in hand if card_to_rank(c) > last_rank]
            if bigger:
                return [min(bigger, key=card_to_rank)]
            return []
        
        # 尝试出对子
        if len(last_move) == 2 and 52 not in last_move:
            ranks = {}
            for c in hand:
                r = card_to_rank(c)
                if r not in ranks:
                    ranks[r] = []
                ranks[r].append(c)
            for r in sorted(ranks.keys()):
                if r > last_rank and len(ranks[r]) >= 2:
                    return ranks[r][:2]
            return []
        
        # 其他情况：过牌
        return []