#include "gc.h"

#include "storage.h"

// 改 Object dist dist_uid disk_vector
void do_swap(int disk_id, int pos1, int pos2)
{
    if (pos2 > 0) swap(pos1, pos2);
    if (pos2 > 0)
    {
        int id1 = disk[disk_id][pos1], id2 = disk[disk_id][pos2];
        int rep_id1, rep_id2;
        for (int i = 1; i <= REP_NUM; ++i)
        {
            if (object[id1].replica[i] == disk_id) rep_id1 = i;
            if (object[id2].replica[i] == disk_id) rep_id2 = i;
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
        for (int i = 1; i <= REP_NUM; ++i)
        {
            if (object[id].replica[i] == disk_id) rep_id = i;
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

void gc_action()
{
    scanf("%*s %*s");
    printf("GARBAGE COLLECTION\n");

    int left_k = K;
    vector<array<int, 2>> action[MAX_DISK_NUM];

    for (int i = 1; i <= N; i++)
    {
        printf("0\n");
    }
    fflush(stdout);
}