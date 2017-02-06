#pragma once
#include <iostream>
#include <vector>

#define SIZE_ALPHABET 38
#define MAX_HISTORY 100
#define mapping(x) (((x) >= 'a' && (x) <= 'z') ? ((x)-'a') : ((x) == '.' ? 36 : ((x) == ' ' ? 37 : ((x)-'0'+26)))) 

struct TrieNode{
    TrieNode *next[SIZE_ALPHABET];
    int num_add;
    int add_hist[MAX_HISTORY];
    int num_del;
    int del_hist[MAX_HISTORY];
};

struct Trie{
    TrieNode node;
    int cnt;
};

void initTrie(Trie** trie);
void destroyTrie(Trie** trie);

void addNgram(Trie* trie, int ts, char *ngram);
void delNgram(Trie* trie, int ts, char *ngram);

std::vector<std::string> queryNgram(Trie* trie, int ts, char *query);

