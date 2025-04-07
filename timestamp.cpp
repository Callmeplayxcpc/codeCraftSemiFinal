#include "timestamp.h"

#include <iostream>
using namespace std;
void timestamp_action()
{  // 仅输入输出，不用优化
    scanf("%*s%d", &timestamp);
    printf("TIMESTAMP %d\n", timestamp);
    fflush(stdout);
}