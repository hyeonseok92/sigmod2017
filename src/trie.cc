#include "trie.h"
#include <iostream>
#include <assert.h>
#include <vector>
#include <jemalloc/jemalloc.h>

void initTrie(TrieNode** node){
    newTrieNode(*node);
}

void destroyTrie(TrieNode* node){
    for (TrieMap::iterator it = node->next.begin(); it !=  node->next.end(); it++){
        destroyTrie(it->second);
    }
    freeTrieNode(node);
}

void addNgram(TrieNode* node, std::string const& ngram){
    TrieMap::iterator temp;
    TrieNode *newNode;
    for (std::string::const_iterator it = ngram.begin(); it != ngram.end(); it++){
        assert(node != NULL);
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
        temp = node->next.find(*it);
        if (temp == node->next.end())
            return;
        next = temp->second;
        if (node->ts != 0xFFFFFFFF || (node->next.size() > 1 && next->next.size() <= 1)){
            last_branch = node;
            last_branch_next = temp;
        }
        node = next;
    }
    if (node->next.size()){
        node->ts = 0xFFFFFFFF;
        return;
    }
    node = last_branch_next->second;
    while(node->next.size()){
        next = node->next.begin()->second;
        freeTrieNode(node);
        node = next;
    }
    freeTrieNode(node);
    last_branch->next.erase(last_branch_next);
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
