#pragma once
#include <jemalloc/jemalloc.h>
#include <iostream>
#include <vector>
#include "config.h"

struct TrieNode{
    unsigned int ts;
    mbyte_t cache_ch;
    TrieNode *cache_next;
    TrieMap next;
};

struct cand_t{
    TrieNode *from;
    const char *start;
    int size;
};

#define MBYTE_SIZE sizeof(mbyte_t)

#ifdef USE_CALLOC
#define newTrieNode(x) do{\
    (x) = (TrieNode*) calloc(1, sizeof(TrieNode));\
    (x)->next = TrieMap();\
}while(0)
#define freeTrieNode(x) free(x)
#else
#define newTrieNode(x) do{\
    (x) = new TrieNode;\
    (x)->cache_ch = 0;\
}while(0)
#define freeTrieNode(x) delete (x)
#endif

#define TRY_SIGN(node) do{\
    node_ts = (node)->ts;\
    while (node_ts < my_ts){\
        if (__sync_bool_compare_and_swap(&(node)->ts, node_ts, my_ts)){\
            cand.from = (node);\
            cand.start = query;\
            cand.size = (it - query)+1;\
            cands->emplace_back(cand);\
            break;\
        }\
        node_ts = (node)->ts;\
    }\
}while(0)

#define NOT_NGRAM 0xFFFFFFFF

inline void addNgram(TrieNode* node, const char *ngram){
    TrieMap::iterator temp;
    TrieNode *newNode;
    for (const char *it = ngram; *it; ){
        mbyte_t key = 0;
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++)
            key += (((mbyte_t)*it) << (i*8));

        if (node->cache_ch == 0){
            newTrieNode(newNode);
            newNode->ts = NOT_NGRAM;
            node->cache_ch = key;
            node->cache_next = newNode;
            node = newNode;
            continue;
        }
        if (node->cache_ch == key){
            node = node->cache_next;
            continue;
        }
        temp = node->next.find(key);
        if (temp == node->next.end()){
            newTrieNode(newNode);
            newNode->ts = NOT_NGRAM;
            node->next[key] = newNode;
            node = newNode;
        }
        else
            node = temp->second;
    }
    node->ts = 0;
}

inline void delNgram(TrieNode *node, const char *ngram){
    TrieMap::iterator temp;
    TrieNode *next;
    const char *it;
    TrieNode *last_branch = node;
    TrieMap::iterator last_branch_next = node->next.find(*ngram);

    for (it = ngram; *it; ){
        if (node->cache_ch == 0)
            return;

        mbyte_t key = 0;
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++)
            key += ((mbyte_t)(*it) << (i*8));

        if (node->cache_ch == key){
            next = node->cache_next;
            if (node->ts != NOT_NGRAM || (node->next.size() && !next->next.size())){
                last_branch = node;
                last_branch_next = node->next.end();
            }
            node = next;
            continue;
        }

        temp = node->next.find(key);
        if (temp == node->next.end())
            return;
        next = temp->second;
        //If this is the end of a ngram or this node have more than 1 child node, and next have less than 2 child node
        if (node->ts != NOT_NGRAM || (node->next.size() && !next->next.size())){ 
            last_branch = node;
            last_branch_next = temp;
        }
        node = next;
    }

    if (node->cache_ch){ //If there is more than 1 child node
        node->ts = NOT_NGRAM;
        return;
    }
    if (last_branch_next == last_branch->next.end()){
        node = last_branch->cache_next;
        if (last_branch->next.size()){
            last_branch->cache_ch = last_branch->next.begin()->first;
            last_branch->cache_next = last_branch->next.begin()->second;
            last_branch->next.erase(last_branch->next.begin());
        }
        else
            last_branch->cache_ch = 0;
    }
    else{
        node = last_branch_next->second;
        last_branch->next.erase(last_branch_next);
    }

    while(node->cache_ch){ //If there is more than 1 child node
        next = node->cache_next;
        freeTrieNode(node);
        node = next;
    }
    freeTrieNode(node);
}

inline void queryNgram(std::vector<cand_t> *cands, unsigned int my_ts, TrieNode* node, const char *query){
    TrieMap::iterator temp;
    unsigned int node_ts;
    cand_t cand;
    const char *it = query;
    while(*it){
        if (node->cache_ch == 0)
            return;
        mbyte_t key = 0;
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++){
            key += ((mbyte_t)(*it) << (i*8));
            if (*(it+1) == ' '){
                if (node->cache_ch == 0)
                    continue;
                if (node->cache_ch == key){
                    TRY_SIGN(node->cache_next);
                    continue;
                }

                temp = node->next.find(key);
                if (temp != node->next.end())
                    TRY_SIGN(temp->second);
            }
        }
        if (node->cache_ch == key){
            node = node->cache_next;
            continue;
        }

        temp = node->next.find(key);
        if (temp == node->next.end())
            return;
        node = temp->second;
    }
    it--;
    TRY_SIGN(node);
}

void touchTrie(TrieNode* node){
    if (node->ts != NOT_NGRAM)
        node->ts = 0;
    if (node->cache_ch == 0)
        return;
    touchTrie(node->cache_next);
    for (TrieMap::iterator it = node->next.begin(); it !=  node->next.end(); it++){
        touchTrie(it->second);
    }
}
