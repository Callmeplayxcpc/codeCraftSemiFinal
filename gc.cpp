#include "gc.h"

#include "storage.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>

// 改 Object disk disk_uid disk_vector tag_pos
// disk_empty 在外面改
void do_swap(int disk_id, int pos1, int pos2)
{
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

// 交换让磁盘中相同tag的块更加紧凑，让右端点不断向左靠，或者让左端点不断向右靠
// 现在只允许交换到空位上
vector<array<int,2>> do_gc1(int disk_id){
    int ord[MAX_LABEL];
    for(int i=1;i<=M;++i)ord[i]=i;
    sort(ord+1,ord+M+1,[&](int x,int y){return disk_size[disk_id][x]>disk_size[disk_id][y];});
    vector<array<int,2>> res;
    int left_k = K;
    for(int i=1;i<=M;++i){
        int tag = ord[i];
        if(disk_size[disk_id][ord[i]] == 0){
            break;
        }
        if(tag&1){
            int L=*tag_pos[disk_id][tag].begin();
            for(auto it=disk_empty[disk_id].begin();it!=disk_empty[disk_id].end() && left_k;){
                int R=*tag_pos[disk_id][tag].rbegin();
                if(*it > L && *it < R){
                    do_swap(disk_id, *it, R);
                    res.push_back({*it, R});
                    auto nex=disk_empty[disk_id].erase(it);
                    int nexp=0;
                    if(nex==disk_empty[disk_id].end())nexp=R;
                    else nexp=*nex;
                    disk_empty[disk_id].insert(R);
                    it=disk_empty[disk_id].find(nexp);
                    left_k--;
                }
                else{
                    if(*it > R) break;
                    it++;
                }
            }
        }
        else{
            int R=*tag_pos[disk_id][tag].rbegin();
            for(auto it=disk_empty[disk_id].rbegin();it!=disk_empty[disk_id].rend() && left_k;){
                int L=*tag_pos[disk_id][tag].begin();
                if(*it > L && *it < R){
                    do_swap(disk_id, *it, L);
                    res.push_back({*it, L});
                    auto forward_it = it.base();
                    --forward_it; 
                    auto nex = disk_empty[disk_id].erase(forward_it);
                    int nexp=0;
                    if(nex==disk_empty[disk_id].begin())nexp=L;
                    else nexp=*(nex--);
                    disk_empty[disk_id].insert(L); 
                    it=set<int>::reverse_iterator(disk_empty[disk_id].find(nexp));
                    left_k--;
                }
                else{
                    if(*it < L) break;
                    it++;
                }
            }
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
        auto gc_action=do_gc1(i);
        std::cout<<gc_action.size()<<"\n";
        for(auto [x,y]:gc_action){
            std::cout<<x<<" "<<y<<"\n";
        }
    }
    fflush(stdout);
}

