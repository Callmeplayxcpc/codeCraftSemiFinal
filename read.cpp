#include "read.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "storage.h"

vector<int> out_time_request[EXTRA_TIME];  // 存储超时请求

int ptr[2][20], last_time[2][20];  // 复赛 有两根针 第一维表示第几根针 第二维表示哪个磁盘的针
//**ptr代表第i个磁盘的指针在哪个单元，为了方便实现，它的值是0-V-1，实际位置是ptr[i]+1
//**last_time表示第i个磁盘上个时间片最后一次操作的读取时间是多少，是为了跨时间片维护，如果该操作是移动，那就置为大值

void timeOutRequest(vector<int> &busyId)
{
    for (int request_id : out_time_request[timestamp % EXTRA_TIME])  // 获取哪些请求超时
    {
        if (!request[request_id].is_done&&!object[request[request_id].object_id].is_delete)  // 如果这个请求还没完成
        {
            busyId.push_back(request_id);  // 记录超时请求
            request[request_id].is_done = true;  // 标记请求完成
        }
        int object_id = request[request_id].object_id;  // 获取对象id
        for (int block_id : request[request_id].rest)   // 获取对象在哪个块
        {
            // 删除这个对象在这个块上的请求
            if (object[object_id].request[block_id].count({request_id, block_id})) object[object_id].request[block_id].erase({request_id, block_id});
            if (!object[object_id].request[block_id].size())  // 如果这个对象在这个块上已经被完全删除了
            {
                for (int copy_id = 1; copy_id <= 3; copy_id++)
                {
                    int disk_id = object[object_id].replica[copy_id];
                    int unit_id = object[object_id].unit[copy_id][block_id];
                    if (disk_vector[disk_id].count(unit_id)) disk_vector[disk_id].erase(unit_id);
                }
            }
        }
    }
    vector<int>().swap(out_time_request[timestamp % EXTRA_TIME]);
}

int cal_min_dist(int ptr[], int disk_id, int to)
{
    //**计算 指针以及第disk_id个磁盘中所有待读单元 到to单元的最短距离（这里特指到达to）
    //**神奇的贪心策略，但是很奇怪
    //**这个贪心没有干过 按磁盘待读取单元数多少去排序 的策略
    int res = to - ptr[disk_id] - 1;
    if (res < 0) res += V;
    if (disk_vector[disk_id].size())
    {
        int tmp;
        auto it = disk_vector[disk_id].upper_bound(to);  // 求to在环上的前驱
        if (it == disk_vector[disk_id].begin()) tmp = to - *prev(disk_vector[disk_id].end());
        else tmp = to - *prev(it);
        if (tmp < 0) tmp += V;
        res = min(res, tmp);
    }
    return res;
}

int cal_to_pos(int disk_id, int pos)
{
    //**计算第disk_id个磁盘从第pos个单元出发下一个待读取单元在哪
    auto it = disk_vector[disk_id].lower_bound(pos);
    if (it == disk_vector[disk_id].end()) return *disk_vector[disk_id].begin();
    else return *it;
}

int cal_min_near_dist(int disk_id, int pos)
{
    //**计算 指针以及第disk_id个磁盘中所有待读单元 与to单元的最短距离（不特指顺序）
    int resL = min(cal_min_dist(ptr[0], disk_id, pos), cal_min_dist(ptr[1], disk_id, pos));
    int resR = cal_to_pos(disk_id, pos) - pos;
    if (resR < 0) resR += V;
    return min(resL, resR);
}

int cal_weight(int disk_id, int pos)  // test
{
    static array<long double, 2> weight_to_choose_disk = {A, B};  // 前者越大则距离更重要，后者越大则任务数更重要 关注A的值 B的值在下面算 保持A+B=1
    weight_to_choose_disk[1] = 1 - weight_to_choose_disk[0];
    return -cal_min_near_dist(disk_id, pos) * weight_to_choose_disk[0] - disk_vector[disk_id].size() * weight_to_choose_disk[1];
};

