"""调试脚本：模拟Bot完整调用链，获取完整崩溃信息"""
import sys
import os
import subprocess
import json

# 项目根目录
project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
bot_path = os.path.join(project_root, "my_bot2.exe")

# 场景1: 叫分 - 玩家1（非第一个叫分）
print("=" * 60)
print("场景1: 玩家1叫分（有前面的叫分历史）")
req1 = [{"own": [4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20], "bid": [3]}]
p = subprocess.Popen([bot_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
out, err = p.communicate(json.dumps({"requests": req1, "responses": [], "data": ""}).encode())
print(f"返回码: {p.returncode}")
if err:
    print(f"stderr: {err.decode('utf-8', errors='replace')}")
if out:
    try:
        result = json.loads(out.decode('utf-8', errors='replace').strip())
        print(f"输出: {json.dumps(result, ensure_ascii=False)[:200]}")
    except:
        print(f"原始输出: {out.decode('utf-8', errors='replace')[:200]}")

# 场景2: 首次出牌（地主20张牌）
print("\n" + "=" * 60)
print("场景2: 地主首次出牌（20张手牌）")
own_20 = list(range(20))
req2 = [{
    "history": [[], []],
    "publiccard": [29, 8, 14],
    "own": own_20,
    "landlord": 1,
    "pos": 1,
    "finalbid": 2
}]
p = subprocess.Popen([bot_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
out, err = p.communicate(json.dumps({"requests": req2, "responses": [], "data": ""}).encode())
print(f"返回码: {p.returncode}")
if err:
    print(f"stderr: {err.decode('utf-8', errors='replace')}")
if out:
    try:
        result = json.loads(out.decode('utf-8', errors='replace').strip())
        print(f"输出: {json.dumps(result, ensure_ascii=False)[:200]}")
    except:
        print(f"原始输出: {out.decode('utf-8', errors='replace')[:200]}")

# 场景3: 农民首次出牌（17张手牌，需要跟牌）
print("\n" + "=" * 60)
print("场景3: 农民首次出牌（跟牌场景）")
own_17 = list(range(17))
req3 = [{
    "history": [[0, 5], []],
    "publiccard": [29, 8, 14],
    "own": own_17,
    "landlord": 0,
    "pos": 1,
    "finalbid": 1
}]
p = subprocess.Popen([bot_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
out, err = p.communicate(json.dumps({"requests": req3, "responses": [], "data": ""}).encode())
print(f"返回码: {p.returncode}")
if err:
    print(f"stderr: {err.decode('utf-8', errors='replace')}")
if out:
    try:
        result = json.loads(out.decode('utf-8', errors='replace').strip())
        print(f"输出: {json.dumps(result, ensure_ascii=False)[:200]}")
    except:
        print(f"原始输出: {out.decode('utf-8', errors='replace')[:200]}")

# 场景4: history字段传点数而非牌号（之前测试里Bot返回的是点数）
print("\n" + "=" * 60)
print("场景4: history传点数（模拟Bot返回点数后的再次调用）")
req4 = [{
    "history": [[3, 4, 5, 6, 7], []],
    "own": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    "landlord": 1,
    "pos": 0,
    "publiccard": [29, 8, 14],
    "finalbid": 1
}]
p = subprocess.Popen([bot_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
out, err = p.communicate(json.dumps({"requests": req4, "responses": [], "data": ""}).encode())
print(f"返回码: {p.returncode}")
if err:
    print(f"stderr: {err.decode('utf-8', errors='replace')}")
if out:
    try:
        result = json.loads(out.decode('utf-8', errors='replace').strip())
        print(f"输出: {json.dumps(result, ensure_ascii=False)[:200]}")
    except:
        print(f"原始输出: {out.decode('utf-8', errors='replace')[:200]}")