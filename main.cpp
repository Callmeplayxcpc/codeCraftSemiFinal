#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "actions.h"
#include "storage.h"

int read_cnt[20];

#ifndef A_VALUE
#define A_VALUE 0.19  //** 这里的值要在cmake指定 这里指定没用
#endif

#ifndef B_VALUE
#define B_VALUE 0
#endif

int main()
{
    // T: 时间片数 1 ≤ 𝑇 ≤ 86400
    // M: 对象标签数 1 ≤ 𝑀 ≤ 16
    // N: 磁盘数 3 ≤ 𝑁 ≤ 10
    // V: 每个磁盘的单元数 1 ≤ 𝑉 ≤ 16384
    // G: 每个磁头每个时间片的令牌数 64 ≤ 𝐺 ≤ 500 初赛是1000
    // K: 每次垃圾回收事件每个硬盘最多的交换存储单元的操作次数 0 ≤ 𝐾 ≤ 100
    scanf("%d%d%d%d%d%d", &T, &M, &N, &V, &G, &K);

    A = A_VALUE;
    B = B_VALUE;

    // 打印 A_VALUE 和 B_VALUE 来检查它们是否正确传递
    // std::cerr << "A_VALUE: " << A_VALUE << std::endl;
    // std::cerr << "B_VALUE: " << B_VALUE << std::endl;

    // std::cerr << "A=" << A << " B=" << B << std::endl;

    // 全局预处理阶段：读取三个分块数据（删除、写入、读取的统计数据，此处用 %*d 跳过）
    for (int i = 1; i <= M; i++)
    {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }
    for (int i = 1; i <= M; i++)
    {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++)
        {
            scanf("%*d");
        }
    }
    // 读取写入数据
    int read_tot = 0;
    for (int i = 1; i <= M; i++)
    {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++)
        {
            int t;
            scanf("%d", &t);
            read_cnt[i] += t;
            read_tot += t;
        }
    }
    for (int i = 1; i <= M; ++i)
    {
        tag_weights[i] = ceil((long double)read_cnt[i] / read_tot * 1e6);
        total_tag_weights += tag_weights[i];
        tag_weights[i] += tag_weights[i - 1];
    }
    // 输出预处理完成标志
    printf("OK\n");
    fflush(stdout);

    // 按时间片循环处理各类交互事件
    for (int t = 1; t <= T + EXTRA_TIME; t++)
    {
        timestamp_action();  // 处理时间片事件
        delete_action();     // 处理删除事件
        write_action();      // 处理写入事件
        read_action();       // 处理读取事件
        if (t % FRE_PER_SLICING == 0)
        {
            gc_action();
        }
    }
    // 处理结束，清理资源
    clean();

    return 0;
}
