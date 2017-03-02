#include "trie.h"
#include <iostream>
#include <assert.h>
#include <vector>
#include <jemalloc/jemalloc.h>

void initTrie(TrieNode** node){
    newTrieNode(*node);
}

void destroyTrie(TrieNode* node){
    for (TrieMap::iterator it = node->next.begin(); it !=  node->next.end(); it++)
        destroyTrie(it->second);
    if (node->hist)
        freeHist(node->hist);
    if (node->ts)
        freeTs(node->ts);
    freeTrieNode(node);
}

void addNgram(TrieNode* node, unsigned int ts, std::string const& ngram){
    TrieMap::iterator temp;
    TrieNode *newNode;
    for (std::string::const_iterator it = ngram.begin(); it != ngram.end(); it++){
        temp = node->next.find(*it);
        if (temp == node->next.end()){
            newTrieNode(newNode);
            node->next[*it] = newNode;
            node = newNode;
        }
        else
            node = temp->second;
    }
    if (node->hist == NULL)
        newHist(node->hist);
    if (node->hist->size()%2 == 0)  // only insert ngram when it have not existed
        node->hist->emplace_back(ts);
}

void delNgram(TrieNode *trie, unsigned int ts, std::string const& ngram){
    TrieMap::iterator temp;
    TrieNode *node = trie;
    std::string::const_iterator it;
    for (it = ngram.begin(); it != ngram.end(); it++){
        temp = node->next.find(*it);
        if (temp == node->next.end()) // Delete ngram which is already deleted
            return;
        node = temp->second;
    }
    if (it == ngram.end()){
        if (node->hist && node->hist->size()%2 == 1)  // only insert ngram when it have not existed
            node->hist->emplace_back(ts);
        return;
    }
}

void queryNgram(std::vector<cand_t> *cands, unsigned int ts, TrieNode* node, const char *query){
    TrieMap::iterator temp;
    int l, r, m, last_hist;
    for (const char *it = query; ; it++){
        if ((*it == ' ' || *it == 0) && node->hist != NULL){
            l = 0;
            r = node->hist->size()-1;
            last_hist = 1;
            while (l <= r){
                m = (l + r) /2;
                if ((*node->hist)[m] < ts){
                    last_hist = m;
                    l = m + 1;
                }
                else{
                    r = m - 1;
                }
            }
            if (last_hist % 2 == 0){ // last hist is add
                if (node->ts == NULL)
                    newTs(node->ts);
                if (node->ts->find(ts) == node->ts->end()){
                    node->ts->insert(ts);
                    cands->emplace_back(std::make_pair(query, (int)(it-query)));
                }
            }
            if (*it == 0)
                return;
        }
        temp = node->next.find(*it);
        if (temp == node->next.end())
            return;
        node = temp->second;
    }
}
