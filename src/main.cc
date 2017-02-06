#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <assert.h>
#include "trie.h"
#define BUF_SIZE 1024

Trie *trie;

void input(){
    char buf[BUF_SIZE];
    while(1){
        fgets(buf, sizeof(buf), stdin);
        if (buf[0] == 'S'){
            break;
        }
        else{
            buf[strlen(buf)-1] = 0;
            addNgram(trie, 0, buf);
        }
    }
    printf("R\n");
}

void workload(){
    char buf[BUF_SIZE];
    char cmd;
    int i = 0;
    while(1){
        i++;
        scanf("%c ", &cmd);
        if (cmd == 'F'){
            break;
        }
        fgets(buf, BUF_SIZE, stdin);
        buf[strlen(buf)-1] = 0;
        if (cmd == 'A'){
            addNgram(trie, i, buf);
        }
        else if (cmd == 'Q'){
            std::vector<std::string> res = queryNgram(trie, i, buf);
            if (res.size()){
                printf("%s", res[0].c_str());
                for (int i = 1; i < res.size(); i++){
                    printf("|%s", res[i].c_str());
                }
                printf("\n");
            }
        }
        else if (cmd == 'D'){
            delNgram(trie, i, buf);
        }
    }
}

int main(int argc, char *argv[]){
    freopen("input.txt", "r", stdin);
    initTrie(&trie);
    input();
    workload();
    destroyTrie(&trie);
    return 0;
}
