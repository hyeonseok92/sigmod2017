#pragma once
#include "config.h"

struct mtask_t{
    unsigned int task_id;
    unsigned int offset;
};

struct ThrArg{
    int head;
    int tail;
    const char *operations[MAX_BATCH_SIZE];
    int tid;
};
