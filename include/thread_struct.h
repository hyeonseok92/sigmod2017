#pragma once
#include <iostream>
#define MAX_BATCH_SIZE 100000

struct Operation{
    char cmd;
    const char *str;
};
struct ThrArg{
    int head;
    int tail;
    Operation operations[MAX_BATCH_SIZE];
    int tid;
};
