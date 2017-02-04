#include "trie.h"
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

void insertTrie(Trie* trie, std::string ngram){
    int m;
    TrieNode* node = &trie->node;
    TrieNode* newNode = NULL;
    int sid = __sync_fetch_and_add(&trie->num_ngram, 1);
    trie->ngrams[sid].ngram = ngram;
    for (char& c : ngram){
        m = mapping(ngram);
        if (node.next[m] == NULL){
            if (!newNode)
                newNode = newTrieNode();
            if (__sync_bool_compare_and_swap(&node.next[m], NULL, newNode)){
                newNode = NULL;
            }
        }
        node = node.next[m];
    }
    if (newNode)
        free(newNode);
    if (!node.matched)
        node.matched = sid;
}
