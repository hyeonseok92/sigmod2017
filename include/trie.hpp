#pragma once
#include <jemalloc/jemalloc.h>
#include <iostream>
#include <vector>
#include "config.h"

struct TrieNode{
    unsigned int last_query_id;
    mbyte_t cache_ch;
    TrieNode *cache_next;
    TrieMap next;
};

struct res_t{
    unsigned int task_id;
    unsigned int size;
    const char *start;
};

#define MBYTE_SIZE sizeof(mbyte_t)
#define NOT_NGRAM 0xFFFFFFFF

#ifdef USE_CALLOC
#define newTrieNode(x) do{\
    (x) = (TrieNode*) calloc(1, sizeof(TrieNode));\
    (x)->last_query_id = NOT_NGRAM;\
    (x)->next = TrieMap();\
}while(0)
#define freeTrieNode(x) free(x)
#else
#define newTrieNode(x) do{\
    (x) = new TrieNode;\
    (x)->last_query_id = NOT_NGRAM;\
    (x)->cache_ch = 0;\
}while(0)
#define freeTrieNode(x) delete (x)
#endif

#define TRY_SIGN(node, query_id, res, s, e, r) do{\
    if ((node)->last_query_id < query_id){\
        (node)->last_query_id = query_id;\
        r.start = s;\
        r.size = (e-s);\
        res->emplace_back(r);\
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
            node->next[key] = newNode;
            node = newNode;
        }
        else
            node = temp->second;
    }
    node->last_query_id = 0;
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
            next = node->cache_next;
            if (node->last_query_id != NOT_NGRAM || node->next.size()){
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
        if (node->last_query_id != NOT_NGRAM || node->next.size()){ 
            last_branch = node;
            last_branch_next = temp;
        }
        node = next;
    }

    if (node->cache_ch){ //If there is more than 1 child node
        node->last_query_id = NOT_NGRAM;
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

inline void queryNgram(std::vector<res_t> *res, unsigned int query_id, unsigned int task_id, TrieNode* node, const char *query){
    TrieMap::iterator temp;
    const char *it = query;
    res_t r;
    r.task_id = task_id;
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
                    TRY_SIGN(node->cache_next, query_id, res, query, it+1, r);
                    continue;
                }

                temp = node->next.find(key);
                if (temp != node->next.end())
                    TRY_SIGN(temp->second, query_id, res, query, it+1, r);
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
    TRY_SIGN(node, query_id, res, query, it, r);
}

void touchTrie(TrieNode* node){
    if (node->last_query_id != NOT_NGRAM)
        node->last_query_id = 0;
    if (node->cache_ch == 0)
        return;
    touchTrie(node->cache_next);
    for (TrieMap::iterator it = node->next.begin(); it !=  node->next.end(); it++){
        touchTrie(it->second);
    }
}
