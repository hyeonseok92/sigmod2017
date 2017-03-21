#include <iostream>
#include <string.h>
#include <vector>
#include <unordered_set>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "util.h"
#include "config.h"
#include "trie.hpp"
#include "thread_struct.h"


TrieNode trie[NUM_THREAD];
pthread_t threads[NUM_THREAD];
unsigned int ts = 0;
ThrArg args[NUM_THREAD];

unsigned int started[NUM_THREAD];
unsigned int finished[NUM_THREAD];
std::vector<cand_t> res[NUM_THREAD];
std::string tasks[MAX_BATCH_SIZE];
int tp; //can be -1 so do not define as unsigned
int sync_val;

void *thread_main(void *arg){
    ThrArg *myqueue = (ThrArg*)arg;
    int tid = myqueue->tid;
    stick_to_core(tid+1);
    my_yield();
    TrieNode *my_trie = &trie[tid];
    std::vector<cand_t> *my_res = &res[tid];
    const char *op;
    my_res->reserve(RES_RESERVE);
#ifdef NEXT_RESERVE
    my_trie->next.reserve(NEXT_RESERVE);
#endif
    __sync_synchronize();
    __sync_fetch_and_add(&sync_val, 1);
    while(1){
        if (myqueue->head == myqueue->tail){
            if (started[tid] >= ts){
                my_yield();
                continue;
            }
            else{
                __sync_synchronize(); //Load Memory barrier
                if (myqueue->head != myqueue->tail) continue;
                __sync_synchronize(); //Prevent Code Relocation
                started[tid]++;
                const char *query_str = &tasks[tp][2];
                int size_query = (int)tasks[tp].size()-2;

                int size = 1 + (size_query -1) / NUM_THREAD;
                int my_sign = my_sign(ts, tid);
                const char *start = query_str + size * tid;
                const char *end = start + size;
                if (end-query_str > size_query)
                    end = query_str + size_query;
                myqueue->head = myqueue->tail = 0;
                my_res->clear();
                __sync_synchronize(); //Prevent Code Relocation
                for(const char *c = start; c < end; c++){
                    while(c != end && *(c-1) != ' ') c++;
                    if (c == end)
                        break;
                    int hval = my_hash(*c);
                    while(started[hval] != ts)
                        my_yield();
                    queryNgram(my_res, my_sign, &trie[hval], c);
                }
                __sync_synchronize(); //Prevent Code Relocation
                finished[tid]++;
                continue;
            }
        }
        else{
            op = myqueue->operations[myqueue->head++];
            if (op[0] == 'A')
                addNgram(my_trie, &op[2]);
            else
                delNgram(my_trie, &op[2]);
            continue;
        }
    }
    pthread_exit((void*)NULL);
}

void initThread(){
    for (int i = 0; i < NUM_THREAD; i++){
        args[i].tid = i;
        pthread_create(&threads[i], NULL, &thread_main, (void *)&args[i]);
    }
}
void destroyThread(){
    for (int i = 0; i < NUM_THREAD; i++)
        pthread_join(threads[i], NULL);
}

void input(){
    std::string buf;
    while(1){
        std::getline(std::cin, buf);
        if (buf.compare("S") == 0)
            break;
        else
            addNgram(&trie[my_hash(*buf.begin())], &buf[0]);
    }
}

inline void print(){
    bool print_answer = false;
    for (int i = 0; i < NUM_THREAD; i++){
        while(finished[i] != ts) my_yield();
        unsigned int my_sign = my_sign(ts, i);
        for (std::vector<cand_t>::const_iterator it = res[i].begin(); it != res[i].end(); it++){
            if (*it->from == my_sign){
                if (print_answer)
                    std::cout << "|";
                print_answer = true;
                std::cout.write(it->start, it->size);
            }
        }
    }
    if (!print_answer)
        std::cout << -1;
    std::cout << std::endl;
}

inline void chkOverflowTs(){
    if (unlikely(ts >= MAX_TS)){
        ts = 0;
        for (int i = 0; i < NUM_THREAD; i++){
            touchTrie(&trie[i]);
            started[i] = finished[i] = 0;
        }
    }
}

inline void chkOverflowTp(){
    if (unlikely(tp >= MAX_BATCH_SIZE-1)){
        tasks[tp] = "Q ";
        __sync_synchronize(); //Prevent Code Relocation
        ts++;
        __sync_synchronize(); //Prevent Code Relocation
        for (int i = 0; i < NUM_THREAD; i++)
            while(finished[i] != ts) my_yield();
        tp = 0;
        chkOverflowTs();
    }
}


void workload(){
    for (int i = 0; i < NUM_BUF_RESERVE; i++){
        tasks[i].reserve(BUF_RESERVE);
    }
    while(sync_val != NUM_THREAD)
        my_yield();
    printf("R\n");
    fflush(stdout);
    while(1){
        std::getline(std::cin, tasks[tp]);
        if (tasks[tp][0] == 'F'){
            fflush(stdout);
            std::getline(std::cin, tasks[tp]);
            if (tasks[tp][0] == '\0'){
                exit(0);
            }
        }
        if (tasks[tp][0] != 'Q'){
            ThrArg *worker = &args[my_hash(tasks[tp][2])];
            worker->operations[worker->tail] = &tasks[tp][0];
            __sync_synchronize(); //Prevent Code Relocation
            ++worker->tail; // queue overflow is processed together when task overflow occured
        }
        else{
#ifdef DBG_TS
            std::cerr << ts << std::endl;
#endif
            __sync_synchronize(); //Store Memory barrier
            ts++;
            __sync_synchronize(); //Prevent Code Relocation
            print();
            tp = -1;
            chkOverflowTs();
        }
        ++tp;
        chkOverflowTp();
    }
}

int main(int argc, char *argv[]){
    stick_to_core(0);
    my_yield();
    std::ios::sync_with_stdio(false);
#ifdef DBG_LOG
    std::cerr<<"initTree done" << std::endl;
#endif
    initThread();
#ifdef DBG_LOG
    std::cerr<<"initThread done" << std::endl;
#endif
    input();
#ifdef DBG_LOG
    std::cerr<<"input done" << std::endl;
#endif
    workload();
#ifdef DBG_LOG
    std::cerr<<"workload done" << std::endl;
#endif
/*
// Not use when we remove global_flag
    destroyThread();
#ifdef DBG_LOG
    std::cerr<<"destroyThread done" << std::endl;
#endif
    for (int i = 0; i < NUM_THREAD; i++)
        destroyTrie(&trie[i]);
#ifdef DBG_LOG
    std::cerr<<"destroyTrie done" << std::endl;
#endif
*/
    return 0;
}
