#include "delete.h"

#include <cstdio>
#include <cstdlib>
#include <set>
using namespace std;
void do_object_delete(const int *object_unit, int *disk_unit, int size)
{   //object_unit[i]表示某个对象在某个磁盘中，第i块的存在哪一个单元
    //disk_unit[i]表示某个磁盘的第i个单元存的对象序号值
    //size表示该对象块数量
    for (int i = 1; i <= size; i++)
    {
        disk_unit[object_unit[i]] = 0;
    }
}

void delete_action()
{
    int n_delete;                    // 当前时间片需要删除的对象数量
    int abort_num = 0;               // 累计被取消的读请求数量
    static int _id[MAX_OBJECT_NUM];  // 存储删除对象的编号

    // 读取删除对象的数量
    scanf("%d", &n_delete);
    for (int i = 1; i <= n_delete; i++)
    {
        scanf("%d", &_id[i]);
    }

    // 对于每个要删除的对象，遍历其相关的读请求链（通过 last_request_point 链表维护）
    for (int i = 1; i <= n_delete; i++)
    {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        // 遍历该对象对应的所有请求，统计未完成的请求
        while (current_id != 0)
        {
            if (!request[current_id].is_done)
            {
                abort_num++;
            }
            current_id = request[current_id].prev_id;
        }
    }

    // 输出被取消的请求数量
    printf("%d\n", abort_num);
    // 再次遍历，输出每个被取消的读请求编号，并清理对象对应硬盘中的数据
    for (int i = 1; i <= n_delete; i++)
    {
        int id = _id[i];
        int current_id = object[id].last_request_point;
        while (current_id != 0)
        {
            if (!request[current_id].is_done)
            {
                printf("%d\n", current_id);
            }
            current_id = request[current_id].prev_id;
        }
        // 对于该对象的每个副本，清除对应硬盘中的数据块
        for (int j = 1; j <= REP_NUM; j++)
        {
            do_object_delete(object[id].unit[j], disk[object[id].replica[j]], object[id].size);
            for (int k = 1; k <= object[id].size; k++)
            {  //**删除对象时候顺便把磁盘中相关的待读取单元都删了
                if (disk_vector[object[id].replica[j]].count(disk[object[id].replica[j]][k])) disk_vector[object[id].replica[j]].erase(disk[object[id].replica[j]][k]);
            }
            disk_size[object[id].replica[j]][0] -= object[id].size;  //**更新占用单元数
            disk_size[object[id].replica[j]][object[id].tag] -= object[id].size;
        }

        // 标记该对象已被删除
        object[id].is_delete = true;

        // 维护磁盘中每个标签对象总数
        tag_num[object[id].tag] -= object[id].size;
        for (int ltn = object[id].tag; ltn <= M; ltn++)  // ltn -> larger than and equal to now object.id
            tag_num[ltn] -= object[id].size;
    }
    fflush(stdout);
}