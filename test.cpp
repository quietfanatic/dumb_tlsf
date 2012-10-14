
#include <stdio.h>
#include <unistd.h>
#include "dumb_tlsf.cpp"

using namespace dumb_tlsf;

struct big {
    char x [4096];
};

big* bigs [100000];


int main () {
    uint i;
    for (i = 0; i < 100000; i++) {
        bigs[i] = dalloc<big>();
        bigs[i]->x[0] = 1;
    }
    printf("%u\n", i);
    for (i = 0; i < 100000; i++) {
        dfree(bigs[i]);
    }
    printf("%u\n", i);
    for (i = 0; i < 100000; i++) {
        bigs[i] = dalloc<big>();
        bigs[i]->x[0] = 1;
    }
    printf("%u\n", i);
    sleep(100000);
    return 0;
}


