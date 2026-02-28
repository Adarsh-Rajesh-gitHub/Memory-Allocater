#include "tdmm.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

void oneToEightMB(alloc_strat_e a) {
    FILE* file;
    file = fopen("data.csv", "w");
    if(file == NULL) {
        fprintf(stderr, "error opening file");
        return;
    }
    t_init(a);
    for(int i = 0; i < 24; i++) {
        clock_t startM, endM, startF, endF;
        double timeM, timeF;

        startM = clock();
        void* ptr = t_malloc(1ULL << i);
        endM = clock();
        timeM = (double)(endM-startM)/CLOCKS_PER_SEC;

        startF = clock();
        t_free(ptr);
        endF = clock();
        timeF = (double)(endF-startF)/CLOCKS_PER_SEC;

        fprintf(file,
                "%d, %d, %f, %f\n",
                a,
                1ULL << i,
                timeM,
                timeF);
    }
    fclose(file);
}


int main() {

    oneToEightMB(BEST_FIT);
    return 0;
}