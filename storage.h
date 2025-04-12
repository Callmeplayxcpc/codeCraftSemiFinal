#ifndef STORAGE_H
#define STORAGE_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

// 常量定义
// #define MAX_DISK_NUM (10 + 1)
// #define MAX_DISK_SIZE (16384 + 1)
// #define MAX_REQUEST_NUM (30000000 + 1)
// #define MAX_OBJECT_NUM (100000 + 1)
// #define REP_NUM (3)
// #define FRE_PER_SLICING (1800)
// #define EXTRA_TIME (105)

constexpr int MAX_DISK_NUM = 11;
constexpr int MAX_DISK_SIZE = 16385;
constexpr int MAX_REQUEST_NUM = 30000001;
constexpr int MAX_OBJECT_NUM = 100001;
constexpr int REP_NUM = 3;
constexpr int MAX_SPLIT_NUM = 5;
constexpr int FRE_PER_SLICING = 1800;
constexpr int EXTRA_TIME = 105;
constexpr int MAX_LABEL = 20;

using namespace std;

// 数据结构定义
typedef struct Request_
{
    int object_id;
    int prev_id;  // 链式绑定相同对象的请求
    bool is_done;
    set<int> rest;  //**该请求对应对象尚未被读取的块，（值为1，2，...，object.size）

} Request;

typedef struct Object_
{
    int replica[REP_NUM + 1];                       // 第i个副本的磁盘编号
    int *unit[REP_NUM + 1];                         // 第i个副本的第j块存在哪个单元
    set<array<int, 2>> request[MAX_SPLIT_NUM + 1];  //**存储该对象的第i个块与哪些请求相关，存的值是request_id
    int size;                                       // 对象大小
    int tag;                                        // 标签
    int last_request_point;                         // 链式查询关于该对象的所有请求
    bool is_delete;

} Object;

// 全局变量声明
extern Request request[MAX_REQUEST_NUM];
extern Object object[MAX_OBJECT_NUM];

extern int T, M, N, V, G, K;

extern double A, B;  // 调参
extern double C, D;

extern int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
//**下标意义与disk[][]完全一样，完全可以与disk合并为array<int,2>数组，
//**其中存的值是第i块硬盘的第j个单元所存的块是这个块所属对象的第几个块
extern int disk_uid[MAX_DISK_NUM][MAX_DISK_SIZE];

extern set<int> disk_vector[MAX_DISK_NUM];  //**容器，存储每个硬盘的所有待读取单元
extern int disk_size[MAX_DISK_NUM][MAX_LABEL];     //**存储磁盘的被占用单元数，因为写入策略是优先挑空闲空间大的磁盘 第一维是磁盘编号，第二维是标签编号 0表示现在占用数
extern set<int> disk_empty[MAX_DISK_NUM]; //储存每个磁盘空位，用于垃圾回收
extern set<int> tag_pos[MAX_DISK_NUM][MAX_LABEL];  // 统计每个标签的在磁盘上的位置

extern int tag_weights[MAX_LABEL];
extern int total_tag_weights;
// 利用tag设置起点，根据奇偶指定方向

extern int timestamp;

extern int tag_num[MAX_LABEL];
extern int total_object_num;

extern int g[86600];

#endif  // STORAGE_H
