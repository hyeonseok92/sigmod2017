#include "trie.h"
#include <iostream>
#include <assert.h>
#include <vector>
#include <jemalloc/jemalloc.h>

void initTrie(TrieNode** node){
    newTrieNode(*node);
}

void destroyTrieNode(TrieNode* node){
    for (TrieMap::iterator it = node->next.begin(); it !=  node->next.end(); it++){
        destroyTrie(it->second);
    }
    freeTrieNode(node);
}

void destroyTrie(TrieNode* node){
    for (TrieMap::iterator it = node->next.begin(); it !=  node->next.end(); it++){
        destroyTrieNode(it->second);
    }
}

void addNgram(TrieNode* node, std::string const& ngram){
    TrieMap::iterator temp;
    TrieNode *newNode;
    for (std::string::const_iterator it = ngram.begin(); it != ngram.end(); it++){
        assert(node != NULL);
        if (node->cache_ch == 0){
            newTrieNode(newNode);
            newNode->ts = 0xFFFFFFFF;
            node->cache_ch = *it;
            node->cache_next = newNode;
            node = newNode;
            continue;
        }
        if (node->cache_ch == *it){
            node = node->cache_next;
            continue;
        }
        temp = node->next.find(*it);
        if (temp == node->next.end()){
            newTrieNode(newNode);
            newNode->ts = 0xFFFFFFFF;
            node->next[*it] = newNode;
            node = newNode;
        }
        else
            node = temp->second;
    }
    node->ts = 1;
}

void delNgram(TrieNode *node, std::string const& ngram){
    TrieMap::iterator temp;
    std::string::const_iterator it;

    for (it = ngram.begin(); it != ngram.end(); it++){
        if (node->cache_ch == 0)
            return;
        if (node->cache_ch == *it){
            node = node->cache_next;
            continue;
        }

        temp = node->next.find(*it);
        if (temp == node->next.end())
            return;
        node = temp->second;
    }
    node->ts = 0xFFFFFFFF;
}

void queryNgram(std::vector<cand_t> *cands, unsigned int my_ts, TrieNode* node, const char *query){
    std::string buf;
    TrieMap::iterator temp;
    unsigned int node_ts;
    for (const char* it = query; *it != 0; it++){
        node_ts = node->ts;
        while (node_ts < my_ts && *it == ' '){
            if (__sync_bool_compare_and_swap(&node->ts, node_ts, my_ts)){
                cands->emplace_back(make_pair(buf, node));
                break;
            }
            node_ts = node->ts;
        }
        buf += *it;
        if (node->cache_ch == 0)
            return;
        if (node->cache_ch == *it){
            node = node->cache_next;
            continue;
        }

        temp = node->next.find(*it);
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
    return;
}
