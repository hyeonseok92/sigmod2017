#pragma once
#include <iostream>
//#include <map>
#include <unordered_map>
#include <vector>

typedef unsigned int mbyte_t;
//typedef unsigned short mbyte_t;
//typedef unsigned char mbyte_t;
#define MBYTE_SIZE sizeof(mbyte_t)

struct TrieNode;
//typedef std::map<mbyte_t, TrieNode*> TrieMap;
typedef std::unordered_map<mbyte_t, TrieNode*> TrieMap;
struct TrieNode{
    unsigned int ts;
    mbyte_t cache_ch;
    TrieNode *cache_next;
    TrieMap next;
};

struct cand_t{
    TrieNode *from;
    const char *start;
    int size;
};
//typedef std::pair<std::string, TrieNode*> cand_t;
#define MY_TS(tid) (((ts) << 6) | (NUM_THREAD-tid))

#define USE_CALLOC

#ifdef USE_CALLOC
#define newTrieNode(x) do{\
    (x) = (TrieNode*) calloc(1, sizeof(TrieNode));\
    (x)->next = TrieMap();\
}while(0)
#define freeTrieNode(x) free(x)
#else
#define newTrieNode(x) do{\
    (x) = new TrieNode;\
    (x)->cache_ch = 0;\
}while(0)
#define freeTrieNode(x) delete (x)
#endif

void initTrie(TrieNode** node);
void destroyTrie(TrieNode* node);

void addNgram(TrieNode* node, const char *ngram);
void delNgram(TrieNode* node, const char *ngram);
void queryNgram(std::vector<cand_t> *cands, unsigned int my_ts, TrieNode* trie, const char *query);
