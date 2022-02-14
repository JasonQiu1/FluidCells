#ifndef BUCKETQUEUE_H
#define BUCKETQUEUE_H

// Bucket queue
typedef struct BucketQueue {
    void*** buckets;
    int* bucketLens;
    int nmBucketsMax;
    int bucketSizeMax;
} BucketQueue;

// Returns a malloced bucket queue. The caller must call delBucketQueue on it.
BucketQueue* createBucketQueue(int nmBucketsMax, int bucketSizeMax);

// Inserts an entry with priority to bq if unique.
// Returns 0 if bad priority, duplicate found, or no more space. 1 otherwise.
int bucketQueueInsert(void* e, int priority, 
                      BucketQueue* bq, int(*cmp)(void*,void*));

// Removes the last entry of the first nonempty bucket. The caller must free it
// Returns NULL if no more entries. The min entry otherwise.
void* bucketQueueExtractMin(BucketQueue* bq);

// Frees everything in an EMPTY bucket queue.
// Returns 0 if bq is not empty. Returns 1 otherwise.
int delBucketQueue(BucketQueue* bq);

#endif // BUCKETQUEUE_H
