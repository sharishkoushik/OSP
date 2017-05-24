/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
//#define MIN_ORDER 12  /* NB 2 ^ 12 = 4K */
#define MIN_ORDER 12    /* NB 2 ^ 16 = 64K */
#define MAX_ORDER 20

//#define MIN_ORDER 10
//#define MAX_ORDER 14

#define PAGE_SIZE (1<<MIN_ORDER) /* 2 ^ 12 = 4096 (4K), 2 ^ 16 = 65536(64K) */
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
        + (unsigned long)g_memory)

#if 0
fprintf(stderr, "%s(), %s:%d: " fmt,			\
        __func__, __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
    fprintf(stderr, " " fmt,			\
##__VA_ARGS__)

#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
    struct list_head list;

    /* TODO: DECLARE NECESSARY MEMBER VARIABLES */
    int page_idx;       /* 0 - 255 */    
    char *page_addr;    /* page start address in g_memory */
    int block_order;    /* free block order to which the page belongs eg 16, 17 etc */
    int page_free;      /* Page is free or not */    
    //	  int block_size;     /* free block size to which the page belongs eg 64k, 128k etc */    
    //    int block_start_pg_index; /* free block start page index */
    //    int block_end_pg_index; /* free block end page index */

} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER]; /* 1048576 1024K */

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE]; /* All pages occupy g_memory */

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

void buddy_init();
void *buddy_alloc(int size);
void buddy_free(void *addr);

void buddy_dump();
void buddy_dump2();

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**
 * Initialize the buddy system
 */
void buddy_init()
{
    int i;
    int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;/*256*/

    for (i = 0; i < n_pages; i++) {
        /* TODO: INITIALIZE PAGE STRUCTURES */
        /* 	struct list_head list */
        g_pages[i].page_idx = i;
        g_pages[i].page_free = 1;
        g_pages[i].page_addr = &g_memory[PAGE_SIZE*i];
    }

    /* initialize freelist */
    for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
        INIT_LIST_HEAD(&free_area[i]);
    }

    /* add the entire memory as a freeblock */
    list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
    g_pages[0].block_order = MAX_ORDER;
    g_pages[0].page_free = 1;

    /* Fill entries in free pages */
    for (i = 0; i < (1<<MAX_ORDER)/PAGE_SIZE; i++) {
        g_pages[i].page_free = 1;
    }

    /*debug - start */
    PDEBUG ("=== free_area[] 0x%x, entries %d\n", free_area, MAX_ORDER);
    for (i = 12; i <= MAX_ORDER; i++) {
        PDEBUG("free_area[%d] = 0x%x =next==> 0x%x\n", i, &free_area[i], free_area[i].next);
    }
    PDEBUG("=== g_pages 0x%x, page_t size %d, entries %d\n", g_pages, sizeof(page_t), (1<<MAX_ORDER)/PAGE_SIZE);
    for (i = 0; i < ((1<<MAX_ORDER)/PAGE_SIZE); i++) {
        PDEBUG("g_pages[%d]=0x%x\n", i, &g_pages[i]);
    }
    PDEBUG("=== g_memory[] 0x%x\n", g_memory);
    PDEBUG("===\n"); 
    /*debug - end */

}

/**
 * Allocate a memory block.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
    /* TODO: IMPLEMENT THIS FUNCTION */
    /* Debug */
    page_t *list_entry_page;
    int high, low, page_idx, page_idx_adjust, split_list_count, pg_idx_first;
    int block_order, i, j;
    unsigned long resize;
    unsigned long ret;

    if (size <= (1<<MIN_ORDER)) {
        block_order = MIN_ORDER;
    } else if (size > (1<<MIN_ORDER) && size < (1 << MAX_ORDER)) {
        for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
            if (size < (1<<i)) {/* i will be the right order to allocate */
                block_order = i;
                break;
            }
        }
    } else {
        PDEBUG ("Requested allocation cannot be met\n");
        ret = (unsigned long)NULL;
    }

    /* Free memory block of size of order block_order is needed 
     * Loop through all free lists(from order 12 to 20) to find
     * available list having equal to block_order.
     * If space is avaiable in that list then allocated ELSE
     * we split a larger block as many times as necessary 
     * to get a block of a suitable size
     * */

    for (j = block_order; j <= MAX_ORDER; j++) {
        if (list_empty(&free_area[j])) {
            PDEBUG ("Free area of order [%d] not available. Check next higher order \n", j);/* If exact block is free;then allocate */
            continue;
        }

        /* Number to times to split list to create block size of block_order */
        split_list_count = j - block_order;

        /* Get page_t having this list head */
        list_entry_page = list_entry(free_area[j].next, page_t, list);
        pg_idx_first = list_entry_page->page_idx;/* Index of first available free page of the block */

        /* remove from free list */
        list_entry_page->page_free = 0;
        list_del(&list_entry_page->list);

        high = j;
        low = block_order;
        resize = 1 << high;/* Total block size to resize */
        while (high > low) {
            high--;
            resize >>= 1;/* Size after divide by 2 */

            PDEBUG("high %d, resize %d(offset in g_memory), page index %d\n", high, resize, ADDR_TO_PAGE((g_memory + resize)));

            page_idx = resize/PAGE_SIZE + pg_idx_first;/*no. of pages in divided size*/
            
            /* Divide block into two and add right part to free list */
            list_add(&g_pages[page_idx].list, &free_area[high]);
            g_pages[page_idx].block_order = high;
            g_pages[page_idx].page_free = 1;
        }

        /* Calculate page memory to return */
        if (split_list_count) {
            page_idx_adjust = resize/PAGE_SIZE;
            g_pages[page_idx - page_idx_adjust].block_order = block_order;
            g_pages[page_idx - page_idx_adjust].page_free = 0;
            ret = (unsigned long)g_pages[page_idx - page_idx_adjust].page_addr;
        } else {
            g_pages[pg_idx_first].block_order = block_order;
            g_pages[pg_idx_first].page_free = 0;
            ret = (unsigned long)g_pages[pg_idx_first].page_addr;
        }

        break; /* while loop */
    }

    PDEBUG("Page address to return 0x%x, resize %d\n", ret, resize);  
    return (void *)ret;
}

