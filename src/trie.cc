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
    char cache_ch;
    for (std::string::const_iterator it = ngram.begin(); it != ngram.end(); it++){
        cache_ch = CACHE_CH(node->cache);
        assert(node != NULL);
        if (cache_ch == 0){
            newTrieNode(newNode);
            newNode->ts = 0xFFFFFFFF;
            node->cache |= ((unsigned long long)*it) << 56;
            node->cache |= ((unsigned long long)newNode);
            if (IS_ALPHA(cache_ch)) {
                node->bitmap |= FLAG_ALPHA(cache_ch);
            }
            node = newNode;
            continue;
        }
        if (cache_ch == *it){
            node = CACHE_NEXT(node->cache);
            continue;
        }
        temp = node->next.find(*it);
        if (temp == node->next.end()){
            newTrieNode(newNode);
            newNode->ts = 0xFFFFFFFF;
            node->next[*it] = newNode;
            if (IS_ALPHA(*it)) {
                node->bitmap |= FLAG_ALPHA(*it);
            }

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
    char cache_ch;
    char erase_ch;

    for (it = ngram.begin(); it != ngram.end(); it++){
        cache_ch = CACHE_CH(node->cache);
        if (cache_ch == 0)
            return;
        if (cache_ch == *it){
            next = CACHE_NEXT(node->cache);
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
    if (cache_ch){ //If there is more than 1 child node
        node->ts = 0xFFFFFFFF;
        return;
    }
    if (last_branch_next == last_branch->next.end()){
        node = CACHE_NEXT(last_branch->cache);
        if (last_branch->next.size()){
            last_branch->cache = 0;
            last_branch->cache |= ((unsigned long long)last_branch->next.begin()->first << 56);
            last_branch->cache |= ((unsigned long long)last_branch->next.begin()->second);
            erase_ch = last_branch->next.begin()->first;
            if (IS_ALPHA(erase_ch)) {
                last_branch->bitmap &= UNFLAG_ALPHA(erase_ch);
            }

            last_branch->next.erase(last_branch->next.begin());
        }
        else {
            erase_ch = CACHE_CH(last_branch->cache); 
            if (IS_ALPHA(erase_ch)) {
                last_branch->bitmap &= UNFLAG_ALPHA(erase_ch);
            }
            last_branch->cache = 0;
        }
    }
    else{
        node = last_branch_next->second;
        
        if (IS_ALPHA(last_branch_next->first)) {
            last_branch->bitmap &= UNFLAG_ALPHA(last_branch_next->first);
        }
        last_branch->next.erase(last_branch_next);
    }
    while(node->cache){ //If there is more than 1 child node
        next = CACHE_NEXT(node->cache);
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
        if (node->cache == 0)
            return;
        if (CACHE_CH(node->cache) == *it){
            node = CACHE_NEXT(node->cache);
            continue;
        }

        if (IS_ALPHA(*it)) {
            // look bitmap first to check if this character is in the map
            if (!(node->bitmap & FLAG_ALPHA(*it))) {
                return;
            }
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
