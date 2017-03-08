#pragma once
#include <iostream>
#include <map>
//#include <unordered_map>
#include <vector>

#define CACHE_CH(p)     ((char)(p>>56))
#define CACHE_NEXT(p)   ((TrieNode*)(p&0x0000FFFFFFFFFFFF))
#define IS_ALPHA(c)     ((c>>5)==0x03)
#define FLAG_ALPHA(c)   (1<<(c&0x1F))
#define UNFLAG_ALPHA(c) (0xFF^(1<<(c&0x1F)))

struct TrieNode;
typedef std::map<char, TrieNode*> TrieMap;
//typedef std::unordered_map<char, TrieNode*> TrieMap;
struct TrieNode{
    unsigned int ts;
//    char cache_ch;
//    TrieNode *cache_next;
    unsigned int bitmap;
    unsigned long long cache;
    TrieMap next;
};

typedef std::pair<std::string, TrieNode*> cand_t;
#define MY_TS(tid) (((ts) << 6) | (NUM_THREAD-tid))

#define USE_CALLOC

#ifdef USE_CALLOC
#define newTrieNode(x) do{\
    (x) = (TrieNode*) calloc(1, sizeof(TrieNode));\
    (x)->next.clear();\
}while(0)
#define freeTrieNode(x) free(x)
#else
#define newTrieNode(x) do{\
    (x) = new TrieNode;\
    (x)->cache_ch = 0;
}while(0)
#define freeTrieNode(x) delete (x)
#endif

void initTrie(TrieNode** node);
void destroyTrie(TrieNode* node);

void addNgram(TrieNode* node, std::string const &ngram);
void delNgram(TrieNode* node, std::string const &ngram);
void queryNgram(std::vector<cand_t> *cands, unsigned int my_ts, TrieNode* trie, const char *query);
