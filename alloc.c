#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

struct obj_metadata {
    size_t size;
    struct obj_metadata *prev, *next;
    int is_free; //0 if taken, 1 if free
};

struct obj_metadata *heap_start = NULL; //pointer to address of the start of the heap
struct obj_metadata *heap_end = NULL;
void *freelist = NULL;


size_t alignedSize(size_t size) { //align size to sizeof(long) (8 bytes)
    if (size % 8 != 0) {
        size += (8 - (size % 8));
    }
    return size;
}
void addToFreelist(struct obj_metadata *block) {
    if (!freelist) { //No freelist available yet
        block->prev = NULL;
        block->next = NULL;
        block->is_free = 1;
        freelist = block;
    }
    else { //insert at the start of freelist
        struct obj_metadata *freeblock = freelist;
        freeblock->prev = block;
        block->next = freeblock;
        block->prev = NULL;
        freelist = block;
    }
}
//Initialize node, block->size only includes the available space, not sizeof(struct)
struct obj_metadata *initNode(size_t size) {
    struct obj_metadata* block;
    size_t reqsize = alignedSize(size) + sizeof(struct obj_metadata);
    block = sbrk(reqsize); //Allocate space for required size + structure size
    block->size = alignedSize(size); //Only space not structure!!!
    block->prev = NULL;
    block->next = NULL;
    block->is_free = 0;
    return block;
}

struct obj_metadata *FindBlock(size_t reqsize) {
    if (!freelist) return NULL;
    struct obj_metadata *freeblock = freelist;
    while (freeblock && freeblock->next) { //Loop through
        if (freeblock->is_free == 1 && (freeblock->size > sizeof(struct obj_metadata) + alignedSize(reqsize))) {
            //Set new address for free block with remaning sie
            struct obj_metadata *tail = (struct obj_metadata *)(((char *) freeblock) + alignedSize(reqsize) + sizeof(struct obj_metadata));
            tail->is_free = 1;
            tail->size = alignedSize(freeblock->size - alignedSize(reqsize) - sizeof(struct obj_metadata));
            addToFreelist(tail); //Insert new free block in the freelist
            //Update values in allocated block
            freeblock->size = alignedSize(reqsize);
            freeblock->is_free = 0;
            return freeblock;
        }
        if (freeblock->is_free == 1 && alignedSize(reqsize) <= freeblock->size) {
            freeblock->is_free = 0;
            return freeblock;
        }
        freeblock = freeblock->next;
    }
    return NULL;
}

void *mymalloc(size_t size) {
    struct obj_metadata *block;
    if (size <= 0) {
      return NULL;
    }
    block = FindBlock(size); //Try to find a block in the freelist
    if (block) return block + 1; //If block was found return allocated space
    else (block = initNode(size)); //Create a new node when there is no right block available in the freelist

    if (!heap_start) { //Heap empty
        heap_start = block;
        heap_end = heap_start;
        return heap_start + 1;
    }
    else { //Append block to the end of the heap
        heap_end->next = block;
        block->prev = heap_end;
        heap_end = block;
    }
    return block + 1; //point to allocated space
}

void *mycalloc(size_t nmemb, size_t size) {
    size_t blocksize = alignedSize(size) * nmemb;
    void *ptr = mymalloc(blocksize);
    memset(ptr, 0, blocksize);
    return ptr;
}

void mergeBlocks(struct obj_metadata *block) {
    block->size = block->size + block->next->size + sizeof(struct obj_metadata); //Combine the two block sizes, size of struct gets included
    block->next = block->next->next; //Update linked-list links
    if (block->next) block->next->prev = block;
    else heap_end = block;
}

void myfree(void *ptr) {
    if (!ptr || !heap_start) return;
    struct obj_metadata *block = (struct obj_metadata*) ptr - 1; //-1 since the address points to the allocated space, we need to point to the metadataheader
    block->is_free = 1;
    if(block->next && block->next->is_free) mergeBlocks(block);
    addToFreelist(block);

}

void *myrealloc(void *ptr, size_t size) {
    if(!ptr) return mymalloc(size);
    if (ptr && size == 0) {
        myfree(ptr);
        return NULL;
    }
    struct obj_metadata *block = (struct obj_metadata *)ptr - 1;
    if (block->size >= alignedSize(size)) return ptr; //If size is smaller than the current block
    void *newptr = mymalloc(size);
    memcpy(newptr, ptr, block->size); //Copy contents of old allocated block into new block
    return newptr;
}

/*
 * Enable the code below to enable system allocator support for your allocator.
 * Doing so will make debugging much harder (e.g., using printf may result in
 * infinite loops).
 */

#if 0
void *malloc(size_t size) { return mymalloc(size); }
void *calloc(size_t nmemb, size_t size) { return mycalloc(nmemb, size); }
void *realloc(void *ptr, size_t size) { return myrealloc(ptr, size); }
void free(void *ptr) { myfree(ptr); }
#endif
