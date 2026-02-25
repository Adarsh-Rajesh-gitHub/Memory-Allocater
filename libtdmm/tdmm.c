#include "tdmm.h"
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	//total amount containing metadata for payload and payload
	//size = used+8
	size_t size;
	//what got pulled for direct user needs
	size_t used;
	bool free;
	//ptr to memory
	void *payload;
	struct Block *next;
} Block;



alloc_strat_e cur = NULL;
Block* start = NULL;

void t_init(alloc_strat_e strat) {
	cur = strat;
	if(start == NULL) {
		//+8 for meta data and then +8 done to pointer to return actual start
		start->payload = mmap(NULL, 1000000+8, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0) + 8;
		start->free = true;
		start->size = 1000000+8;
		start->used = 1000000;
		start->next = NULL;
	}
	else {
		while(start != NULL) {
			start->free = true;
			start = start->next;
		}
	}
}

//NOTE: Fix by removing payload field and instead putting the mmaps ot intialize the Block* ptrs
//there is currently an error as the Block* don't have a memery
//you will simulate the memory as the actual pointers them selves will be at those memory starts
void *t_malloc(size_t size) {
	if(cur == NULL || start == NULL) {
		fprintf(stderr, "malloc without starting process");
	}
	if(cur == FIRST_FIT) {
		while(start->used >= size && start->free) {
			if(start->next == NULL) break;
			start = start->next;
		}
		//found space
		if(start->used >= size && start->free) {
			//fill current node and make new node right after with leftover space~
			if(start->used - size > 8) {
				//take leftover space out
				start->used -= start->used-size;
				start->size -= start->size-(size+8);
				start->free = false;
				//create new block after w. new ptr and leftover
				Block* new;
				new->payload = start->payload+size+8;
				new->free = true;
				new->size = start->size-(size+8);
				new->used = start->used-size-8;
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
			Block* new;
			//amount to ask mmap for rounded up to the nearest page including meta data
			u_int64_t pages = (int)ciel(((float)size+8)/((float)4096));
			new->payload = mmap(NULL, pages*4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0) + 8;
			new->free = false;
			new->size = pages*4096;
			new->used = pages*4096-8;
			new->next = NULL;
			start->next = new;
		}
	}
}

void t_free(void *ptr) {
  	// TODO: Implement this
}
