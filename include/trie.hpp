#pragma once
#include <jemalloc/jemalloc.h>
#include <iostream>
#include <vector>
#include "config.h"

struct TrieNode{
    mbyte_t cache_ch;
    unsigned long long int cache_next;
    TrieMap next;
    unsigned int sign;
};

struct cand_t{
    unsigned int *from;
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
    (x)->cache_next = 0;\
}while(0)
#define freeTrieNode(x) delete (x)
#endif

#define BIT_NGRAM 0x8000000000000000ULL

#define IS_NGRAM(x) ((x)->cache_next & BIT_NGRAM)
#define SET_NGRAM(x) do{\
    (x)->cache_next |= BIT_NGRAM;\
    (x)->sign = 0;\
}while(0)
#define GET_CACHE_NEXT(x) ((TrieNode*)((x)->cache_next & ~BIT_NGRAM))
#define CLEAR_NGRAM(x) do{\
    (x)->cache_next &= ~BIT_NGRAM;\
}while(0)

#define TRY_SIGN(node, my_sign, cands, s, e) do{\
    TrieNode *target = (node);\
    if (IS_NGRAM(target) && target->sign < my_sign){\
        unsigned int node_sign = target->sign;\
        while (node_sign < my_sign){\
            if (__sync_bool_compare_and_swap(&target->sign, node_sign, my_sign)){\
                cand_t cand;\
                cand.from = &target->sign;\
                cand.start = s;\
                cand.size = (e-s)+1;\
                cands->emplace_back(cand);\
                break;\
            }\
            node_sign = target->sign;\
        }\
    }\
}while(0)

inline void addNgram(TrieNode* node, const char *it){
    TrieMap::iterator temp;
    TrieNode *newNode;
    while(*it){
        mbyte_t key = 0;
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++)
            key += (((mbyte_t)*it) << (i*8));

        if (node->cache_ch == 0){
            newTrieNode(newNode);
            node->cache_ch = key;
            node->cache_next = IS_NGRAM(node) | (unsigned long long int)newNode;
            node = newNode;
            continue;
        }
        if (node->cache_ch == key){
            node = GET_CACHE_NEXT(node);
            continue;
        }
        temp = node->next.find(key);
        if (temp == node->next.end()){
            newTrieNode(newNode);
            node->next[key] = newNode;
            node = newNode;
        }
        else
            node = temp->second;
    }
    SET_NGRAM(node);
}

inline void delNgram(TrieNode *node, const char *it){
    TrieMap::iterator temp;
    TrieNode *next;
    TrieNode *last_branch;
    TrieMap::iterator last_branch_next;
    mbyte_t key = 0;
    while(*it){
        if (node->cache_ch == 0)
            return;

        key = 0;
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++)
            key += ((mbyte_t)(*it) << (i*8));

        if (node->cache_ch == key){
            next = GET_CACHE_NEXT(node);
            if (IS_NGRAM(node) || (node->next.size() && !next->next.size())){
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
        if (IS_NGRAM(node) || (node->next.size() && !next->next.size())){ 
            last_branch = node;
            last_branch_next = temp;
        }
        node = next;
    }

    if (node->cache_ch){ //If there is more than 1 child node
        CLEAR_NGRAM(node);
        return;
    }
    if (last_branch_next == last_branch->next.end()){
        node = GET_CACHE_NEXT(last_branch);
        if (last_branch->next.size()){
            last_branch->cache_ch = last_branch->next.begin()->first;
            last_branch->cache_next = IS_NGRAM(last_branch) |
                    (unsigned long long int) last_branch->next.begin()->second;
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
        next = GET_CACHE_NEXT(node);
        freeTrieNode(node);
        node = next;
    }
    freeTrieNode(node);
}

inline void queryNgram(std::vector<cand_t> *cands, unsigned int my_sign, TrieNode* node, const char *query){
    TrieMap::iterator temp;
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
                    TRY_SIGN(GET_CACHE_NEXT(node), my_sign, cands, query, it);
                    continue;
                }

                temp = node->next.find(key);
                if (temp != node->next.end())
                    TRY_SIGN(temp->second, my_sign, cands, query, it);
            }
        }
        if (node->cache_ch == key){
            node = GET_CACHE_NEXT(node);
            continue;
        }

        temp = node->next.find(key);
        if (temp == node->next.end())
            return;
        node = temp->second;
    }
    it--;
    TRY_SIGN(node, my_sign, cands, query, it);
}

void touchTrie(TrieNode* node){
    if (IS_NGRAM(node))
        node->sign = 0;
    if (node->cache_ch == 0)
        return;
    touchTrie(GET_CACHE_NEXT(node));
    for (TrieMap::iterator it = node->next.begin(); it !=  node->next.end(); it++){
        touchTrie(it->second);
    }
}
