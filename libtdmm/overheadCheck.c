#include "tdmm.h"
#include <stdio.h>
#include <stdlib.h>

//randomly allocates and frees 100 blocks of memory of sizes 1000-100000 bytes
void blocksOverOps(alloc_strat_e a) {
    FILE* file;
    file=fopen("data2.csv","w");
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

    int opId=0;

    {
        int b=retBlocks();
        unsigned long long req=(unsigned long long)retRequested();
        unsigned long long b32=(unsigned long long)b*32ULL;
        double ratio=(req==0ULL)?0.0:(double)b32/(double)req;
        fprintf(file,"%d,%d,%c,%d,%llu,%f\n",a,opId++,'I',b,b32,ratio);
    }

    for(int it=0;it<100;it++) {
        int ind=rand()%n;

        if(ptrs[ind]!=NULL) {
            t_free(ptrs[ind]);
            ptrs[ind]=NULL;

            int b=retBlocks();
            unsigned long long req=(unsigned long long)retRequested();
            unsigned long long b32=(unsigned long long)b*32ULL;
            double ratio=(req==0ULL)?0.0:(double)b32/(double)req;
            fprintf(file,"%d,%d,%c,%d,%llu,%f\n",a,opId++,'F',b,b32,ratio);
        } else {
            ptrs[ind]=t_malloc((size_t)sizes[ind]);

            int b=retBlocks();
            unsigned long long req=(unsigned long long)retRequested();
            unsigned long long b32=(unsigned long long)b*32ULL;
            double ratio=(req==0ULL)?0.0:(double)b32/(double)req;
            fprintf(file,"%d,%d,%c,%d,%llu,%f\n",a,opId++,'M',b,b32,ratio);
        }
    }

    for(int i=0;i<n;i++) {
        if(ptrs[i]!=NULL) {
            t_free(ptrs[i]);
            ptrs[i]=NULL;

            int b=retBlocks();
            unsigned long long req=(unsigned long long)retRequested();
            unsigned long long b32=(unsigned long long)b*32ULL;
            double ratio=(req==0ULL)?0.0:(double)b32/(double)req;
            fprintf(file,"%d,%d,%c,%d,%llu,%f\n",a,opId++,'F',b,b32,ratio);
        }
    }

    fclose(file);
}

int main() {
    blocksOverOps(WORST_FIT);
    return 0;
}