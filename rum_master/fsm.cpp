/*
 * Memory management functions
 *
 * @author paul harvey
 *
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "fsm.h"
#include "util.h"
#include <pthread.h>
#include <mutex>

//extern "C" {

/*============================================================================*/
#define DEBUG               1
#define UINT(a)             ((unsigned int)a)
#define UINT64(a)             ((uint64_t)a)

// To print out information to track memory usage over time, define MEMORY_USAGE_TRACK when compiling

//#define MEMORY_USAGE_TRACK
//#define FRAGMENTATION_TRACK
#define DIAGNOSTICS


#ifdef DIAGNOSTICS
#define DEBUG_MEM 1
static unsigned allocated = 0;
static unsigned available = 0;
#else
#define DEBUG_MEM 0
#endif

#if DEBUG 
static void *last_max_malloc;
#endif
/*============================================================================*/
static std::mutex       memory_mutex;    // controlling acces to channels
//static std::mutex       Attr;

#if DEBUG
static unsigned int     locked = 0;
#endif
/*============================================================================*/
typedef void* Align; //used to allign to the WORD bit boundary (nxt platform)

union header {
    struct {
        union header *ptr; //next on the free list
        size_t size; //size of the block
    } s;
    Align x; //used to align
};

typedef union header Header;

static Header base;
static Header *freep = NULL;

// Heap memory needs to be aligned to 4 bytes on ARM (NXT)
#define WORD_SIZE 4
#define WORD_SHIFT 2
#define TO_WORDS(b) (((b) + (WORD_SIZE-1)) >> WORD_SHIFT)
#define MEM_ROUND(w) (((w) + (WORD_SIZE-1)) & ~(WORD_SIZE-1))
#define MEM_TRUNC(w) ((w) & ~(WORD_SIZE -1)) 

