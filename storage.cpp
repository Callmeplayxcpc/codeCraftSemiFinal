#include "storage.h"

// 全局变量定义
Request request[MAX_REQUEST_NUM];
Object object[MAX_OBJECT_NUM];

int T;  // 时间片数量（未加105）
int M;  // 对象标签种类数量
int N;  // 硬盘个数
int V;  // 硬盘单元数
int G;  // 时间片内令牌数
int K;  // 每个硬盘最多交换单元次数

double A, B;
int C;

// double A=A_VALUE, B=B_VALUE; // 调参

int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
//**下标意义与disk[][]完全一样，完全可以与disk合并为array<int,2>数组，
//**其中存的值是第i块硬盘的第j个单元所存的块是这个块所属对象的第几个块
int disk_uid[MAX_DISK_NUM][MAX_DISK_SIZE];

set<int> disk_vector[MAX_DISK_NUM];  //**容器，存储每个硬盘的所有待读取单元
int disk_size[MAX_DISK_NUM][MAX_LABEL];     //**存储磁盘的被占用单元数，因为写入策略是优先挑空闲空间大的磁盘 第一维是磁盘编号，第二维是标签编号 0表示现在占用数
set<int> disk_empty[MAX_DISK_NUM]; //储存每个磁盘空位，用于垃圾回收
set<int> tag_pos[MAX_DISK_NUM][MAX_LABEL]; // 统计每个标签的在磁盘上的位置

int tag_weights[MAX_LABEL];  // 在write_single_rep2，6，7中使用，代表不同标签数据的权重
int total_tag_weights;       // tag_weights之和
// 利用tag设置起点，根据奇偶指定方向

int timestamp;  // 当前交互阶段处于哪一个时间片

int tag_num[MAX_LABEL];  // 在write_single_rep4中使用，代表不同标签在磁盘存储的总块数
int total_object_num;    // tag_num之和
