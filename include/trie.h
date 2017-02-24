#pragma once
#include <iostream>
#include <map>
#include <vector>

typedef unsigned int mbyte_t;
#define MBYTE_SIZE sizeof(mbyte_t)

struct TrieNode;
typedef std::map<mbyte_t, TrieNode*> TrieMap;
struct TrieNode{
    unsigned int ts;
    int cnt;
    TrieMap next;
};

typedef std::pair<std::string, TrieNode*> cand_t;
#define MY_TS(tid) (((ts) << 6) | (NUM_THREAD-tid))
#define newTrieNode() ((TrieNode*) calloc(1, sizeof(TrieNode)))

void initTrie(TrieNode** node);
void destroyTrie(TrieNode* node);

void addNgram(TrieNode* node, const char *ngram);
void delNgram(TrieNode* node, const char *ngram);
void queryNgram(std::vector<cand_t> *cands, unsigned int my_ts, TrieNode* trie, const char *query);
