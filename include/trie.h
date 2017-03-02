#pragma once
#include <iostream>
#include <map>
//#include <unordered_map>
#include <vector>
#include <unordered_set>
#include "thread_struct.h"

struct TrieNode;
typedef std::map<char, TrieNode*> TrieMap;
//typedef std::unordered_map<char, TrieNode*> TrieMap;
struct TrieNode{
    std::vector<unsigned int> *hist;
    std::unordered_set<unsigned int> *ts;
    TrieMap next;
};

//#define USE_CALLOC

#ifdef USE_CALLOC
#define newTrieNode(x) do{\
    (x) = (TrieNode*) calloc(1, sizeof(TrieNode));\
    (x)->next.clear();\
}while(0)
#define freeTrieNode(x) free(x)

#define newHist(x) do{\
    (x) = (std::vector<unsigned int>*) calloc(1, sizeof(std::vector<unsigned int>));\
    (x)->clear();\
}while(0)
#define freeHist(x) free(x)

#define newTs(x) do{\
    (x) = (std::unordered_set<unsigned int>*) calloc(1, sizeof(std::unordered_set<unsigned int>));\
    (x)->clear();\
}while(0)
#define freeTs(x) free(x)
#else
#define newTrieNode(x) do{\
    (x) = new TrieNode;\
    (x)->hist = 0;\
    (x)->ts = 0;\
}while(0)
#define freeTrieNode(x) delete (x)

#define newHist(x) do{\
    (x) = new std::vector<unsigned int>;\
}while(0)
#define freeHist(x) delete (x)

#define newTs(x) do{\
    (x) = new std::unordered_set<unsigned int>;\
}while(0)
#define freeTs(x) delete (x)
#endif

void initTrie(TrieNode** node);
void destroyTrie(TrieNode* node);

void addNgram(TrieNode* node, unsigned int ts, std::string const &ngram);
void delNgram(TrieNode* node, unsigned int ts, std::string const &ngram);
void queryNgram(std::vector<cand_t> *cands, unsigned int ts, TrieNode* trie, const char *query);
