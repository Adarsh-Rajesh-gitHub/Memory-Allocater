#include "tdmm.h"
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Block {
	//metadata is the size of the struct itself which is 25(1ptr and 2size_t and 1bool0 + 7(from padding)
	size_t size;
	//what got pulled for direct user needs
	size_t usable;
	bool free;
	struct Block *next;
} Block;



alloc_strat_e cur;
Block* start = NULL;

//for tracking
u_int64_t requested = 0;
u_int64_t allocated = 0;
uint64_t blocks = 0;

void t_init(alloc_strat_e strat) {
	blocks = 1;
	allocated = 0;
	cur = strat;
	if(start == NULL) {
		//+32 for meta data and then +32 done to pointer to return actual start
		start = (Block*)mmap(NULL, 1000000+sizeof(Block), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		start->free = true;
		start->size = 1000000+sizeof(Block);
		start->usable = 1000000;
		start->next = NULL;
		//for tracking
		requested+=1000000+sizeof(Block);
	}
	//if init already called then make all blocks free
	else {
		Block* iter = start;
		while(iter != NULL) {
			iter->free = true;
			iter = iter->next;
		}
	}
}

//helper for once node found to load in memory on, it handles loading in memeory and creating next block with leftover or case where more memory needed
void* loadIn(size_t size, Block* start) {
	//found space
		if(start->usable >= size && start->free) {
			//fill current node and make new node right after with leftover space~
			if(start->usable-size > sizeof(Block)) {
				blocks++;
				//leftover space is start->size-size
				//create new block after w. new ptr which is prev block ptr + leftover
				Block* new = (Block*)(((char*)start) + sizeof(Block) + size);
				new->free = true;
				new->size = start->size-size-sizeof(Block);
				new->usable = new->size-sizeof(Block);
				new->next = NULL;
				//take leftover space out
				start->usable = size;
				start->size = size+sizeof(Block);
				start->free = false;
				//connect new into the sequence
				Block* temp = start->next;
				start->next = new;
				new->next = temp;
			}
			else {
				start->free = false;
			}	
			return ((char*)start) + sizeof(Block);
		}
		//no space found and at end
		else {
			//amount to ask mmap for rounded up to the nearest page including meta data
			uint64_t pages = (int)ceil(((float)size+sizeof(Block))/((float)4096));
			Block* new = (Block*)mmap(NULL, pages*4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			//for tracking
			requested+=pages*4096;
			new->free = false;
			new->size = pages*4096;
			new->usable = new->size-sizeof(Block);
			new->next = NULL;
			start->next = new;
			blocks++;
			//fill current node and make new node right after with leftover space~
			if(new->usable-size > sizeof(Block)) {
				Block* extra = (Block*)(((char*)new) + sizeof(Block) + size);
				extra->free = true;
				extra->size = new->size-size-sizeof(Block);
				extra->usable = extra->size-sizeof(Block);
				extra->next = NULL;
				//take leftover space out
				new->usable = size;
				new->size = size+sizeof(Block);
				new->free = false;
				//connect extra into the sequence
				new->next = extra;
				blocks++;
			}
			return ((char*)new) + sizeof(Block);
		}
}

//NOTE: Fix by removing payload field and instead putting the mmaps to intialize the Block* ptrs
//there is currently an error as the Block* don't have a memery
//you will simulate the memory as the actual pointers themselves will be at those memory starts
void *t_malloc(size_t size) {
	
	if((cur != FIRST_FIT && cur != BEST_FIT && cur != WORST_FIT) || start == NULL) {
		fprintf(stderr, "malloc without starting process");
	}
	if(size%4 != 0) size = (size & ~3) + 4;
	//for tracking
	allocated +=size;
	if(cur == FIRST_FIT) {
		Block* iter = start;
		while(iter->usable < size || !iter->free) {
			if(iter->next == NULL) break;
			iter = iter->next;
		}
		return loadIn(size, iter);
		// //found space
		// if(start->usable >= size && start->free) {
		// 	//fill current node and make new node right after with leftover space~
		// 	if(start->usable-size > sizeof(Block)) {
		// 		//leftover space is start->size-size
		// 		//take leftover space out
		// 		start->usable -= start->size-size;
		// 		start->size -= start->size-size;
		// 		start->free = false;
		// 		//create new block after w. new ptr which is prev block ptr + leftover
		// 		Block* new = start+start->size-size;
		// 		new->free = true;
		// 		new->size = start->size-size;
		// 		new->usable = new->size-sizeof(Block);
		// 		new->next = NULL;
		// 		//connect new into the sequence
		// 		Block* temp = start->next;
		// 		start->next = new;
		// 		new->next = temp;
		// 	}
		// 	else {
		// 		start->free = false;
		// 	}	
		// }
		// //no space found and at end
		// else {
		// 	//amount to ask mmap for rounded up to the nearest page including meta data
		// 	uint64_t pages = (int)ciel(((float)size+sizeof(Block))/((float)4096));
		// 	Block* new = (Block*)mmap(NULL, pages*4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
		// 	new->free = false;
		// 	new->size = pages*4096;
		// 	new->usable = new->size-sizeof(Block);
		// 	new->next = NULL;
		// 	start->next = new;
		// 	//fill current node and make new node right after with leftover space~
		// 	if(new->usable-size > sizeof(Block)) {
		// 		//leftover space is start->size-size
		// 		//take leftover space out
		// 		new->usable -= new->size-size;
		// 		new->size -= new->size-size;
		// 		new->free = false;
		// 		//create new block after w. new ptr which is prev block ptr + leftover
		// 		Block* extra = new+new->size-size;
		// 		extra->free = true;
		// 		extra->size = new->size-size;
		// 		extra->usable = extra->size-sizeof(Block);
		// 		extra->next = NULL;
		// 		//connect extra into the sequence
		// 		new->next = extra;
		// 	}
		// }

	}
	else if(cur == BEST_FIT) {
		Block* ptr = NULL;
		//loop through and then pick free block with smallest difference in size available and size needed 
		//set to max value uint_64t can hold
		uint64_t differential = (1ULL << 63)-1;
		uint64_t curDiff;
		Block* iter = start;
		while(iter->next != NULL) {
			if(iter->free && iter->usable >= size) {
				curDiff = iter->usable-size;
				if(curDiff < differential) {
					differential = curDiff;
					ptr = iter;
				}
			}
			iter = iter->next;
		}
		if(ptr == NULL) ptr = iter;

		// //no match found so have to set to end
		// if(ptr == NULL) {
		// 	ptr = iter->next;
		// }
		return loadIn(size, ptr);
	}
	else if(cur == WORST_FIT) {
		Block* ptr = NULL;
		int64_t differential = -1;
		int64_t curDiff;
		Block* iter = start;
		while(iter != NULL) {
			if(iter->free && iter->usable >= size) {
				curDiff = iter->usable-size;
				if(curDiff > differential) {
					differential = curDiff;
					ptr = iter;
				}
			}
			if(iter->next != NULL) iter = iter->next;
			else break;
		}
		//no match found so have to set to end
		if(ptr == NULL) {
			ptr = iter;
		}
		return loadIn(size, ptr);
	}
	else {
		fprintf(stderr, "illegal enum val");
	}
}

void t_free(void *ptr) {
	Block* iter = start;
  	while((((char*)iter) + sizeof(Block)) != ptr) {
		iter = iter->next;
		if(iter == NULL) {
			fprintf(stderr, "tried free nonexisting pointer");
		}
	}
	//for tracking
	allocated -= iter->usable;
	iter->free = true;
	//coalese
	Block* coalesce = start;
	while(coalesce->next != NULL) {
		//free checks and also if they are adjacent in memory
		if(coalesce->free && coalesce->next->free && ((char*)coalesce+coalesce->size) == (char*)coalesce->next) {
			Block* attach = coalesce->next->next;
			coalesce->size += coalesce->next->size;
			coalesce->usable = coalesce->size-sizeof(Block);
			coalesce->next = attach;
			blocks--;
			continue;
		}
		if(coalesce->next != NULL) coalesce = coalesce->next;
		else break;
	}
}

void printBlocks() {
	Block* iter = start;
	while(iter != NULL) {
		printf("Block at %p, size: %zu, usable: %zu, free: %d\n", (void*)iter, iter->size, iter->usable, iter->free);
		iter = iter->next;
	}
	printf("___________________________________________________________________\n");
}

double memoryUtilization() {
	return (double) allocated/requested;
}

int retBlocks() {
	return blocks;
}
int retRequested() {
	return requested;
}