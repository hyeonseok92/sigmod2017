#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <assert.h>
#include "trie_word.h"
#include "trie_ngram.h"
#define BUF_SIZE 1024

TrieWord *trieWord;
Trie *trie;

void input(){
    std::string buf;
    std::string word;
    std::vector<int> ngram;
    while(1){
        std::getline(std::cin, buf);
        if (buf == "S"){
            break;
        }
        else{
            ngram.clear();
            for (std::stringstream ss(buf); ss >> word; ){
                ngram.push_back(insertTrieWord(trieWord, word));
            }
            insertTrie(trie, ngram);
        }
    }
    std::cout << "R" << std::endl;
}

void workload(){
    std::vector<int> v;
    std::vector<int> r;
    v.push_back(3);
    r = searchTrie(trie, v);
    std::cout << r.size() << std::endl;
    v.push_back(1);
    r = searchTrie(trie, v);
    std::cout << r.size() << std::endl;
}

int main(int argc, char *argv[]){
    freopen("input.txt", "r", stdin);
    initTrieWord(&trieWord);
    initTrie(&trie);
    input();
    workload();
    destroyTrie(&trie);
    destroyTrieWord(&trieWord);
    return 0;
}
