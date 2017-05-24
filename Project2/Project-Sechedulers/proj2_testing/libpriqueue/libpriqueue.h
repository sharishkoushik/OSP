/** @file libpriqueue.h
*/

#ifndef LIBPRIQUEUE_H_
#define LIBPRIQUEUE_H_


/* priqueue_node_t */
typedef struct _priqueue_entry_t
{
    void * entry_data;    /* Will be used as pointer to any data type (struct job_t in our case) */ 
    struct _priqueue_entry_t * next;
} priqueue_entry_t;


/**
  Priqueue Data Structure
  */
typedef struct _priqueue_t
{
    priqueue_entry_t * head; 
    int size; 
    int(*comp)(const void *, const void *);
} priqueue_t;


void   priqueue_init     (priqueue_t *q, int(*comparer)(const void *, const void *));

int    priqueue_offer    (priqueue_t *q, void *ptr);
void * priqueue_peek     (priqueue_t *q);
void * priqueue_poll     (priqueue_t *q);
void * priqueue_at       (priqueue_t *q, int index);
int    priqueue_remove   (priqueue_t *q, void *ptr);
void * priqueue_remove_at(priqueue_t *q, int index);
int    priqueue_size     (priqueue_t *q);

void   priqueue_destroy  (priqueue_t *q);

/* Function to print the queue */
void priqueue_print(priqueue_t *q);

#endif /* LIBPQUEUE_H_ */
