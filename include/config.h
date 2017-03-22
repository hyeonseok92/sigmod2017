#pragma once
//#include <map>
#include <unordered_map>

typedef unsigned int mbyte_t;
//typedef unsigned short mbyte_t;
//typedef unsigned char mbyte_t;

struct TrieNode;

//typedef std::map<mbyte_t, TrieNode*> TrieMap;
typedef std::unordered_map<mbyte_t, TrieNode*> TrieMap;

//#define DBG_TS
//#define DBG_LOG

#define USE_CALLOC

#define MTASK_RESERVE 1000000
#define RES_RESERVE 1000000
#define MAX_BATCH_SIZE 2//10//10000000
#define BUF_RESERVE 10000
#define Q_ID_RESERVE 10000
#define NUM_THREAD 38
//#define NEXT_RESERVE 1

#define my_hash(x) (((mbyte_t)(x))%NUM_THREAD)
#define my_yield() __sync_synchronize()//pthread_yield()
