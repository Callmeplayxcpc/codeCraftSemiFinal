#!/usr/bin/env python3
import os
import sys
import subprocess
import re
import json
import math
import random
import multiprocessing

# 使用全局字典缓存评估结果
evaluation_cache = {}  # key: (round(A,5), round(B,5), round(C,5), round(D,5)) , value: score

def run_test(A_value, B_value, C_value, D_value, root_dir, build_dir):
    """
    优化后的评估函数：
      - 使用缓存避免重复构建和测试。
      - 构建时增加并行编译参数以加速进程。
      - 接受参数 A, B, C 与 D。
    """
    global evaluation_cache
    # 四舍五入后作为缓存键（可调精度）
    key = (round(A_value, 5), round(B_value, 5), round(C_value, 5), round(D_value, 5))
    if key in evaluation_cache:
        print(f"使用缓存结果: A = {A_value:.6f}, B = {B_value:.6f}, C = {C_value:.6f}, D = {D_value:.6f}, Score = {evaluation_cache[key]:.4f}")
        return evaluation_cache[key]

    # 清除构建缓存
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

    # 配置 cmake，传入参数 A、B、C 和 D
    cmake_config = subprocess.run(
        ["cmake",
         "-DA_VALUE={}".format(A_value),
         "-DB_VALUE={}".format(B_value),
         "-DC_VALUE={}".format(C_value),
         "-DD_VALUE={}".format(D_value),
         ".."],
        cwd=build_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    if cmake_config.returncode != 0:
        print("CMake 配置失败:", cmake_config.stderr, file=sys.stderr)
        return None

    # 使用并行构建（默认使用 os.cpu_count() 个核心）
    num_cores = str(os.cpu_count() or 1)
    cmake_build = subprocess.run(
        ["cmake", "--build", ".", "--", "-j", num_cores],
        cwd=build_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    if cmake_build.returncode != 0:
        print("构建失败:", cmake_build.stderr, file=sys.stderr)
        return None

    # 运行测试程序
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
            evaluation_cache[key] = score  # 缓存结果
            return score
        except Exception as e:
            print("解析 JSON 失败:", e, file=sys.stderr)
            return None
    else:
        print("未在输出中找到有效的分数信息", file=sys.stderr)
        return None

def simulated_annealing(root_dir):
    """
    优化后的模拟退火：
      - 冷却因子调为 0.95，延长高温阶段，使搜索更充分；
      - 增加最大迭代次数；
      - 在扰动时确保扰动因子不低于下限 min_step_factor。
      - 新增参数 C 与 D 的优化，参数范围分别为 [100, 10000] 与 [1, 5000]。
    """
    build_dir = os.path.join(root_dir, "build")
    log_file_path = os.path.join(root_dir, "output_log.txt")
    with open(log_file_path, "w", encoding="utf-8") as log_file:
        # 模拟退火参数
        T_init = 400000.0    # 初始温度
        T = T_init           # 当前温度
        T_min = 10000.0      # 终止温度
        cooling_rate = 0.96  # 更缓的冷却
        max_iter = 200       # 增加迭代次数

        # A 和 B 的扰动参数
        base_step_A = 0.1    # 对 A 允许的扰动范围
        base_step_B = 2.7    # 对 B 允许的扰动范围
        
        # 新增参数 C 和 D 的扰动参数（各取范围的 10%）
        base_step_C = (10000 - 100) * 0.1  # 约 990.0
        base_step_D = (5000 - 1) * 0.1      # 约 499.9

        min_step_factor = 0.2  # 在低温时也保持至少 20% 的基础步长

        # 初始参数（可调整为已知较优解加快搜索）
        A_current = 0.912225
        B_current = 14.018311
        C_current = 5050.0   # 可根据实际情况调整初始值
        D_current = 2500.0   # 可根据实际情况调整初始值

        score_current = run_test(A_current, B_current, C_current, D_current, root_dir, build_dir)
        if score_current is None:
            err_msg = "初始测试运行失败。"
            print(err_msg, file=sys.stderr)
            log_file.write(err_msg + "\n")
            sys.exit(1)

        best_A, best_B, best_C, best_D, best_score = A_current, B_current, C_current, D_current, score_current

        init_msg = "起始参数: A = {:.6f}, B = {:.6f}, C = {:.6f}, D = {:.6f}, Score = {:.4f}\n".format(
            A_current, B_current, C_current, D_current, score_current)
        print(init_msg, end="")
        log_file.write(init_msg)

        for iter in range(max_iter):
            if T < T_min:
                break

            # 计算扰动幅度因子，确保不低于最小比例
            step_factor = max(T / T_init, min_step_factor)

            # 生成 A 和 B 的候选解并限幅
            delta_A = random.uniform(-base_step_A, base_step_A) * step_factor
            A_candidate = max(0.0, min(1.0, A_current + delta_A))

            delta_B = random.uniform(-base_step_B, base_step_B) * step_factor
            B_candidate = max(3.0, min(30.0, B_current + delta_B))

            # 生成 C 和 D 的候选解并限幅
            delta_C = random.uniform(-base_step_C, base_step_C) * step_factor
            C_candidate = max(100.0, min(10000.0, C_current + delta_C))

            delta_D = random.uniform(-base_step_D, base_step_D) * step_factor
            D_candidate = max(1.0, min(5000.0, D_current + delta_D))

            # 评估候选解得分
            score_candidate = run_test(A_candidate, B_candidate, C_candidate, D_candidate, root_dir, build_dir)
            if score_candidate is None:
                continue

            candidate_info = ("迭代 {:>3} 候选解: A_candidate = {:.6f}, B_candidate = {:.6f}, "
                              "C_candidate = {:.6f}, D_candidate = {:.6f}, score_candidate = {:.4f}\n"
                              ).format(iter+1, A_candidate, B_candidate, C_candidate, D_candidate, score_candidate)
            print(candidate_info, end="")
            log_file.write(candidate_info)

            # 判断接受候选解
            if score_candidate >= score_current:
                decision_msg = "迭代 {:>3}: 直接接受候选解。\n".format(iter+1)
                A_current, B_current, C_current, D_current, score_current = A_candidate, B_candidate, C_candidate, D_candidate, score_candidate
                if score_candidate > best_score:
                    best_A, best_B, best_C, best_D, best_score = A_candidate, B_candidate, C_candidate, D_candidate, score_candidate
            else:
                delta_score = score_candidate - score_current
                p = math.exp(delta_score / T)
                rand_val = random.random()
                if rand_val < p:
                    decision_msg = ("迭代 {:>3}: 以概率接受候选解 (p = {:.6f}, rand = {:.6f})\n"
                                    .format(iter+1, p, rand_val))
                    A_current, B_current, C_current, D_current, score_current = A_candidate, B_candidate, C_candidate, D_candidate, score_candidate
                else:
                    decision_msg = ("迭代 {:>3}: 拒绝候选解 (p = {:.6f}, rand = {:.6f})\n"
                                    .format(iter+1, p, rand_val))
            print(decision_msg, end="")
            log_file.write(decision_msg)

            # 降温更新
            T *= cooling_rate

            iteration_msg = ("迭代 {:>3}: 当前解: A_current = {:.6f}, B_current = {:.6f}, "
                             "C_current = {:.6f}, D_current = {:.6f}, score_current = {:.4f}, 温度 T = {:.6f}\n"
                             .format(iter+1, A_current, B_current, C_current, D_current, score_current, T))
            print(iteration_msg, end="")
            log_file.write(iteration_msg)

        final_msg = "\n最终最优结果: A = {:.6f}, B = {:.6f}, C = {:.6f}, D = {:.6f}, Score = {:.4f}\n".format(
            best_A, best_B, best_C, best_D, best_score)
        print(final_msg, end="")
        log_file.write(final_msg)
        return best_A, best_B, best_C, best_D, best_score

if __name__ == "__main__":
    # 假设当前工作目录为项目根目录，且 build 文件夹位于该根目录中
    root_directory = os.getcwd()
    simulated_annealing(root_directory)