// assumes that uint64_t is the same size as a void*
static uint64_t heapStart;
static uint64_t heapEnd;
static char setup = 0;
/*============================================================================*/
inline void memory_module_lock(){
    memory_mutex.lock();
#if DEBUG
    PRINTF("mLock: %d\n", locked);
    locked++;
#endif
}
/*============================================================================*/
inline void memory_module_unlock(){
#if DEBUG
    PRINTF("mUnock: %d\n",locked);
    locked--;
#endif
    memory_mutex.unlock();
}
/*============================================================================*/
void mem_init(void* start, void* end){

  memory_module_lock();
  //thread_mutex_attr_init(&Attr);
  //thread_mutex_attr_settype(&Attr, THREAD_MUTEX_RECURSIVE);
  //thread_mutex_init(&memory_mutex, &Attr);

  /* align upwards */
  //heapStart = (void*) MEM_ROUND((unsigned int)start);

  // we add 1 so as to not start at 0 ( 0 == NULL)
  //heapStart = (void*) MEM_ROUND((unsigned int)start + 1);
  heapStart = (uint64_t)start;

  /* align downwards */
  //heapEnd = (void*) MEM_TRUNC((unsigned int)end);
  heapEnd = (uint64_t)end;

  PRINTF("start: %lu\n", start);
  PRINTF("end  : %lu\n", end);

  PRINTF("Mem-init\n");
  PRINTF("free %lu bytes\n", UINT64(heapEnd)-UINT64(heapStart));

  // so the heap size is end - start
  // lets give this space to malloc then
  Header* heap = (Header*)heapStart;
  heap->s.size = ((heapEnd - heapStart) + sizeof(Header) - 1) / sizeof(Header) + 1;

  // try and fail to allocate
  mem_base_alloc(10);

  // then fill the heap
  //PRINTF("first_time_malloc\n");
  //base.s.ptr = freep = (Header*)heapStart;
  //base.s.size = ((heapEnd - heapStart) + sizeof(Header) - 1) / sizeof(Header) + 1;
  mem_base_free((void*) (heap + sizeof(Header)));
    
    
#if DEBUG_MEM
  allocated = 0;
  available = heap->s.size;
#endif
  PRINTF("mem_setup\n");
  setup = 1;
  // by this point, the heap should be a available to malloc
  memory_module_unlock();
}
/*============================================================================*/
void* mem_base_alloc(uint64_t nbytes){
    Header *p, *prevp;
    unsigned nunits;

    memory_module_lock();

#if DEBUG_MEM
    PRINTF("avail %d, free %d\n", available, allocated);
    PRINTF("mem_base_alloc\n");
#endif
    nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
    

#if DEBUG_MEM
    PRINTF("MALLOC: %d units %d bytes\n", nunits, nbytes);
    unsigned tmp = ((nunits -1) * sizeof(Header)) + 1  - sizeof(Header);
    PRINTF("MALLOC: header = %d, nbytes %d\n", sizeof(Header), tmp);
#endif
    
#if DEBUG_MEM
    if(setup){
        printf("Malloc: %d(%d + %d - 1)/%d(%d + 1) = %d\n",(nbytes + sizeof(Header) - 1),nbytes,sizeof(Header),sizeof(Header)+1, sizeof(Header), nunits  );
        printf("Malloc: units=%d, bytes=%d \n", nunits, nbytes);
    }
#endif

    if ((prevp = freep) == NULL) //need to initalise the free list
    {
        PRINTF("first_time_malloc\n");
        base.s.ptr = freep = prevp = &base;
        base.s.size = 0;
    }

#if DEBUG_MEM
    if (setup) {
        Header *j;

        printf("Malloc: prevP : %p\n", prevp);
        printf("Malloc: freep : %p\n", freep);
        printf("******************pre-Malloc*********************\n");
        for (j = prevp->s.ptr; j != freep; prevp = j, j = j->s.ptr)
            printf("Malloc: cur (%p) | Sz %ld | nxt (%p)\n", j, j->s.size, j->s.ptr);
        printf("Malloc: cur (%p) | Sz %ld | nxt (%p)\n", j, j->s.size, j->s.ptr);
        printf("*************************************************\n");
        prevp = freep;
    }
#endif
    for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
        PRINTF("alloc: lk fr slt\n");
        if (p->s.size >= nunits) //found a big enough slot
        {
            PRINTF("Mlc: Fnd slot %u @ %p for request %u\n", p->s.size, p->s.ptr, nunits);
            //PRINTF("Mlc: found slot\n");
            //PRINTF("%u\n", p->s.size);
            if (p->s.size == nunits){ //exactly fits
                //PRINTF("Malloc: found exact\n");
                prevp->s.ptr = p->s.ptr;
            } else {
                //PRINTF("Malloc: Make new sized block\n");
                p->s.size -= nunits;
                //PRINTF("mloc: make space\n");
                p += p->s.size;
                //PRINTF("mloc: mv ptr\n");
                p->s.size = nunits;
                //PRINTF("mloc: set nw sz\n");
                //PRINTF("Malloc: Make new sized block\n");
            }

            freep = prevp;
#if DEBUG
            printf("******************post-Malloc********************\n");
            Header *j;
            for (j = prevp->s.ptr; j != freep; prevp = j, j = j->s.ptr)
                printf("Malloc: current (%p) | Size %ld | next (%p)\n", j, j->s.size, j->s.ptr);
            printf("Malloc: current (%p) | Size %ld | next (%p)\n", j, j->s.size, j->s.ptr);
            printf("*************************************************\n");
            prevp = freep;
            printf("Malloc: allocate (%p) | Size %ld | next (%p)\n", p, p->s.size, p->s.ptr);
#endif
            
#if DEBUG_MEM
            if(setup){
                allocated += p->s.size;
                available -= p->s.size;
            }
#endif
            memory_module_unlock();
            return (void *) (p + sizeof(Header));
        }
        if (p == freep){ //wrapped around list without finding anything
            memory_module_unlock();
            ERROR("Malloc: wrapped and found nothing\n");
            return NULL;
        }
    }
}
/*============================================================================*/
void mem_base_free(void* ap){
    PRINTF("mem_base_free\n");
    Header *bp, *p;
    
    if(ap == NULL){
        return;
    }
#if DEBUG_MEM
    PRINTF("avail %d, free %d\n", available, allocated);
#endif
    
    memory_module_lock();
    bp = (Header *) ap - sizeof(Header); //the -1 points us back at the header of this block
    
#if DEBUG
    if(setup){
        printf("            *****                       ******\n");
        printf("Free: FREEYING: (%p) | Size %d | next (%p)\n", bp, bp->s.size, bp->s.ptr);
        printf("            *****                       ******\n");
    }
#endif
#if DEBUG
    Header *j, *prevp;
    prevp = freep;
    printf("******************pre-Free***********************\n");
    for (j = prevp->s.ptr; j != freep; prevp = j, j = j->s.ptr)
        printf("FREE: current (%p) | Size %ld | next (%p)\n", j, j->s.size, j->s.ptr);
    printf("FREE: current (%p) | Size %ld | next (%p)\n", j, j->s.size, j->s.ptr);
    printf("*************************************************\n");
    prevp = freep;
#endif
    
    //go through and find where this block should fit in
    for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) {
        PRINTF("Free: current(%u), next (%u) looking at (%u)\n", p ,p->s.ptr, bp);
        PRINTF("%u && ( %u || %u)\n",p >= p->s.ptr, bp>p, bp < p->s.ptr);
#if DEBUG
        if (bp == p) {
            PRINTF("We seems to have found space which is already free\n");
            PRINTF("Free: current   : (%p) | Size %ld | next (%p)\n", p, p->s.size, p->s.ptr);
            PRINTF("Free: allocated : (%p) | Size %ld | next (%p)\n", bp, bp->s.size, bp->s.ptr);
            while (1);
        }
#endif
        if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
           break; //freed block at start or end
    }
