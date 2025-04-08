#include "clean.h"
#include <bits/stdc++.h>
using namespace std;
void clean()
{
    for (auto &obj : object)
    {
        for (int i = 1; i <= REP_NUM; i++)
        {
            if (obj.unit[i] == nullptr) continue;
            free(obj.unit[i]);
            obj.unit[i] = nullptr;
        }
    }
}