void readRequest()
{
    int n_read;
    int request_id, object_id;
    scanf("%d", &n_read);
    for (int i = 1; i <= n_read; i++)
    {
        scanf("%d%d", &request_id, &object_id);
        request[request_id].object_id = object_id;                           // 记录请求的对象id
        request[request_id].prev_id = object[object_id].last_request_point;  // 记录上一个请求的id
        object[object_id].last_request_point = request_id;                   // 更新对象的上一个请求id
        request[request_id].is_done = false;                                 // 标记请求未完成
        out_time_request[timestamp % EXTRA_TIME].push_back(request_id);      // 记录超时请求

        for (int k = 1; k <= object[object_id].size; k++)
        {
            request[request_id].rest.insert(k);
            int d = 1;
            for (int j = 1; j <= 3; j++)
            {
                int mn = object[object_id].replica[d], now = object[object_id].replica[j];
                // 第d个副本是当前最优的副本，第j个副本是现在的副本
                // mn代表第d个副本对应的磁盘编号，now代表第j个
                int to1 = object[object_id].unit[d][k], to2 = object[object_id].unit[j][k];

                long double mask1 = cal_weight(mn, to1), mask2 = cal_weight(now, to2);

                if (mask1 < mask2) d = j;  //**按最短距离判断磁盘优劣
            }
            int mn = object[object_id].replica[d];
            disk_vector[mn].insert(object[object_id].unit[d][k]);  // 待处理单元放入磁盘容器
            object[object_id].request[k].insert({request_id, k});  // 这个vec存储该对象的第i个块与哪些请求相关，存的值是request_id
        }
    }
}