#if DEBUG_MEM
    if(setup){
        allocated -= bp->s.size;
        available += bp->s.size;
    }
    PRINTF("FREE: picked (%p) | Size %ld | next (%p)\n", j, j->s.size, j->s.ptr);
#endif
    //if we have the top half of a block, combine with the following one,
    //else just add us in
    if (bp + bp->s.size == p->s.ptr) {
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
        PRINTF("Free: can combine\n");
    } else {
        bp->s.ptr = p->s.ptr;
        PRINTF("Free: just add to list\n");
    }

    //if we have the rest of this block (p), combine with bp, else add to list
    if ((p + p->s.size) == bp) {
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
        PRINTF("Free: can combine\n");
    } else {
        p->s.ptr = bp;
        PRINTF("Free: Add to list\n");
    }

    // printf("3\n");
    freep = p;
    memory_module_unlock();
    //enable_preemption();
#if DEBUG
    prevp = freep;
    PRINTF("******************post-compact-Free***********************\n");
    for (j = prevp->s.ptr; j != freep; prevp = j, j = j->s.ptr)
        PRINTF("FREE: current (%p) | Size %ld | next (%p)\n", j, j->s.size, j->s.ptr);
    PRINTF("FREE: current (%p) | Size %ld | next (%p)\n", j, j->s.size, j->s.ptr);
    printf("*************************************************\n");
#endif
}
/*============================================================================*/
void* mem_base_realloc(void* p, unsigned int size){
    void* new_ptr;
    
    // just like malloc
    if(p == NULL){
        return mem_base_alloc(size);
    }
    
    // we free the pointer
    if(p != NULL && size == 0){
        mem_base_free(p);
        return NULL;
    }
    
    // otherwise we have to try and find size bytes at the end of p
    printf("realloc!!!!\n");
    
    // If size is smaller than the current memory
    if(size <= ((Header*)p - sizeof(Header))->s.size){
        return p;
    }
    
    new_ptr = mem_base_alloc(size);
    memcpy(new_ptr, p, size);
    mem_base_free(p);
    
    return new_ptr;
}
/*============================================================================*/
/*============================================================================*/
//} /*extern c*/