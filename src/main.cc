#include <iostream>
#include <string.h>
#include <vector>
#include <unordered_set>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "trie.h"
#include "thread_struct.h"

#define RES_RESERVE 128
#define CMD_RESERVE 4
#define BUF_RESERVE 1024*1024
#define my_hash(x) (((unsigned char)(x))%NUM_THREAD)

//#define TRACE_WORK

#define USE_YIELD

#ifdef USE_YIELD
#define my_yield() pthread_yield()
#else
#define my_yield() usleep(1)
#endif

#define FLAG_INIT 0
#define FLAG_INIT_END 1
#define FLAG_WORK 2
#define FLAG_WORK_END 3

pthread_t threads[NUM_THREAD];
ThrArg args[NUM_THREAD];

int global_flag = FLAG_INIT;
QueryInfo raw_queries[MAX_BATCH_SIZE];
int raw_query_tail;
int raw_query_head;
int sync_val;

void *thread_main(void *arg){
#ifdef TRACE_WORK
    int cntwork = 0;
#endif
    TrieNode *my_trie;
    ThrArg *myqueue = (ThrArg*)arg;
    int modify_head = 0;
    int query_head = 0;
    initTrie(&my_trie);

    // Start of Init
    while(1){
        if (modify_head != myqueue->modify_tail){
            ModifyOp *op = &(myqueue->modify_ops[modify_head++]);
            addNgram(my_trie, op->ts, op->str);
        }
        else if (global_flag == FLAG_INIT){
            my_yield();
            continue;
        }
        else{
//            __sync_synchronize(); //Load Memory barrier
//            if (modify_head != myqueue->modify_tail)
//                continue;
            break;
        }
    }
    __sync_synchronize(); //Prevent Code Relocation
    myqueue->modify_tail = modify_head = 0;
    myqueue->query_tail = query_head = 0;
    __sync_synchronize(); //Prevent Code Relocation
    __sync_fetch_and_add(&sync_val, 1);
    while(global_flag != FLAG_WORK)
        my_yield();
    __sync_synchronize(); //Prevent Code Relocation

    // Start of Workload
    while(1){
        if (modify_head != myqueue->modify_tail){
#ifdef TRACE_WORK
            cntwork++;
#endif
            ModifyOp *op = &(myqueue->modify_ops[modify_head++]);
            if (op->cmd == 'A')
                addNgram(my_trie, op->ts, op->str);
            else
                delNgram(my_trie, op->ts, op->str);
        }
        else if (query_head != myqueue->query_tail){
//            __sync_synchronize();//Load Memory barrier
//            if (modify_head != myqueue->modify_tail)
//                continue;
            QueryOp *op = &(myqueue->query_ops[query_head++]); 
            while(op->ts == 0)
                my_yield();
            if (op->start_point != NULL){
                queryNgram(op->res, op->ts, my_trie, op->start_point);
                op->ts = 0;//clear for reuse
            }
            else{
                __sync_synchronize();
                __sync_fetch_and_add(op->is_end, 1);
            }
        }
        else if (raw_query_head != raw_query_tail){
            int my_raw_query_head = raw_query_head;
            if (__sync_bool_compare_and_swap(&raw_query_head, my_raw_query_head, my_raw_query_head+1)){
                QueryInfo *queryinfo = &raw_queries[my_raw_query_head];
                for (const char *c = &(*raw_queries[my_raw_query_head].query.begin()); *c != 0;){
                    c++;

                    unsigned int work_tid = my_hash(*c);
                    ThrArg *worker = &args[work_tid];
                    int my_tail = __sync_fetch_and_add(&worker->query_tail,1);
                    worker->query_ops[my_tail].start_point = c;
                    worker->query_ops[my_tail].res = &queryinfo->res[work_tid];
                    __sync_synchronize();
                    //Signal to worker that this op is available
                    worker->query_ops[my_tail].ts = queryinfo->ts;

                    while(*c != ' ' && *c != 0) c++;
                }
                for (int i = 0; i < NUM_THREAD; i++){
                    ThrArg *worker = &args[i];
                    int my_tail = __sync_fetch_and_add(&worker->query_tail,1);
                    worker->query_ops[my_tail].start_point = NULL;
                    worker->query_ops[my_tail].is_end = &(queryinfo->finished);
                    __sync_synchronize();
                    //Signal to worker that this op is available
                    worker->query_ops[my_tail].ts = queryinfo->ts;
                }
                continue;
            }
        }
        else if (global_flag == FLAG_WORK){
            my_yield();
            continue;
        }
        else{
#ifdef TRACE_WORK
            fprintf(stderr, "%2d : %5d\n", tid,cntwork);
#endif
            break;
        }
    }
    destroyTrie(my_trie);
    pthread_exit((void*)NULL);
}

