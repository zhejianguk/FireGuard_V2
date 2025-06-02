#ifndef DEQUE_H
#define DEQUE_H

#define MAX 256
#define MAX_MASK (MAX-1)

typedef struct dequeue
{
  uint64_t data[MAX];
  int rear,front;
} dequeue;
 

static inline void initialize(dequeue *P) {
  P->rear=-1;
  P->front=-1;
}
 
static inline int empty(dequeue *P) {
  return (P->rear == -1) ? 1 : 0;
}
 
 /*
static inline int full(dequeue *P) {
  if((P->rear+1)&MAX_MASK==P->front)
    return(1);
  return(0);
*/

static inline void enqueueR(dequeue *P, uint64_t x) {
  if(empty(P)){
    P->rear=0;
    P->front=0;
    P->data[0]=x;
  } else {
    P->rear=(P->rear+1)&MAX_MASK;
    P->data[P->rear]=x;
  }
}
 
static inline void enqueueF(dequeue *P,uint64_t x) {
  if(empty(P)) {
    P->rear=0;
    P->front=0;
    P->data[0]=x;
  } else {
    P->front=(P->front-1+MAX)&MAX_MASK;
    P->data[P->front]=x;
  }
}
 
static inline uint64_t dequeueF(dequeue *P)  {
  uint64_t x;
  x=P->data[P->front];
  if(P->rear==P->front) {//delete the last element
    initialize(P);
  } else {
    P->front=(P->front+1)&MAX_MASK;
  }
  return(x);
}

static inline void dequeueF_noR(dequeue *P)  {
  if(P->rear==P->front) {//delete the last element
    initialize(P);
  } else {
    P->front=(P->front+1)&MAX_MASK;
  }
}

 
static inline uint64_t dequeueR(dequeue *P) {
  uint64_t x;
  x=P->data[P->rear];
  if(P->rear==P->front)
    initialize(P);
  else
    P->rear=(P->rear-1+MAX)&MAX_MASK;
  return(x);
}
 
static inline uint64_t queueT (dequeue *P)  {
  uint64_t x;
  x=P->data[P->front];
  return(x);
}
 

#endif