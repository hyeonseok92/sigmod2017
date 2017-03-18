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

#define MAX_TS 0x3FFFFFF //because my_sign use 6 shifted ts
#define MAX_BATCH_SIZE 1000000
#define NUM_THREAD 37
//#define NEXT_RESERVE 1
#define RES_RESERVE 128
#define BUF_RESERVE 1024*1024
#define NUM_BUF_RESERVE 10000

#define my_hash(x) (((mbyte_t)(x))%NUM_THREAD)
#define my_sign(ts, tid) (((ts) << 6) | (NUM_THREAD-tid))
#define my_yield() __sync_synchronize()//pthread_yield()
