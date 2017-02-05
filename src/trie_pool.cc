#if 0
#include "trie.h"
#include <assert.h>

#define DEF_TNPOOL_SIZE 128
TrieNode *pool;
int size_pool;
int pool_free;

void initTNPool(){
    assert(pool == NULL);
    pool = (TrieNode*) malloc(sizeof(TrieNode)*DEF_TNPOOL_SIZE);
    size_pool = DEF_TNPOOL_SIZE;
}

void destroyTNPool(){
    free(NodePool);
    NodePool = NULL;
    size_pool = 0;
    pool_free = 0;
}

TrieNode *newTN(){
    int my_pool;
    while(1){
        my_pool = __sync_fetch_and_add(&pool_free, 1);
    }
    return NULL;
}

void delTN(TrieNode *node){
}
#endif
