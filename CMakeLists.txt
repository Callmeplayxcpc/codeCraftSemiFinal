cmake_minimum_required(VERSION 3.8)  # 不要修改
project(CodeCraft)                   # 不要修改

set(CMAKE_CXX_STANDARD      17)      # 不要修改
set(CMAKE_C_STANDARD        11)      # 不要修改

set(EXECUTABLE_OUTPUT_PATH  ${PROJECT_SOURCE_DIR}/) # 不要修改
option(BUILD_SHARED_LIBS    ""  OFF) # 不要修改

# 设置 A_VALUE 和 B_VALUE 的默认值
if(NOT DEFINED A_VALUE)
    set(A_VALUE 0.637104)  # 默认值
endif()

if(NOT DEFINED B_VALUE)
    set(B_VALUE 12)  # 默认值
endif()

# 输出传入的参数，确保它们正确传递
# message(STATUS "A_VALUE is ${A_VALUE}")
# message(STATUS "B_VALUE is ${B_VALUE}")

# 获取源文件
aux_source_directory(./ cur_src)  # 不要修改

# 创建可执行文件
add_executable(code_craft ${cur_src})  # ！！！不要修改 code_craft 名称，直接影响结果；可以根据语法在 ${cur_src} 后面追加

# 通过 CMake 将 A_VALUE 和 B_VALUE 传递给 C++ 代码
target_compile_definitions(code_craft PRIVATE A_VALUE=${A_VALUE} B_VALUE=${B_VALUE})

# 添加极致优化选项
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)  # 启用 LTO

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
-O3 \
-flto \
-march=native \
-mtune=native \
-ffast-math \
-funsafe-math-optimizations \
-fstrict-aliasing \
-fomit-frame-pointer \
-fno-exceptions \
-fno-rtti \
-fno-stack-protector \
-fno-ident \
-fmerge-all-constants \
-fno-semantic-interposition \
-frename-registers \
-falign-functions=64 \
-falign-jumps=64 \
-falign-loops=64 \
-falign-labels=64 \
-DNDEBUG ")

# 可选：开启 Unity Build（合并源文件，优化编译/链接/inline）
set_target_properties(code_craft PROPERTIES UNITY_BUILD ON)




