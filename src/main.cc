#include <stdio.h>
#include <string.h>
#include <assert.h>
#define BUF_SIZE 1024

char **ngrams;
int num_ngrams;

int main(int argc, char *argv[]){
    int n;
    char buf[BUF_SIZE];
    assert(1 == 2);
    freopen("input.txt", "r", stdin);
    ngrams = (char**)malloc(0);
    num_ngrams = 0;
    while(1){
        fgets(buf, BUF_SIZE, stdin);
        n = strlen(buf);
        if (buf[n-1] == '\n'){
            buf[n-1] = 0;
        }
        if (buf[0] == 'S' && buf[1] == 0){
            break;
        }
        else{
            ngrams = (char**)realloc(ngrams, sizeof(char*)*(num_ngrams+1));
        }
    }
    printf("R\n");
    return 0;
}
