#include "tdmm.h"
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	//metadata is the size of the struct itself which is 25(1ptr and 2size_t and 1bool0 + 7(from padding)
	size_t size;
	//what got pulled for direct user needs
	size_t usable;
	bool free;
	struct Block *next;
} Block;



alloc_strat_e cur = NULL;
Block* start = NULL;

void t_init(alloc_strat_e strat) {
	cur = strat;
	if(start == NULL) {
		//+32 for meta data and then +32 done to pointer to return actual start
		start = (Block*)mmap(NULL, 1000000+32, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
		start->free = true;
		start->size = 1000000+32;
		start->usable = 1000000;
		start->next = NULL;
	}
	//if init already called then make all blocks free
	else {
		while(start != NULL) {
			start->free = true;
			start = start->next;
		}
	}
}

//helper for once node found to load in memory on, it handles loading in memeory and creating next block with leftover or case where more memory needed
void loadIn(size_t size, Block* start) {
	//found space
		if(start->usable >= size && start->free) {
			//fill current node and make new node right after with leftover space~
			if(start->usable-size > 32) {
				//leftover space is start->size-size
				//take leftover space out
				start->usable -= start->size-size;
				start->size -= start->size-size;
				start->free = false;
				//create new block after w. new ptr which is prev block ptr + leftover
				Block* new = start+start->size-size;
				new->free = true;
				new->size = start->size-size;
				new->usable = new->size-32;
				new->next = NULL;
				//connect new into the sequence
				Block* temp = start->next;
				start->next = new;
				new->next = temp;
			}
			else {
				start->free = false;
			}	
		}
		//no space found and at end
		else {
			//amount to ask mmap for rounded up to the nearest page including meta data
			uint64_t pages = (int)ciel(((float)size+32)/((float)4096));
			Block* new = (Block*)mmap(NULL, pages*4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
			new->free = false;
			new->size = pages*4096;
			new->usable = new->size-32;
			new->next = NULL;
			start->next = new;
			//fill current node and make new node right after with leftover space~
			if(new->usable-size > 32) {
				//leftover space is start->size-size
				//take leftover space out
				new->usable -= new->size-size;
				new->size -= new->size-size;
				new->free = false;
				//create new block after w. new ptr which is prev block ptr + leftover
				Block* extra = new+new->size-size;
				extra->free = true;
				extra->size = new->size-size;
				extra->usable = extra->size-32;
				extra->next = NULL;
				//connect extra into the sequence
				new->next = extra;
			}
		}
}

//NOTE: Fix by removing payload field and instead putting the mmaps to intialize the Block* ptrs
//there is currently an error as the Block* don't have a memery
//you will simulate the memory as the actual pointers themselves will be at those memory starts
void *t_malloc(size_t size) {
	if(cur == NULL || start == NULL) {
		fprintf(stderr, "malloc without starting process");
	}
	if(cur == FIRST_FIT) {
		while(start->usable <= size || !start->free) {
			if(start->next == NULL) break;
			start = start->next;
		}
		loadIn(size, start);
		// //found space
		// if(start->usable >= size && start->free) {
		// 	//fill current node and make new node right after with leftover space~
		// 	if(start->usable-size > 32) {
		// 		//leftover space is start->size-size
		// 		//take leftover space out
		// 		start->usable -= start->size-size;
		// 		start->size -= start->size-size;
		// 		start->free = false;
		// 		//create new block after w. new ptr which is prev block ptr + leftover
		// 		Block* new = start+start->size-size;
		// 		new->free = true;
		// 		new->size = start->size-size;
		// 		new->usable = new->size-32;
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
		// 	uint64_t pages = (int)ciel(((float)size+32)/((float)4096));
		// 	Block* new = (Block*)mmap(NULL, pages*4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
		// 	new->free = false;
		// 	new->size = pages*4096;
		// 	new->usable = new->size-32;
		// 	new->next = NULL;
		// 	start->next = new;
		// 	//fill current node and make new node right after with leftover space~
		// 	if(new->usable-size > 32) {
		// 		//leftover space is start->size-size
		// 		//take leftover space out
		// 		new->usable -= new->size-size;
		// 		new->size -= new->size-size;
		// 		new->free = false;
		// 		//create new block after w. new ptr which is prev block ptr + leftover
		// 		Block* extra = new+new->size-size;
		// 		extra->free = true;
		// 		extra->size = new->size-size;
		// 		extra->usable = extra->size-32;
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
		while(start->next != NULL) {
			if(start->free && start->usable >= size) {
				curDiff = start->usable-size;
				if(curDiff < differential) {
					differential = curDiff;
					ptr = start;
				}
			}
			start = start->next;
		}
		//no match found so have to set to end
		if(ptr == NULL) {
			ptr = start->next;
		}
		loadIn(size, ptr);
	}
	else if(cur == WORST_FIT) {
		Block* ptr = NULL;
		int64_t differential = -1;
		int64_t curDiff;
		while(start->next != NULL) {
			if(start->next && start->usable >= size) {
				curDiff = start->usable-size;
				if(curDiff > differential) {
					differential = curDiff;
					ptr = start;
				}
			}
		}
		//no match found so have to set to end
		if(ptr == NULL) {
			ptr = start->next;
		}
		loadIn(size, ptr);
	}
	else {
		fprintf(stderr, "illegal enum val");
	}
}

void t_free(void *ptr) {
  	while(start!=ptr) {
		start = start->next;
		if(start == NULL) {
			fprintf(stderr, "tried free nonexisting pointer");
		}
	}
	start->free = true;
}
