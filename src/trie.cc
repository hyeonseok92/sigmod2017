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
    TrieNode *next;
    std::string::const_iterator it;
    TrieNode *last_branch = node;
    TrieMap::iterator last_branch_next = node->next.find(*ngram.begin());

    for (it = ngram.begin(); it != ngram.end(); it++){
        if (node->cache_ch == 0)
            return;
        if (node->cache_ch == *it){
            next = node->cache_next;
            if (node->ts != 0xFFFFFFFF || (node->next.size() && !next->next.size())){
                last_branch = node;
                last_branch_next = node->next.end();
            }
            node = next;
            continue;
        }

        temp = node->next.find(*it);
        if (temp == node->next.end())
            return;
        next = temp->second;
        //If this is the end of a ngram or this node have more than 1 child node, and next have less than 2 child node
        if (node->ts != 0xFFFFFFFF || (node->next.size() && !next->next.size())){ 
            last_branch = node;
            last_branch_next = temp;
        }
        node = next;
    }
    if (node->cache_ch){ //If there is more than 1 child node
        node->ts = 0xFFFFFFFF;
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
