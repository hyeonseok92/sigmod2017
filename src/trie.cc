#include "trie.h"
#include <iostream>
#include <assert.h>
#include <vector>
#include <unordered_set>
#include <jemalloc/jemalloc.h>

#define newTrieNode() ((TrieNode*) calloc(1, sizeof(TrieNode)))

#define JEMALLOC_POOL_SIZE 10000

void initTrie(TrieNode** node){
    *node = (TrieNode*) calloc(1, sizeof(TrieNode));
    (*node)->next.clear();
}
/*
void initJemalloc(){
    TrieNode *dummy;
    for (int i = 0; i < JEMALLOC_POOL_SIZE; i++){
        dummy = newTrieNode();
        free(dummy);
    }
}
*/

void destroyTrie(TrieNode* node){
    for (TrieMap::iterator it = node->next.begin(); it !=  node->next.end(); it++){
        destroyTrie(it->second);
    }
    free(node);
}

void addNgram(TrieNode* node, std::string const& ngram){
    TrieMap::iterator temp;
    TrieNode *newNode;
    for (std::string::const_iterator it = ngram.begin(); it != ngram.end(); it++){
        node->cnt++;
        temp = node->next.find(*it);
        if (temp == node->next.end()){
            newNode = newTrieNode();
            newNode->next.clear();
            node->next[*it] = newNode;
            node = newNode;
        }
        else
            node = temp->second;
    }
    node->cnt++;
    node->exist = true;
}

void delNgram(TrieNode *node, std::string const& ngram){
    TrieMap::iterator temp;
    TrieNode *next;
    std::string::const_iterator it;

    if (ngram.begin() == ngram.end()){
        node->cnt--;
        node->exist = false;
        return;
    }
    
    for (it = ngram.begin(); it != ngram.end(); it++){
        node->cnt--;
        temp = node->next.find(*it);
        assert(temp != node->next.end());
        next = temp->second;
        if (next->cnt == 1){
            node->next.erase(temp);
            node = next;
            break;
        }
        node = next;
    }
    if (it == ngram.end()){
        node->cnt--;
        node->exist = false;
        return;
    }
    for (++it; it != ngram.end(); it++){
        next = node->next.begin()->second;
        free(node);
        node = next;
    }
    free(node);
}

std::vector<std::string> queryNgram(TrieNode* node,const char *query){
    std::vector<std::string> res;
    std::string buf;
    TrieMap::iterator temp;
    for (const char* it = query; *it != 0; it++){
        if (node->exist && (*it == 0 || *it == ' ')){
            res.emplace_back(buf);
        }
        buf += *it;
        temp = node->next.find(*it);
        if (temp == node->next.end()){
            return res;
        }
        node = temp->second;
    }
    if (node->exist)
        res.emplace_back(buf);
    return res;
}