void initThread(){
    for (int i = 0; i < NUM_THREAD; i++){
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
            ThrArg *worker = &args[my_hash(*buf.begin())];
            worker->modify_ops[worker->modify_tail].cmd = 'A';
            worker->modify_ops[worker->modify_tail].str = buf;
            worker->modify_ops[worker->modify_tail].ts = 1;
            __sync_synchronize(); //Prevent Code Relocation
            worker->modify_tail++;
        }
    }
}

void printQuery(){
    unsigned int it[NUM_THREAD];
    int tot;
    QueryInfo *queryinfo;
    for (int i = 0; i < raw_query_tail; i++){
        queryinfo = &raw_queries[i];
        while(queryinfo->finished != NUM_THREAD)
            my_yield();
        tot = 0;
        for (int j = 0; j < NUM_THREAD; j++){
            it[j] = 0;
            tot += queryinfo->res[j].size();
        }
        if (tot == 0){
            printf("-1\n");
        }
        else{
            const char *c = (const char *)-1;
            int size;
            int tid = 0;
            for (int k = 0; k < NUM_THREAD; k++){
                if (it[k] < queryinfo->res[k].size() && queryinfo->res[k][it[k]].first < c){
                    c = queryinfo->res[k][it[k]].first;
                    size = queryinfo->res[k][it[k]].second;
                    tid = k;
                }
            }
            printf("%.*s", size, c);
            it[tid]++;
            for (int j = 1; j < tot; j++){
                c = (const char *)-1;
                for (int k = 0; k < NUM_THREAD; k++){
                    if (it[k] < queryinfo->res[k].size() && queryinfo->res[k][it[k]].first < c){
                        c = queryinfo->res[k][it[k]].first;
                        size = queryinfo->res[k][it[k]].second;
                        tid = k;
                    }
                }
                printf("|%.*s", size, c);
                it[tid]++;
            }
            printf("\n");
        }
    }
    fflush(stdout);
    for (int i = 0; i < raw_query_tail; i++){
        for (int j = 0; j < NUM_THREAD; j++)
            raw_queries[i].res[j].clear();
    }
    raw_query_head = raw_query_tail = 0;
}

void workload(){
    unsigned int ts = 1;
    std::string cmd;
    std::string buf;
    cmd.reserve(CMD_RESERVE);
    buf.reserve(BUF_RESERVE);
    global_flag = FLAG_INIT_END;
    while(sync_val != NUM_THREAD)
        my_yield();
    global_flag = FLAG_WORK;
    printf("R\n");
    fflush(stdout);
    while(!std::cin.eof()){
        std::cin >> cmd;
        if (cmd.compare("F") == 0){
            printQuery();
            std::cin >> cmd;
            if (cmd.compare("F") == 0){
                global_flag = FLAG_WORK_END;
                break;
            }
        }
        if (cmd.compare("Q") != 0){
            std::getline(std::cin, buf);
            buf.erase(buf.begin());
            ThrArg *worker = &args[my_hash(*buf.begin())];
            worker->modify_ops[worker->modify_tail].cmd = *(cmd.begin());
            worker->modify_ops[worker->modify_tail].str = buf;
            worker->modify_ops[worker->modify_tail].ts = ts;
            __sync_synchronize(); //Prevent Code Relocation
            worker->modify_tail++;
        }
        else{
            ts++;
#ifdef DBG_TS
            std::cout << ts << " ";
#endif
            raw_queries[raw_query_tail].ts = ts;
            raw_queries[raw_query_tail].finished = 0;
            std::getline(std::cin, raw_queries[raw_query_tail].query);
            __sync_synchronize();
            raw_query_tail++;
        }
    }
}

int main(int argc, char *argv[]){
    std::ios::sync_with_stdio(false);
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
    destroyThread();
#ifdef DBG_LOG
    std::cerr<<"destroyThread done" << std::endl;
#endif
    return 0;
}
