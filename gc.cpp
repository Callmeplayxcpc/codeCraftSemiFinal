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
    auto calWhichPtr=[&](int unit_id)
    {
        return unit_id>=divide_line[disk_id];
    };
    if (!disk[disk_id][pos1]&&!disk[disk_id][pos2]) return;
    if (disk[disk_id][pos2] > 0) swap(pos1, pos2);
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
            if (object[id1].replica[i] == disk_id) rep_id1 = i;
            if (object[id2].replica[i] == disk_id) rep_id2 = i;
        }
        object[id1].unit[rep_id1][disk_uid[disk_id][pos1]] = pos2;
        object[id2].unit[rep_id2][disk_uid[disk_id][pos2]] = pos1;
        
        int p1=calWhichPtr(pos1),p2=calWhichPtr(pos2);

        if (disk_vector[disk_id][p1].count(pos1) && !disk_vector[disk_id][p2].count(pos2))
        {
            disk_vector[disk_id][p1].erase(pos1);
            disk_vector[disk_id][p2].insert(pos2);
        }
        else if (disk_vector[disk_id][p2].count(pos2) && !disk_vector[disk_id][p1].count(pos1))
        {
            disk_vector[disk_id][p2].erase(pos2);
            disk_vector[disk_id][p1].insert(pos1);
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
            if (object[id].replica[i] == disk_id) rep_id = i;
        }

        object[id].unit[rep_id][disk_uid[disk_id][pos1]] = pos2;

        int p1=calWhichPtr(pos1),p2=calWhichPtr(pos2);
        if (disk_vector[disk_id][p1].count(pos1))
        {
            disk_vector[disk_id][p1].erase(pos1);
            disk_vector[disk_id][p2].insert(pos2);
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
    for (int i = 1; i <= M; ++i) ord[i] = i;
    sort(ord + 1, ord + M + 1, [&](int x, int y) { return disk_size[disk_id][x] > disk_size[disk_id][y]; });
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
                    if (*it > R) break;
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
                    if (*it < L) break;
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
    for (int label_id = 1; label_id <= M; label_id++)
    {
        int start = ceil((long double)tag_weights[label_id - 1] * V / total_tag_weights);
        if (!start)
            start = 1;
        int end = (start == 1 ? V : start - 1);
        if (label_id & 1)
        {

            for (int i = start; i != end; i = i % V + 1)
            {
                if (!disk[disk_id][i] && !disk[disk_id][i - 1])
                    continue;
                if (!disk[disk_id][i])
                {
                    int toSwap = 0;
                    for (int j = end; j != i; j = (j == 1 ? V : j - 1))
                        if (disk[disk_id][j] == label_id)
                        {
                            toSwap = j;
                            break;
                        }
                    if (toSwap)
                    {
                        do_swap(disk_id, i, toSwap);
                        res.push_back({i, toSwap});
                        left_k--;
                    }

                    else
                        continue;
                }
            }
        }
        else
        {
            for (int i = end; i != start; i = (i == 1 ? V : i - 1))
            {
                if (!disk[disk_id][i] && !disk[disk_id][i - 1])
                    continue;
                if (!disk[disk_id][i])
                {
                    int toSwap = 0;
                    for (int j = i % V + 1; j != end; j = j % V + 1)
                        if (disk[disk_id][j] == label_id)
                        {
                            toSwap = j;
                            break;
                        }
                    if (toSwap)
                    {
                        do_swap(disk_id, i, toSwap);
                        res.push_back({i, toSwap});
                        left_k--;
                    }

                    else
                        continue;
                }
            }
        }
        if (!left_k)
            break;
    }

    return res;
}
vector<array<int, 2>> do_gc3(int disk_id) // 配合write_single_rep7的策略，把分配后空间内的空位尽量换回该标签的值
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
        if (!start)
            start = 1;
        int end = (start == 1 ? V : start - 1);
        if (label_id & 1)
        {

            for (int i = start; i != end; i = i % V + 1)
            {
                if (!disk[disk_id][i - 1])
                    continue;
                if (!disk[disk_id][i])
                {
                    int toSwap = 0;
                    for (int j = end; j != i; j = (j == 1 ? V : j - 1))
                        if (disk[disk_id][j] == label_id)
                        {
                            toSwap = j;
                            break;
                        }
                    if (toSwap)
                    {
                        do_swap(disk_id, i, toSwap);
                        res.push_back({i, toSwap});
                        left_k--;
                    }

                    else
                        break;
                }
                if (!left_k)
                    break;
            }
        }
        else
        {
            for (int i = end; i != start; i = (i == 1 ? V : i - 1))
            {
                if (!disk[disk_id][i - 1])
                    continue;
                if (!disk[disk_id][i])
                {
                    int toSwap = 0;
                    for (int j = i % V + 1; j != end; j = j % V + 1)
                        if (disk[disk_id][j] == label_id)
                        {
                            toSwap = j;
                            break;
                        }
                    if (toSwap)
                    {
                        do_swap(disk_id, i, toSwap);
                        res.push_back({i, toSwap});
                        left_k--;
                    }

                    else
                        break;
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
void adjust_disk(int disk_id)
{
    while (disk_vector[disk_id][0].size()<disk_vector[disk_id][1].size())
    {
        int x=*disk_vector[disk_id][1].begin();
        disk_vector[disk_id][1].erase(x);
        disk_vector[disk_id][0].insert(x);
    }
    while (disk_vector[disk_id][0].size()>disk_vector[disk_id][1].size())
    {
        int x=*disk_vector[disk_id][0].rbegin();
        disk_vector[disk_id][0].erase(x);
        disk_vector[disk_id][1].insert(x);
    }
    if (disk_vector[disk_id][1].size())
        divide_line[disk_id]=*disk_vector[disk_id][1].begin();
}
void gc_action()
{

    scanf("%*s %*s");
    printf("GARBAGE COLLECTION\n");
    for (int i = 1; i <= N; i++)
    {

        vector<array<int, 2>> gc_action;
        gc_action = do_gc1(i);
        adjust_disk(i);
        std::cout << gc_action.size() << "\n";
        for (auto [x, y] : gc_action)
        {
            std::cout << x << " " << y << "\n";
        }
    }
    fflush(stdout);
}
