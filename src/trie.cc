#include "trie.h"
#include <iostream>
#include <assert.h>
#include <vector>
#include <unordered_set>

#define newTrieNode() ((TrieNode*) calloc(1, sizeof(TrieNode)))

void initTrie(Trie** trie){
    *trie = (Trie*) calloc(1, sizeof(Trie));
}

void destroyTrieNode(TrieNode* node){
    for (int i = 0; i < SIZE_ALPHABET; i++){
        if (node->next[i])
            destroyTrieNode(node->next[i]);
    }
    free(node);
}

void destroyTrie(Trie** trie){
    for (int i = 0; i < SIZE_ALPHABET; i++){
        if ((*trie)->node.next[i])
            destroyTrieNode((*trie)->node.next[i]);
    }
    free(*trie);
    *trie = NULL;
}

void addNgram(Trie* trie, int ts, char *ngram){
    int m;
    TrieNode* node = &trie->node;
    TrieNode* newNode = NULL;
    for (char *c = ngram; *c != 0; c++){
        m = mapping(*c);
        if (node->next[m] == NULL){
            if (!newNode)
                newNode = newTrieNode();
            if (__sync_bool_compare_and_swap(&node->next[m], NULL, newNode)){
                newNode = NULL;
            }
        }
        node = node->next[m];
    }
    if (newNode)
        free(newNode);

    node->add_hist[__sync_fetch_and_add(&node->num_add, 1)] = ts;
}

void delNgram(Trie* trie, int ts, char* ngram){
    int m;
    TrieNode* node = &trie->node;
    TrieNode* newNode = NULL;
    for (char *c = ngram; *c != 0; c++){
        m = mapping(*c);
        if (node->next[m] == NULL){
            if (!newNode)
                newNode = newTrieNode();
            if (__sync_bool_compare_and_swap(&node->next[m], NULL, newNode)){
                newNode = NULL;
            }
        }
        node = node->next[m];
    }
    if (newNode)
        free(newNode);

    node->del_hist[__sync_fetch_and_add(&node->num_del, 1)] = ts;
}

std::vector<std::string> queryNgramLow(Trie* trie, int ts, char *subquery){
    int m;
    std::vector<std::string> res;
    std::string buf;
    TrieNode* node = &trie->node;
    for (char *c = subquery; ; c++){
        if ((*c == ' ' || *c == 0) && node->num_add > 0){
            int last_add = 0;
            for (int i = 0; i < node->num_add; i++){
                if (node->add_hist[i] < ts && node->add_hist[i] > last_add){
                    last_add = node->add_hist[i];
                }
            }
            if (ts == 1002 && buf == std::string("components")){
                ts = ts;
            }
            for (int i = 0; i < node->num_del; i++){
                if (node->del_hist[i] < ts && node->del_hist[i] > last_add){
                    last_add = -1;
                    break;
                }
            }
            if (last_add >= 0){
                res.emplace_back(buf);
            }
        }
        if (*c == 0) break;
        buf += *c;
        m = mapping(*c);
        if (node->next[m] == NULL){
            return res;
        }
        node = node->next[m];
    }
    return res;
}

std::vector<std::string> queryNgram(Trie* trie, int ts, char *query){
    std::vector<std::string> tmp;
    std::vector<std::string> res;
    std::unordered_set<std::string> dup_chk;
    while(*query != 0){
        tmp = queryNgramLow(trie, ts, query);
        for (std::string& s : tmp){
            if (dup_chk.find(s) == dup_chk.end()){
                dup_chk.insert(s);
                res.emplace_back(s);
            }
        }
        while(*query != ' ' && *query != 0)
            query++;
        query++;
    }
    return res;
}
