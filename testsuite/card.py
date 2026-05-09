"""
牌面和牌型工具模块
定义扑克牌常量、牌号→点数映射、发牌等基础操作
"""

# 牌号映射表：0~53 对应牌面
# 格式: (点数, 花色)  点数: 3~15(2), 16(小王), 17(大王)
# 花色: 0=红桃, 1=方块, 2=黑桃, 3=草花

CARD_INFO = {
    # 点数为3
    0: (3, 0),   1: (3, 1),   2: (3, 2),   3: (3, 3),
    # 点数为4
    4: (4, 0),   5: (4, 1),   6: (4, 2),   7: (4, 3),
    # 点数为5
    8: (5, 0),   9: (5, 1),  10: (5, 2),  11: (5, 3),
    # 点数为6
    12: (6, 0), 13: (6, 1), 14: (6, 2), 15: (6, 3),
    # 点数为7
    16: (7, 0), 17: (7, 1), 18: (7, 2), 19: (7, 3),
    # 点数为8
    20: (8, 0), 21: (8, 1), 22: (8, 2), 23: (8, 3),
    # 点数为9
    24: (9, 0), 25: (9, 1), 26: (9, 2), 27: (9, 3),
    # 点数为10
    28: (10, 0), 29: (10, 1), 30: (10, 2), 31: (10, 3),
    # 点数为J(11)
    32: (11, 0), 33: (11, 1), 34: (11, 2), 35: (11, 3),
    # 点数为Q(12)
    36: (12, 0), 37: (12, 1), 38: (12, 2), 39: (12, 3),
    # 点数为K(13)
    40: (13, 0), 41: (13, 1), 42: (13, 2), 43: (13, 3),
    # 点数为A(14)
    44: (14, 0), 45: (14, 1), 46: (14, 2), 47: (14, 3),
    # 点数为2(15)
    48: (15, 0), 49: (15, 1), 50: (15, 2), 51: (15, 3),
    # 小王(16), 大王(17)
    52: (16, -1), 53: (17, -1),
}

# 花色名称
SUIT_NAMES = {0: "♥", 1: "♦", 2: "♠", 3: "♣"}
# 点数名称
RANK_NAMES = {
    3: "3", 4: "4", 5: "5", 6: "6", 7: "7", 8: "8",
    9: "9", 10: "10", 11: "J", 12: "Q", 13: "K", 14: "A",
    15: "2", 16: "小王", 17: "大王"
}

ALL_CARDS = list(range(54))  # 整副牌 0~53


def card_to_rank(card_id: int) -> int:
    """牌号转点数 (3~17)"""
    return CARD_INFO[card_id][0]


def card_to_str(card_id: int) -> str:
    """牌号转可读字符串，如 '♥3'"""
    rank, suit = CARD_INFO[card_id]
    if suit == -1:
        return RANK_NAMES[rank]
    return f"{SUIT_NAMES[suit]}{RANK_NAMES[rank]}"


def hand_to_str(cards: list) -> str:
    """手牌列表转字符串"""
    return " ".join(card_to_str(c) for c in sorted(cards))


def create_deck(seed=None):
    """
    创建一副牌并随机打乱
    Args:
        seed: 随机种子，用于可重复测试
    Returns:
        打乱后的牌列表 (54张牌号)
    """
    import random
    rng = random.Random(seed)
    deck = ALL_CARDS[:]
    rng.shuffle(deck)
    return deck


def deal(deck, num_players=3):
    """
    发牌
    Args:
        deck: 打乱后的整副牌
        num_players: 玩家数
    Returns:
        (hands, remaining)
        hands: 每个玩家的手牌列表 (每人17张)
        remaining: 底牌 (3张)
    """
    hands = []
    for i in range(num_players):
        hands.append(deck[i*17:(i+1)*17])
    remaining = deck[51:]
    return hands, remaining