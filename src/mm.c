#include <string.h>
#include <stdio.h>
#include <unistd.h>


void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

typedef struct header {
    size_t size;
    void *curr;
    struct  header* next;
} header;





static header* FreeLists[13] = {NULL};

/* When requesting memory from the OS using sbrk(), request it in
 * increments of CHUNK_SIZE. */

/*
 * This function, defined in bulk.c, allocates a contiguous memory
 * region of at least size bytes.  It MAY NOT BE USED as the allocator
 * for pool-allocated regions.  Memory allocated using bulk_alloc()
 * must be freed by bulk_free().
 *
 * This function will return NULL on failure.
 */
extern void *bulk_alloc(size_t size);

/*
 * This function is also defined in bulk.c, and it frees an allocation
 * created with bulk_alloc().  Note that the pointer passed to this
 * function MUST have been returned by bulk_alloc(), and the size MUST
 * be the same as the size passed to bulk_alloc() when that memory was
 * allocated.  Any other usage is likely to fail, and may crash the
 * program.
 */
extern void bulk_free(void *ptr, size_t size);

/*
 * This function computes the log base 2 of the allocation block size
 * for a given allocation.  To find the allocation block size from the
 * result of this function, use 1 << block_size(x).
 *
 */
static inline __attribute__((unused)) int block_index(size_t x) {
    if (x <= 8) {
        return 5;
    } else {
        return 32 - __builtin_clz((unsigned int)x + 7);
    }
}

#define CHUNK_SIZE (1<<12)

void *malloc(size_t size) {
    if(size <= 0){
        return NULL;
    }

    size_t index = block_index(size);
    // printf("head %p\n", FreeLists[index]);

    if (size > 4088) {
        void *bulk = bulk_alloc(size + 8);
        header *b = (header *) bulk;
        b->size = size + 8;
        if( size%2 != 0){
            b->size = b ->size + 5;
        }
        b->curr = bulk + sizeof(header);
        b->next = NULL;
        return b->curr;
    }

    header *check = FreeLists[index];

    if (check) {
        header *curr1 = (header *) FreeLists[index];

        while(curr1 != NULL) {
            if (curr1->size == 1 << index) {

                curr1->size ^= 1;
                if (curr1->next == NULL){
                    FreeLists[index] = NULL;
                }
                else{
                    FreeLists[index] = curr1->next;
                }
                curr1->next = NULL;
                return curr1->curr;

            }
            curr1 = curr1->next;
        }


    }

    if (FreeLists[index] == NULL) {

        void *chunk = sbrk(CHUNK_SIZE);
        if (chunk == (void *) -1) {
            return NULL;
        }

        size_t block_size = 1 << index;
        int amount_blocks = CHUNK_SIZE / block_size;


        // first free node
        void *start = chunk;
        header *head = (header *) (start);
        head->size = block_size;
        //  printf("Size okay : %zu\n", head->size);

        //  printf("Size okay : %zu\n", head->size);

        void *help = chunk + sizeof(header);
        head->curr = help;
        head->next = NULL;

        FreeLists[index] = head;


        header *next = NULL;
        header *give = NULL;
        for (int i = 1; i < amount_blocks; i++) {
            // pointer in which it is where chunk starts in memory + block_size
            start = start + block_size;
            head = (header *) start;
            head->size = block_size ;
            help = start + sizeof(header);
            head->curr = help;
            head->next = next;
            next = head;
            if (amount_blocks - i  == 1){
                give = head;
            }

        }

        FreeLists[index]->next = give;
        return malloc(size);

    }

    return NULL;
}

/*
  Calloc should create allocations compatible with those created by malloc().  In particular, any
 * allocations of a total size <= 4088 bytes must be pool allocated,
 * while larger allocations must use the bulk allocator.
 *
 * calloc() (see man 3 calloc) returns a cleared allocation large enough
 * to hold nmemb elements of size size.  It is cleared by setting every
 * byte of the allocation to 0.  You should use the function memset()
 * for this (see man 3 memset).
 */
void *calloc(size_t nmemb, size_t size) {
    if(size ==  0  || nmemb ==  0){
        return NULL;
    }


    void *ptr = malloc(nmemb * size);
    memset(ptr, 0, nmemb * size);
    return ptr;

}

/*
 * Realloc should create allocations
 * compatible with those created by malloc(), honoring the pool
 * alocation and bulk allocation rules.  It must move data from the
 * previously-allocated block to the newly-allocated block if it cannot
 * resize the given block directly.  See man 3 realloc for more
 * information on what this means.

 */
void *realloc(void *ptr, size_t size) {
    if(size == 0 && ptr != NULL){
        free(ptr);
        return NULL;
    }

    if(ptr == NULL){
        return malloc(size);
    }


    void *update = ptr - sizeof(header);
    header *head = (header *)update;


    size_t s = head->size & ~1;

    if (size+8 <= s){
        return ptr;
    }

    void *new = malloc(size);

    if (new == NULL){
        return NULL;
    }

    memcpy(new,ptr,s);
    free(ptr);

    return new;


}


/*
 * free()successfully free a region of
 * memory allocated by any of the above allocation routines, whether it
 * is a pool- or bulk-allocated region.
 *
 */

void free(void *ptr) {

    if(ptr != NULL){

        void *update = ptr - sizeof(header);
        header *up = (header *)update;



        if(up->size  <= 4097){
            size_t check = block_index(up->size - 8);
            if(up->size != 1 << check){
                up->size = up->size & ~1;
                size_t index = block_index(up->size-8);
                up->next = FreeLists[index];
                FreeLists[index] = up;
            }
        }

        if(up->size > 4097){
            if((up-> size - 5) %2 != 0){
                up->size = up-> size - 5;
            }

            bulk_free(ptr-sizeof(header), up->size);
        }

    }
}
