#ifndef QUEUE_H
#define QUEUE_H

#define QMAX 2048

typedef struct queue
{
  uint64_t data[QMAX];
  int top;
} queue;


static inline void q_initialize(queue *P) {
  P->top = -1;
}

static inline int q_empty(queue *P) {
  return (P->top == -1) ? 1 : 0;
}

static inline void q_enqueueF(queue *P, uint64_t x) {
    P->top+=1;
    P->data[P->top] = x;
}

static inline uint64_t q_dequeueF(queue *P) {
  uint64_t x;
  x = P->data[P->top];
  P->top-=1;
  return x;
}

#endif