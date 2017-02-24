#include "trie.h"
#include <iostream>
#include <assert.h>
#include <vector>
#include <jemalloc/jemalloc.h>

void initTrie(TrieNode** node){
    *node = (TrieNode*) calloc(1, sizeof(TrieNode));
    (*node)->next.clear();
}

void destroyTrie(TrieNode* node){
    for (TrieMap::iterator it = node->next.begin(); it !=  node->next.end(); it++){
        destroyTrie(it->second);
    }
    free(node);
}

void addNgram(TrieNode* node, const char *ngram){
    TrieMap::iterator temp;
    TrieNode *newNode;
    for (const char *it = ngram; *it; ){
        mbyte_t key = 0;
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++)
            key += (((mbyte_t)*it) << (i*8));
        node->cnt++;
        temp = node->next.find(key);
        if (temp == node->next.end()){
            newNode = newTrieNode();
            newNode->ts = 0xFFFFFFFF;
            newNode->next.clear();
            node->next[key] = newNode;
            node = newNode;
        }
        else
            node = temp->second;
    }
    node->cnt++;
    node->ts = 0;
}

void delNgram(TrieNode *node, const char *ngram){
    TrieMap::iterator temp;
    TrieNode *next;
    const char *it;

    if (*ngram == 0){
        node->cnt--;
        node->ts = 0xFFFFFFFF;
        return;
    }
    
    for (it = ngram; *it; ){
        node->cnt--;
        mbyte_t key = 0;
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++)
            key += ((mbyte_t)(*it) << (i*8));
        temp = node->next.find(key);
        next = temp->second;
        if (next->cnt == 1){
            node->next.erase(temp);
            node = next;
            break;
        }
        node = next;
    }
    if (!(*it)){
        node->cnt--;
        node->ts = 0xFFFFFFFF;
        return;
    }
    for (++it; *it; ){
        mbyte_t key = 0;
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++)
            key += ((mbyte_t)(*it) << (i*8));
        next = node->next.begin()->second;
        free(node);
        node = next;
    }
    free(node);
}

void queryNgram(std::vector<cand_t> *cands, unsigned int my_ts, TrieNode* node, const char *query){
    std::string buf;
    TrieMap::iterator temp;
    unsigned int node_ts;
    for (const char* it = query; *it; ){
        mbyte_t key = 0;
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++){
            key += ((mbyte_t)(*it) << (i*8));
            buf += *it;
            if (*(it+1) == ' '){
                temp = node->next.find(key);
                if (temp != node->next.end()){
                    node_ts = temp->second->ts;
                    while (node_ts < my_ts){
                        if (__sync_bool_compare_and_swap(&temp->second->ts, node_ts, my_ts)){
                            cands->emplace_back(make_pair(buf, temp->second));
                            break;
                        }
                        node_ts = temp->second->ts;
                    }
                }
            }
        }
        temp = node->next.find(key);
        if (temp == node->next.end())
            return;
        node = temp->second;
    }
    node_ts = node->ts;
    while (node_ts < my_ts){
        if (__sync_bool_compare_and_swap(&node->ts, node_ts, my_ts)){
            cands->emplace_back(make_pair(buf, node));
            break;
        }
        node_ts = node->ts;
    }
}
