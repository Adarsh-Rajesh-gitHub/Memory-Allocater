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
    struct Block *prev;
	struct Block *next;
} Block;

extern alloc_strat_e cur;
static Block* start = NULL;


void buddy_t_init(alloc_strat_e strat) {
    start = (Block*)mmap(NULL, 1 << 20, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    start->free = true;
	start->size = 1 << 20;
	start->usable = start->size - sizeof(Block);
	start->next = NULL;
    start->prev = NULL;
}

//helper for once node found to load in memory on, it handles loading in memeory and creating next block with leftover or case where more memory needed
void* buddyloadin(size_t size, Block* start) {
    if(start->usable > size && start->free) {
        //split block and make used block the one after
        if(start->size > size*2 + 2*sizeof(Block)) {
            while(start->size > size*2 + 2*sizeof(Block)) {
                Block* new = (Block*)(((char*)start) + start->size/2);
                new->free = true;   
                new->size = start->size/2;
                new->usable = new->size - sizeof(Block);
                start->size = start->size/2;
                start->usable = start->size - sizeof(Block);

                new->prev = start;
                new->next = start->next;
                start->next = new;

                start = start->next;
            }
            //fill new with the stuff
            start->free = false;
        }
        //case where just right amount(between .5 to 1x of space utilization)
        else {
            start->free = false;
        }
        return ((char*)start) + sizeof(Block);
    }
    //mmap and then call buddyt_malloc(recursive)
    else {
        //determine number of pages to ask
        uint64_t pages  = 1;
        while(pages * 4096 < sizeof(Block) + size) {
            pages <<= 1;
        }
        Block* new = (Block*)mmap(NULL, pages*4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        new->size = pages*4096;
        new->usable = new->size - sizeof(Block);
        new->free = true;
        new->next = start->next;
        new->prev = start;
        start->next = new;
        return buddyt_malloc(size);
    }
}

void* buddyt_malloc(size_t size) {
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

		return buddyloadin(size, ptr);
        //this should have either the smallest block that can fit the size or the end of the list if no block can fit the size
}

void buddyt_free(void *ptr) {
	Block* iter = start;
    while(iter != NULL && (((char*)iter) + sizeof(Block)) != ptr) {
        iter = iter->next;
    }
    if(iter == NULL) {
        fprintf(stderr, "tried free nonexisting pointer");
        return;
    }
	iter->free = true;
	//coalese

	Block* coalesce = iter;
    bool occurred = true;
	while(occurred) {
		//free checks and also if they are adjacent in memory
        occurred = false;
		if(coalesce->prev != NULL && coalesce->prev->free && coalesce->prev->size == coalesce->size && ((char*)coalesce-coalesce->size) == (char*)coalesce->prev) {
			// Block* attach = coalesce->prev->prev;
			// coalesce->size *=2;
			// coalesce->usable = coalesce->size-sizeof(Block);
			// coalesce->prev = attach;
            // attach->next = coalesce;
            // occurred = true;
            Block* attach = coalesce->next;
			coalesce->prev->size *=2;
			coalesce->prev->usable= coalesce->prev->size-sizeof(Block);
			coalesce->prev->next = attach;
            if(attach != NULL) attach->prev = coalesce->prev;
            coalesce = coalesce->prev;
            occurred = true;
		}
        if(coalesce->next != NULL && coalesce->next->free &&  coalesce->next->size == coalesce->size && ((char*)coalesce+coalesce->size) == (char*)coalesce->next) {
            Block* attach = coalesce->next->next;
			coalesce->size *=2;
			coalesce->usable = coalesce->size-sizeof(Block);
			coalesce->next = attach;
            if(attach != NULL) attach->prev = coalesce;
            occurred = true;
        }
	}
}