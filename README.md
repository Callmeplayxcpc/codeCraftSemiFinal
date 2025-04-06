## HuaweiCodeCraft

### 让gpt喂了一些比较关键的思路 可以参照一下

由于github文件限制 data文件夹下没有放大样例 可以自己放+测试
实现了如下两个脚本方便测试(脚本在本地使用可能要加权限 chmod +x file.sh)

go.sh 编译C++文件并生成可执行文件 然后调用python3运行执行器
使用方法：./go.sh

dabao.sh 打包压缩提交所需要的文件
使用方法：./dabao.sh

# 读策略：

# 写策略：
    write_single_rep1：顺序写入，奇数单独处理多出来的块
    write_single_rep2：根据pre input的读取数量分布给每个tag分配对应大小的磁盘空间
    write_single_rep3：给每个对象的siz一定权重，据此分配磁盘空间（目前较好）
    write_single_rep4: 根据每个时间片组的各tag数量去分配对应的磁盘空间
    write_single_rep5: 按当前时间片所在组的各tag数量(在磁盘中)，与4的区别在于，5使用了未来一小段信息。
    write_single_rep6: 在write single rep2基础上，将对象拆成 size/2块大小为2的，和size%2块大小为1的，分别从该标签磁盘空间的两端开始放
    write_single_rep7: 在write single rep2基础上，尽量把对象连在一起
    
## test_parameter.sh使用说明

./test_parameter.sh 即可使用 循环A 找到最优的A B策略
输出会重定向到output_log.txt 出来之后丢给gpt分析叫他找最优就行

若要提交或其他方法测试：
A B的值不再由main.cpp决定 而是编译时期由CMake决定
找到如下代码修改就行
```cpp
# 设置 A_VALUE 和 B_VALUE 的默认值
if(NOT DEFINED A_VALUE)
    set(A_VALUE 0.18)  # 默认值
endif()

if(NOT DEFINED B_VALUE)
    set(B_VALUE 0)  # 默认值
endif()
```
