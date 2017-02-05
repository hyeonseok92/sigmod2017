#pragma once
#include <iostream>

#define MAX_WORD 10000
#define SIZE_ALPHABET 37
#define mapping(x) (((x) >= 'a' && (x) <= 'z') ? ((x)-'a') : ((x) == '.' ? 36 : ((x)-'0'+26))) 

struct TrieWordNode{
    int matched;
    TrieWordNode *next[SIZE_ALPHABET];
};

struct TrieWord{
    TrieWordNode node;
    int cnt;
    std::string words[MAX_WORD+1];
};

void initTrieWord(TrieWord** trie);

void destroyTrieWord(TrieWord** trie);

int insertTrieWord(TrieWord* trie, std::string ngram);

int searchTrieWord(TrieWord* trie, std::string word);
