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

#define NUM_THREAD 40
#define my_hash(x) (((mbyte_t)(x))%NUM_THREAD)

#define USE_YIELD

#ifdef USE_YIELD
#define my_yield() pthread_yield()
#else
#define my_yield() usleep(1)
#endif

#define FLAG_NONE 0
#define FLAG_QUERY 1
#define FLAG_END 3

TrieNode *trie;
pthread_t threads[NUM_THREAD];
ThrArg args[NUM_THREAD];

unsigned int ts;

int global_flag;
unsigned int started[NUM_THREAD];
unsigned int finished[NUM_THREAD];
std::vector<cand_t> res[NUM_THREAD];
const char *query_str;
int size_query;

void *thread_main(void *arg){
    ThrArg *myqueue = (ThrArg*)arg;
    int tid = myqueue->tid;
    std::vector<cand_t> *my_res = &res[tid];
    Operation *op;
    while(1){
        while(myqueue->head == myqueue->tail){
            if (global_flag == FLAG_NONE){
                my_yield();
                continue;
            }
            else if (global_flag == FLAG_QUERY){
                if (started[tid] == ts){
                    my_yield();
                    continue;
                }
                __sync_synchronize(); //Load Memory barrier
                if (myqueue->head != myqueue->tail) break;
                __sync_synchronize(); //Prevent Code Relocation
                int size = 1 + (size_query-1) / NUM_THREAD;
                const char *start = query_str + size * tid;
                const char *end = start + size;
                if (end-query_str > size_query){
                    end = query_str + size_query;
                }
                myqueue->head = myqueue->tail = 0;
                started[tid] = ts;
                my_res->clear();
                __sync_synchronize(); //Prevent Code Relocation
                for(const char *c = start; c < end; c++){
                    if (*c == ' ')
                        c++;
                    else
                        while(c != end && *(c-1) != ' ') c++;
                    if (c == end)
                        break;
                    mbyte_t key = 0;
                    const char *d = c;
                    for (unsigned int i = 0; i < MBYTE_SIZE && *d; i++, d++){
                        key += ((mbyte_t)(*d) << (i*8));
                        if (*(d+1) == ' ' || *(d+1) == 0){
                            while(started[my_hash(key)] != ts)
                                my_yield();
                        }
                    }
                    while(started[my_hash(key)] != ts)
                        my_yield();
                    queryNgram(my_res, MY_TS(tid), trie, c);
                }
                __sync_synchronize(); //Prevent Code Relocation
                finished[tid] = ts;
            }
            else
                pthread_exit((void*)NULL);
        }
        op = &(myqueue->operations[myqueue->head++]);
        mbyte_t key = 0;
        const char *it = &(*op->str.begin());
        for (unsigned int i = 0; i < MBYTE_SIZE && *it; i++, it++)
            key += ((mbyte_t)(*it) << (i*8));
        TrieNode *node = trie->next[key];
        if (op->cmd == 'A'){
            if (!node){
                node = newTrieNode();
                node->ts = 0xFFFFFFFF;
                node->next.clear();
                trie->next[key] = node;
            }
            addNgram(node, it);
        }
        else
            delNgram(node, it);
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
    for (int i = 0; i < NUM_THREAD; i++){
        pthread_join(threads[i], NULL);
    }
}

void input(){
    std::string buf;
    while(1){
        std::getline(std::cin, buf);
        if (buf.compare("S") == 0){
            break;
        }
        else{
            addNgram(trie, buf.c_str());
        }
    }
}

void workload(){
    std::string cmd;
    std::string buf;
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
            mbyte_t key = 0;
            std::string::const_iterator it = buf.begin();
            for (unsigned int i = 0; i < MBYTE_SIZE && it != buf.end(); i++, it++)
                key += ((mbyte_t)(*it) << (i*8));
            if (trie->next.find(key) == trie->next.end())
                trie->next[key] = NULL;
            ThrArg *worker = &args[my_hash(key)];
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
                while(finished[i] != ts) my_yield();
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
    std::ios::sync_with_stdio(false);
    initTrie(&trie);
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
    destroyTrie(trie);
#ifdef DBG_LOG
    std::cerr<<"destroyTrie done" << std::endl;
#endif
    return 0;
}
