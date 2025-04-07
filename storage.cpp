#include "storage.h"

// 全局变量定义
Request request[MAX_REQUEST_NUM];
Object object[MAX_OBJECT_NUM];

int T, M, N, V, G, K;

double A, B;

// double A=A_VALUE, B=B_VALUE; // 调参

int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
//**下标意义与disk[][]完全一样，完全可以与disk合并为array<int,2>数组，
//**其中存的值是第i块硬盘的第j个单元所存的块是这个块所属对象的第几个块
int disk_uid[MAX_DISK_NUM][MAX_DISK_SIZE];

set<int> disk_vector[20];  //**容器，存储每个硬盘的所有待读取单元
int disk_size[20][20];     //**存储磁盘的被占用单元数，因为写入策略是优先挑空闲空间大的磁盘 第一维是磁盘编号，第二维是标签编号 0表示现在占用数

int tag_weights[MAX_LABEL];
int total_tag_weights;
// 利用tag设置起点，根据奇偶指定方向

int timestamp;

int tag_num[MAX_LABEL];
int total_object_num;
