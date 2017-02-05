#include "trie_ngram.h"
#include <iostream>
#include <assert.h>

#define newTrieNode() ((TrieNode*) calloc(1, sizeof(TrieNode)))

void initTrie(Trie** trie){
    *trie = (Trie*) calloc(1, sizeof(Trie));
}

void destroyTrieNode(TrieNode* node){
    int i;
    for (i = 0; i < MAX_WORD; i++){
        if (node->next[i])
            destroyTrieNode(node->next[i]);
    }
    free(node);
}

void destroyTrie(Trie** trie){
    int i;
    for (i = 0; i < MAX_WORD; i++){
        if ((*trie)->node.next[i])
            destroyTrieNode((*trie)->node.next[i]);
    }
    free(*trie);
    *trie = NULL;
}

void insertTrie(Trie* trie, std::vector<int> ngram){
    TrieNode* node = &trie->node;
    TrieNode* newNode = NULL;
    for (int& w : ngram){
        assert(w != 0);
        if (node->next[w] == NULL){
            if (!newNode)
                newNode = newTrieNode();
            if (__sync_bool_compare_and_swap(&node->next[w], NULL, newNode)){
                newNode = NULL;
            }
        }
        node = node->next[w];
    }
    if (newNode)
        free(newNode);
    if (!node->matched){
        if (__sync_bool_compare_and_swap(&node->matched, 0, 1)){
            node->matched = __sync_add_and_fetch(&trie->cnt, 1);
            trie->ngrams[node->matched-1].ngram = ngram; 
        }
    }
}

std::vector<int> searchTrie(Trie* trie, std::vector<int> query){
    std::vector<int> res;
    TrieNode* node = &trie->node;
    for (int& w : query){
        assert(w != 0);
        if (node->next[w] == NULL){
            break;
        }
        if (node->matched){
            res.push_back(node->matched);
        }
        node = node->next[w];
    }
    if (node->matched){
        res.push_back(node->matched);
    }
    return res;
}
