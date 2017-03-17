#pragma once
#include <iostream>
#define MAX_BATCH_SIZE 1000000

struct ThrArg{
    int head;
    int tail;
    const char *operations[MAX_BATCH_SIZE];
    int tid;
};
