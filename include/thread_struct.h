#pragma once
#include <iostream>
#define MAX_BATCH_SIZE 1000

struct Operation{
    char cmd;
    std::string str;
};
struct ThrArg{
    int head;
    int tail;
    Operation operations[MAX_BATCH_SIZE];
    int tid;
};
