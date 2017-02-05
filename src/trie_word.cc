#include "trie_word.h"
#include <iostream>
#include <assert.h>

#define newTrieWordNode() ((TrieWordNode*) calloc(1, sizeof(TrieWordNode)))

void initTrieWord(TrieWord** trie){
    *trie = (TrieWord*) calloc(1, sizeof(TrieWord));
}

void destroyTrieWordNode(TrieWordNode* node){
    int i;
    for (i = 0; i < SIZE_ALPHABET; i++){
        if (node->next[i])
            destroyTrieWordNode(node->next[i]);
    }
    free(node);
}

void destroyTrieWord(TrieWord** trie){
    int i;
    for (i = 0; i < SIZE_ALPHABET; i++){
        if ((*trie)->node.next[i])
            destroyTrieWordNode((*trie)->node.next[i]);
    }
    free(*trie);
    *trie = NULL;
}

int insertTrieWord(TrieWord* trie, std::string word){
    int m;
    TrieWordNode* node = &trie->node;
    TrieWordNode* newNode = NULL;
    for (char& c : word){
        assert(c != 0);
        m = mapping(c);
        if (node->next[m] == NULL){
            if (!newNode)
                newNode = newTrieWordNode();
            if (__sync_bool_compare_and_swap(&node->next[m], NULL, newNode)){
                newNode = NULL;
            }
        }
        node = node->next[m];
    }
    if (newNode)
        free(newNode);
    if (!node->matched){
        if (__sync_bool_compare_and_swap(&node->matched, 0, 1)){
            node->matched = __sync_add_and_fetch(&trie->cnt, 1);
            trie->words[node->matched] = word;
        }
    }
    return node->matched;
}

int searchTrieWord(TrieWord* trie, std::string word){
    int m;
    TrieWordNode* node = &trie->node;
    for (char& c : word){
        assert(c != 0);
        m = mapping(c);
        if (node->next[m] == NULL){
            return 0;
        }
        node = node->next[m];
    }
    return node->matched;
}
