#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "tdmm.h"

typedef struct Block {
	size_t size;
	size_t usable;
	bool free;
	struct Block *next;
} Block;

extern Block* start;
extern void* loadIn(size_t size, Block* start);


// converts pointer to pointer+ metadata
static Block* usableToBlock(void *p){
	return (Block*)(((char*)p)-sizeof(Block));
}

//last block
static Block* getTail(){
	Block* it=start;
	while(it&&it->next) it=it->next;
	return it;
}


static void testInit(){
	t_init(FIRST_FIT);
	assert(start);
	assert(start->free==true);
	assert(start->usable==1000000);
	assert(start->next==NULL);
	assert(retBlocks()==1);
    printf("requested: %d\n", retRequested());
	assert(retRequested()>=1000000+(int)sizeof(Block));
}

//loadIn splits an existing free block 
static void testLoadInSplit(){
	t_init(FIRST_FIT);
	int blocksBefore=retBlocks();

	void* userPtr=loadIn(100,start);
	assert(userPtr);
	Block* header=usableToBlock(userPtr);

	assert(header==start);
	assert(header->free==false);
	assert(header->usable==100);
	assert(header->size==100+sizeof(Block));
	assert(header->next);
	assert(header->next->free==true);
	assert((char*)header+header->size==(char*)header->next);
	assert(retBlocks()==blocksBefore+1);
}

//loadIn calls mmap extend when no space
static void testLoadInExtend(){
	t_init(FIRST_FIT);

	void* most=t_malloc(999900);
	assert(most);

	Block* last=getTail();
	assert(last);

	int reqBefore=retRequested();
	int blocksBefore=retBlocks();

	void* userPtr=loadIn(2000,last);
	assert(userPtr);

	assert(retBlocks()>=blocksBefore+1);

	Block* header=usableToBlock(userPtr);
	assert(header->free==false);
	assert(header->usable==2000||header->usable==((2000+3)&~(size_t)3));
}

//t_malloc aligns to 4 
static void testMallocAlign(){
	t_init(FIRST_FIT);

	void* p=t_malloc(1);
	assert(p);
	assert(((uintptr_t)p&3)==0);

	Block* h=usableToBlock(p);
	assert(h->free==false);
	assert(h->usable==4);
}

//first-fit reuses freed block
static void testMallocReuseFirstFit(){
	t_init(FIRST_FIT);

	void* a=t_malloc(100);
	void* b=t_malloc(100);
	assert(a&&b);

	t_free(a);

	void* c=t_malloc(80);
	assert(c);
}

//t_free + coalesces behavior
static void testFreeCoalesceAdj(){
	t_init(FIRST_FIT);

	int base=retBlocks();
	void* a=t_malloc(100);
	void* b=t_malloc(200);
	int after=retBlocks();
	assert(after<=base+2);

	Block* ha=usableToBlock(a);
	Block* hb=usableToBlock(b);

	t_free(b);
	t_free(a);

	assert(retBlocks()<after);
}

//no coalesce across non-adjacent mmaps
static void testNoCrossMmapCoalesce(){
	t_init(FIRST_FIT);

	void* big=t_malloc(999900);
	void* tailBlk=t_malloc(100);
	void* ext=t_malloc(2000);
	assert(big&&tailBlk&&ext);

	t_free(tailBlk);
	int before=retBlocks();
	t_free(ext);

	Block* it=start;
	while(it&&it->next){
		if(it->free&&it->next->free){
			assert((char*)it+it->size==(char*)it->next);
		}
		it=it->next;
	}
	assert(retBlocks()>=before-2);
}

//best-fit picks smallest free block big enough
static void testBestFitPick(){
	t_init(BEST_FIT);

	void* a=t_malloc(100);
	void* b=t_malloc(200);
	void* c=t_malloc(300);
	assert(a&&b&&c);

	t_free(a);
	t_free(c);

	void* x=t_malloc(80);
	assert(x);
	assert(usableToBlock(x)==usableToBlock(a));
}

//worst-fit picks largest free block big enough
static void testWorstFitPick(){
	t_init(WORST_FIT);

	void* a=t_malloc(100);
	void* b=t_malloc(200);
	void* c=t_malloc(300);
	assert(a&&b&&c);

	t_free(a);
	t_free(c);

	void* x=t_malloc(80);
	assert(x);
	assert(usableToBlock(x)==usableToBlock(c));
}

static void testTracking(){
	t_init(FIRST_FIT);
	void* a=t_malloc(100);
	void* b=t_malloc(200);
	assert(a&&b);

	double mesure=memoryUtilization();
	assert(mesure>=0.0);
	assert(retRequested()>0);
	assert(retBlocks()>=1);
}

int main(){
	testInit();

	testLoadInSplit();
	testLoadInExtend();

	testMallocAlign();
	testMallocReuseFirstFit();

	testFreeCoalesceAdj();
	testNoCrossMmapCoalesce();

	testBestFitPick();
	testWorstFitPick();

	testTracking();

	puts("pass");
	return 0;
}