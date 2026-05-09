"""
Bot执行器模块
负责启动Bot子进程、传递JSON输入、解析JSON输出、检测超时和崩溃
"""
import subprocess
import json
import time
import os
import sys


class BotResult:
    """Bot执行结果"""
    def __init__(self, success: bool, action, time_used: float,
                 error_msg: str = None, crashed: bool = False,
                 timed_out: bool = False):
        self.success = success          # 是否成功
        self.action = action            # 动作结果（叫分int或出牌list）
        self.time_used = time_used      # 耗时（秒）
        self.error_msg = error_msg      # 错误信息
        self.crashed = crashed          # 是否崩溃
        self.timed_out = timed_out      # 是否超时


class BotRunner:
    """
    Bot进程管理器
    负责和被测试的Bot可执行文件进行JSON通信
    """

    def __init__(self, bot_path: str, timeout: float = 5.0):
        """
        Args:
            bot_path: Bot可执行文件路径
            timeout: 单次调用超时时间（秒）
        """
        self.bot_path = bot_path
        self.timeout = timeout
        self.process = None

    def _build_request(self, turn_id: int, history: list, **kwargs) -> dict:
        """
        构建Bot请求JSON
        
        Args:
            turn_id: 回合编号
            history: 历史出牌列表
            **kwargs: 额外字段，如 own, publiccard, landlord, pos, bid 等
        
        Returns:
            符合Botzone格式的请求字典
        """
        request = {}
        
        # 叫分阶段
        if "bid" in kwargs:
            request["bid"] = kwargs["bid"]
        
        # 出牌阶段
        if "history" in kwargs:
            request["history"] = kwargs["history"]
        else:
            request["history"] = history
        
        if "publiccard" in kwargs:
            request["publiccard"] = kwargs["publiccard"]
        if "own" in kwargs:
            request["own"] = kwargs["own"]
        if "landlord" in kwargs:
            request["landlord"] = kwargs["landlord"]
        if "pos" in kwargs:
            request["pos"] = kwargs["pos"]
        if "finalbid" in kwargs:
            request["finalbid"] = kwargs["finalbid"]
        
        return request

    def call_bot(self, requests_list: list, responses_list: list,
                 data: any = "") -> BotResult:
        """
        调用Bot执行一次决策
        
        Args:
            requests_list: 到目前为止的所有请求列表
            responses_list: 到目前为止的所有响应列表
            data: 会话数据
        
        Returns:
            BotResult对象
        """
        # 构建完整输入
        full_input = {
            "requests": requests_list,
            "responses": responses_list,
            "data": data
        }
        input_text = json.dumps(full_input, ensure_ascii=False)
        
        start_time = time.perf_counter()
        try:
            # 启动子进程
            proc = subprocess.Popen(
                [self.bot_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=os.path.dirname(self.bot_path) if os.path.isfile(self.bot_path) else None
            )
            
            try:
                stdout, stderr = proc.communicate(
                    input=input_text.encode("utf-8"),
                    timeout=self.timeout
                )
                elapsed = time.perf_counter() - start_time
                
                # 检查返回码
                if proc.returncode != 0:
                    err_msg = stderr.decode("utf-8", errors="replace") if stderr else ""
                    return BotResult(
                        success=False,
                        action=None,
                        time_used=elapsed,
                        error_msg=f"进程返回码={proc.returncode}, stderr={err_msg[:200]}",
                        crashed=True
                    )
                
                # 解析输出
                try:
                    output_str = stdout.decode("utf-8", errors="replace").strip()
                    output = json.loads(output_str)
                    
                    # 提取response字段
                    if "response" in output:
                        action = output["response"]
                    else:
                        action = output
                    
                    return BotResult(
                        success=True,
                        action=action,
                        time_used=elapsed,
                        error_msg=None,
                        crashed=False,
                        timed_out=False
                    )
                    
                except json.JSONDecodeError as e:
                    return BotResult(
                        success=False,
                        action=None,
                        time_used=elapsed,
                        error_msg=f"JSON解析失败: {e}",
                        crashed=True
                    )
                    
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
                elapsed = time.perf_counter() - start_time
                return BotResult(
                    success=False,
                    action=None,
                    time_used=elapsed,
                    error_msg=f"超时（限制{self.timeout}秒）",
                    crashed=False,
                    timed_out=True
                )
                
        except FileNotFoundError:
            return BotResult(
                success=False,
                action=None,
                time_used=0,
                error_msg=f"找不到Bot可执行文件: {self.bot_path}",
                crashed=True
            )
        except Exception as e:
            elapsed = time.perf_counter() - start_time
            return BotResult(
                success=False,
                action=None,
                time_used=elapsed,
                error_msg=f"执行异常: {str(e)}",
                crashed=True
            )