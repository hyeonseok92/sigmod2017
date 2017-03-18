#pragma once
#include "config.h"

struct ThrArg{
    int head;
    int tail;
    const char *operations[MAX_BATCH_SIZE];
    int tid;
};