void read(int diskId, int ptr[], int last_time[], vector<int> &finish)  // 选择好第几根针 就能保持原来的逻辑
{
    string res;  //**该磁盘在该时间片内的操作
    //**处理jump-----------------------------------------------------
    if (!disk_vector[diskId].size())
    {
        cout << "#\n";
        return;
    }

    int to = cal_to_pos(diskId, ptr[diskId] + 1);  //**读取顺序策略是不管进入容器顺序，优先读取距离最近的
    int dis = to - ptr[diskId] - 1;                // 距离目标单元的距离
    if (dis < 0) dis += V;
    if (dis > G)
    {
        res += "j " + to_string(to);
        ptr[diskId] = to - 1;
        last_time[diskId] = 0;
        cout << res << '\n';
        return;
    }
    //**-------------------------------------------------------------
    static constexpr int read_time[8] = {64, 52, 42, 34, 28, 23, 19, 16};  // 已读i次后下次读所需时间
    static pair<int, string> pass_read_dp[70][10];                         //**当前时间片内已读取j个待读单元，已经连续读了k次，此时{剩余的最大令牌数，操作序列} 用作DP

    for (int j = 0; j < 8; j++) pass_read_dp[0][j] = pair<int, string>(0, "");

    pass_read_dp[0][last_time[diskId]] = pair<int, string>(G, "");  // 初始化
    for (int j = 0; j < 70; j++)                                    // 最多读1000/16个单元，1000是G的最大值
    {
        if (!disk_vector[diskId].size())  // 没有要读的
        {
            pair<int, string> best_option = {-1, ""};  // 花费时间最少的操作
            for (int k = 0; k < 8; k++) best_option = max(best_option, pass_read_dp[j][k]);
            for (int k = 0; k < 8; k++)
                if (best_option == pass_read_dp[j][k]) last_time[diskId] = k;
            res = best_option.second;
            break;
        }

        int to = cal_to_pos(diskId, ptr[diskId] + 1);  //**读取顺序策略是不管进入容器顺序，优先读取距离最近的
        int dis = to - ptr[diskId] - 1;                // 距离目标单元的距离
        if (dis < 0) dis += V;
        for (int k = 0; k < 8; k++) pass_read_dp[j + 1][k] = {-1, ""};  // 剩余时间小于0就不可行了
        for (int k = 0; k < 8; k++)
        {
            int rest_time = pass_read_dp[j][k].first;   // 剩余时间
            string option = pass_read_dp[j][k].second;  // 操作
            // 情况1，dis=0;
            if (!dis)
            {
                int nxt = min(7, k + 1);                                                                                              // 要read了 算下一个位置是哪
                pass_read_dp[j + 1][nxt] = max(pass_read_dp[j + 1][nxt], pair<int, string>(rest_time - read_time[k], option + "r"));  // dp更新
            }
            else
            {
                int rest_time1 = rest_time;
                string option1 = option;  // 一直pass
                for (int dis_i = 1; dis_i <= dis; dis_i++) rest_time1--, option1 += 'p';

                rest_time1 -= read_time[0], option1 += 'r';
                pass_read_dp[j + 1][1] = max(pair<int, string>(rest_time1, option1), pass_read_dp[j + 1][1]);

                int rest_time2 = rest_time;
                string option2 = option;  // 一直read
                int read_times = k;       // 连续读取次数
                for (int dis_i = 1; dis_i <= dis; dis_i++)
                {
                    rest_time2 -= read_time[read_times];
                    read_times = min(7, read_times + 1);
                    option2 += 'r';
                }
                rest_time2 -= read_time[read_times];
                read_times = min(7, read_times + 1);
                option2 += 'r';

                pass_read_dp[j + 1][read_times] = max(pair<int, string>(rest_time2, option2), pass_read_dp[j + 1][read_times]);
            }
        }

        pair<int, string> best_option = {-1, ""};
        for (int k = 0; k < 8; k++) best_option = max(best_option, pass_read_dp[j + 1][k]);

        if (best_option.first >= 0)  // 可以到达当前
        {
            ptr[diskId] = to % V;
            // ptr[i]=(to-1+V)%V;
            for (auto [request_id, uid] : object[disk[diskId][to]].request[disk_uid[diskId][to]])  // 更新相关请求
            {
                if (request[request_id].is_done) continue;

                if (!request[request_id].rest.count(uid)) continue;

                request[request_id].rest.erase(uid);
                if (!request[request_id].rest.size())  //**该请求被完成
                {
                    finish.push_back(request_id);
                    request[request_id].is_done = true;
                }
            }

            set<array<int, 2>>().swap(object[disk[diskId][to]].request[disk_uid[diskId][to]]);  // 清空并释放空间
            disk_vector[diskId].erase(to);
        }
        else
        {
            for (int k = 0; k < 8; k++) best_option = max(best_option, pass_read_dp[j][k]);
            for (int k = 0; k < 8; k++)
                if (best_option == pass_read_dp[j][k]) last_time[diskId] = k;
            res = best_option.second;  // 不能read到下一个了 现在最优策略就是一直pass
            while (best_option.first-- && ptr[diskId] + 1 != to) ptr[diskId] = (ptr[diskId] + 1) % V, res += 'p', last_time[diskId] = 0;

            break;
        }
    }
    if (res[0] != 'j') res += "#";
    cout << res << '\n';
}

void read_action()
{
    vector<int> busyId;
    timeOutRequest(busyId);// 超时请求

    readRequest();// 读取请求

    vector<int> finish;  // 此次完成的请求
    for (int i = 1; i <= N; i++)
    {
        read(i, ptr[0], last_time[0], finish);
        read(i, ptr[1], last_time[1], finish);
    }
    cout << finish.size() << '\n';
    for (int v : finish) cout << v << '\n';

    cout<<busyId.size()<<'\n';
    for (int v : busyId) cout << v << '\n';  // 输出超时请求

    fflush(stdout);  // 这里上面的IO都是cout，可以最后进行优化
}