#include "storage.h"

// 全局变量定义
Request request[MAX_REQUEST_NUM];
Object object[MAX_OBJECT_NUM];

int T, M, N, V, G, K;

// #ifndef A_VALUE
// #define A_VALUE 0.19
// #endif

// #ifndef B_VALUE
// #define B_VALUE 0
// #endif

double A, B;

// double A=A_VALUE, B=B_VALUE; // 调参

int disk[MAX_DISK_NUM][MAX_DISK_SIZE];
//**下标意义与disk[][]完全一样，完全可以与disk合并为array<int,2>数组，
//**其中存的值是第i块硬盘的第j个单元所存的块是这个块所属对象的第几个块
int disk_uid[MAX_DISK_NUM][MAX_DISK_SIZE];
