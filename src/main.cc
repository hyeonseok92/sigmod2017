#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <assert.h>
#include "trie.h"
#define INPUT_BUF_SIZE 1024
#define WORK_BUF_SIZE 262144

Trie *trie;

void input(){
    char buf[INPUT_BUF_SIZE];
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
    char cmd;
    char buf[WORK_BUF_SIZE];
    int ts = 0;
    fflush(stdout);
    while(!feof(stdin)){
        ts++;
        scanf("%c", &cmd);
        if (cmd == 'F'){
            fflush(stdout);
            continue;
        }
        scanf(" ");
        fgets(buf, WORK_BUF_SIZE, stdin);
        buf[strlen(buf)-1] = 0;
        if (cmd == 'A'){
            addNgram(trie, ts, buf);
        }
        else if (cmd == 'Q'){
            std::vector<std::string> res = queryNgram(trie, ts, buf);
            if (res.size()){
                printf("%s", res[0].c_str());
                for (int i = 1; i < res.size(); i++){
                    printf("|%s", res[i].c_str());
                }
                printf("\n");
            }
        }
        else if (cmd == 'D'){
            if (buf[0] =='c' && buf[1] == 'o' && buf[2] == 'm' && buf[3] == 'p'){
                buf[0] = 'c';
            }
            delNgram(trie, ts, buf);
        }
    }
}

int main(int argc, char *argv[]){
//    freopen("input.txt", "r", stdin);
    initTrie(&trie);
    input();
    workload();
    destroyTrie(&trie);
    return 0;
}
