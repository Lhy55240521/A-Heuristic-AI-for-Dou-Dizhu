"""
分析100局对战的Bot决策表现
"""
import sys
import os
import json
import time
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from testsuite.runner import TestRunner
from testsuite.config import DEFAULT_CONFIG


def analyze_decision_quality(games_data, player_id=0):
    """
    分析指定玩家的决策质量
    games_data: list of dict, 包含每局游戏的数据
    player_id: 要分析的玩家
    """
    issues = []
    
    for g in games_data:
        gid = g.get("game_id", "?")
        result = g.get("result", "")
        bot_calls = g.get(f"bot{player_id}_calls", 0)
        bot_total_time = g.get(f"bot{player_id}_total_time", 0)
        bot_avg_time = g.get(f"bot{player_id}_avg_time", 0)
        bot_crashes = g.get(f"bot{player_id}_crashes", 0)
        bot_timeouts = g.get(f"bot{player_id}_timeouts", 0)
        bot_final_hand = g.get(f"bot{player_id}_final_hand", 0)
        bot_win = g.get(f"bot{player_id}_win", False)
        
        # 分析指标
        # 1. 崩溃/超时
        if bot_crashes > 0:
            issues.append({
                "game": gid,
                "type": "崩溃",
                "detail": f"玩家{player_id}崩溃{bot_crashes}次",
                "severity": "严重"
            })
        if bot_timeouts > 0:
            issues.append({
                "game": gid,
                "type": "超时",
                "detail": f"玩家{player_id}超时{bot_timeouts}次",
                "severity": "严重"
            })
        
        # 2. 耗时异常
        if bot_avg_time > 0.9 and bot_avg_time > 0:
            issues.append({
                "game": gid,
                "type": "耗时偏高",
                "detail": f"玩家{player_id}平均耗时{bot_avg_time:.3f}秒（接近1秒上限）",
                "severity": "警告"
            })
        
        # 3. 残局残留牌很多
        if bot_final_hand >= 10 and not bot_win:
            issues.append({
                "game": gid,
                "type": "残局牌多",
                "detail": f"玩家{player_id}结束时剩余{bot_final_hand}张牌（未获胜）",
                "severity": "提示"
            })
        
        # 4. 叫分异常（靠结果推断）
        # 如果Bot当地主且败了，或者当农民且败了但叫了高分
        
    return issues


def run_analysis():
    print("=" * 60)
    print("斗地主Bot 100局对战分析")
    print("=" * 60)
    
    # 先测试my_bot.exe（崩溃少）
    config = DEFAULT_CONFIG.copy()
    config["num_games"] = 100
    config["seed"] = 42
    config["bot_path"] = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "my_bot.exe")
    config["log_to_console"] = False  # 关掉细节日志，加快速度
    
    print(f"Bot路径: {config['bot_path']}")
    print(f"测试局数: {config['num_games']}")
    print(f"随机种子: {config['seed']}")
    print(f"开始时间: {time.strftime('%H:%M:%S')}")
    print("=" * 60)
    
    start = time.time()
    runner = TestRunner(config)
    runner.run_all()
    elapsed = time.time() - start
    
    print(f"\n完成时间: {time.strftime('%H:%M:%S')}")
    print(f"总耗时: {elapsed:.0f}秒 (平均每局{elapsed/config['num_games']:.1f}秒)")
    print()
    
    # 汇总统计
    results = runner.results
    total_games = len(results)
    
    print("=" * 60)
    print("100局对战汇总统计")
    print("=" * 60)
    
    for p in range(3):
        wins = sum(1 for r in results if r.get(f"bot{p}_win"))
        crashes = sum(r.get(f"bot{p}_crashes", 0) for r in results)
        timeouts = sum(r.get(f"bot{p}_timeouts", 0) for r in results)
        total_time = sum(r.get(f"bot{p}_total_time", 0) for r in results)
        max_time = max(r.get(f"bot{p}_max_time", 0) for r in results)
        total_calls = sum(r.get(f"bot{p}_calls", 0) for r in results)
        avg_time_per_call = total_time / total_calls if total_calls > 0 else 0
        
        landlord_count = sum(1 for r in results if r.get("landlord") == p)
        landlord_wins = sum(1 for r in results if r.get("landlord") == p and r.get(f"bot{p}_win"))
        farmer_wins = wins - landlord_wins
        farmer_count = total_games - landlord_count
        
        print(f"\n--- 玩家{p} ---")
        print(f"  总胜局: {wins}/{total_games} ({wins/total_games*100:.1f}%)")
        print(f"  当地主: {landlord_wins}/{landlord_count} ({landlord_wins/landlord_count*100:.1f}% 胜率)" if landlord_count > 0 else "  当地主: 0次")
        print(f"  当农民: {farmer_wins}/{farmer_count} ({farmer_wins/farmer_count*100:.1f}% 胜率)" if farmer_count > 0 else "  当农民: 0次")
        print(f"  崩溃次数: {crashes}")
        print(f"  超时次数: {timeouts}")
        print(f"  总调用次数: {total_calls}")
        print(f"  平均每步耗时: {avg_time_per_call:.3f}秒")
        print(f"  最大单步耗时: {max_time:.3f}秒")
    
    # 分析决策问题
    print("\n" + "=" * 60)
    print("决策分析报告（玩家0）")
    print("=" * 60)
    
    issues = analyze_decision_quality(results, 0)
    
    if not issues:
        print("  未发现明显决策问题")
    else:
        severity_count = {"严重": 0, "警告": 0, "提示": 0}
        for iss in issues:
            severity_count[iss["severity"]] = severity_count.get(iss["severity"], 0) + 1
        
        print(f"  严重问题: {severity_count.get('严重', 0)}个")
        print(f"  警告: {severity_count.get('警告', 0)}个")
        print(f"  提示: {severity_count.get('提示', 0)}个")
        
        # 只显示严重和警告
        for iss in issues:
            if iss["severity"] in ("严重", "警告"):
                print(f"  [{iss['severity']}] 第{iss['game']}局: {iss['detail']}")
    
    # 地主胜率分析
    landlord_total = sum(1 for r in results if r.get("landlord") in (0,1,2))
    landlord_wins_total = sum(1 for r in results 
        for p in range(3) if r.get("landlord") == p and r.get(f"bot{p}_win"))
    
    print(f"\n--- 总体统计 ---")
    print(f"  地主胜率: {landlord_wins_total}/{landlord_total} ({landlord_wins_total/landlord_total*100:.1f}%)" if landlord_total > 0 else "  地主胜率: N/A")
    print(f"  农民胜率: {(landlord_total - landlord_wins_total)}/{landlord_total} ({(landlord_total-landlord_wins_total)/landlord_total*100:.1f}%)" if landlord_total > 0 else "  农民胜率: N/A")
    
    # 全局性能
    all_calls = sum(sum(r.get(f"bot{p}_calls", 0) for p in range(3)) for r in results)
    all_time = sum(sum(r.get(f"bot{p}_total_time", 0) for p in range(3)) for r in results)
    all_crashes = sum(sum(r.get(f"bot{p}_crashes", 0) for p in range(3)) for r in results)
    print(f"\n  总Bot调用次数: {all_calls}")
    print(f"  总Bot运行时间: {all_time:.0f}秒")
    print(f"  总崩溃次数: {all_crashes}")


if __name__ == "__main__":
    run_analysis()