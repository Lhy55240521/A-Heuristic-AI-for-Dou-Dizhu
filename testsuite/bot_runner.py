"""
Bot执行器模块
负责启动Bot子进程、传递JSON输入、解析JSON输出、检测超时和崩溃
"""
import subprocess
import json
import time
import os
import sys
from datetime import datetime

# 调试日志目录
DEBUG_LOG_DIR = "testsuite/output/debug"
os.makedirs(DEBUG_LOG_DIR, exist_ok=True)


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

    def __init__(self, bot_path: str, timeout: float = 5.0, python_path: str = None):
        """
        Args:
            bot_path: Bot可执行文件路径
            timeout: 单次调用超时时间（秒）
            python_path: Python解释器路径（None则用sys.executable）
        """
        self.bot_path = bot_path
        self.timeout = timeout
        self.process = None
        self._persistent = False
        self._is_py_bot = bot_path.lower().endswith(".py")
        self._python_path = python_path or os.environ.get("PYTHON_PATH", "") or sys.executable
        if self._is_py_bot:
            env_copy = os.environ.copy()
            if env_copy.get("DZZERO_PERSISTENT") == "1":
                self._persistent = True

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
                 data: any = "", game_id: int = 0, player_id: int = 0) -> BotResult:
        """
        调用Bot执行一次决策
        
        Args:
            requests_list: 到目前为止的所有请求列表
            responses_list: 到目前为止的所有响应列表
            data: 会话数据
            game_id: 当前游戏ID（用于日志记录）
            player_id: 当前玩家ID（用于日志记录）
        
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
        bot_cmd = [self.bot_path]
        if self._is_py_bot:
            bot_cmd = [self._python_path, "-u", self.bot_path]
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
        debug_log_file = os.path.join(DEBUG_LOG_DIR, f"debug_{game_id:04d}_p{player_id}_{timestamp}.json")
        
        start_time = time.perf_counter()

        if self._persistent:
            return self._call_persistent(full_input, input_text, bot_cmd, 
                                         game_id, player_id, timestamp, debug_log_file, start_time)
        try:
            # 启动子进程
            proc = subprocess.Popen(
                bot_cmd,
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
                    
                    # 记录调试日志
                    debug_data = {
                        "game_id": game_id,
                        "player_id": player_id,
                        "timestamp": timestamp,
                        "bot_path": self.bot_path,
                        "input": full_input,
                        "output": None,
                        "stdout": stdout.decode("utf-8", errors="replace") if stdout else None,
                        "stderr": err_msg,
                        "return_code": proc.returncode,
                        "time_used": elapsed,
                        "success": False,
                        "error": f"进程返回码={proc.returncode}"
                    }
                    with open(debug_log_file, "w", encoding="utf-8") as f:
                        json.dump(debug_data, f, ensure_ascii=False, indent=2)
                    
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
                    
                    # 记录调试日志
                    debug_data = {
                        "game_id": game_id,
                        "player_id": player_id,
                        "timestamp": timestamp,
                        "bot_path": self.bot_path,
                        "input": full_input,
                        "output": output,
                        "stdout": output_str,
                        "stderr": stderr.decode("utf-8", errors="replace") if stderr else None,
                        "return_code": proc.returncode,
                        "time_used": elapsed,
                        "success": True,
                        "error": None
                    }
                    with open(debug_log_file, "w", encoding="utf-8") as f:
                        json.dump(debug_data, f, ensure_ascii=False, indent=2)
                    
                    return BotResult(
                        success=True,
                        action=action,
                        time_used=elapsed,
                        error_msg=None,
                        crashed=False,
                        timed_out=False
                    )
                    
                except json.JSONDecodeError as e:
                    # 记录调试日志
                    debug_data = {
                        "game_id": game_id,
                        "player_id": player_id,
                        "timestamp": timestamp,
                        "bot_path": self.bot_path,
                        "input": full_input,
                        "output": None,
                        "stdout": stdout.decode("utf-8", errors="replace") if stdout else None,
                        "stderr": stderr.decode("utf-8", errors="replace") if stderr else None,
                        "return_code": proc.returncode,
                        "time_used": elapsed,
                        "success": False,
                        "error": f"JSON解析失败: {e}"
                    }
                    with open(debug_log_file, "w", encoding="utf-8") as f:
                        json.dump(debug_data, f, ensure_ascii=False, indent=2)
                    
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
                
                # 记录调试日志
                debug_data = {
                    "game_id": game_id,
                    "player_id": player_id,
                    "timestamp": timestamp,
                    "bot_path": self.bot_path,
                    "input": full_input,
                    "output": None,
                    "stdout": None,
                    "stderr": None,
                    "return_code": -1,
                    "time_used": elapsed,
                    "success": False,
                    "error": f"超时（限制{self.timeout}秒）"
                }
                with open(debug_log_file, "w", encoding="utf-8") as f:
                    json.dump(debug_data, f, ensure_ascii=False, indent=2)
                
                return BotResult(
                    success=False,
                    action=None,
                    time_used=elapsed,
                    error_msg=f"超时（限制{self.timeout}秒）",
                    crashed=False,
                    timed_out=True
                )
                
        except FileNotFoundError:
            # 记录调试日志
            debug_data = {
                "game_id": game_id,
                "player_id": player_id,
                "timestamp": timestamp,
                "bot_path": self.bot_path,
                "input": full_input,
                "output": None,
                "stdout": None,
                "stderr": None,
                "return_code": -2,
                "time_used": 0,
                "success": False,
                "error": f"找不到Bot可执行文件: {self.bot_path}"
            }
            with open(debug_log_file, "w", encoding="utf-8") as f:
                json.dump(debug_data, f, ensure_ascii=False, indent=2)
            
            return BotResult(
                success=False,
                action=None,
                time_used=0,
                error_msg=f"找不到Bot可执行文件: {self.bot_path}",
                crashed=True
            )
        except Exception as e:
            elapsed = time.perf_counter() - start_time
            
            # 记录调试日志
            debug_data = {
                "game_id": game_id,
                "player_id": player_id,
                "timestamp": timestamp,
                "bot_path": self.bot_path,
                "input": full_input,
                "output": None,
                "stdout": None,
                "stderr": str(e),
                "return_code": -3,
                "time_used": elapsed,
                "success": False,
                "error": f"执行异常: {str(e)}"
            }
            with open(debug_log_file, "w", encoding="utf-8") as f:
                json.dump(debug_data, f, ensure_ascii=False, indent=2)
            
            return BotResult(
                success=False,
                action=None,
                time_used=elapsed,
                error_msg=f"执行异常: {str(e)}",
                crashed=True
            )

    def _call_persistent(self, full_input, input_text, bot_cmd,
                         game_id, player_id, timestamp, debug_log_file, start_time):
        if self.process is None or self.process.poll() is not None:
            try:
                env_copy = os.environ.copy()
                self.process = subprocess.Popen(
                    bot_cmd,
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    cwd=os.path.dirname(self.bot_path) if os.path.isfile(self.bot_path) else None,
                    env=env_copy
                )
            except FileNotFoundError:
                return BotResult(success=False, action=None, time_used=0,
                                 error_msg=f"找不到Bot可执行文件: {self.bot_path}", crashed=True)
            except Exception as e:
                return BotResult(success=False, action=None, time_used=0,
                                 error_msg=f"启动进程失败: {str(e)}", crashed=True)

        try:
            self.process.stdin.write((input_text + "\n").encode("utf-8"))
            self.process.stdin.flush()

            import threading
            result_line = [None]
            def read_line():
                result_line[0] = self.process.stdout.readline()

            reader = threading.Thread(target=read_line, daemon=True)
            reader.start()
            reader.join(timeout=self.timeout)
            elapsed = time.perf_counter() - start_time

            if reader.is_alive():
                return BotResult(success=False, action=None, time_used=elapsed,
                                 error_msg=f"超时（限制{self.timeout}秒）", crashed=False, timed_out=True)

            if not result_line[0]:
                self.process = None
                return BotResult(success=False, action=None, time_used=elapsed,
                                 error_msg="进程已退出", crashed=True)

            output_str = result_line[0].decode("utf-8", errors="replace").strip()
            output = json.loads(output_str)
            action = output.get("response", output)

            debug_data = {
                "game_id": game_id, "player_id": player_id, "timestamp": timestamp,
                "bot_path": self.bot_path, "input": full_input, "output": output,
                "stdout": output_str, "time_used": elapsed, "success": True, "error": None
            }
            with open(debug_log_file, "w", encoding="utf-8") as f:
                json.dump(debug_data, f, ensure_ascii=False, indent=2)

            return BotResult(success=True, action=action, time_used=elapsed)

        except json.JSONDecodeError as e:
            elapsed = time.perf_counter() - start_time
            return BotResult(success=False, action=None, time_used=elapsed,
                             error_msg=f"JSON解析失败: {e}", crashed=True)
        except Exception as e:
            elapsed = time.perf_counter() - start_time
            self.process = None
            return BotResult(success=False, action=None, time_used=elapsed,
                             error_msg=f"通信异常: {str(e)}", crashed=True)

    def cleanup(self):
        if self.process and self.process.poll() is None:
            try:
                self.process.stdin.close()
                self.process.terminate()
                self.process.wait(timeout=3)
            except Exception:
                try:
                    self.process.kill()
                except Exception:
                    pass
            self.process = None