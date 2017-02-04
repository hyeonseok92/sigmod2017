#include <iostream>
#include <vector>

#define MAX_NGRAM 10000
#define SIZE_ALPHABET 37
#define mapping(x) (((x) >= 'a' && (x) <= 'z') ? ((x)-'a') : ((x) == '.' ? 36 : ((x)-'0'+26))) 

struct TrieNode{
    int matched;
    TrieNode *next[SIZE_ALPHABET];
};

struct Ngram{
    std::string ngram;
    std::vector<int> history;
}

struct Trie{
    TrieNode node;
    Ngram ngrams[MAX_NGRAM];
    int num_ngram;
};
