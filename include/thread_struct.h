#pragma once
#include <iostream>
#define MAX_BATCH_SIZE 1000000
#define MAX_QUERY_SIZE 1000000
#define NUM_THREAD 1

struct ModifyOp{
    char cmd;
    unsigned int ts;
    std::string str;
}; 
typedef std::pair<const char*, int>cand_t;
struct QueryInfo{
    unsigned int ts;
    std::string query;
    std::vector<cand_t> res[NUM_THREAD];
    unsigned int finished;
};

struct QueryOp{
    unsigned int ts;
    std::vector<cand_t> *res;
    //std::vector<const char*> start_points;
    const char* start_point;
    unsigned int *is_end;
};

struct ThrArg{
    int modify_tail;
    ModifyOp modify_ops[MAX_BATCH_SIZE];
    int query_tail;
    QueryOp query_ops[MAX_QUERY_SIZE];
};
