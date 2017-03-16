#include <iostream>
#include <string.h>
#include <vector>
#include <unordered_set>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "trie.h"
#include "thread_struct.h"

//#define DBG_TS
//#define DBG_LOG

#define NUM_THREAD 37
//#define NEXT_RESERVE 4096
#define RES_RESERVE 128
#define BUF_RESERVE 1024*1024
#define NUM_BUF_RESERVE 10000
#define my_hash(x) (((mbyte_t)(x))%NUM_THREAD)

//#define TRACE_WORK

#define my_yield() __sync_synchronize()//pthread_yield()

#define FLAG_NONE 0
#define FLAG_QUERY 1
#define FLAG_END 2

TrieNode trie[NUM_THREAD];
pthread_t threads[NUM_THREAD];
ThrArg args[NUM_THREAD];

unsigned int ts = 1;

int global_flag;
unsigned int started[NUM_THREAD][16];
unsigned int finished[NUM_THREAD][16];
std::vector<cand_t> res[NUM_THREAD];
std::string tasks[MAX_BATCH_SIZE];
int tp;
const char *query_str;
int size_query;
int sync_val;

//http://stackoverflow.com/questions/1407786/how-to-set-cpu-affinity-of-a-particular-pthread
int stick_to_core(int core_id) {
   int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
   if (core_id < 0 || core_id >= num_cores)
      return EINVAL;

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   pthread_t current_thread = pthread_self();    
   return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void *thread_main(void *arg){
#ifdef TRACE_WORK
    int cntwork = 0;
#endif
    ThrArg *myqueue = (ThrArg*)arg;
    int tid = myqueue->tid;
    stick_to_core(tid+1);
    my_yield();
    TrieNode *my_trie = &trie[tid];
    std::vector<cand_t> *my_res = &res[tid];
    const char *op;
    my_res->reserve(RES_RESERVE);
    //my_trie->next.reserve(NEXT_RESERVE);
    __sync_synchronize();
    __sync_fetch_and_add(&sync_val, 1);
    while(1){
        if (myqueue->head == myqueue->tail){
            if (global_flag == FLAG_NONE){
                my_yield();
                continue;
            }
            else if (global_flag == FLAG_QUERY){
                if (started[tid][0] == ts){
                    my_yield();
                    continue;
                }
                __sync_synchronize(); //Load Memory barrier
                if (myqueue->head != myqueue->tail) continue;
                __sync_synchronize(); //Prevent Code Relocation
                int size = 1 + (size_query-1) / NUM_THREAD;
                const char *start = query_str + size * tid;
                const char *end = start + size;
                if (end-query_str > size_query)
                    end = query_str + size_query;
                myqueue->head = myqueue->tail = 0;
                started[tid][0] = ts;
                my_res->clear();
                __sync_synchronize(); //Prevent Code Relocation
                for(const char *c = start; c < end; c++){
                    while(c != end && *(c-1) != ' ') c++;
                    if (c == end)
                        break;
                    int hval = my_hash(*c);
                    while(started[hval][0] != ts)
                        my_yield();
                    queryNgram(my_res, MY_SIGN(tid), &trie[hval], c);
                }
                __sync_synchronize(); //Prevent Code Relocation
                finished[tid][0] = ts;
                continue;
            }
            else{
#ifdef TRACE_WORK
                fprintf(stderr, "%2d : %5d\n", tid,cntwork);
#endif
                pthread_exit((void*)NULL);
            }
        }
        else{
#ifdef TRACE_WORK
            cntwork++;
#endif
            op = myqueue->operations[myqueue->head++];
            if (op[0] == 'A')
                addNgram(my_trie, &op[2]);
            else
                delNgram(my_trie, &op[2]);
            continue;
        }
    }
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

void workload(){
    for (int i = 0; i < NUM_BUF_RESERVE; i++){
        tasks[i].reserve(BUF_RESERVE);
    }
    while(sync_val != NUM_THREAD)
        my_yield();
    printf("R\n");
    fflush(stdout);
    while(!std::cin.eof()){
        std::getline(std::cin, tasks[tp]);
        if (tasks[tp][0] == 'F'){
            fflush(stdout);
            std::getline(std::cin, tasks[tp]);
            if (tasks[tp][0] == '\0'){
                global_flag = FLAG_END;
                break;
            }
        }
        if (tasks[tp][0] != 'Q'){
            ThrArg *worker = &args[my_hash(tasks[tp][2])];
            worker->operations[worker->tail] = &tasks[tp][0];
            __sync_synchronize(); //Prevent Code Relocation
            worker->tail++;
        }
        else{
            ts++;
#ifdef DBG_TS
            std::cout << ts << " ";
#endif
            query_str = &tasks[tp][2];
            size_query = tasks[tp].size()-2;
            __sync_synchronize(); //Prevent Code Relocation
            global_flag = FLAG_QUERY;
            __sync_synchronize(); //Prevent Code Relocation
            bool print_answer = false;
            for (int i = 0; i < NUM_THREAD; i++){
                while(finished[i][0] != ts) my_yield();
                unsigned int my_ts = MY_SIGN(i);
                for (std::vector<cand_t>::const_iterator it = res[i].begin(); it != res[i].end(); it++){
                    if (it->from->ts == my_ts){
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
            tp = -1;
            global_flag = FLAG_NONE;
        }
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
    destroyThread();
#ifdef DBG_LOG
    std::cerr<<"destroyThread done" << std::endl;
#endif
    for (int i = 0; i < NUM_THREAD; i++)
        destroyTrie(&trie[i]);
#ifdef DBG_LOG
    std::cerr<<"destroyTrie done" << std::endl;
#endif
    return 0;
}
