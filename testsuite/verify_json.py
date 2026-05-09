"""验证.py: 直接打印发给Bot的完整JSON，与 lianxu.json 对比"""
import sys
import os
import json
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from testsuite.bot_runner import BotRunner
from testsuite.card import card_to_str

# 模拟一个简易裁判，跟踪发给Bot的内容
bot_path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "my_bot2.exe")

# ======== 场景：玩家0是地主 ========
print("=" * 70)
print("场景：玩家0叫分3分 → 成为地主 → 首次出牌")
print("=" * 70)

# --- Turn 0: 叫分 ---
bid_request = {
    "own": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    "bid": []
}
bot_requests = [bid_request]
bot_responses = []
print("\n【Turn 0: 叫分】")
print(f"requests = {json.dumps(bot_requests, ensure_ascii=False)}")
print(f"responses = {json.dumps(bot_responses)}")

# Bot返回3
bot_responses.append(3)

# --- Turn 1: 首次出牌 (地主) ---
first_play = {
    "history": [[], []],
    "publiccard": [29, 20, 8],        # 底牌
    "own": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],  # 初始17张
    "landlord": 0,                     # 地主是玩家0
    "pos": 0,                          # Bot位置
    "finalbid": 3                      # 底分
}
bot_requests.append(first_play)

print("\n【Turn 1: 首次出牌】")
print(f"完整输入JSON:")
full_input = {
    "requests": bot_requests,
    "responses": bot_responses,
    "data": ""
}
print(json.dumps(full_input, indent=2, ensure_ascii=False))

# 与lianxu.json对比
print("\n" + "=" * 70)
print("与 lianxu.json 对比（玩家1视角，首次出牌）")
print("=" * 70)
lianxu_first_play = {
    "requests": [
        {"own": [34, 22, 28, 16, 7, 42, 48, 10, 44, 0, 26, 3, 32, 5, 13, 33, 9], "bid": [1]},
        {"history": [[], []], "own": [34, 22, 28, 16, 7, 42, 48, 10, 44, 0, 26, 3, 32, 5, 13, 33, 9],
         "publiccard": [29, 20, 8], "landlord": 1, "pos": 1, "finalbid": 3}
    ],
    "responses": [3]
}
print(json.dumps(lianxu_first_play, indent=2, ensure_ascii=False))

print("\n" + "=" * 70)
print("结构对比：")
print(f"  我的 requests[{len(bot_requests)}] 元素类型: {[type(r).__name__ for r in bot_requests]}")
print(f"  lianxu requests[2] 元素类型: [dict, dict]")
print(f"  我的首次出请求包含 history: {'history' in first_play}")
print(f"  我的首次出请求包含 own: {'own' in first_play}")
print(f"  我的首次出请求包含 publiccard: {'publiccard' in first_play}")
print(f"  我的首次出请求包含 landlord: {'landlord' in first_play}")
print(f"  我的首次出请求包含 pos: {'pos' in first_play}")
print(f"  我的首次出请求包含 finalbid: {'finalbid' in first_play}")
print("=" * 70)

# ======== 场景2：后续出牌（跟牌） ========
print("\n\n" + "=" * 70)
print("场景2：后续出牌（前面有人出了牌）")
print("=" * 70)

# Bot之前出过顺子 [0,1,2,4,5,6,8,12]（地主第一手）
bot_responses.append([0, 1, 2, 4, 5, 6, 8, 12])

# 另外两家过牌，轮到Bot再次出牌（主动）
next_play = {
    "history": [[], []]  # 上家和上上家都过牌
}
bot_requests.append(next_play)

print("\n【Turn 2: 后续出牌（主动）】")
full_input2 = {
    "requests": bot_requests,
    "responses": bot_responses,
    "data": ""
}
print(json.dumps(full_input2, indent=2, ensure_ascii=False))

# 与lianxu.json对比（第3个请求）
print("\n" + "=" * 70)
print("与 lianxu.json 对比（后续出牌）")
print("=" * 70)
lianxu_next = {
    "requests": [
        {"own": [34, 22, 28, 16, 7, 42, 48, 10, 44, 0, 26, 3, 32, 5, 13, 33, 9], "bid": [1]},
        {"history": [[], []], "own": [34, 22, 28, 16, 7, 42, 48, 10, 44, 0, 26, 3, 32, 5, 13, 33, 9],
         "publiccard": [29, 20, 8], "landlord": 1, "pos": 1, "finalbid": 3},
        {"history": [[], []]}
    ],
    "responses": [3, [13, 16, 22, 26, 28]]
}
print(json.dumps(lianxu_next, indent=2, ensure_ascii=False))