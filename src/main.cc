#include <iostream>
#include <string.h>
#include <vector>
#include <unordered_set>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include "util.h"
#include "config.h"
#include "trie.hpp"
#include "thread_struct.h"

TrieNode trie[NUM_THREAD];
pthread_t threads[NUM_THREAD];
std::vector<mtask_t> mtasks[NUM_THREAD][NUM_THREAD];
unsigned int ts = 0;
ThrArg args[NUM_THREAD];

std::string tasks[MAX_BATCH_SIZE];
char delimiter_chk[MAX_BATCH_SIZE];

std::vector<res_t> res[NUM_THREAD];

int tp;
int preproc_tp;
int proc_tp;
int global_flag;
int sync_val;

#define FLAG_IDLE 0
#define FLAG_SCAN 1
#define FLAG_PREPROC 2
#define FLAG_PROC 3
#define FLAG_END 4

inline mbyte_t get_key(const char *c){
    mbyte_t key = 0;
    for (unsigned int i = 0; i < MBYTE_SIZE && *c; ++i, ++c)
        key |= (((mbyte_t)*c) << (i*8));
    return key;
}

void *thread_main(void *arg){
    ThrArg *myqueue = (ThrArg*)arg;
    int tid = myqueue->tid;
    stick_to_core(tid+1);
    my_yield();
    TrieNode *my_trie = &trie[tid];
    std::vector<mtask_t> *my_mtasks = mtasks[tid];
    std::vector<res_t> *my_res = &res[tid];
    __sync_fetch_and_add(&sync_val, 1);

    while(1){
        if (global_flag == FLAG_SCAN || global_flag == FLAG_PREPROC){
            int my_tp = __sync_fetch_and_add(&preproc_tp, 1);
            while(my_tp >= tp && global_flag != FLAG_PROC) my_yield();
            if (my_tp >= tp && global_flag == FLAG_PROC)
                continue;

            mtask_t mtask;
            mtask.task_id = my_tp;

            //Add / Delete
            if (tasks[my_tp][0] != 'Q'){
                mtask.offset = 0;
                mtasks[my_hash(get_key(&tasks[my_tp][2]))][tid].emplace_back(mtask);
            }
            else{
                //Query
                const char *start = &tasks[my_tp][0];
                for (const char *it = start+2;; it++){
                    while(*(it-1) != ' ' && *it) it++;
                    if (!(*it)) break;

                    mbyte_t key = 0;
                    mtask.offset = it - start;
                    const char *c = it;

                    for (unsigned int i = 0; i < MBYTE_SIZE && *c; ++i, ++c){
                        key |= (((mbyte_t)*c) << (i*8));
                        if (*(c+1) == ' '){
                            std::vector<mtask_t> *tmp = &mtasks[my_hash(key)][tid];
                            if (!tmp->size() ||
                                tmp->rbegin()->task_id != mtask.task_id ||
                                tmp->rbegin()->offset != mtask.offset)
                            //We don't need to input duplicate mtask
                            tmp->emplace_back(mtask);
                        }
                    }
                    std::vector<mtask_t> *tmp = &mtasks[my_hash(key)][tid];
                    if (!tmp->size() ||
                        tmp->rbegin()->task_id != mtask.task_id ||
                        tmp->rbegin()->offset != mtask.offset)
                        //We don't need to input duplicate mtask
                        tmp->emplace_back(mtask);
                }
            }
            __sync_fetch_and_add(&proc_tp, 1);
        }
        else if (global_flag == FLAG_PROC){
            //std::vector<mtask_t>::const_iterator its[NUM_THREAD];
            const mtask_t *its[NUM_THREAD];
            for (int i = 0; i < NUM_THREAD; i++)
                its[i] = &my_mtasks[i][0];

            while(1){
                const mtask_t *best = NULL;
                int best_i = 0;
                for (int i = 0; i < NUM_THREAD; i++){
                    if (its[i] == &(*my_mtasks[i].rbegin())) //Terminated Result
                        continue;

                    if (best == NULL || best->task_id > its[i]->task_id ||
                        (best->task_id == its[i]->task_id &&
                        best->offset > its[i]->offset)){
                        //task_id smaller first, offset smaller second
                        best = &(*its[0]);
                        best_i = i;
                    }
                }
                if (best == NULL) break;
                its[best_i]++;
                const char *op = &tasks[best->task_id][best->offset];
                if (*op == 'A') 
                    addNgram(my_trie, op+2);
                else if(*op == 'D')//Add / Del
                    delNgram(my_trie, op+2);
                else //Query
                    queryNgram(my_res, best->task_id, my_trie, op);
            }
            while(global_flag != FLAG_PROC)
                my_yield();
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
        else{
            addNgram(&trie[my_hash(get_key(&buf[0]))],&buf[0]);
        }
    }
}

inline void print(){
    /*
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
    */
}

void workload(){
    for (int i = 0; i < NUM_BUF_RESERVE; i++){
        tasks[i].reserve(BUF_RESERVE);
    }
    global_flag = FLAG_SCAN;
    while(sync_val != NUM_THREAD)
        my_yield();
    printf("R\n");
    fflush(stdout);
    while(1){
        std::getline(std::cin, tasks[tp]);
        if (tasks[tp][0] == 'F' || tp >= MAX_BATCH_SIZE-1){
            global_flag = FLAG_PREPROC;
            sync_val = 0;
            __sync_synchronize();
            while(proc_tp != tp)
                my_yield();

            __sync_synchronize();
            global_flag = FLAG_PROC;
            __sync_synchronize();
            while(sync_val != NUM_THREAD)
                my_yield();
            __sync_synchronize();

            //std::vector<res_t>::const_iterator its[NUM_THREAD];
            const res_t *its[NUM_THREAD];
            for (int i = 0; i < NUM_THREAD; i++)
                its[i] = &res[i][0];

            while(1){
                const res_t *best = NULL;
                int best_i = 0;
                for (int i = 0; i < NUM_THREAD; i++){
                    if (its[i] == &(*res[i].rbegin()))
                        continue;
                    if (best == NULL || best->task_id > its[i]->task_id ||
                        (best->task_id == its[i]->task_id && 
                        (best->start > its[i]->start ||
                        (best->start == its[i]->start && best->size > its[i]->size)))){
                        //task_id smaller first, start smaller second, size smaller third
                        best = &(*its[0]);
                        best_i = i;
                    }
                }
                if (best == NULL) break;
                std::cout.write(best->start, best->size);
                ++its[best_i];
            }
            fflush(stdout);
            tp = 0;
            std::getline(std::cin, tasks[tp]);
            if (tasks[tp][0] == '\0')
                exit(0);
            preproc_tp = 0;
            proc_tp = 0;
            __sync_synchronize();
            global_flag = FLAG_SCAN;
        }
        __sync_synchronize();
        tp++;
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
