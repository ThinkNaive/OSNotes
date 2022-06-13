// http://jyywiki.cn/pages/OS/2022/demos/peterson-simple.c
// gcc peterson.c -lpthread && ./a.out

#include "thread.h"

int sum = 0;
int N = 1000000;

int flagA = 0;
int flagB = 0;
int tag = 0;

void Ta() {
    flagA = 1;
    tag = 2;
    while (flagB && tag!=1);
    // critical
    for (int i=0; i<N; ++i) ++sum;
    // exit
    flagA = 0;
}

void Tb() {
    flagB = 1;
    tag = 1;
    while (flagA && tag!=2);
    // critical
    for (int i=0; i<N; ++i) ++sum;
    // exit
    flagB = 0;
}

int main() {
    create(Ta);
    create(Tb);
    join();
    printf("%d\n", sum);
}
