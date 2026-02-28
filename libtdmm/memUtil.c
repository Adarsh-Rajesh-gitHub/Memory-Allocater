#include "tdmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//randomly allocates and frees 100 blocks of memory of sizes 1000-100000 bytes
void memrandom100(alloc_strat_e a) {
    FILE* file;
    file=fopen("data1.csv","w");
    if(file==NULL) {
        fprintf(stderr,"error opening file");
        return;
    }

    t_init(a);

    int n=100;
    int sizes[100];
    void* ptrs[100];

    for(int i=0;i<n;i++) {
        sizes[i]=(i+1)*1000;
        ptrs[i]=NULL;
    }

    srand(0);

    clock_t base=clock();
    int eventId=0;

    for(int it=0;it<100;it++) {
        int ind=rand()%n;

        if(ptrs[ind]!=NULL) {
            t_free(ptrs[ind]);
            ptrs[ind]=NULL;

            double t=(double)(clock()-base)/CLOCKS_PER_SEC;
            double u=memoryUtilization();
            fprintf(file,"%d,%d,F,%d,%f,%f\n",a,eventId++,sizes[ind],t,u);
        } else {
            ptrs[ind]=t_malloc((size_t)sizes[ind]);

            double t=(double)(clock()-base)/CLOCKS_PER_SEC;
            double u=memoryUtilization();
            fprintf(file,"%d,%d,M,%d,%f,%f\n",a,eventId++,sizes[ind],t,u);
        }
    }

    for(int i=0;i<n;i++) {
        if(ptrs[i]!=NULL) {
            t_free(ptrs[i]);
            ptrs[i]=NULL;

            double t=(double)(clock()-base)/CLOCKS_PER_SEC;
            double u=memoryUtilization();
            fprintf(file,"%d,%d,F,%d,%f,%f\n",a,eventId++,sizes[i],t,u);
        }
    }

    fclose(file);
}

int main() {
    memrandom100(BEST_FIT);
    return 0;
}