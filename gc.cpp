#include "gc.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>

#include "storage.h"

// 改 Object disk disk_uid disk_vector tag_pos
// disk_empty 在外面改
void do_swap(int disk_id, int pos1, int pos2)
{
    if (disk[disk_id][pos2] > 0)
        swap(pos1, pos2);
    if (disk[disk_id][pos2] > 0)
    {
        int id1 = disk[disk_id][pos1], id2 = disk[disk_id][pos2];
        int rep_id1, rep_id2;

        tag_pos[disk_id][object[id1].tag].erase(pos1);
        tag_pos[disk_id][object[id1].tag].insert(pos2);

        tag_pos[disk_id][object[id2].tag].erase(pos2);
        tag_pos[disk_id][object[id2].tag].insert(pos1);

        for (int i = 1; i <= REP_NUM; ++i)
        {
            if (object[id1].replica[i] == disk_id)
                rep_id1 = i;
            if (object[id2].replica[i] == disk_id)
                rep_id2 = i;
        }
        object[id1].unit[rep_id1][disk_uid[disk_id][pos1]] = pos2;
        object[id2].unit[rep_id2][disk_uid[disk_id][pos2]] = pos1;

        if (disk_vector[disk_id].count(pos1) && !disk_vector[disk_id].count(pos2))
        {
            disk_vector[disk_id].erase(pos1);
            disk_vector[disk_id].insert(pos2);
        }
        else if (disk_vector[disk_id].count(pos2) && !disk_vector[disk_id].count(pos1))
        {
            disk_vector[disk_id].erase(pos2);
            disk_vector[disk_id].insert(pos1);
        }

        swap(disk[disk_id][pos1], disk[disk_id][pos2]);
        swap(disk_uid[disk_id][pos1], disk_uid[disk_id][pos2]);
    }
    else
    {
        int id = disk[disk_id][pos1];
        int rep_id;

        tag_pos[disk_id][object[id].tag].erase(pos1);
        tag_pos[disk_id][object[id].tag].insert(pos2);

        for (int i = 1; i <= REP_NUM; ++i)
        {
            if (object[id].replica[i] == disk_id)
                rep_id = i;
        }

        object[id].unit[rep_id][disk_uid[disk_id][pos1]] = pos2;

        if (disk_vector[disk_id].count(pos1))
        {
            disk_vector[disk_id].erase(pos1);
            disk_vector[disk_id].insert(pos2);
        }

        swap(disk[disk_id][pos1], disk[disk_id][pos2]);
        swap(disk_uid[disk_id][pos1], disk_uid[disk_id][pos2]);
    }
}

