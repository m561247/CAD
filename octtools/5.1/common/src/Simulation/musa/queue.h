#ifndef QUEUE_H
#define QUEUE_H

typedef struct scheduleItem {
    int schedule;
} scheduleItem;

typedef struct queue {
    int size;
    int count;
    scheduleItem** array;
    int head;
    int tail;
    int mask;
} musaQueue;




extern void musaQueueInit();
extern void musaQueueResize();
extern void musaQueueEnq();
extern int musaQueueDeq();
extern void musaQueueClear();
extern int musaInQueue();
#endif