/**
 * Free an allocated memory block.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
    /* TODO: IMPLEMENT THIS FUNCTION */
    page_t *page, *list_entry_page;
    int order, page_idx, pg_order, buddy_idx, buddy_order, buddy_free, coalesced_idx;

    page_idx = ADDR_TO_PAGE(addr);
    page = &g_pages[page_idx];
    pg_order = page->block_order;

    order = pg_order;
    while (order < (MAX_ORDER)) {
        /* checks for the right buddy */
        PDEBUG("Get the buddy index\n");
        /* B2 = B1 XOR (1 << order) MOST IMPORTANT LOGIC OF BUDDY ALGORITHM to find buddy given
         * starting page index of block B1 and size of block in power of 2(order). 
         * B1 = page index of block B1, order = (size of block B1 in power of 2 / page size)
         * B2 = index of buddy block B2 */
        buddy_idx = page_idx ^ ((1<<order)/PAGE_SIZE);
        buddy_order = g_pages[buddy_idx].block_order;
        buddy_free = g_pages[buddy_idx].page_free;

        PDEBUG ("\npage_idx = %d, pg_order = %d, Right: buddy_idx = %d, buddy_order = %d, buddy_free = %d\n", 
                   page_idx, order, buddy_idx, buddy_order, buddy_free);

        /*if not buddy break from while loop */
        if (!((buddy_order == order) && buddy_free)) {
            PDEBUG("Buddy with index buddy_idx not free\n", buddy_idx);
            break;
        }

        list_entry_page = list_entry(free_area[buddy_order].next, page_t, list);
        /* Free buddy page if free from free_area[buddy_order] list */
        list_del(&list_entry_page->list);

        coalesced_idx = buddy_idx & page_idx;
        page = page + (coalesced_idx - page_idx);
        page_idx = coalesced_idx;
        /* Adjust order and free flag of combined large page */
        g_pages[page_idx].block_order = order + 1; /* next higher */
        g_pages[page_idx].page_free =  1;

        order++;
        PDEBUG ("Coalesced: coalesced_idx %d, page 0x%x, order %d\n", coalesced_idx, page, order);
    }

    list_add(&g_pages[page_idx].list, &free_area[order]);
    g_pages[page_idx].page_free = 1;
}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
    int o;
    for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
        struct list_head *pos;
        int cnt = 0;
        list_for_each(pos, &free_area[o]) {
            cnt++;
        }
        printf("%d:%dK ", cnt, (1<<o)/1024);
    }
    printf("\n");
}


int main(int argc, char *argv[])
{
    char *A, *B, *C, *D;
    char *E, *F, *G, *H, *I, *J, *K, *L, *M, *N, *P;
    

    buddy_init();


    A = buddy_alloc(80*1024);
    buddy_dump();


    B = buddy_alloc(60*1024);
    buddy_dump();

    C = buddy_alloc(80*1024);
    buddy_dump();

    buddy_free(A);
    buddy_dump();

    D = buddy_alloc(32*1024);
    buddy_dump();

    buddy_free(B);
    buddy_dump();

    buddy_free(D);
    buddy_dump();

    buddy_free(C);
    buddy_dump();

#if 0
    buddy_dump();

    A = buddy_alloc(3.6*1024);
    buddy_dump();

    B = buddy_alloc(1.5*1024);
    buddy_dump();

    C = buddy_alloc(1.2*1024);
    D = buddy_alloc(1.9*1024);
    E = buddy_alloc(2.7*1024);
    buddy_dump();

    buddy_free(C);
    buddy_dump();

    buddy_free(B);
    buddy_dump();

    F = buddy_alloc(1.5*1024);
    G = buddy_alloc(1.6*1024);
    buddy_dump();

    buddy_free(D);
    buddy_dump();

    printf("\nfree A\n");
    buddy_free(A);
    buddy_dump();

    buddy_free(G);
    buddy_dump();

    H = buddy_alloc(6.8*1024);
    buddy_free(E);
    I = buddy_alloc((850/1024)*1024);
    buddy_dump();

    J = buddy_alloc((610/1024)*1024);
    K = buddy_alloc(1.6*1024);
    L = buddy_alloc((750/1024)*1024);
    buddy_free(H);
    buddy_dump();

    M = buddy_alloc(3.9*1024);
    N = buddy_alloc(1.7*1024);
    buddy_free(L);
    printf("free L\n");
    buddy_dump();
    buddy_free(F);
    buddy_dump();

    buddy_free(K);
    P = buddy_alloc((670/1024)*1024);
    buddy_free(I);
    buddy_dump();

    buddy_free(N);
    buddy_free(J);
    buddy_dump();
#endif

    return 0;
}