// 交换让磁盘中相同tag的块更加紧凑，让右端点不断向左靠，或者让左端点不断向右靠
// 现在只允许交换到空位上
vector<array<int, 2>> do_gc1(int disk_id)
{
    int ord[MAX_LABEL];
    for (int i = 1; i <= M; ++i)
        ord[i] = i;
    sort(ord + 1, ord + M + 1, [&](int x, int y)
         { return disk_size[disk_id][x] > disk_size[disk_id][y]; });
    vector<array<int, 2>> res;
    int left_k = K;
    for (int i = 1; i <= M; ++i)
    {
        int tag = ord[i];
        if (disk_size[disk_id][ord[i]] == 0)
        {
            break;
        }
        if (tag & 1)
        {
            int L = *tag_pos[disk_id][tag].begin();
            for (auto it = disk_empty[disk_id].begin(); it != disk_empty[disk_id].end() && left_k;)
            {
                int R = *tag_pos[disk_id][tag].rbegin();
                if (*it > L && *it < R)
                {
                    do_swap(disk_id, *it, R);
                    res.push_back({*it, R});
                    auto nex = disk_empty[disk_id].erase(it);
                    int nexp = 0;
                    if (nex == disk_empty[disk_id].end())
                        nexp = R;
                    else
                        nexp = *nex;
                    disk_empty[disk_id].insert(R);
                    it = disk_empty[disk_id].find(nexp);
                    left_k--;
                }
                else
                {
                    if (*it > R)
                        break;
                    it++;
                }
            }
        }
        else
        {
            int R = *tag_pos[disk_id][tag].rbegin();
            for (auto it = disk_empty[disk_id].rbegin(); it != disk_empty[disk_id].rend() && left_k;)
            {
                int L = *tag_pos[disk_id][tag].begin();
                if (*it > L && *it < R)
                {
                    do_swap(disk_id, *it, L);
                    res.push_back({*it, L});
                    auto forward_it = it.base();
                    --forward_it;
                    auto nex = disk_empty[disk_id].erase(forward_it);
                    int nexp = 0;
                    if (nex == disk_empty[disk_id].begin())
                        nexp = L;
                    else
                        nexp = *(nex--);
                    disk_empty[disk_id].insert(L);
                    it = set<int>::reverse_iterator(disk_empty[disk_id].find(nexp));
                    left_k--;
                }
                else
                {
                    if (*it < L)
                        break;
                    it++;
                }
            }
        }
    }
    return res;
}
vector<array<int, 2>> do_gc2(int disk_id) // 配合write_single_rep7的策略，把分配后空间内的空位尽量换回该标签的值
{
    int left_k = K;
    vector<array<int, 2>> res;

    for (int label_id = 1; label_id <= M; label_id++) // 不使用ord分数高一点
    {
        int start = ceil((long double)tag_weights[label_id - 1] * V / total_tag_weights);

        int end = V + start - 1;
        if (label_id & 1)
        {
            for (int i = start; i <= end; i++)
            {
                if (!disk[disk_id][i % V + 1] && !disk[disk_id][(i + V - 1) % V + 1])
                    continue;
                if (!disk[disk_id][i % V + 1])
                {
                    int toSwap = 0;
                    for (int j = end; j > i; j--)
                        if (object[disk[disk_id][j % V + 1]].tag == label_id)
                        {
                            toSwap = j % V + 1;
                            break;
                        }
                    if (toSwap)
                    {
                        do_swap(disk_id, i % V + 1, toSwap);
                        res.push_back({i % V + 1, toSwap});
                        left_k--;
                    }

                    else
                        continue;
                }
                if (!left_k)
                    break;
            }
        }
        else
        {
            for (int i = end; i >= start; i--)
            {
                if (!disk[disk_id][i % V + 1] && !disk[disk_id][(i + 1) % V + 1])
                    continue;
                if (!disk[disk_id][i % V + 1])
                {
                    int toSwap = 0;
                    for (int j = i + 1; j <= end; j++)
                        if (object[disk[disk_id][j % V + 1]].tag == label_id)
                        {
                            toSwap = j % V + 1;
                            break;
                        }
                    if (toSwap)
                    {
                        do_swap(disk_id, i % V + 1, toSwap);
                        res.push_back({i % V + 1, toSwap});
                        left_k--;
                    }

                    else
                        continue;
                }
                if (!left_k)
                    break;
            }
        }
        if (!left_k)
            break;
    }

    return res;
}
vector<array<int, 2>> do_gc3(int disk_id) // 配合write_single_rep7的策略，把分配后空间内的非该标签值尽量换回该标签的值
{
    int left_k = K;
    vector<array<int, 2>> res;
    int ord[MAX_LABEL];
    for (int i = 1; i <= M; ++i)
        ord[i] = i;
    sort(ord + 1, ord + M + 1, [&](int x, int y)
         { return disk_size[disk_id][x] > disk_size[disk_id][y]; });
    for (int _id = 1; _id <= M; _id++)
    {
        int label_id = ord[_id];
        int start = ceil((long double)tag_weights[label_id - 1] * V / total_tag_weights);
        int end = V + start - 1;
        if (label_id & 1)
        {
            for (int i = start; i <= end; i++)
            {
                if (!disk[disk_id][i % V + 1] && !disk[disk_id][(i + V - 1) % V + 1])
                    continue;
                if (object[disk[disk_id][i % V + 1]].tag != label_id)
                {
                    int toSwap = 0;
                    for (int j = end; j > i; j--)
                        if (object[disk[disk_id][j % V + 1]].tag == label_id)
                        {
                            toSwap = j % V + 1;
                            break;
                        }
                    if (toSwap)
                    {
                        do_swap(disk_id, i % V + 1, toSwap);
                        res.push_back({i % V + 1, toSwap});
                        left_k--;
                    }

                    else
                        continue;
                }
                if (!left_k)
                    break;
            }
        }
        else
        {
            for (int i = end; i >= start; i--)
            {
                if (!disk[disk_id][i % V + 1] && !disk[disk_id][(i + 1) % V + 1])
                    continue;
                if (object[disk[disk_id][i % V + 1]].tag != label_id)
                {
                    int toSwap = 0;
                    for (int j = i + 1; j <= end; j++)
                        if (object[disk[disk_id][j % V + 1]].tag == label_id)
                        {
                            toSwap = j % V + 1;
                            break;
                        }
                    if (toSwap)
                    {
                        do_swap(disk_id, i % V + 1, toSwap);
                        res.push_back({i % V + 1, toSwap});
                        left_k--;
                    }

                    else
                        continue;
                }
                if (!left_k)
                    break;
            }
        }
        if (!left_k)
            break;
    }

    return res;
}
vector<array<int, 2>> do_gc4(int disk_id) // 配合write_single_rep7的策略，把分配后空间内的非该标签值尽量换回该标签的值
{
    int left_k = K;
    vector<array<int, 2>> res;
    vector<int> toChangelocation[MAX_LABEL][MAX_LABEL]; // 有哪些位置希望放i结果放了j
    for (int label_id = 1; label_id <= M; label_id++)
    {
        int start = ceil((long double)tag_weights[label_id - 1] * V / total_tag_weights);
        int end = V + start - 1;
        if (label_id & 1)
        {
            int cnt = 0; // cnt超过K没意义
            for (int i = start; i <= start + min(V, 200); i++)
            {
                if (disk[disk_id][i % V + 1] && object[disk[disk_id][i % V + 1]].tag != label_id)
                    toChangelocation[label_id][object[disk[disk_id][i % V + 1]].tag].push_back(i % V + 1), cnt++;
                if (cnt > K)
                    break;
            }
        }
        else
        {
            int cnt = 0; // cnt超过K没意义
            for (int i = end; i >= end - min(V, 200); i--)
            {
                if (disk[disk_id][i % V + 1] && object[disk[disk_id][i % V + 1]].tag != label_id)
                    toChangelocation[label_id][object[disk[disk_id][i % V + 1]].tag].push_back(i % V + 1), cnt++;
      
                if (cnt > K)
                    break;
            }
        }
    }
    for (int label_id1 = 1; label_id1 <= M; label_id1++)
        for (int label_id2 = 1; label_id2 <= M; label_id2++)
            reverse(toChangelocation[label_id1][label_id2].begin(), toChangelocation[label_id1][label_id2].end()); // 优先交换排在前面的
//-------------- 方案1：想要每种搭配都换一点
    // bool changed = true;
    // while (changed) 
    // {
    //     changed = false;
    //     for (int label_id1 = 1; label_id1 <= M; label_id1++)
    //         for (int label_id2 = label_id1 + 1; label_id2 <= M; label_id2++)
    //         {
    //             if (toChangelocation[label_id1][label_id2].size() && toChangelocation[label_id2][label_id1].size())
    //             {
    //                 int x = toChangelocation[label_id1][label_id2].back(), y = toChangelocation[label_id2][label_id1].back();
    //                 toChangelocation[label_id1][label_id2].pop_back(), toChangelocation[label_id2][label_id1].pop_back();
    //                 do_swap(disk_id, x, y);
    //                 res.push_back({x, y});
    //                 left_k--;
    //                 changed = true;
    //                 if (!left_k)
    //                     return res;
    //             }
    //         }
    // }
//--------------- 方案2：优先做那些出现多的标签
    int ord[MAX_LABEL];
    for (int i = 1; i <= M; ++i)
        ord[i] = i;
    sort(ord + 1, ord + M + 1, [&](int x, int y)
         { return disk_size[disk_id][x] > disk_size[disk_id][y]; });
    for (int _id1 = 1; _id1 <= M; _id1++)
        for (int _id2=_id1+1;_id2<=M;_id2++)
        {
            int label_id1 = ord[_id1],label_id2=ord[_id2];
            if (toChangelocation[label_id1][label_id2].size() && toChangelocation[label_id2][label_id1].size())
            {
                int x = toChangelocation[label_id1][label_id2].back(), y = toChangelocation[label_id2][label_id1].back();
                toChangelocation[label_id1][label_id2].pop_back(), toChangelocation[label_id2][label_id1].pop_back();
                do_swap(disk_id, x, y);
                res.push_back({x, y});
                left_k--;
                if (!left_k)
                    return res;
            }
        }
    return res;
}
void gc_action()
{
    scanf("%*s %*s");
    printf("GARBAGE COLLECTION\n");
    for (int i = 1; i <= N; i++)
    {
        auto gc_action = do_gc4(i);
        printf("%d\n", (int)gc_action.size());
        for (auto [x, y] : gc_action)
        {
            printf("%d %d\n", x, y);
        }
    }
    fflush(stdout);
}
