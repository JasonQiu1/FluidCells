#ifndef INTPAIRQUEUE_H
#define INTPAIRQUEUE_H

#include "intpair.h"

// FIFO - push to back, pop from front
// - front pointer is on the next element to pop
// - back is outside of the queue on the next place to push an element
// not push-safe
// - everything will get screwed up if you push over maxsize elements 
typedef struct intpairqueue {
    int maxsize;
    intpair* array;
    intpair* front;
    intpair* back;
} intpairqueue;

intpairqueue* createintpairqueue(int maxsize) {
    intpairqueue* newqueue = malloc(sizeof *newqueue);
    newqueue->maxsize = maxsize;
    newqueue->array = malloc(maxSize * sizeof *newqueue->array);
    newqueue->front = array;
    newqueue->back = array;
    return newqueue; 
}

// Assume back is always in a valid push position
void pushintpairqueue(intpairqueue* queue, intpair pair) {
    *queue->back = pair;
    if (queue->back - queue->array == maxsize - 1) {
        queue->back = queue->array; 
    } else {
        queue->back++;
    }
}

intpair popintpairqueue(intpairqueue* queue) {
    if (queue->front - queue->array == maxsize - 1) {
        queue->front = queue->array;
        return queue->array + maxsize - 1;
    }
    return queue->front++;
}

void clearintpairqueue(intpairqueue* queue) {
    queue->front = queue->array;
    queue->back = queue->array;
}

int haselementintpairqueue(intpairqueue* queue) {
    return queue->front != queue->back;
}

#endif
