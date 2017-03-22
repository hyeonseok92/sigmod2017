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

struct res_pq_t{
    res_t *r;
    res_t *end;
};

bool operator < (res_pq_t a, res_pq_t b){
    if (a.r->task_id > b.r->task_id ||
            (a.r->task_id == b.r->task_id && 
             (a.r->start > b.r->start ||
              (a.r->start == b.r->start && a.r->size > b.r->size))))
        return true;
    return false;
}

typedef std::priority_queue<res_pq_t> res_queue_t;

struct mtask_pq_t{
    mtask_t *m;
    mtask_t *end;
};
bool operator < (mtask_pq_t a, mtask_pq_t b){
    if (a.m->task_id > b.m->task_id ||
            (a.m->task_id == b.m->task_id &&
             a.m->offset > b.m->offset))
        return true;
    return false;
}

typedef std::priority_queue<mtask_pq_t> mtask_queue_t;


TrieNode trie[NUM_THREAD] __attribute__((aligned(0x40)));
pthread_t threads[NUM_THREAD] __attribute__((aligned(0x40)));
std::vector<mtask_t> mtasks[NUM_THREAD][NUM_THREAD] __attribute__((aligned(0x40)));

std::string tasks[MAX_BATCH_SIZE] __attribute__((aligned(0x40)));
std::vector<res_t> res[NUM_THREAD] __attribute__((aligned(0x40)));

int tp  __attribute__((aligned(0x40)));
int preproc_tp __attribute__((aligned(0x40)));
int proc_tp __attribute__((aligned(0x40)));
int global_flag __attribute__((aligned(0x40)));
int sync_val __attribute__((aligned(0x40)));
int cnt_batch __attribute__((aligned(0x40)));

#define FLAG_SCAN 0
#define FLAG_PREPROC 1
#define FLAG_PROC 2

inline mbyte_t get_key(const char *c){
    mbyte_t key = 0;
    for (unsigned int i = 0; i < MBYTE_SIZE && *c; ++i, ++c)
        key |= (((mbyte_t)*c) << (i*8));
    return key;
}

void *thread_main(void *arg){
    int tid = (unsigned long long)arg;
    stick_to_core(tid+1);
    my_yield();
    TrieNode *my_trie = &trie[tid];
    std::vector<mtask_t> *my_mtasks = mtasks[tid];
    std::vector<res_t> *my_res = &res[tid];
    mtask_queue_t queue;
    unsigned int cnt_query = 0;
    unsigned int last_query_id = 0xFFFFFFFF;
    //for (int i = 0; i < NUM_THREAD; i++)
    //    my_mtasks[i].reserve(MTASK_RESERVE);
    //my_res->reserve(RES_RESERVE);
    __sync_synchronize();
    __sync_fetch_and_add(&sync_val, 1);

    while(1){
        if (global_flag == FLAG_SCAN || global_flag == FLAG_PREPROC){
            int my_tp = __sync_fetch_and_add(&preproc_tp, 1);
            while(my_tp >= tp && global_flag != FLAG_PROC){
                my_yield();
            }
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
            __sync_synchronize();
        }
        else if (global_flag == FLAG_PROC){
            __sync_synchronize();
            int cur_batch = cnt_batch;

            for (int i = 0; i < NUM_THREAD; i++){
                if (my_mtasks[i].size()){
                    mtask_pq_t dum;
                    dum.m = &my_mtasks[i][0];
                    dum.end = &my_mtasks[i][my_mtasks[i].size()];
                    queue.push(dum);
                }
            }
            while(!queue.empty()){
                mtask_pq_t best = queue.top();
                queue.pop();
                const char *op = &tasks[best.m->task_id][best.m->offset];
                if (best.m->offset == 0){//Add / Del
                    if (*op == 'A') 
                        addNgram(my_trie, op+2);
                    else if(*op == 'D')
                        delNgram(my_trie, op+2);
                }
                else{ //Query
                    if (last_query_id != best.m->task_id){
                        cnt_query++;
                        if (cnt_query == 0xFFFFFFFF){
                            touchTrie(my_trie);
                            cnt_query = 1;
                        }
                        last_query_id = best.m->task_id;
                    }
                    queryNgram(my_res, cnt_query, best.m->task_id, my_trie, op);
                }
                if (++best.m != best.end)
                    queue.push(best);
            }
            __sync_synchronize();
            __sync_fetch_and_add(&sync_val, 1);
            for (int i = 0; i < NUM_THREAD; i++)
                my_mtasks[i].clear();

            last_query_id = 0xFFFFFFFF;

            while(global_flag == FLAG_PROC && cur_batch == cnt_batch){
                my_yield();
            }
            my_res->clear();
            __sync_synchronize();
        }
    }
    pthread_exit((void*)NULL);
}

void initThread(){
    for (int i = 0; i < NUM_THREAD; i++){
        pthread_create(&threads[i], NULL, &thread_main, (void*)i);
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

void workload(){
    res_queue_t queue;
    std::vector<unsigned int> q_task_ids;
//    for (int i = 0; i < MAX_BATCH_SIZE/10; i++){
//        tasks[i].reserve(BUF_RESERVE);
//    }
//    q_task_ids.reserve(Q_ID_RESERVE);
    global_flag = FLAG_SCAN;
    q_task_ids.emplace_back(0xFFFFFFFF);
    __sync_synchronize();
    while(sync_val != NUM_THREAD){
        my_yield();
    }
    std::cout<<"R"<<std::endl;
    fflush(stdout);
    while(1){
        std::getline(std::cin, tasks[tp]);
        if (tasks[tp][0] == 'Q')
            q_task_ids.emplace_back(tp);

        if (tasks[tp][0] == 'F' || tp >= MAX_BATCH_SIZE-1){
            if (tp >= MAX_BATCH_SIZE-1){
                __sync_synchronize();
                tp++;
            }
            ++cnt_batch;
            sync_val = 0;
            global_flag = FLAG_PREPROC;
            __sync_synchronize();
            while(proc_tp != tp){
                my_yield();
            }

            __sync_synchronize();
            global_flag = FLAG_PROC;
            __sync_synchronize();
            while(sync_val != NUM_THREAD){
                my_yield();
            }
            __sync_synchronize();

            for (int i = 0; i < NUM_THREAD; i++){
                if (res[i].size()){
                    res_pq_t dum;
                    dum.r = &res[i][0];
                    dum.end = &res[i][res[i].size()];
                    queue.push(dum);
                }
            }

            std::vector<unsigned int>::const_iterator qid = q_task_ids.begin();
            bool printed = false;
            while(!queue.empty()){
                res_pq_t best = queue.top();
                queue.pop();

                if (*qid != best.r->task_id){
                    if (printed)
                        std::cout<<std::endl;
                    for (++qid; *qid != best.r->task_id; ++qid)
                        std::cout << "-1" << std::endl;
                    printed = false;
                }
                if (printed)
                    std::cout << "|";
                else
                    printed = true;

                std::cout.write(best.r->start, best.r->size);
                if (++best.r != best.end)
                    queue.push(best);
            }
            if (printed)
                std::cout<<std::endl;
            for (++qid;qid != q_task_ids.end(); ++qid)
                std::cout << "-1" << std::endl;

            fflush(stdout);
            tp = 0;
            preproc_tp = 0;
            proc_tp = 0;
            __sync_synchronize();
            global_flag = FLAG_SCAN;
            q_task_ids.clear();
            q_task_ids.emplace_back(0xFFFFFFFF);
            continue;
        }
        if (tasks[tp][0] == '\0')
            exit(0);
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
