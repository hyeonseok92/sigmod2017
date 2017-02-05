#pragma once
#include <iostream>
#include <vector>
#include "trie_word.h"
#define MAX_NGRAM 1000000

struct TrieNode{
    int matched;
    TrieNode *next[MAX_WORD+1];
};

struct Ngram{
    std::vector<int> ngram;
    std::vector<int> history;
};

struct Trie{
    TrieNode node;
    int cnt;
    Ngram ngrams[MAX_NGRAM];
};

void initTrie(Trie** trie);

void destroyTrie(Trie** trie);

void insertTrie(Trie* trie, std::vector<int> ngram);

std::vector<int> searchTrie(Trie* trie, std::vector<int> query);
