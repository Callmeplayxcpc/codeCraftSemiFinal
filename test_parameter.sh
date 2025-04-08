#!/bin/bash

# 输出文件
OUTPUT_FILE="output_log.txt"
ORIGINAL_DIR=$(pwd)  # 保存当前目录路径

# 清空输出文件
> "$ORIGINAL_DIR/$OUTPUT_FILE"

# 进入构建目录
cd build

# A 的循环：0.0 到 1.0，步长 0.01
for A in $(seq 0 0.04 1 | xargs printf "%.2f\n"); do

    {
        # 清理构建缓存
        rm -rf CMakeCache.txt CMakeFiles

        # 配置和构建
        cmake -DA_VALUE="$A" ..
        cmake --build .

        # 运行测试
        echo "Running test with A=$A"
        cd ..
        python3 ./run.py ./interactor ./data/sample_practice.in ./code_craft
        cd build
    } 2>&1 | grep -E '^Running test with A=|^ok {' >> "$ORIGINAL_DIR/$OUTPUT_FILE"
done