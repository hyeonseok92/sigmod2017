#pragma once
#include <iostream>
#include <map>
//#include <unordered_map>
#include <vector>

struct TrieNode;
typedef std::map<char, TrieNode*> TrieMap;
//typedef std::unordered_map<char, TrieNode*> TrieMap;
struct TrieNode{
    unsigned int ts;
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
    (x)->cnt = 0;\
}while(0)
#define freeTrieNode(x) delete (x)
#endif

void initTrie(TrieNode** node);
void destroyTrie(TrieNode* node);

void addNgram(TrieNode* node, std::string const &ngram);
void delNgram(TrieNode* node, std::string const &ngram);
void queryNgram(std::vector<cand_t> *cands, unsigned int my_ts, TrieNode* trie, const char *query);
