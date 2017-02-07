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

void initTrie(TrieNode** node);
void destroyTrie(TrieNode* node);

void addNgram(TrieNode* node, std::string const &ngram);
void delNgram(TrieNode* node, std::string const &ngram);
std::vector<std::string> queryNgram(TrieNode* trie, const char *query);
