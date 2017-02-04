#include <iostream>
#include <vector>

#define SIZE_ALPHABET 37
#define mapping(x) (((x) >= 'a' && (x) <= 'z') ? ((x)-'a') : ((x) == '.' ? 36 : ((x)-'0'+26))) 

struct TrieWordNode{
    int matched;
    TrieWordNode *next[SIZE_ALPHABET];
};

struct TrieWord{
    TrieWordNode node;
    std::vector<>[SIZE_ALPHABET];
};
