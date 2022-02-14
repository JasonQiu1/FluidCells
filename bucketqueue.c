#include <stdlib.h>

#include "bucketqueue.h"

// Returns a malloced bucket queue. The caller must call delBucketQueue on it.
BucketQueue* createBucketQueue(int nmBucketsMax, int bucketSizeMax) {
    BucketQueue* newBQ = malloc(sizeof *newBQ);
    newBQ->nmBucketsMax = nmBucketsMax;
    newBQ->buckets = malloc(nmBucketsMax * sizeof *newBQ->buckets); 

    newBQ->bucketSizeMax = bucketSizeMax;
    newBQ->bucketLens = malloc(bucketSizeMax * sizeof *newBQ->bucketLens);
    for (int i = 0; i < nmBucketsMax; i++) {
        newBQ->buckets[i] = malloc(bucketSizeMax * sizeof **newBQ->buckets);
        newBQ->bucketLens[i] = 0;
    }

    return newBQ;
}

// Inserts an entry with priority to bq if unique.
// Returns 0 if bad priority, duplicate found, or no more space. 1 otherwise.
int bucketQueueInsert(void* e, int priority, 
                      BucketQueue* bq, int(*cmp)(void*,void*)) 
{
    if (priority < 0 || priority >= bq->nmBucketsMax || 
        bq->bucketLens[priority] >= bq->bucketSizeMax) 
    {
        return 0;
    }

    for (int i = 0; i < bq->bucketLens[priority]; i++) {
        if (!cmp(e, bq->buckets[priority][i])) {
            return 0;
        }
    }

    bq->buckets[priority][bq->bucketLens[priority]++] = e;
    return 1;
}

// Removes the last entry of the first nonempty bucket. The caller must free it.
// Returns NULL if no more entries. The min entry otherwise.
void* bucketQueueExtractMin(BucketQueue* bq) {
    for (int i = 0; i < bq->nmBucketsMax; i++) {
        if (bq->bucketLens[i] > 0) {
            return bq->buckets[i][--bq->bucketLens[i]];
        }
    }
    return NULL;
}

// Frees everything in an EMPTY bucket queue.
// Returns 0 if bq is not empty. Returns 1 otherwise.
int delBucketQueue(BucketQueue* bq) {
    for (int i = 0; i < bq->nmBucketsMax; i++) {
        if (bq->bucketLens[i] > 0) {
            return 0;
        }
    }
    free(bq->bucketLens);

    for (int i = 0; i < bq->nmBucketsMax; i++) {
        free(bq->buckets[i]);
    }
    free(bq->buckets);

    free(bq);
    return 1;
}
