#!/usr/bin/env python3
import os
import sys
import subprocess
import re
import json
import math
import random

def run_test(A_value, B_value, root_dir, build_dir):
    """
    配置、构建并运行测试程序，返回测试得分（浮点数）。

    步骤：
      1. 在 build 目录中清除 CMake 缓存文件（CMakeCache.txt 和 CMakeFiles），
         以确保使用干净的构建环境；
      2. 调用 cmake 进行配置时传入 A_value 和 B_value（确保 CMakeLists.txt 已处理这两个变量）；
      3. 执行构建过程；
      4. 切换到项目根目录运行测试程序 run.py，将测试输出中以 "ok " 开头的 JSON 数据解析，
         并返回其中的 "score" 作为浮点数得分；
      5. 如果构建或解析失败，返回 None。
    """
    try:
        subprocess.run(
            ["rm", "-rf", "CMakeCache.txt", "CMakeFiles"],
            cwd=build_dir,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
    except subprocess.CalledProcessError as e:
        print("清除构建缓存失败：", e, file=sys.stderr)
        return None

    cmake_config = subprocess.run(
        ["cmake", "-DA_VALUE={}".format(A_value), "-DB_VALUE={}".format(B_value), ".."],
        cwd=build_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    if cmake_config.returncode != 0:
        print("CMake 配置失败:", cmake_config.stderr, file=sys.stderr)
        return None

    cmake_build = subprocess.run(
        ["cmake", "--build", "."],
        cwd=build_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    if cmake_build.returncode != 0:
        print("构建失败:", cmake_build.stderr, file=sys.stderr)
        return None

    test_run = subprocess.run(
        ["python3", "./run.py", "./interactor", "./data/sample_practice.in", "./code_craft"],
        cwd=root_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    output = test_run.stdout + "\n" + test_run.stderr

    # 从输出中搜索以 "ok " 开头的 JSON 字符串，并解析出 "score"
    match = re.search(r'^ok\s+(.*)$', output, re.MULTILINE)
    if match:
        json_str = match.group(1).strip()
        try:
            data = json.loads(json_str)
            score = float(data.get("score", 0))
            return score
        except Exception as e:
            print("解析 JSON 失败:", e, file=sys.stderr)
            return None
    else:
        print("未在输出中找到有效的分数信息", file=sys.stderr)
        return None

def simulated_annealing(root_dir):
    """
    使用模拟退火算法寻找最佳参数组合：
      - 参数 A 的范围为 [0, 1]；
      - 参数 B 的范围为 [3, 30]（均为浮点数）。

    为保证候选生成更合理，采用固定基础扰动步长再乘以 (T / T_init) 生成候选扰动：
      - 对 A，基础步长设为 0.1；
      - 对 B，区间 [3,30] 宽度为 27，10% 为 2.7；
    即：delta_A = random.uniform(-0.1, 0.1) * (T / T_init)
         delta_B = random.uniform(-2.7, 2.7) * (T / T_init)

    模拟退火参数：
      - 初始温度 T_init 设为 400,000
      - 终止温度 T_min 设为 10,000
      - 冷却因子为 0.95
      - 最大迭代次数设为 100

    在每次迭代中记录候选解（A_candidate, B_candidate）、候选得分及接受决策，
    并写入日志文件 output_log.txt 中，同时在终端输出。
    """
    # 构建目录和日志文件路径
    build_dir = os.path.join(root_dir, "build")
    log_file_path = os.path.join(root_dir, "output_log.txt")
    log_file = open(log_file_path, "w", encoding="utf-8")
    
    # 模拟退火温度参数设定
    T_init = 400000.0    # 初始温度设为 400,000
    T = T_init           # 当前温度从初始温度开始
    T_min = 10000.0      # 当温度降低到 10,000 以下时终止迭代
    cooling_rate = 0.95  # 每迭代一次温度乘以 0.95
    max_iter = 100       # 最大迭代次数

    # 固定基础扰动步长，根据参数区间设置
    base_step_A = 0.1    # 对 A，允许扰动范围 ±0.1
    base_step_B = 2.7    # 对 B，允许扰动范围 ±2.7（10% 的 [3,30] 区间宽度 27）

    # 初始参数：A 取 0.5；B 取 [3,30] 的中值，即 (3+30)/2 = 16.5  可以人为调控为已知比较优的A B增加搜索效率
    A_current = 0.912225
    B_current = 14.018311
    score_current = run_test(A_current, B_current, root_dir, build_dir)
    if score_current is None:
        err_msg = "初始测试运行失败。"
        print(err_msg, file=sys.stderr)
        log_file.write(err_msg + "\n")
        log_file.close()
        sys.exit(1)

    # 记录全局最优解，初始解即为全局最优
    best_A = A_current
    best_B = B_current
    best_score = score_current
    
    init_msg = "起始参数: A = {:.6f}, B = {:.6f}, Score = {:.4f}\n".format(A_current, B_current, score_current)
    print(init_msg, end="")
    log_file.write(init_msg)

    # 进入迭代
    for iter in range(max_iter):
        if T < T_min:
            break

        # 根据当前温度按 (T / T_init) 调整扰动幅度
        delta_A = random.uniform(-base_step_A, base_step_A) * (T / T_init)
        A_candidate = A_current + delta_A
        A_candidate = max(0.0, min(1.0, A_candidate))  # 确保 A_candidate 在 [0,1]

        delta_B = random.uniform(-base_step_B, base_step_B) * (T / T_init)
        B_candidate = B_current + delta_B
        B_candidate = max(3.0, min(30.0, B_candidate))   # 确保 B_candidate 在 [3,30]

        # 评估候选解的得分
        score_candidate = run_test(A_candidate, B_candidate, root_dir, build_dir)
        if score_candidate is None:
            continue

        # 记录候选解信息到日志和终端
        candidate_info = ("迭代 {:>3} 候选解: A_candidate = {:.6f}, B_candidate = {:.6f}, "
                          "score_candidate = {:.4f}\n").format(iter+1, A_candidate, B_candidate, score_candidate)
        print(candidate_info, end="")
        log_file.write(candidate_info)

        # 如果候选解比当前解得分更高，则直接接受
        if score_candidate >= score_current:
            decision_msg = "迭代 {:>3}: 直接接受候选解。\n".format(iter+1)
            A_current, B_current, score_current = A_candidate, B_candidate, score_candidate
            # 更新全局最优解（如果比历史最佳还高）
            if score_candidate > best_score:
                best_A, best_B, best_score = A_candidate, B_candidate, score_candidate
        else:
            # 如果候选解得分较低，则以一定概率接受
            # 计算得分差（负数）
            delta_score = score_candidate - score_current
            p = math.exp(delta_score / T)
            rand_val = random.random()
            if rand_val < p:
                decision_msg = ("迭代 {:>3}: 以概率接受候选解 (p = {:.6f}, rand = {:.6f})\n"
                                .format(iter+1, p, rand_val))
                A_current, B_current, score_current = A_candidate, B_candidate, score_candidate
            else:
                decision_msg = ("迭代 {:>3}: 拒绝候选解 (p = {:.6f}, rand = {:.6f})\n"
                                .format(iter+1, p, rand_val))
        print(decision_msg, end="")
        log_file.write(decision_msg)

        # 冷却降温更新温度
        T *= cooling_rate

        # 记录当前解状态及温度
        iteration_msg = ("迭代 {:>3}: 当前解: A_current = {:.6f}, B_current = {:.6f}, "
                         "score_current = {:.4f}, 温度 T = {:.6f}\n"
                         .format(iter+1, A_current, B_current, score_current, T))
        print(iteration_msg, end="")
        log_file.write(iteration_msg)

    # 记录并输出最终找到的最优结果
    final_msg = "\n最终最优结果: A = {:.6f}, B = {:.6f}, Score = {:.4f}\n".format(best_A, best_B, best_score)
    print(final_msg, end="")
    log_file.write(final_msg)
    log_file.close()
    
    return best_A, best_B, best_score

if __name__ == "__main__":
    # 假设当前工作目录为项目根目录，且 build 文件夹位于该根目录中，
    # 日志将保存在 output_log.txt 文件中
    root_directory = os.getcwd()
    simulated_annealing(root_directory)
