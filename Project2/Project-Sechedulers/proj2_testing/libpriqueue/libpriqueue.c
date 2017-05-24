/** @file libpriqueue.c
*/

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"


/**
  Initializes the priqueue_t data structure.

  Assumtions
  - You may assume this function will only be called once per instance of priqueue_t
  - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
  */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
    q->head = NULL; 
    q->size = 0; 
    q->comp = comparer;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
  */
int priqueue_offer(priqueue_t *q, void *ptr)
{
	int result, qindex;
	priqueue_entry_t *new_entry, *temp_entry; 

	/* Allocate memory for the new node */
    new_entry = malloc(sizeof(priqueue_entry_t));
    new_entry->entry_data = ptr;
    new_entry->next = NULL;

    q->size++;

	/* If queue is empty */
    if (q-> head == NULL) {
        q->head = new_entry;
		qindex = 0;
        return qindex;
    }

    result = q->comp(ptr, q->head->entry_data); 
    if (result < 0) {
        new_entry->next = q->head; 
        q->head = new_entry; 
        return 0; 
    }

    qindex = 1;
    for(temp_entry = q->head; temp_entry->next != NULL; temp_entry = temp_entry->next){
        result = q->comp(ptr, temp_entry->next->entry_data);
        if (result < 0) {
            new_entry->next = temp_entry->next; 
            temp_entry->next = new_entry; 
            return qindex; 
        }
        qindex++; 
    }

    temp_entry->next = new_entry; 
    return qindex; 
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
  */
void *priqueue_peek(priqueue_t *q)
{
    if (q->head == NULL) {
        return NULL;
    }
    else {
        return q->head->entry_data;
    }
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
  */
void *priqueue_poll(priqueue_t *q)
{
	priqueue_entry_t * temp_entry;
	void *qhead;

    if (q->head == NULL) {
        return NULL;
    } else {
        priqueue_entry_t * temp_entry = q->head; 
        q->head = q->head->next; 
        qhead = temp_entry->entry_data; 
        free(temp_entry); 
        (q->size)--;
        return qhead; 
    }
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
  */
void *priqueue_at(priqueue_t *q, int index)
{
	int i = 0;
    void *qentry = NULL;
	priqueue_entry_t * temp_entry;

    if (index >= q->size) {
        qentry = NULL;
    } else {
        for (temp_entry = q->head; i <= index; temp_entry = temp_entry->next) {
            if (i == index && temp_entry != NULL) {
                qentry =  temp_entry->entry_data;
                break;
            }
            i++;/*NB in for condition */
        }
    }
    return qentry; 
}


/**
  Removes all instances of ptr from the queue. 

  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
  */
int priqueue_remove(priqueue_t *q, void *ptr)
{
    priqueue_entry_t * temp_entry = q -> head; 
	void *removed_entry;
    int index = 0, entries_rem = 0, temp;

    while (temp_entry != NULL) {
        if(temp_entry->entry_data == ptr) {
            temp_entry = temp_entry->next; 
            removed_entry = priqueue_remove_at(q, index); 
            index--;
            entries_rem++;
        } else {
            temp_entry = temp_entry->next;
        }
        index++; 
    }
    return entries_rem;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
  */
void *priqueue_remove_at(priqueue_t *q, int index)
{
    void * ret_entry = NULL; 
    priqueue_entry_t * temp_entry;
    priqueue_entry_t * prev; /* Previous node to one to be deleted */
	priqueue_entry_t * curr;
    int i = 0;

    if (index >= q->size || index < 0 || (index == 0 && q->size ==0)) {
        ret_entry = NULL;
    } else if (index == 0) {
        /* Remove the first node */
        ret_entry = q->head->entry_data; 
        temp_entry = q->head; 
        q->head = q->head->next; 
        free(temp_entry);
        q->size--;
    } else {
		for (curr = q->head; i < index; i++) {
			prev = curr;
			curr = curr->next;
		}

		temp_entry = curr;

		if (temp_entry != NULL) {
			ret_entry = temp_entry->entry_data;
			prev->next = temp_entry->next;
			free (temp_entry);
		} else {
			ret_entry = NULL;
		}
		q->size--;
    }

    return ret_entry;
}


/**
  Returns the number of elements in the queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
  */
int priqueue_size(priqueue_t *q)
{
    return q->size;
}


/**
  Destroys and frees all the memory associated with q.

  @param q a pointer to an instance of the priqueue_t data structure
  */
void priqueue_destroy(priqueue_t *q)
{
    priqueue_entry_t * curr = q->head; 
    priqueue_entry_t * next = curr->next; 

    if (q == NULL) {
        return;
    }

    while (next != NULL) {
        free(curr);
        curr = next; 
        next = next->next;
    }

    free(curr);
}

/* Function to print the queue */
void priqueue_print(priqueue_t *q)
{
    priqueue_entry_t * curr = q->head; 
    priqueue_entry_t * next = curr->next; 

    if (q == NULL) {
        printf ("Queue is empty\n");
        return;
    }

    printf ("The entries in queue are:");
    while (next != NULL) {
        //printf ("0x%x ", (unsigned int)curr->entry_data);
        curr = next; 
        next = next->next; 
    }
}
