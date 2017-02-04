#include "trie_word.h"
#include <iostream>
#define NULL 0

#define newTrieNode() ((TrieNode*) calloc(1, sizeof(TrieNode)))

void initTrie(Trie* trie){
    trie = (Trie*) calloc(1, sizeof(Trie));
}

void destroyTrieNode(TrieNode* node){
    int i;
    for (i = 0; i < SIZE_ALPHABET; i++){
        if (node.next[i])
            destroyTrie(node.next[i]);
    }
    free(node);
}

void destroyTrie(Trie* trie){
    int i;
    for (i = 0; i < SIZE_ALPHABET; i++){
        if (trie->node.next[i])
            destroyTrieNode(trie->node.next[i]);
    }
    free(trie);
}

void insertTrieWord(TrieWord* trie, std::string ngram){
    int m;
    TrieWordNode* node = &trie->node;
    for (char& c : ngram){
        assert(c != 0);
        m = mapping(ngram);
        if (node.next[m] == NULL){
            node.next[m] = newTrieNode();
        }
        node = node.next[m];
    }
    node.matched = __sync_add_and_fetch(&trie->cnt, 1);
}
