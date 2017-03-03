#include <iostream>
#include <string.h>
#include <vector>
#include <unordered_set>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "trie.h"
#include "thread_struct.h"

#define NUM_THREAD 39
#define RES_RESERVE 128
#define CMD_RESERVE 4
#define BUF_RESERVE 1024*1024
#define my_hash(x) (((unsigned char)(x))%NUM_THREAD)

//#define TRACE_WORK

#define USE_YIELD

#ifdef USE_YIELD
#define my_yield() pthread_yield()
#else
#define my_yield()
#endif

#define FLAG_NONE 0
#define FLAG_QUERY 1
#define FLAG_END 2

TrieNode *trie[NUM_THREAD];
pthread_t threads[NUM_THREAD];
ThrArg args[NUM_THREAD];

unsigned int ts;

int global_flag;
unsigned int started[NUM_THREAD][16];
unsigned int finished[NUM_THREAD][16];
std::vector<cand_t> res[NUM_THREAD];
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
    TrieNode *my_trie = trie[tid];
    std::vector<cand_t> *my_res = &res[tid];
    Operation *op;
    my_res->reserve(RES_RESERVE);
    __sync_synchronize();
    __sync_fetch_and_add(&sync_val, 1);
    while(1){
        while(myqueue->head == myqueue->tail){
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
                if (myqueue->head != myqueue->tail) break;
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
                    if (*c == ' ')
                        c++;
                    else
                        while(c != end && *(c-1) != ' ') c++;
                    if (c == end)
                        break;
                    int hval = my_hash(*c);
                    while(started[hval][0] != ts)
                        my_yield();
                    queryNgram(my_res, MY_TS(tid), trie[hval], c);
                }
                __sync_synchronize(); //Prevent Code Relocation
                finished[tid][0] = ts;
            }
            else{
#ifdef TRACE_WORK
                fprintf(stderr, "%2d : %5d\n", tid,cntwork);
#endif
                pthread_exit((void*)NULL);
            }
        }
#ifdef TRACE_WORK
        cntwork++;
#endif
        op = &(myqueue->operations[myqueue->head++]);
        if (op->cmd == 'A')
            addNgram(my_trie, op->str);
        else
            delNgram(my_trie, op->str);
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
            addNgram(trie[my_hash(*buf.begin())], buf);
    }
}

void workload(){
    std::string cmd;
    std::string buf;
    cmd.reserve(CMD_RESERVE);
    buf.reserve(BUF_RESERVE);
    while(sync_val != NUM_THREAD)
        my_yield();
    printf("R\n");
    fflush(stdout);
    while(!std::cin.eof()){
        std::cin >> cmd;
        if (cmd.compare("F") == 0){
            fflush(stdout);
            std::cin >> cmd;
            if (cmd.compare("F") == 0){
                global_flag = FLAG_END;
                break;
            }
        }
        std::getline(std::cin, buf);
        if (cmd.compare("Q") != 0){
            buf.erase(buf.begin());
            ThrArg *worker = &args[my_hash(*buf.begin())];
            worker->operations[worker->tail].cmd = *(cmd.begin());
            worker->operations[worker->tail].str = buf;
            __sync_synchronize(); //Prevent Code Relocation
            worker->tail++;
        }
        else{
            ts++;
#ifdef DBG_TS
            std::cout << ts << " ";
#endif
            query_str = buf.c_str();
            size_query = buf.size();
            __sync_synchronize(); //Prevent Code Relocation
            global_flag = FLAG_QUERY;
            __sync_synchronize(); //Prevent Code Relocation
            bool print_answer = false;
            for (int i = 0; i < NUM_THREAD; i++){
                while(finished[i][0] != ts) my_yield();
                unsigned int my_ts = MY_TS(i);
                for (std::vector<cand_t>::const_iterator it = res[i].begin(); it != res[i].end(); it++){
                    if (it->second->ts == my_ts){
                        if (print_answer)
                            std::cout << "|";
                        print_answer = true;
                        std::cout << it->first;
                    }
                }
            }
            if (!print_answer)
                std::cout << -1;
            std::cout << std::endl;
            global_flag = FLAG_NONE;
        }
    }
}

int main(int argc, char *argv[]){
    stick_to_core(0);
    my_yield();
    std::ios::sync_with_stdio(false);
    for (int i = 0; i < NUM_THREAD; i++)
        initTrie(&trie[i]);
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
        destroyTrie(trie[i]);
#ifdef DBG_LOG
    std::cerr<<"destroyTrie done" << std::endl;
#endif
    return 0;
}
