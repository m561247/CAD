#include "copyright.h"
#include "port.h"
#include "utility.h"
#include "musa.h"
#include "queue.h"

void musaQueueInit( q , number)
    musaQueue* q;
    int number;
{
    q->count = 0;
    q->size = 100;
    q->array = ALLOC( scheduleItem*, q->size );
    q->tail = 0;
    q->head = 0;
    q->mask = 1 << number;
}

void musaQueueResize( q )
    musaQueue* q;
{
    scheduleItem* item = 0;
    int newsize = q->size * 2;
    scheduleItem** tmparray = ALLOC( scheduleItem*, newsize );
    int    count = 0;

    while ( musaQueueDeq(  q, &item ) ) {
	tmparray[count++] = item;
    }
    q->head = 0;
    q->tail = count;
    q->count = count;
    FREE( q->array );
    q->array = tmparray;
    q->size = newsize;
}

void musaQueueEnq( q,  item )
    musaQueue* q;
    scheduleItem* item;
{
    if ( musaInQueue( q, item ) ) return; /* Already in queue. */
    if ( q->count >= q->size ) {
	musaQueueResize( q );
    }
    q->array[q->tail++] = item;
    if ( q->tail == q->size ) q->tail = 0;
    item->schedule |= q->mask;
    q->count++;
}

int musaQueueDeq( q, item )
    musaQueue* q;
    scheduleItem** item;
{
    if ( q->count == 0 ) return 0;	/* No elements */
    *item = q->array[q->head++];
    if ( q->head == q->size ) q->head = 0;
    q->count--;
    (*item)->schedule &= ~q->mask;
    return 1;
}


int musaInQueue( q, item )
    musaQueue* q;
    scheduleItem* item;
{
    return  ( item->schedule & q->mask );
}


void musaQueueClear( q )
    musaQueue* q;
{
    while ( q->count ) {
	scheduleItem* item;
	musaQueueDeq( q , &item );
    }
    q->head = q->tail = 0;
}
