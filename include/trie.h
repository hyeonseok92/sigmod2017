#pragma once
#include <iostream>
#include <map>
#include <vector>

struct TrieNode;
typedef std::map<char, TrieNode*> TrieMap;
struct TrieNode{
    bool exist;
    int cnt;
    TrieMap next;
};

#define newTrieNode() ((TrieNode*) calloc(1, sizeof(TrieNode)))

void initTrie(TrieNode** node);
void destroyTrie(TrieNode* node);
//void initJemalloc();

void addNgram(TrieNode* node, std::string const &ngram);
void delNgram(TrieNode* node, std::string const &ngram);
std::vector<std::string> queryNgram(TrieNode* trie, const char *query);
