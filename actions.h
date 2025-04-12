#ifndef ACTIONS_H
#define ACTIONS_H

#include "storage.h"

// 各操作函数的声明
void timestamp_action();
void delete_action();
void write_action();
void read_action();
void clean();
void gc_action();
void divide_action();

extern int total_tag_weights;
extern int tag_weights[MAX_LABEL];

#endif  // ACTIONS_H
