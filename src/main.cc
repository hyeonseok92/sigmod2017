#include <iostream>
#include <string.h>
#include <vector>
#include <unordered_set>
#include <assert.h>
#include "trie.h"

TrieNode *trie;

void input(){
    std::string buf;
    while(1){
        std::getline(std::cin, buf);
        //std::cerr << buf << std::endl;
        if (buf.compare("S") == 0){
            break;
        }
        else{
            addNgram(trie, buf);
        }
    }
    printf("R\n");
}

void workload(){
    std::string cmd;
    int ts = 0;
    std::string buf;
    fflush(stdout);
    while(!std::cin.eof()){
        ++ts;
        std::cin >> cmd;
        //std::cerr << cmd;
        if (cmd.compare("F") == 0){
            fflush(stdout);
            std::cin >> cmd;
            if (cmd.compare("F") == 0){
                break;
            }
            //std::cerr << cmd;
        }
        std::cin.get();
        std::getline(std::cin, buf);
        //std::cerr << " " << buf << std::endl; 
        if (cmd.compare("A") == 0){
            addNgram(trie, buf);
        }
        else if (cmd.compare("Q") == 0){
            std::vector<std::string> res;
            std::unordered_set<std::string> exist_chk;
            std::vector<std::string> tmp;
            res.clear();
            for (std::string::const_iterator it = buf.begin();; it++){
                tmp = queryNgram(trie, &(*it));
                for (std::vector<std::string>::iterator it2 = tmp.begin(); it2 != tmp.end(); it2++){
                    if (exist_chk.find(*it2) == exist_chk.end()){
                        res.emplace_back(*it2);
                        exist_chk.insert(*it2);
                    }
                }
                while(it != buf.end() && *it != ' ') it++;
                if (it == buf.end())
                    break;
            }
            if (res.size()){
                std::cout << res[0];
                for (std::vector<std::string>::const_iterator it = ++res.begin(); it != res.end(); it++){
                    std::cout << "|" << *it;
                }
            }
            else{
                std::cout << -1;
            }
            std::cout << std::endl;
        }
        else if (cmd.compare("D") == 0){
            delNgram(trie, buf);
        }
    }
}

int main(int argc, char *argv[]){
//    freopen("input.txt", "r", stdin);
    initTrie(&trie);
    input();
    workload();
    destroyTrie(trie);
    return 0;
}
